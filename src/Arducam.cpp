#if defined(_WIN32) || defined(_WIN64)
#include "stdafx.h"
#endif
#include "Arducam.hpp"

#define USB_CPLD_I2C_ADDRESS 0x46

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

void ArducamCamera::dumpDeviceInfo() {
	Uint32 ret = 0, version = 0;
	Cpld_info_t cpld_info;
	ret = ArduCam_readReg_8_8(handle, USB_CPLD_I2C_ADDRESS, 0x00, &version);
	ret = ArduCam_readReg_8_8(handle, USB_CPLD_I2C_ADDRESS, 0x05, &cpld_info.year);
	ret = ArduCam_readReg_8_8(handle, USB_CPLD_I2C_ADDRESS, 0x06, &cpld_info.mouth);
	ret = ArduCam_readReg_8_8(handle, USB_CPLD_I2C_ADDRESS, 0x07, &cpld_info.day);

	std::string version_str = "v" + std::to_string(version >> 4) + "." + std::to_string(version & 0x0F);
	strcpy(cpld_info.version, version_str.c_str());

	printf("CPLD version: %s year: %d mouth: %d day: %d\n", cpld_info.version, cpld_info.year, cpld_info.mouth, cpld_info.day);

	Uint8 data[16];
	Usb_info_t usb_info;
	ret = ArduCam_getboardConfig(handle, 0x80, 0x00, 0x00, 2, data);
	Uint8 usbType = cameraCfg.usbType;
	usb_info.interface_type = usbType == 4 ? 2 : 3;
	usb_info.device_type = usbType == 3 || usbType == 4 ? 3 : 2;
	std::string data_str = "v" + std::to_string(data[0] & 0xFF) + "." + std::to_string(data[1] & 0xFF);
	strcpy(usb_info.fw_version, data_str.c_str());
	printf("fw_version: %s interface_type: %d device_type: %d\n", usb_info.fw_version, usb_info.interface_type, usb_info.device_type);
}