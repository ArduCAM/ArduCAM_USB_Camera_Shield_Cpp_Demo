#pragma once
#ifndef _ARDUCAM_UTILS_H_
#define _ARDUCAM_UTILS_H_
#include <iostream>

#ifdef __linux__
#include <ArduCamLib.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include "ArduCamlib.h"
#endif

#include <opencv2/opencv.hpp>

void showHelp();
bool camera_initFromFile(std::string filename, ArduCamHandle &cameraHandle, ArduCamCfg &cameraCfg, int &color_mode, int index = 0);
cv::Mat ConvertImage(ArduCamOutData* frameData, int color_mode);
cv::Mat UnpackRaw10(ArduCamOutData* frameData, int drop_row = 0);
#endif // !
