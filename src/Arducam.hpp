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

typedef struct {
	char version[16];
	Uint32 year;
	Uint32 mouth; 
	Uint32 day;
} Cpld_info_t;

typedef struct {
	char fw_version[16];
	Uint32 interface_type;
	Uint32 device_type;
} Usb_info_t;

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

private:
	void capture();

private:
	Semaphore captureSem;
	std::thread captureThread;
	ArduCamHandle handle = nullptr;
	ArduCamCfg cameraCfg;
	volatile bool _running = false;

};
