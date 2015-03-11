#include "microscope.h"

#include <cmath>

static double distance(cv::Vec3f from, cv::Vec3f to) {
    return std::sqrt(
                (from[0]-to[0]) * (from[0]-to[0]) + (from[1]-to[1]) * (from[1]-to[1])
    );
}

static double average(const std::vector<double> &v) {
    double d = 0;
    for (size_t i=0; i<v.size(); ++i) d += v[i];
    return d / v.size();
}
static double stddev(const std::vector<double> &v, double mean=NAN) {
    if (isnan(mean)) mean = average(v);

    double d = 0;
    for (size_t i=0; i<v.size(); ++i) d += (v[i]-mean) * (v[i]-mean);
    return std::sqrt(d / v.size());
}

void microscope_calibration::recalibrate(const std::vector<cv::Vec3f> &dots, double dist) {
    if (!dots.size()) return;
    dbgDots = dots;

    // Gather some statistics
    double minx = dots[0][0];
    double maxx = dots[0][0];
    double miny = dots[0][1];
    double maxy = dots[0][1];

    for (size_t i=1; i!=dots.size(); ++i) {
        if (minx > dots[i][0]) minx = dots[i][0];
        if (maxx < dots[i][0]) maxx = dots[i][0];
        if (miny > dots[i][1]) miny = dots[i][1];
        if (maxy < dots[i][1]) maxy = dots[i][1];
    }

    // Detect zoom range
    if (dots.size() >= 20) { // High node count = low zoom.

        // Find neighbors
        std::vector<neighbors_t> neighbors(dots.size());
        for (size_t j=0; j!=neighbors.size(); ++j) {
            double x = dots[j][0];
            double y = dots[j][1];
            for (size_t i=0; i!=dots.size(); ++i) {
                if (i==j) continue;

                double nx = dots[i][0];
                double ny = dots[i][1];

                // Up neighbor
                if (ny < y && abs(nx-x) < abs(ny-y)/2) {
                    if (neighbors[j].up == -1) {
                        neighbors[j].up = i;
                    } else if (distance(dots[j], dots[i]) < distance(dots[j], dots[neighbors[j].up])) {
                        neighbors[j].up = i;
                    }
                }

                // Down neighbor
                if (ny > y && abs(nx-x) < abs(ny-y)/2) {
                    if (neighbors[j].down == -1) {
                        neighbors[j].down = i;
                    } else if (distance(dots[j], dots[i]) < distance(dots[j], dots[neighbors[j].down])) {
                        neighbors[j].down = i;
                    }
                }

                // Left neighbor
                if (nx < x && abs(ny-y) < abs(nx-x)/2) {
                    if (neighbors[j].left == -1) {
                        neighbors[j].left = i;
                    } else if (distance(dots[j], dots[i]) < distance(dots[j], dots[neighbors[j].left])) {
                        neighbors[j].left = i;
                    }
                }

                // Right neighbor
                if (nx > x && abs(ny-y) < abs(nx-x)/2) {
                    if (neighbors[j].right == -1) {
                        neighbors[j].right = i;
                    } else if (distance(dots[j], dots[i]) < distance(dots[j], dots[neighbors[j].right])) {
                        neighbors[j].right = i;
                    }
                }
            }
        }

        // Statistical analisys of dot distances
        double dx_avg, dy_avg, dx_stddev, dy_stddev;

        // Determine distances
        std::vector<double> dist_h, dist_v;
        for (size_t i=0; i<dots.size(); ++i) {
            if (neighbors[i].left  != -1) dist_h.push_back(fabs(dots[neighbors[i].left ][0] - dots[i][0]));
            if (neighbors[i].right != -1) dist_h.push_back(fabs(dots[neighbors[i].right][0] - dots[i][0]));
            if (neighbors[i].up    != -1) dist_v.push_back(fabs(dots[neighbors[i].up   ][1] - dots[i][1]));
            if (neighbors[i].down  != -1) dist_v.push_back(fabs(dots[neighbors[i].down ][1] - dots[i][1]));
        }

        // Now average
        dx_avg = average(dist_h);
        dy_avg = average(dist_v);

        // Then calculate deviation
        dx_stddev = stddev(dist_h, dx_avg);
        dy_stddev = stddev(dist_v, dy_avg);

        // Discard bad samples (2*stddev away from mean)
        for (size_t i=0; i<dist_h.size(); ++i) {
            if (fabs(dist_h[i] - dx_avg) > 2*dx_stddev) {
                dist_h.erase(dist_h.begin()+i);
                --i;
            }
        }
        for (size_t i=0; i<dist_v.size(); ++i) {
            if (fabs(dist_v[i] - dy_avg) > 2*dy_stddev) {
                dist_v.erase(dist_v.begin()+i);
                --i;
            }
        }

        // Discard bad neighbor connections (for debugging / graphing only)
        for (size_t i=0; i<dots.size(); ++i) {
            if (neighbors[i].left  != -1) if (fabs(dots[neighbors[i].left ][0] - dots[i][0]) > 2*dx_stddev) neighbors[i].left  = -1;
            if (neighbors[i].right != -1) if (fabs(dots[neighbors[i].right][0] - dots[i][0]) > 2*dx_stddev) neighbors[i].right = -1;
            if (neighbors[i].up    != -1) if (fabs(dots[neighbors[i].up   ][1] - dots[i][1]) > 2*dy_stddev) neighbors[i].up    = -1;
            if (neighbors[i].down  != -1) if (fabs(dots[neighbors[i].down ][1] - dots[i][1]) > 2*dy_stddev) neighbors[i].down  = -1;
        }

        // Now redo average
        dx_avg = average(dist_h);
        dy_avg = average(dist_v);

        // Pixel aspect ratio (horizontal / vertical)
        double pixel_aspect = dx_avg / dy_avg;

        // Find center node
        cv::Vec3f center(2);
        center[0] = (maxx + minx) / 2;
        center[1] = (maxy + miny) / 2;
        int centernode = 0;
        for (size_t i=1; i!=dots.size(); ++i) {
            if (distance(center, dots[centernode]) > distance(center, dots[i])) centernode = i;
        }

        // Save center node as reference
        ref_x = dots[centernode][0];
        ref_y = dots[centernode][1];

        // Find center-left node
        int center_left = centernode;
        while (neighbors[center_left].left != -1) center_left = neighbors[center_left].left;
        dbgCenterLeft = cv::Point(dots[center_left][0], dots[center_left][1]);

        // Find center-right node
        int center_right = centernode;
        while (neighbors[center_right].right != -1) center_right = neighbors[center_right].right;
        dbgCenterRight = cv::Point(dots[center_right][0], dots[center_right][1]);

        // Find tilt
        tilt = std::atan2(
                (dots[center_right][1] - dots[center_left][1]) * pixel_aspect, // Y
                (dots[center_right][0] - dots[center_left][0])                 // X
        );

        // Pixel dimensions (mm / pixel)
        kx = dist * std::cos(tilt) / dx_avg;
        ky = dist * std::cos(tilt) / dy_avg;

        // Save neighbors list for debugging / graphing
        dbgNeighbors = neighbors;

    } else { // Low node count = high zoom
        // Find largest 2 nodes
        size_t largest = 0;
        size_t large   = 1;
        if (dots[0][2] < dots[1][2]) {
            largest = 1;
            large   = 0;
        }
        for (size_t i=2; i<dots.size(); ++i) {
            if(dots[i][2] > dots[largest][2]) {
                large = largest;
                largest = i;
                continue;
            }
            if(dots[i][2] > dots[large][2]) {
                large = i;
                continue;
            }
        }

        double dx = dots[largest][0] - dots[large][0];
        double dy = dots[largest][1] - dots[large][1];
        double dist_px = sqrt(dx*dx + dy*dy);

        // Pixel dimensions (mm / pixel)
        kx = dist / dist_px;
        ky = dist / dist_px;

        // Set the only neighboring relation of interest, just for debugging
        dbgNeighbors.clear();
        dbgNeighbors.resize(dots.size());
        dbgNeighbors[large].right = largest;
    }

}

cv::Point microscope_calibration::getNode(double dx, double dy) {
    return cv::Point(
                ref_x + dx / kx * std::cos(tilt) - dy / ky * std::sin(tilt),
                ref_y + dx / kx * std::sin(tilt) + dy / ky * std::cos(tilt)
    );
}

void microscope_calibration::drawNeighbors(cv::Mat &img, cv::Scalar color) {
    for (size_t i = 0; i != dbgDots.size(); ++i) {
        cv::Point dot(dbgDots[i][0], dbgDots[i][1]);
        if (dbgNeighbors[i].up != -1)
            cv::line(img, dot, cv::Point(dbgDots[dbgNeighbors[i].up   ][0], dbgDots[dbgNeighbors[i].up   ][1]), color);
        if (dbgNeighbors[i].down != -1)
            cv::line(img, dot, cv::Point(dbgDots[dbgNeighbors[i].down ][0], dbgDots[dbgNeighbors[i].down ][1]), color);
        if (dbgNeighbors[i].left != -1)
            cv::line(img, dot, cv::Point(dbgDots[dbgNeighbors[i].left ][0], dbgDots[dbgNeighbors[i].left ][1]), color);
        if (dbgNeighbors[i].right != -1)
            cv::line(img, dot, cv::Point(dbgDots[dbgNeighbors[i].right][0], dbgDots[dbgNeighbors[i].right][1]), color);
    }
}
