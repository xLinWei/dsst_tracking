#pragma once

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/traits.hpp>
#include <memory>
#include <iostream>
#include <fstream>
#include "cv_ext.hpp"
#include "mat_consts.hpp"
#include "feature_channels.hpp"
#include "gradientMex.hpp"
#include "math_helper.hpp"
#include "scale_estimator.hpp"
#include "psr.hpp"

namespace cf_tracking
{
	struct DsstParameters
	{
		double padding = static_cast<double>(1);
		double outputSigmaFactor = static_cast<double>(0.05);
		double lambda = static_cast<double>(0.01);
		double learningRate = static_cast<double>(0.012);
		int templateSize = 64;
		int cellSize = 2;

		bool enableTrackingLossDetection = false;
		double psrThreshold = 13.5;
		int psrPeakDel = 1;

		bool enableScaleEstimator = true;
		double scaleSigmaFactor = static_cast<double>(0.25);
		double scaleStep = static_cast<double>(1.02);
		int scaleCellSize = 4;
		int numberOfScales = 33;

		//testing
		bool originalVersion = false;
		int resizeType = cv::INTER_LINEAR;
		bool useFhogTranspose = false;
	};

	class DsstTracker
	{
	public:
		typedef float T; // set precision here double or float
		static const int CV_TYPE = cv::DataType<T>::type;
		typedef cv::Size_<T> Size;
		typedef cv::Point_<T> Point;
		typedef cv::Rect_<T> Rect;
		typedef FhogFeatureChannels<T> FFC;
		typedef DsstFeatureChannels<T> DFC;
		typedef mat_consts::constants<T> consts;

		DsstTracker(DsstParameters paras);
		~DsstTracker();
		bool reinit(const cv::Mat& image, Rect& boundingBox);
		bool update(const cv::Mat& image, Rect& boundingBox);

	private:
		
		bool getTranslationTrainingData(const cv::Mat& image, std::shared_ptr<DFC>& hfNum,
			cv::Mat& hfDen, const Point& pos) const;
		bool getTranslationFeatures(const cv::Mat& image, std::shared_ptr<DFC>& features,
			const Point& pos, T scale) const;
		
		bool evalReponse(const cv::Mat &image, const cv::Mat& response,
			const cv::Point2i& maxResponseIdx,const Rect& tempBoundingBox) const;
		bool detectModel(const cv::Mat& image, cv::Mat& response,
			cv::Point2i& maxResponseIdx);
		bool updateModel(const cv::Mat& image);


	private:
		typedef void(*cvFhogPtr)
			(const cv::Mat& img, std::shared_ptr<DFC>& cvFeatures, int binSize, int fhogChannelsToCopy);
		cvFhogPtr cvFhog = 0;//函数指针

		typedef void(*dftPtr)
			(const cv::Mat& input, cv::Mat& output, int flags);
		dftPtr calcDft = 0;//函数指针

		cv::Mat _cosWindow;
		cv::Mat _y;
		std::shared_ptr<DFC> _hfNumerator;
		cv::Mat _hfDenominator;
		cv::Mat _yf;
		Point _pos;
		Size _templateSz;
		Size _templateSizeNoFloor;
		Size _baseTargetSz;
		//Rect _lastBoundingBox;
		T _scale; // _scale is the scale of the template; not the target
		T _templateScaleFactor; // _templateScaleFactor is used to calc the target scale
		ScaleEstimator<T>* _scaleEstimator;
		//int _frameIdx = 1;
		bool _isInitialized;

		const double _MIN_AREA;
		const double _MAX_AREA_FACTOR;
		const T _PADDING;
		const T _OUTPUT_SIGMA_FACTOR;
		const T _LAMBDA;
		const T _LEARNING_RATE;
		const T _PSR_THRESHOLD;
		const int _PSR_PEAK_DEL;
		const int _CELL_SIZE;
		const int _TEMPLATE_SIZE;
		const std::string _ID;
		const bool _ENABLE_TRACKING_LOSS_DETECTION;
		const int _RESIZE_TYPE;
		const bool _ORIGINAL_VERSION;
		const bool _USE_CCS;
	};
}
