#ifndef MICROSCOPE_H
#define MICROSCOPE_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

struct microscope_calibration {
    struct neighbors_t {
        int up, down, left, right;
        neighbors_t() : up(-1), down(-1), left(-1), right(-1) {}
    };

    double kx; // mm/pixel in X direction
    double ky; // mm/pixel in Y direction

    // First reference marker (closest to center of screen)
    double ref_x;
    double ref_y;
    double tilt; // Tilt angle of calibration grid

    cv::Point dbgCenterRight;
    cv::Point dbgCenterLeft;
    std::vector<cv::Vec3f> dbgDots;
    std::vector<neighbors_t> dbgNeighbors;

    microscope_calibration() : kx(8./640), ky(6./480), ref_x(0), ref_y(0), tilt(0) {}
    void recalibrate(const std::vector<cv::Vec3f> &dots, double dist=1);
    cv::Point getNode(double dx, double dy);
    void drawNeighbors(cv::Mat &img, cv::Scalar color);
};

#endif // MICROSCOPE_H
