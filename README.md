# Dsst_tracking
C++ implementation of Dsst

refer to "https://github.com/klahaag/cf_tracking"

参照上述链接实现DSST目标跟踪，链接里实现了DSST和KCF两种算法，这里将DSST提取出来单独实现，并进行了工程简化方便理解代码，简化的内容有：
  *    将命令行解析从TrackerRun类里提出，并将命令缩减到4个:
  
      -c, "Camera device id"
      -f, "seq_file", "Path to sequence file"
      -b, "Init Bounding Box"
      -s, "Save video as result.avi and output results to result.txt in current dirctory "
      
  *    去掉了重载函数，参考工程里的重载函数只在参数类型上有不同，所以将数据类型固定并去掉了重载函数.
  *    去掉了一些无关紧要的中间函数(这样做代码结构可能会不清晰，但方便理解代码).
  *    将Init和Run的windowTitle统一，参考工程里的选取目标的窗口和显示目标跟踪的窗口不同，所以这里将两个窗口统一.
  
  
  

# References
[1]
```
@article{henriques2015tracking,
title = {High-Speed Tracking with Kernelized Correlation Filters},
author = {Henriques, J. F. and Caseiro, R. and Martins, P. and Batista, J.},
journal = {Pattern Analysis and Machine Intelligence, IEEE Transactions on},
year = {2015}
```


[2]
```
@inproceedings{danelljan2014dsst,
title={Accurate Scale Estimation for Robust Visual Tracking},
author={Danelljan, Martin and H{\"a}ger, Gustav and Khan, Fahad Shahbaz and Felsberg, Michael},
booktitle={Proceedings of the British Machine Vision Conference BMVC},
year={2014}}
```

[3]
```
@inproceedings{danelljan2014colorattributes,
title={Adaptive Color Attributes for Real-Time Visual Tracking},
author={Danelljan, Martin and Khan, Fahad Shahbaz and Felsberg, Michael and Weijer, Joost van de},
booktitle={Conference on Computer Vision and Pattern Recognition (CVPR)},
year={2014}}
```

[4]
```
@article{lsvm-pami,
title = "Object Detection with Discriminatively Trained Part Based Models",
author = "Felzenszwalb, P. F. and Girshick, R. B. and McAllester, D. and Ramanan, D.",
journal = "IEEE Transactions on Pattern Analysis and Machine Intelligence",
year = "2010", volume = "32", number = "9", pages = "1627--1645"}
```

[5]
```
@misc{PMT,
author = {Piotr Doll\'ar},
title = {{P}iotr's {C}omputer {V}ision {M}atlab {T}oolbox ({PMT})},
howpublished = {\url{http://vision.ucsd.edu/~pdollar/toolbox/doc/index.html}}}
```

[6]
```
@inproceedings{bolme2010mosse,
author={Bolme, David S. and Beveridge, J. Ross and Draper, Bruce A. and Yui Man Lui},
title={Visual Object Tracking using Adaptive Correlation Filters},
booktitle={Conference on Computer Vision and Pattern Recognition (CVPR)},
year={2010}}
```
