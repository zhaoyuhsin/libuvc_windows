
#include <winsock2.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "uvc_back.h"
using namespace cv;
using namespace std;
int main(int argc, char** argv) {
    // libuvc doc: https://ken.tossell.net/libuvc/doc/
    uvc_context_t* ctx;
    uvc_error_t res;
    uvc_device_t* dev;

    /* Initialize a UVC service context. Libuvc will set up its own libusb
    * context. Replace NULL with a libusb_context pointer to run libuvc
    * from an existing libusb context. */
    res = uvc_init(&ctx, NULL);

    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    puts("UVC initialized");

    puts("UVC initialized");
    uvc_device** device_list;
    res = uvc_get_device_list(ctx, &device_list);
    printf("uvc_get_device_list() Status : %d\n", res);
    int idx = 0;
    for (;;) {
        dev = device_list[idx];
        if (dev == NULL) break;
        uvc_device_descriptor_t* desc;
        if (uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS) {
            printf("--------------------------- %d ----------------------------------\n", idx);
            printf("Vendor ID:%d Product ID:%d\n", desc->idVendor, desc->idProduct);
            std::cout << desc->bcdUVC << std::endl;
            if (desc->serialNumber != NULL) printf("serialNumber:%s\n", desc->serialNumber);
            if (desc->manufacturer != NULL) printf("manufacturer:%s\n", desc->manufacturer);
            if (desc->product != NULL) printf("product:%s\n", desc->product);
            printf("device_address: %d\n", uvc_get_device_address(dev));
            printf("bus_number: %d\n", uvc_get_bus_number(dev));
            ++idx;
            printf("-----------------------------------------------------------------\n");
        }
        else break;
    }
    printf("Total devices : %d\n", idx);

    IR_Capture cap;
    cap._start(device_list[0], 640, 240, 30);
    int cnt = 0;
    while (++cnt < 3000) {
        IR_Frame out_frame;
        cap.get_frame(out_frame, 0);
        Mat img;
        out_frame.gray(img);
        imshow("img", img);
        waitKey(1);
    }
    return 0;
}