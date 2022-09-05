#if defined(_WIN32) || defined(_WIN64)
#include "stdafx.h"
#include <Windows.h>
#include "arducam_config_parser.h"
#include "ArduCamlib.h"
#endif

#if __linux__
#include <arducam_config_parser.h>
#include <ArduCamLib.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "Utils.h"
#include <time.h>

bool save_raw = false;
int color_mode = 0;

void showHelp() {
	printf(" usage: sudo ./ArduCam_Demo <path/config-file-name>	\
			\n\n example: sudo ./ArduCam_Demo ../Config/AR0134_960p_Color.cfg	\
			\n\n While the program is running, you can press the following buttons in the terminal:	\
			\n\n 's':Save the image to the images folder.	\
			\n\n 'c':Stop saving images.	\
			\n\n 'q':Stop running the program.	\
			\n\n");
}

void configBoard(ArduCamHandle &cameraHandle, Config config) {
	uint8_t u8Buf[10];
	for (uint32_t n = 0; n < config.params[3]; n++) {
		u8Buf[n] = config.params[4 + n];
	}
	ArduCam_setboardConfig(cameraHandle, config.params[0], config.params[1],
		config.params[2], config.params[3], u8Buf);
}


/**
 * read config file and open the camera.
 * @param filename : path/config_file_name
 * @param cameraHandle : camera handle
 * @param cameraCfg :camera config struct
 * @return TURE or FALSE
 * */
bool camera_initFromFile(std::string filename, ArduCamHandle &cameraHandle, ArduCamCfg &cameraCfg, int index) {
	CameraConfigs cam_cfgs;
	memset(&cam_cfgs, 0x00, sizeof(CameraConfigs));
	if (arducam_parse_config(filename.c_str(), &cam_cfgs)) {
		std::cout << "Cannot find configuration file." << std::endl << std::endl;
		showHelp();
		return false;
	}
	CameraParam *cam_param = &cam_cfgs.camera_param;
	Config *configs = cam_cfgs.configs;
	int configs_length = cam_cfgs.configs_length;

	switch (cam_param->i2c_mode) {
	case 0: cameraCfg.emI2cMode = I2C_MODE_8_8; break;
	case 1: cameraCfg.emI2cMode = I2C_MODE_8_16; break;
	case 2: cameraCfg.emI2cMode = I2C_MODE_16_8; break;
	case 3: cameraCfg.emI2cMode = I2C_MODE_16_16; break;
	default: break;
	}

	color_mode = cam_param->format & 0xFF;
	switch (cam_param->format >> 8) {
	case 0: cameraCfg.emImageFmtMode = FORMAT_MODE_RAW; break;
	case 1: cameraCfg.emImageFmtMode = FORMAT_MODE_RGB; break;
	case 2: cameraCfg.emImageFmtMode = FORMAT_MODE_YUV; break;
	case 3: cameraCfg.emImageFmtMode = FORMAT_MODE_JPG; break;
	case 4: cameraCfg.emImageFmtMode = FORMAT_MODE_MON; break;
	case 5: cameraCfg.emImageFmtMode = FORMAT_MODE_RAW_D; break;
	case 6: cameraCfg.emImageFmtMode = FORMAT_MODE_MON_D; break;
	default: break;
	}

	cameraCfg.u32Width = cam_param->width;
	cameraCfg.u32Height = cam_param->height;

	cameraCfg.u32I2cAddr = cam_param->i2c_addr;
	cameraCfg.u8PixelBits = cam_param->bit_width;
	cameraCfg.u32TransLvl = cam_param->trans_lvl;

	if (cameraCfg.u8PixelBits <= 8) {
		cameraCfg.u8PixelBytes = 1;
	}
	else if (cameraCfg.u8PixelBits > 8 && cameraCfg.u8PixelBits <= 16) {
		cameraCfg.u8PixelBytes = 2;
		save_raw = true;
	}

	int ret_val = ArduCam_open(cameraHandle, &cameraCfg, index);
	//int ret_val = ArduCam_autoopen(cameraHandle, &cameraCfg);
	if (ret_val == USB_CAMERA_NO_ERROR) {
		//ArduCam_enableForceRead(cameraHandle);	//Force display image
		for (int i = 0; i < configs_length; i++) {
			uint32_t type = configs[i].type;
			if (((type >> 16) & 0xFF) && ((type >> 16) & 0xFF) != cameraCfg.usbType)
				continue;
			switch (type & 0xFFFF) {
			case CONFIG_TYPE_REG:
				ArduCam_writeSensorReg(cameraHandle, configs[i].params[0], configs[i].params[1]);
				break;
			case CONFIG_TYPE_DELAY:
#ifdef __linux__
				usleep(1000 * configs[i].params[0]);
#endif
#ifdef _WIN32
				Sleep(configs[i].params[0]);
#endif	
				break;
			case CONFIG_TYPE_VRCMD:
				configBoard(cameraHandle, configs[i]);
				break;
			}
		}
		unsigned char u8TmpData[16];
		ArduCam_readUserData(cameraHandle, 0x400 - 16, 16, u8TmpData);
		printf("Serial: %c%c%c%c-%c%c%c%c-%c%c%c%c\n",
			u8TmpData[0], u8TmpData[1], u8TmpData[2], u8TmpData[3],
			u8TmpData[4], u8TmpData[5], u8TmpData[6], u8TmpData[7],
			u8TmpData[8], u8TmpData[9], u8TmpData[10], u8TmpData[11]);
	}
	else {
		std::cout << "Cannot open camera.rtn_val = " << ret_val << std::endl;
		return false;
	}

	return true;
}



cv::Mat JPGToMat(Uint8* bytes, int length) {
	cv::Mat image = cv::Mat(1, length, CV_8UC1, bytes);
	if (length <= 0) {
		image.data = NULL;
		return image;
	}

	image = imdecode(image, cv::IMREAD_ANYCOLOR);
	return image;
}
cv::Mat YUV422toMat(Uint8* bytes, int width, int height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC2, bytes);
	cv::cvtColor(image, image, cv::COLOR_YUV2BGR_YUYV);
	return image;
}

cv::Mat separationImage(Uint8* bytes, int width, int height)
{
	int width_d = width << 1;
	unsigned char* temp1, *temp2;
	temp1 = (unsigned char*)malloc(width);
	temp2 = (unsigned char*)malloc(width);

	for (int k = 0; k < height; k++) {
		for (int i = 0, j = 0; i < width_d; i += 2) {
			temp1[j] = bytes[i + (k * width_d)];
			temp2[j++] = bytes[i + 1 + (k * width_d)];
		}
		memcpy(bytes + (k * width_d), temp1, width);
		memcpy(bytes + (k * width_d + width), temp2, width);
	}
	cv::Mat image = cv::Mat(height, width_d, CV_8UC1, bytes);
	free(temp1);
	free(temp2);
	return image;
}
#define RGB565_RED      0xf800
#define RGB565_GREEN    0x07e0
#define RGB565_BLUE     0x001f
cv::Mat RGB565toMat(Uint8* bytes, int width, int height) {
	unsigned char* temp_data, *ptdata, *data, *data_end;

	data = bytes;
	data_end = bytes + (width * height * 2);

	temp_data = (unsigned char*)malloc(width * height * 3);
	ptdata = temp_data;

	Uint8 r, g, b;
	while (data < data_end) {
		unsigned short temp;

		temp = (*data << 8) | *(data + 1);
		r = (temp & RGB565_RED) >> 8;
		g = (temp & RGB565_GREEN) >> 3;
		b = (temp & RGB565_BLUE) << 3;

		switch (color_mode) {
		case 1:
			*ptdata++ = r;
			*ptdata++ = g;
			*ptdata++ = b;
			break;
		case 0:
		default:
			*ptdata++ = b;
			*ptdata++ = g;
			*ptdata++ = r;
			break;
		}
		data += 2;
	}

	cv::Mat image = cv::Mat(height, width, CV_8UC3);
	memcpy(image.data, temp_data, width * height * 3);
	cv::flip(image, image, 0);
	free(temp_data);
	return image;
}


cv::Mat dBytesToMat(Uint8* bytes, int bit_width, int width, int height) {
	unsigned char* temp_data = (unsigned char*)malloc(width * height);
	int index = 0;
	for (int i = 0; i < width * height * 2; i += 2) {
		unsigned char temp = ((bytes[i + 1] << 8 | bytes[i]) >> (bit_width - 8)) & 0xFF;
		temp_data[index++] = temp;
	}
	cv::Mat image = cv::Mat(height, width, CV_8UC1);
	memcpy(image.data, temp_data, height * width);
	free(temp_data);
	return image;
}

cv::Mat BytestoMat(Uint8* bytes, int width, int height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC1, bytes);
	return image;
}

cv::Mat ConvertImage(ArduCamOutData* frameData) {
	cv::Mat rawImage;
	Uint8* data = frameData->pu8ImageData;
	ArduCamCfg cameraCfg = frameData->stImagePara;
	int height, width;
	width = cameraCfg.u32Width;
	height = cameraCfg.u32Height;

	switch (cameraCfg.emImageFmtMode) {
	case FORMAT_MODE_RGB:
		rawImage = RGB565toMat(data, width, height);
		break;
	case FORMAT_MODE_RAW_D:
		rawImage = separationImage(data, width, height);
		switch (color_mode) {
		case RAW_RG:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerRG2BGR);
			break;
		case RAW_GR:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerGR2BGR);
			break;
		case RAW_GB:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerGB2BGR);
			break;
		case RAW_BG:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerBG2BGR);
			break;
		default:
			cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerRG2BGR);
			break;
		}
		break;
	case FORMAT_MODE_MON_D:
		rawImage = separationImage(data, width, height);
		break;
	case FORMAT_MODE_JPG:
		rawImage = JPGToMat(data, frameData->stImagePara.u32Size);
		break;
	case FORMAT_MODE_RAW:
		if (cameraCfg.u8PixelBytes == 2) {
			rawImage = dBytesToMat(data, frameData->stImagePara.u8PixelBits, width, height);
		}
		else {
			rawImage = BytestoMat(data, width, height);
		}
		switch (color_mode) {
		case RAW_RG:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerRG2BGR);
			break;
		case RAW_GR:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerGR2BGR);
			break;
		case RAW_GB:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerGB2BGR);
			break;
		case RAW_BG:cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerBG2BGR);
			break;
		default:
			cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerRG2BGR);
			break;
		}
		break;
	case FORMAT_MODE_YUV:
		rawImage = YUV422toMat(data, width, height);
		break;
	case FORMAT_MODE_MON:
		if (cameraCfg.u8PixelBytes == 2) {
			rawImage = dBytesToMat(data, frameData->stImagePara.u8PixelBits, width, height);
		}
		else {
			rawImage = BytestoMat(data, width, height);
		}
		break;
	default:
		if (cameraCfg.u8PixelBytes == 2) {
			rawImage = dBytesToMat(data, frameData->stImagePara.u8PixelBits, width, height);
		}
		else {
			rawImage = BytestoMat(data, width, height);
		}
		cv::cvtColor(rawImage, rawImage, cv::COLOR_BayerRG2RGB);
		break;
	}
	return rawImage;

}
