#include "uvc_back.h"

std::map<int, std::string> IR_uvc_error_codes;

bool get_device_list(std::vector<IR_DEVICE>& devices_list)
{
	uvc_context_t* ctx;
	uvc_device_t** dev_list;
	uvc_device_t* dev;
	uvc_device_descriptor_t* desc;
	uvc_error_t res;

	res = uvc_init(&ctx, NULL);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "could not initialize");
		return false;
	}

	res = uvc_get_device_list(
		ctx,
		&dev_list);
	if (res != UVC_SUCCESS) {
		uvc_perror(res, "could not get device list");
		return false;
	}
	int idx = 0;
	while (true)
	{
		dev = dev_list[idx];
		if (dev == NULL)
		{
			break;
		}
		IR_DEVICE device;
		if (uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS)
		{
			device.name = (desc->product != NULL) ? (desc->product) : "unknown";
			device.manufacturer = (desc->manufacturer != NULL) ? (desc->manufacturer) : "unknown";
			device.serialNumber = (desc->serialNumber != NULL) ? (desc->serialNumber) : "unknown";
			device.idProduct = desc->idProduct;
			device.idVendor = desc->idVendor;
			device.device_address = uvc_get_device_address(dev);
			device.bus_number = uvc_get_bus_number(dev);
			char tmp[10];
			sprintf_s(tmp, "%u:%u", device.bus_number, device.device_address);
			device.uid = tmp;
			devices_list.push_back(device);
		}
		uvc_free_device_descriptor(desc);
		idx++;
	}
	uvc_free_device_list(dev_list, 1);
	uvc_exit(ctx);
	return true;
}

bool get_uid(std::vector<IR_DEVICE>& devices_list, std::string& name, std::string& uid)
{
	for (int i = 0; i < devices_list.size(); i++)
	{
		if (name == devices_list[i].name)
		{
			uid = devices_list[i].uid;
			return true;
		}
	}
	return false;
}

void IR_Capture::init_error()
{
	IR_uvc_error_codes[0] = "Success (no error)";
	IR_uvc_error_codes[-1] = "Input/output error.";
	IR_uvc_error_codes[-2] = "Invalid parameter.";
	IR_uvc_error_codes[-3] = "Access denied.";
	IR_uvc_error_codes[-4] = "No such device.";
	IR_uvc_error_codes[-5] = "Entity not found.";
	IR_uvc_error_codes[-6] = "Resource busy.";
	IR_uvc_error_codes[-7] = "Operation timed out.";
	IR_uvc_error_codes[-8] = "Overflow.";
	IR_uvc_error_codes[-9] = "Pipe error.";
	IR_uvc_error_codes[-10] = "System call interrupted.";
	IR_uvc_error_codes[-11] = "Insufficient memory.";
	IR_uvc_error_codes[-12] = "Operation not supported.";
	IR_uvc_error_codes[-50] = "Device is not UVC-compliant.";
	IR_uvc_error_codes[-51] = "Mode not supported.";
	IR_uvc_error_codes[-52] = "Resource has a callback (can't use polling and async)";
	IR_uvc_error_codes[-99] = "Undefined error.";
}

void IR_Capture::_start(uvc_device_t* devv, int width, int height, int fps)
{

	tj_context = tjInitDecompress();
	float bandwidth_factor = 2.0;
	int err;
	err = uvc_init(&ctx, NULL);
	if (err < 0) printf("uvc_init error %d\n", err);
	dev = devv;
	err = uvc_open(dev, &devh);  //UVC_FRAME_FORMAT_YUYV  UVC_FRAME_FORMAT_COMPRESSED
	if (err < 0) printf("uvc_open error %d\n", err);
	uvc_print_diag(devh, stderr);
	err = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_MJPEG, width, height, fps);
	if (err < 0) printf("uvc_get_stream_ctrl_format_size error %d\n", err);
	err = uvc_stream_open_ctrl(devh, &strmh, &ctrl);
	if (err < 0) printf("uvc_stream_open_ctrl error %d\n", err);
	err = uvc_stream_start(strmh, NULL, NULL, bandwidth_factor, 0);
	if (err < 0) printf("uvc_stream_start error %d\n", err);
	_stream_on = 1;
	//_enumerate_formats();

	std::cout << "Stream start." << std::endl;
}

void IR_Capture::_stop()
{
	int status = 0;
	status = uvc_stream_stop(strmh);
	if (status != UVC_SUCCESS)  std::cout << "Can't stop stream: " << IR_uvc_error_codes[status] << "Will ignore this and try to continue " << std::endl;
	else std::cout << "Stream stopped" << std::endl;

	uvc_stream_close(strmh);
	std::cout << "Stream closed" << std::endl;
	_stream_on = 0;
}

void IR_Capture::get_frame(IR_Frame& out_frame, float timeout)
{
	int status, j_width, j_height, jpegSubsamp, header_ok;
	int timeout_usec = int(timeout * 1e6);
	if (!_stream_on)  exit(-1);
	uvc_frame* bgr;
	uvc_frame* uvc_frame = NULL;
	status = uvc_stream_get_frame(strmh, &uvc_frame, timeout_usec);
	if (status != UVC_SUCCESS)
	{
		std::cout << "uvc_stream_get_frame: " << IR_uvc_error_codes[status] << std::endl;
		return;
	}
	if (uvc_frame == NULL)
	{
		std::cout << "Frame pointer is NULL" << std::endl;
		return;
	}
	//bgr = uvc_allocate_frame(uvc_frame->width * uvc_frame->height * 3);
	//uvc_error_t sstatus = uvc_any2bgr(uvc_frame, bgr);
	//if (sstatus) {
	//	printf("read image error : %d\n", sstatus);
	//}
	//std::cout << " uvc_frame " << uvc_frame->width << " " << uvc_frame->height << " " << uvc_frame->data_bytes << endl;
	
	out_frame.tj_context = tj_context;
	out_frame.attach_uvc_frame(uvc_frame, true);
	out_frame.timestamp = uvc_frame->capture_time.tv_sec + (double)uvc_frame->capture_time.tv_usec * (1e-6);
}

void IR_Capture::_enumerate_formats()
{
	const uvc_format_desc_t* format_desc = uvc_get_format_descs(devh);
	uvc_frame_desc* frame_desc;
	int i;
	while (format_desc != NULL)
	{
		frame_desc = format_desc->frame_descs;
		while (frame_desc != NULL)
		{
			if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_MJPEG)
			{
				std::cout << frame_desc->bFrameIndex << std::endl;
				int width = frame_desc->wWidth;
				int height = frame_desc->wHeight;
				std::cout << width << " " << height << std::endl;
				i = 0;
				while (frame_desc->intervals[i])
				{
					std::cout << interval_to_fps(frame_desc->intervals[i]) << std::endl;
					i++;
				}
			}
			frame_desc = frame_desc->next;
		}
		format_desc = format_desc->next;
	}
}

int IR_Capture::interval_to_fps(int interval)
{
	return int(10000000 / interval);
}

void IR_Capture::_de_init_device()
{
	uvc_close(devh);
	devh = NULL;
	uvc_unref_device(dev);
	dev = NULL;
	cout << "UVC device closed" << endl;
}

IR_Capture::~IR_Capture()

{
	if (_stream_on)  _stop();
	if (devh != NULL) _de_init_device();
	if (ctx != NULL)
	{
		uvc_exit(ctx);
		ctx = NULL;
		tjDestroy(tj_context);
		tj_context = NULL;
	}
}

IR_Frame::IR_Frame()
{
	tj_context = NULL;
	_yuv_converted = false;
	_bgr_converted = false;
}

IR_Frame::~IR_Frame()
{
	if (owns_uvc_frame)
	{
		uvc_free_frame(_uvc_frame);
	}
}

// main purpose: convert
void IR_Frame::attach_uvc_frame(uvc_frame* uvc_frame, bool copy)
{
	if (copy)
	{
		_uvc_frame = uvc_allocate_frame(uvc_frame->data_bytes); // allocate memory
		uvc_duplicate_frame(uvc_frame, _uvc_frame);            // copy
		owns_uvc_frame = true;
	}
	else
	{
		_uvc_frame = uvc_frame;  // equal directly 
		owns_uvc_frame = false;
	}
}
void IR_Frame::gray(cv::Mat& Y)
{
	if (_yuv_converted == false)
	{
		jpeg2yuv();
	}
	Y = _yuv_buffer.colRange(0, _uvc_frame->width * _uvc_frame->height).reshape(1, _uvc_frame->height); 
}

void IR_Frame::bgr(cv::Mat& BGR)
{

}

// 7.55ms on 1080p ??
void IR_Frame::jpeg2yuv()
{
	int channels = 1;
	int jpegSubsamp, j_width, j_height;
	int result;
	unsigned int buf_size;
	result = tjDecompressHeader2(tj_context, (unsigned char*)_uvc_frame->data, _uvc_frame->data_bytes, &j_width, &j_height, &jpegSubsamp);
	if (result == -1)
	{
		std::cout << "Turbojpeg could not read jpeg header: " << tjGetErrorStr() << std::endl;
		j_width = _uvc_frame->width;
		j_height = _uvc_frame->height;
		jpegSubsamp = TJSAMP_422;
	}
	buf_size = tjBufSizeYUV(j_width, j_height, jpegSubsamp);
	_yuv_buffer = cv::Mat(1, buf_size, CV_8UC1); // ¿´¿´ºóÃæÊÇ·ñ»ØÊÕÄÚ´æ 
	if (result != -1)
	{
		result = tjDecompressToYUV(tj_context, (unsigned char*)_uvc_frame->data, _uvc_frame->data_bytes, &(_yuv_buffer.data[0]), 0);
	}
	if (result == -1)
	{
		std::cout << "Turbojpeg jpeg2yuv " << tjGetErrorStr() << std::endl;
	}
	yuv_subsampling = jpegSubsamp;
	_yuv_converted = true;
}

void IR_Frame::yuv2bgr()
{
	/*int channels = 3;
	int result;
	_bgr_buffer = */
}