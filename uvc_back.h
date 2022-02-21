#pragma once
#pragma once
#include <libuvc.h>
#include<iostream>
#include <vector>
#include <string>
#include <map>
#include <turbojpeg.h>
#include <opencv2/opencv.hpp>
using namespace std;



struct IR_DEVICE
{
	std::string name;
	std::string manufacturer;
	std::string serialNumber;
	uint16_t idProduct;
	uint16_t idVendor;
	uint8_t device_address;
	uint8_t bus_number;
	std::string uid;
};

class IR_Frame
{


public:
	IR_Frame();
	~IR_Frame();
	void attach_uvc_frame(uvc_frame* uvc_frame, bool copy = true);
	void gray(cv::Mat& Y);
	void bgr(cv::Mat& BGR);
	void jpeg2yuv();
	void yuv2bgr();
	tjhandle tj_context;
	cv::Mat _yuv_buffer;
	cv::Mat _bgr_buffer;
	uvc_frame* _uvc_frame;
	bool _yuv_converted, _bgr_converted;
	int yuv_subsampling;
	double timestamp;
	bool owns_uvc_frame;

};

bool get_device_list(std::vector<IR_DEVICE>& devices_list);
bool get_uid(std::vector<IR_DEVICE>& devices_list, std::string& name, std::string& uid);
class IR_Capture
{
public:
	IR_Capture()
	{

	}
	void init_error();

	void _start(uvc_device_t* devv, int width, int height, int fps);
	void _stop();
	void get_frame(IR_Frame& frame, float timeout = 0);
	void _enumerate_formats();
	int interval_to_fps(int interval);
	void _de_init_device();
	~IR_Capture();

	uvc_context_t* ctx;
	uvc_device_t** dev_list;
	uvc_device_t* dev;
	uvc_device_descriptor_t* desc;
	uvc_device_handle_t* devh;
	uvc_stream_handle_t* strmh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;
	tjhandle tj_context;
	IR_DEVICE device;
	int _stream_on;
	int _configured;
	float _bandwidth_factor;
};

