
#include "dsst_tracker.h"

namespace cf_tracking
{
	DsstTracker::DsstTracker(DsstParameters paras)
		: _isInitialized(false),
		_scaleEstimator(0),
		_PADDING(static_cast<T>(paras.padding)),
		_OUTPUT_SIGMA_FACTOR(static_cast<T>(paras.outputSigmaFactor)),
		_LAMBDA(static_cast<T>(paras.lambda)),
		_LEARNING_RATE(static_cast<T>(paras.learningRate)),
		_CELL_SIZE(paras.cellSize),
		_TEMPLATE_SIZE(paras.templateSize),
		_PSR_THRESHOLD(static_cast<T>(paras.psrThreshold)),
		_PSR_PEAK_DEL(paras.psrPeakDel),
		_MIN_AREA(10),
		_MAX_AREA_FACTOR(0.8),
		_ID("DSSTcpp"),
		_ENABLE_TRACKING_LOSS_DETECTION(paras.enableTrackingLossDetection),
		_ORIGINAL_VERSION(paras.originalVersion),
		_RESIZE_TYPE(paras.resizeType),
		_USE_CCS(true)
	{
		if (paras.enableScaleEstimator)
		{
			ScaleEstimatorParas<T> sp;
			sp.scaleCellSize = paras.scaleCellSize;
			sp.scaleStep = static_cast<T>(paras.scaleStep);
			sp.numberOfScales = paras.numberOfScales;
			sp.scaleSigmaFactor = static_cast<T>(paras.scaleSigmaFactor);
			sp.lambda = static_cast<T>(paras.lambda);
			sp.learningRate = static_cast<T>(paras.learningRate);
			sp.useFhogTranspose = paras.useFhogTranspose;
			sp.resizeType = paras.resizeType;
			sp.originalVersion = paras.originalVersion;
			_scaleEstimator = new ScaleEstimator<T>(sp);
		}

		if (paras.useFhogTranspose)
			cvFhog = &piotr::cvFhogT < T, DFC >;
		else
			cvFhog = &piotr::cvFhog < T, DFC >;

		if (_USE_CCS)
			calcDft = &cf_tracking::dftCcs;
		else
			calcDft = &cf_tracking::dftNoCcs;

		// init dft
		cv::Mat initDft = (cv::Mat_<T>(1, 1) << 1);
		calcDft(initDft, initDft, 0);

		if (CV_MAJOR_VERSION < 3)
		{
			std::cout << "DsstTracker: Using OpenCV Version: " << CV_MAJOR_VERSION << std::endl;
			std::cout << "For more speed use 3.0 or higher!" << std::endl;
		}
	}

	DsstTracker::~DsstTracker()
	{
		delete _scaleEstimator;
	}

	//---------------------------- reinit ----------------------------//
	bool DsstTracker::reinit(const cv::Mat& image, Rect& boundingBox)
	{
		_pos.x = floor(boundingBox.x) + floor(boundingBox.width * 0.5);
		_pos.y = floor(boundingBox.y) + floor(boundingBox.height * 0.5);
		Size targetSize = Size(boundingBox.width, boundingBox.height);

		_templateSz = Size(floor(targetSize.width * (1 + _PADDING)),
			floor(targetSize.height * (1 + _PADDING)));

		_scale = 1.0;

		if (!_ORIGINAL_VERSION)
		{
			// resize to fixed side length _TEMPLATE_SIZE to stabilize FPS
			if (_templateSz.height > _templateSz.width)
				_scale = _templateSz.height / _TEMPLATE_SIZE;
			else
				_scale = _templateSz.width / _TEMPLATE_SIZE;

			_templateSz = Size(floor(_templateSz.width / _scale), floor(_templateSz.height / _scale));
		}

		_baseTargetSz = Size(targetSize.width / _scale, targetSize.height / _scale);
		_templateScaleFactor = 1 / _scale;

		Size templateSzByCells = Size(floor((_templateSz.width) / _CELL_SIZE),
			floor((_templateSz.height) / _CELL_SIZE));

		// translation filter output target
		/*T outputSigma = sqrt(_templateSz.area() / ((1 + _PADDING) * (1 + _PADDING)))
			* _OUTPUT_SIGMA_FACTOR / _CELL_SIZE;*/
		T outputSigma = sqrt(_baseTargetSz.area()) * _OUTPUT_SIGMA_FACTOR / _CELL_SIZE;
		_y = gaussianShapedLabels2D<T>(outputSigma, templateSzByCells);
		calcDft(_y, _yf, 0);

		// translation filter hann window
		cv::Mat cosWindowX;
		cv::Mat cosWindowY;
		cosWindowY = hanningWindow<T>(_yf.rows);
		cosWindowX = hanningWindow<T>(_yf.cols);
		_cosWindow = cosWindowY * cosWindowX.t();

		std::shared_ptr<DFC> hfNum(0);
		cv::Mat hfDen;

		if (getTranslationTrainingData(image, hfNum, hfDen, _pos) == false)
			return false;

		_hfNumerator = hfNum;
		_hfDenominator = hfDen;

		if (_scaleEstimator)
		{
			_scaleEstimator->reinit(image, _pos, targetSize,
				_scale * _templateScaleFactor);
		}

		//_lastBoundingBox = boundingBox;
		_isInitialized = true;
		return true;
	}

	bool DsstTracker::getTranslationTrainingData(const cv::Mat& image, std::shared_ptr<DFC>& hfNum,
		cv::Mat& hfDen, const Point& pos) const
	{
		std::shared_ptr<DFC> xt(0);

		if (getTranslationFeatures(image, xt, pos, _scale) == false)
			return false;

		std::shared_ptr<DFC> xtf;

		if (_USE_CCS)
			xtf = DFC::dftFeatures(xt);
		else
			xtf = DFC::dftFeatures(xt, cv::DFT_COMPLEX_OUTPUT);

		hfNum = DFC::mulSpectrumsFeatures(_yf, xtf, true);
		hfDen = DFC::sumFeatures(DFC::mulSpectrumsFeatures(xtf, xtf, true));

		return true;
	}

	bool DsstTracker::getTranslationFeatures(const cv::Mat& image, std::shared_ptr<DFC>& features,
		const Point& pos, T scale) const
	{
		cv::Mat patch;
		Size patchSize = _templateSz * scale;

		if (getSubWindow(image, patch, patchSize, pos) == false)
			return false;

		if (_ORIGINAL_VERSION)
			depResize(patch, patch, _templateSz);
		else
			resize(patch, patch, _templateSz, 0, 0, _RESIZE_TYPE);

		/*if (_debug != 0)
			_debug->showPatch(patch);*/

		cv::Mat floatPatch;
		patch.convertTo(floatPatch, CV_32FC(3));

		features.reset(new DFC());
		cvFhog(floatPatch, features, _CELL_SIZE, DFC::numberOfChannels() - 1);

		// append gray-scale image
		if (patch.channels() == 1)
		{
			if (_CELL_SIZE != 1)
				resize(patch, patch, features->channels[0].size(), 0, 0, _RESIZE_TYPE);

			features->channels[DFC::numberOfChannels() - 1] = patch / 255.0 - 0.5;
		}
		else
		{
			if (_CELL_SIZE != 1)
				resize(patch, patch, features->channels[0].size(), 0, 0, _RESIZE_TYPE);

			cv::Mat grayFrame;
			cvtColor(patch, grayFrame, cv::COLOR_BGR2GRAY);
			grayFrame.convertTo(grayFrame, CV_TYPE);
			grayFrame = grayFrame / 255.0 - 0.5;
			features->channels[DFC::numberOfChannels() - 1] = grayFrame;
		}

		DFC::mulFeatures(features, _cosWindow);
		return true;
	}

	//---------------------------- update ----------------------------//
	/*bool DsstTracker::update(const cv::Mat& image, Rect& boundingBox)
	{
		return updateAtScalePos(image, _pos, _scale, boundingBox);
	}*/

	bool DsstTracker::update(const cv::Mat& image, Rect& boundingBox)//the old updateAtScalePos()
	{
		//++_frameIdx;

		if (!_isInitialized)
			return false;

		/*T newScale = _scale;
		Point newPos = _pos;*/
		cv::Point2i maxResponseIdx;
		cv::Mat response;

		if (detectModel(image, response, maxResponseIdx) == false)//here _pos & _scale bas updated
			return false;

		// return box
		boundingBox.width = _baseTargetSz.width * _scale;
		boundingBox.height = _baseTargetSz.height * _scale;
		boundingBox.x = _pos.x - boundingBox.width / 2;
		boundingBox.y = _pos.y - boundingBox.height / 2;

		if (_ENABLE_TRACKING_LOSS_DETECTION)
		{
			if (evalReponse(image, response, maxResponseIdx,
				boundingBox) == false)
				return false;
		}

		if (updateModel(image) == false)
			return false;

		return true;
	}

	bool DsstTracker::evalReponse(const cv::Mat &image, const cv::Mat& response,
		const cv::Point2i& maxResponseIdx,
		const Rect& tempBoundingBox) const
	{
		T peakValue = 0;
		T psrClamped = calcPsr(response, maxResponseIdx, _PSR_PEAK_DEL, peakValue);

		/*if (_debug != 0)
		{
			_debug->showResponse(response, peakValue);
			_debug->setPsr(psrClamped);
		}*/

		if (psrClamped < _PSR_THRESHOLD)
			return false;

		// check if we are out of image, too small or too large
		Rect imageRect(Point(0, 0), image.size());
		Rect intersection = imageRect & tempBoundingBox;
		double  bbArea = tempBoundingBox.area();
		double areaThreshold = _MAX_AREA_FACTOR * imageRect.area();
		double intersectDiff = std::abs(bbArea - intersection.area());

		if (intersectDiff > 0.01 || bbArea < _MIN_AREA
			|| bbArea > areaThreshold)
			return false;

		return true;
	}

	bool DsstTracker::detectModel(const cv::Mat& image, cv::Mat& response,
		cv::Point2i& maxResponseIdx)
	{
		// find translation
		std::shared_ptr<DFC> xt(0);

		if (getTranslationFeatures(image, xt, _pos, _scale) == false)
			return false;

		std::shared_ptr<DFC> xtf;
		if (_USE_CCS)
			xtf = DFC::dftFeatures(xt);
		else
			xtf = DFC::dftFeatures(xt, cv::DFT_COMPLEX_OUTPUT);

		std::shared_ptr<DFC> sampleSpec = DFC::mulSpectrumsFeatures(_hfNumerator, xtf, false);
		cv::Mat sumXtf = DFC::sumFeatures(sampleSpec);
		cv::Mat hfDenLambda = addRealToSpectrum<T>(_LAMBDA, _hfDenominator);
		cv::Mat responseTf;

		if (_USE_CCS)
			divSpectrums(sumXtf, hfDenLambda, responseTf, 0, false);
		else
			divideSpectrumsNoCcs<T>(sumXtf, hfDenLambda, responseTf);

		idft(responseTf, response, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);

		double maxResponse;
		minMaxLoc(response, 0, &maxResponse, 0, &maxResponseIdx);

		T posDeltaX = (maxResponseIdx.x + 1 - floor(response.cols / consts::c2_0)) * _scale;
		T posDeltaY = (maxResponseIdx.y + 1 - floor(response.rows / consts::c2_0)) * _scale;
		_pos.x += round(posDeltaX * _CELL_SIZE);//here _pos is updated
		_pos.y += round(posDeltaY * _CELL_SIZE);

		/*if (_debug != 0)
			_debug->showResponse(translationResponse, maxResponse);*/

		if (_scaleEstimator)
		{
			//find scale
			T tempScale = _scale * _templateScaleFactor;

			if (_scaleEstimator->detectScale(image, _pos,
				tempScale) == false)
				return false;

			_scale = tempScale / _templateScaleFactor;//here _scale is updated
		}

		return true;
	}

	bool DsstTracker::updateModel(const cv::Mat& image)
	{
		std::shared_ptr<DFC> hfNum(0);
		cv::Mat hfDen;

		if (getTranslationTrainingData(image, hfNum, hfDen, _pos) == false)
			return false;

		_hfDenominator = (1 - _LEARNING_RATE) * _hfDenominator + _LEARNING_RATE * hfDen;
		DFC::mulValueFeatures(_hfNumerator, (1 - _LEARNING_RATE));
		DFC::mulValueFeatures(hfNum, _LEARNING_RATE);
		DFC::addFeatures(_hfNumerator, hfNum);

		if (_scaleEstimator)
		{
			if (_scaleEstimator->updateScale(image, _pos, _scale * _templateScaleFactor) == false)
				return false;
		}

		return true;
	}


}

