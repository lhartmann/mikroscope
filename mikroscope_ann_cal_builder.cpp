#include "mikroscope_ann_cal_builder.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <fstream>
#include <iomanip>
#include <QDebug>

using namespace std;
using namespace cv;

bool mikroscope_ann_cal_builder(const char *inputImage, const char *outputImage, const char *inputData, const char *outputData) {
    Mat imgin  = imread(inputImage);
    Mat imgout = imread(outputImage);

    // Error checks
    if (!imgin.data || !imgout.data) {
        qDebug("ANN: Failed toopen one of the image files.");
        return false;
    }
    if (imgin.size[0] != imgout.size[0] || imgin.size[1] != imgout.size[1]) {
        qDebug("ANN: Reference images must be same size.");
        return false;
    }
    if (imgin.channels() != 3) {
        qDebug("ANN: Input image must be RGB (3 channels).");
        return false;
    }
    cvtColor(imgin, imgin, CV_BGR2HSV);
    cvtColor(imgout, imgout, CV_BGR2GRAY);
    if (imgout.channels() != 1) {
        qDebug("ANN: Output image must be grayscale (1 channel only).");
        return false;
    }

    blur(imgin,imgin,Size(5,5));
    blur(imgout,imgout,Size(5,5));

    //
    ofstream idf(inputData,  ios::out|ios::trunc | ios::binary);
    ofstream odf(outputData, ios::out|ios::trunc | ios::binary);

    qDebug("ANN: All files loaded or created, processing...");
    for (int r=0; r<imgin.size[0]; ++r) { // Iterate Rows
        for (int c=0; c<imgin.size[1]; ++c) { // Iterate Columns
            unsigned red, green, blue, out;

            red   = imgin.data[r*imgin.step + c*imgin.channels() + 0];
            green = imgin.data[r*imgin.step + c*imgin.channels() + 1];
            blue  = imgin.data[r*imgin.step + c*imgin.channels() + 2];
            out   = imgout.data[r*imgout.step + c*imgout.channels()];

            idf << red << ' ' << green << ' ' << blue << endl;
            odf << out << endl;
        }
    }
    qDebug("ANN: Done.");
    return true;
}
