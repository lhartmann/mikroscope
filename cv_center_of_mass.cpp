#include "cv_center_of_mass.h"

cv::Vec3f find_center_of_mass(cv::Mat &in, cv::Point p, uchar replace_with) {
    cv::Vec3f ret;
    if (!in.data) return ret;
    if (in.channels() != 1) return ret;

    // What color am I looking for
    uchar color = in.data[p.x + p.y*in.step];

    // Look for it
    u_int64_t XX=0, YY=0, NN=0;
    std::vector<cv::Point> vp;
    vp.reserve(in.size[0] * in.size[1]);
    vp.push_back(p);
    for (size_t i=0; i!=vp.size(); ++i) {
        int x = vp[i].x;
        int y = vp[i].y;

        // Ignore point if out of bounds
        if (x<0 || x>=in.size[1] || y<0 || y>=in.size[0]) continue;

        // Ignore point if bad-colored
        if (in.data[x + y*in.step] != color) continue;
        in.data[x + y*in.step] = replace_with;

        // Process point
        NN++;
        XX += x;
        YY += y;
        vp.push_back(cv::Point(x-1, y));
        vp.push_back(cv::Point(x+1, y));
        vp.push_back(cv::Point(x, y-1));
        vp.push_back(cv::Point(x, y+1));

        if (vp.size() > 300000) break;
    }
    ret[0] = double(XX) / NN;
    ret[1] = double(YY) / NN;
    ret[2] = std::sqrt(NN/M_PI); // Radius of the circle with equivalent area
    return ret;
}

void find_center_of_mass(cv::Mat &in, std::vector<cv::Vec3f> &res) {
    res.clear();
    uchar replace_with = 254;
    for (int y=0; y<in.size[0]; y+=10) {
        for (int x=0; x<in.size[1]; x+=10) {
            if (in.data[x + y*in.step] == 255) {
                cv::Point p(x,y);
                res.push_back(find_center_of_mass(in, p, replace_with));
//                replace_with --;
            }
        }
    }

}
