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

extern bool save_raw;
extern int color_mode;
extern ArduCamCfg cameraCfg;
void showHelp();
bool camera_initFromFile(std::string filename, ArduCamHandle &cameraHandle, ArduCamCfg &cameraCfg, int index = 0);
cv::Mat ConvertImage(ArduCamOutData* frameData);
#endif // !
