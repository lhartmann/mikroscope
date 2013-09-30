#ifndef CV_CENTER_OF_MASS_H
#define CV_CENTER_OF_MASS_H

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

cv::Vec3f find_center_of_mass(cv::Mat &in, cv::Point p, uchar replace_with);
void find_center_of_mass(cv::Mat &in, std::vector<cv::Vec3f> &res);

#endif // CV_CENTER_OF_MASS_H
