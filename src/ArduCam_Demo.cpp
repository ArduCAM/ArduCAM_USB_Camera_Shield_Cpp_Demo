// ArduCam_test.cpp : Defines the entry point for the console application.
//
#ifdef __linux__
#include <ArduCamLib.h>
#include <unistd.h>
#include <termios.h>
#endif

#ifdef _WIN32
#include "stdafx.h"
#include <Windows.h>
#include "ArduCamlib.h"
#include <io.h>
#include <direct.h> 
#include <conio.h>
#endif
#include <opencv2/opencv.hpp>
#include <thread>
#include <time.h>
#include <iostream>
#include <istream>
#include <string>
#include "Utils.h"
#include "Arducam.hpp"

using namespace std;
using namespace cv;

void delay_ms(int mills) {
#ifdef __linux__
	usleep(mills * 1000);
#endif
#ifdef _WIN32
	Sleep(mills);
#endif
}

static void display_fps(int index) {
	static thread_local int frame_count = 0;
	static thread_local time_t start = time(NULL);

	frame_count++;

	time_t end = time(NULL);
	if (end - start >= 1) {
		start = end;
		std::cout << "camera: "<< index <<  ", fps: " << frame_count << std::endl;
		frame_count = 0;
	}
}

void display(ArducamCamera *camera, int index) {
	while (true) {
		ArduCamOutData* frameData;
		if (!camera->read(frameData)) {
			std::cout << "read frame failed." << std::endl;
			continue;
		}
		cv::Mat image = ConvertImage(frameData);
		camera->returnFrameBuffer();
		if (!image.data) {
			std::cout << "No image data" << std::endl;
			continue;
		}
		display_fps(index);

		double scale = 640.0 / image.cols;
		cv::resize(image, image,
			cv::Size(), scale, scale, cv::INTER_LINEAR);

		std::string name = "ArduCam";
		name += std::to_string(index);
		cv::imshow(name, image);

		int key = -1;
		key = cv::waitKey(1);
		if (key == 'q')
			break;
	}

	camera->stop();

	camera->closeCamera();
}

int main(int argc,char **argv)
{
	const char * config_file_name;
	if(argc > 1){
		config_file_name = argv[1];
	}else{
		showHelp();
		return 0;
	}


	ArducamCamera *camera = new ArducamCamera();
	
	if (!camera->openCamera(config_file_name, 0)) {
		std::cout << "Failed to open camera." << std::endl;
		return 0;
	}

	camera->start();
	std::thread camera_display(display, camera, 0);

	camera_display.join();
	return 0;
}


