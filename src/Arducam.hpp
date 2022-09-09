#pragma once

#if defined(__linux__)
#include <ArduCamLib.h>
#include <arducam_config_parser.h>
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include "ArduCamlib.h"
#include "arducam_config_parser.h"
#endif

#include <opencv2/opencv.hpp>
#include <thread>
#include <time.h>
#include <iostream>
#include <thread>
#include <string>
#include "Utils.h"
#include "Semaphore.hpp"

class ArducamCamera {
public:
	ArducamCamera(){}
	~ArducamCamera() {}

	bool openCamera(std::string fname, int index = 0);
	void closeCamera();

	void start();
	void stop();

	bool read(ArduCamOutData* &frameData, int timeout = 1500);
	void returnFrameBuffer();

	void dumpDeviceInfo();

	int setCtrl(std::string name, int64_t val);
	uint32_t writeReg_8_8(uint32_t i2cAddr, uint32_t regAddr, uint32_t val);

public:
	int rows = 0;
	int cols = 0;
	int color_mode;
	format_mode format;
	ArduCamCfg cameraCfg = {};

private:
	void capture();

private:
	Semaphore captureSem;
	std::thread captureThread;
	ArduCamHandle handle = nullptr;
	volatile bool _running = false;
};
