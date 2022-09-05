#if defined(_WIN32) || defined(_WIN64)
#include "stdafx.h"
#endif
#include "Arducam.hpp"

bool ArducamCamera::openCamera(std::string fname, int index) {
	return camera_initFromFile(fname, this->handle, this->cameraCfg, index);
}

void ArducamCamera::closeCamera() {
	if (this->_running)
		this->stop();
	if (this->handle != nullptr)
		ArduCam_close(this->handle);
	this->handle = nullptr;
}

void ArducamCamera::start() {
	if (this->_running)
		return;

	ArduCam_setMode(this->handle, CONTINUOUS_MODE);
	this->_running = true;
	this->captureThread = std::thread(&ArducamCamera::capture, this);
}

void ArducamCamera::stop() {
	this->_running = false;
	if (this->captureThread.joinable())
		this->captureThread.join();

	captureSem.cleanup();
}

void ArducamCamera::capture() {
	Uint32 rtn_val = ArduCam_beginCaptureImage(handle);
	if (rtn_val == USB_CAMERA_USB_TASK_ERROR) {
		std::cout << "Error beginning capture, rtn_val = " << rtn_val << std::endl;
		return;
	}
	else {
		std::cout << "Capture began, rtn_val = " << rtn_val << std::endl;
	}

	while (_running) {
		rtn_val = ArduCam_captureImage(handle);
		if (rtn_val == USB_CAMERA_USB_TASK_ERROR) {
			std::cout << "Error capture image, rtn_val = " << rtn_val << std::endl;
			break;
		}
		else if (rtn_val > 0xFF) {
			std::cout << "Error capture image, rtn_val = " << rtn_val << std::endl;
		}
		else if (rtn_val > 0) {
			captureSem.notify(0);
		}
	}
	_running = false;
	ArduCam_endCaptureImage(handle);
	std::cout << "Capture thread stopped." << std::endl;
}

bool ArducamCamera::read(ArduCamOutData* &frameData, int timeout) {
	captureSem.wait_for(1, timeout);

	Uint32 rtn_val = ArduCam_readImage(handle, frameData);
	return rtn_val == USB_CAMERA_NO_ERROR;
}

void ArducamCamera::returnFrameBuffer() {
	ArduCam_del(handle);
}