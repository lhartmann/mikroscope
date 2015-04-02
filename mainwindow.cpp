#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QPainter>
#include <vector>
#include <cmath>
#include "cv_center_of_mass.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "mikroscope_ann_cal_builder.h"

using std::pow;
using std::sqrt;
using std::tanh;

//
void nn_eval(const double *X, double *Y) {
    // Layer 1
    double S1[5];
    S1[0] = tanh( + 1.713924 * X[0] + -0.116803 * X[1] + 0.674870 * X[2]);
    S1[1] = tanh( + 2.060522 * X[0] + -0.279501 * X[1] + -5.946272 * X[2]);
    S1[2] = tanh( + -2.628955 * X[0] + -1.841684 * X[1] + 0.389430 * X[2]);
    S1[3] = tanh( + 0.882897 * X[0] + -0.400442 * X[1] + -2.832571 * X[2]);
    S1[4] = tanh( + 2.998230 * X[0] + -0.496496 * X[1] + -3.344282 * X[2]);

    // Layer 2:
    double S2[3];
    S2[0] = tanh( + 10.821479 * S1[0] + -0.052867 * S1[1] + 3.068331 * S1[2] + 3.574551 * S1[3] + -1.717229 * S1[4]);
    S2[1] = tanh( + 1.152181 * S1[0] + 0.695545 * S1[1] + 1.952370 * S1[2] + -2.196553 * S1[3] + -3.056166 * S1[4]);
    S2[2] = tanh( + 10.806802 * S1[0] + -0.464361 * S1[1] + 3.109297 * S1[2] + 10.143413 * S1[3] + -0.045101 * S1[4]);
    S2[3] = tanh( + -6.483999 * S1[0] + -0.193463 * S1[1] + -1.335866 * S1[2] + -13.256281 * S1[3] + 12.382402 * S1[4]);

    // Layer 3:
    double S3[0];
    S3[0] = tanh( + -14.202525 * S2[0] + -2.836225 * S2[1] + 1.979521 * S2[2] + 15.710655 * S2[3]);

    Y[0] = S3[0];
}
//

double img_avg(cv::Mat &imgin, int channel) {
    quint64 acc = 0;
    for (int r=0; r<imgin.size[0]; ++r) { // Iterate Rows
        for (int c=0; c<imgin.size[1]; ++c) { // Iterate Columns
            acc += imgin.data[r*imgin.step + c*imgin.channels() + channel];
        }
    }
    return double(acc) / imgin.size[0] / imgin.size[1];
}

double img_dev(cv::Mat &imgin, int channel, double mean) {
    double acc = 0;
    for (int r=0; r<imgin.size[0]; ++r) { // Iterate Rows
        for (int c=0; c<imgin.size[1]; ++c) { // Iterate Columns
            acc += pow(imgin.data[r*imgin.step + c*imgin.channels() + channel] - mean, 2);
        }
    }
    return sqrt(acc / imgin.size[0] / imgin.size[1]);
}

void copperdetect(cv::Mat &imgin) {
    cv::Mat m2;
    cvtColor(imgin, m2, CV_BGR2HSV);
    cv::blur(m2,m2,cv::Size(5,5));

    double avgr = img_avg(m2, 0);
    double devr = img_dev(m2, 0, avgr);
    double avgg = img_avg(m2, 1);
    double devg = img_dev(m2, 1, avgg);

    for (int r=0; r<imgin.size[0]; ++r) { // Iterate Rows
        for (int c=0; c<imgin.size[1]; ++c) { // Iterate Columns
            unsigned red, green, blue, out;

            red   = m2.data[r*m2.step + c*m2.channels() + 0];
            green = m2.data[r*m2.step + c*m2.channels() + 1];
            blue  = m2.data[r*m2.step + c*m2.channels() + 2];

            // Test 0: Artificial Neural networks
            if (false) {
                double X[3];
                X[0] = red   / 255.;
                X[1] = green / 255.;
                X[2] = blue  / 255.;
                double Y[1];
                nn_eval(X,Y);

                out = (Y[0]+1) * 255. / 2;
            }

            // Test a: Just show channels isolated
            if (false) {
                out = r < 160 ? red :
                      r < 320 ? green:
                                blue ; //255 * (Y[0] + 1) / 2;
            }


            // Test 1: Standard deviation and sigmoid curve
            if (false) {
                double O = (red-avgr) / devr - (green-avgg) / devg;
                O = 1 / (1 + exp(-O*5));
                out = O * 255.;
            }

            // Test 2: boolean mean comparision
            if (false) {
                if (green > avgg) out = red > avgr ? 128 : 255;
                else              out = red > avgr ?   0 : 128;
            }

            // Test 3: Boolean mean + linear uncertainity regions
            if (false) {
                if      (green > avgg && red < avgr) out = 255;
                else if (green < avgg && red > avgr) out = 0;
                else {
                    double O = -(red-avgr) / devr + (green-avgg) / devg;
                    O = fmin(1,fmax(-1,O));
                    out = (O+1) /2 * 255.;
                }
            }

            // Test 4: Boolean mean + sigmoid uncertainty
            if (true) {
                if      (green > avgg && red < avgr) out = 255;
                else if (green < avgg && red > avgr) out = 0;
                else {
                    double O = -(red-avgr) / devr + (green-avgg) / devg;
                    O = 1 / (1 + exp(-O*7));
                    out = O * 255.;
                }
            }

            imgin.data[r*imgin.step + c*imgin.channels() + 0] = out;
            imgin.data[r*imgin.step + c*imgin.channels() + 1] = out;
            imgin.data[r*imgin.step + c*imgin.channels() + 2] = out;
        }
    }
}

double dominant_line_angle(std::vector<cv::Vec4i> lines) {
    size_t nsets = 180;
    double best = 0;
    size_t besttheta = 0;

    // Try several possible angles
    for (size_t set=0; set<nsets; ++set) {
        double thetaref = M_PI * set / nsets;

        // versor for current angle
        double Xr = cos(thetaref);
        double Yr = sin(thetaref);

        // Accumulate how good this angle is by comparing to all lines
        double goodness = 0;
        for (size_t i=0; i<lines.size(); ++i) {
            cv::Vec4i l = lines[i];

            double dx = l[2] - l[0];
            double dy = l[3] - l[1];

            // Use dot product as an indicator of good alignment
            // Longer lines get a greater weight.
            double g  = Xr * dx + Yr * dy;

            // Accumulate goodness squared to ignore sign.
            goodness += g*g;
        }

        if (goodness > best) {
            best = goodness;
            besttheta = thetaref;
        }
    }

    return besttheta;
}

// Rotate (x,y) around around (x0,y0)
void rotate(double theta, double x0, double y0, double &x, double &y) {
    double dx = x-x0;
    double dy = y-y0;

    double Ct = cos(theta);
    double St = sin(theta);

    double tdx = Ct * dx - St * dy;
    double tdy = St * dx + Ct * dy;

    x = x0 + tdx;
    y = y0 + tdy;
}

double measure_dist(cv::Vec4d l1, cv::Vec4d l2, double theta, cv::Vec4i *measureline=0) {
    // Transform lines, align lines to X. Distance measurement is taken over Y.
    rotate(-theta, 0, 0, l1[0], l1[1]);
    rotate(-theta, 0, 0, l1[2], l1[3]);
    rotate(-theta, 0, 0, l2[0], l2[1]);
    rotate(-theta, 0, 0, l2[2], l2[3]);

    // Make sure leftmost point is first
    if (l1[0] > l1[2]) {
        double t;
        t=l1[0], l1[0]=l1[2], l1[2]=t;
        t=l1[1], l1[1]=l1[3], l1[3]=t;
    }
    if (l2[0] > l2[2]) {
        double t;
        t=l2[0], l2[0]=l2[2], l2[2]=t;
        t=l2[1], l2[1]=l2[3], l2[3]=t;
    }

    // Find the center of the overlapping region
    double left  = std::max(l1[0], l2[0]);
    double right = std::min(l1[2], l2[2]);
    double x0 = (left + right) / 2;

    // Measure distance...
    double y1 = l1[1] + (l1[3]-l1[1]) * (x0-l1[0]) / (l1[2]-l1[0]);
    double y2 = l2[1] + (l2[3]-l2[1]) * (x0-l2[0]) / (l2[2]-l2[0]);
    double dy = fabs(y2-y1);

    // Create measuring line for display purposes
    if (measureline) {
        // (x0,y1) to (x0,y2)
        double xa = x0;
        double ya = y1;
        double xb = x0;
        double yb = y2;

        // Now translate back to the image coordinates
        rotate(theta, 0, 0, xa, ya);
        rotate(theta, 0, 0, xb, yb);

        (*measureline)[0] = xa + 0.5;
        (*measureline)[1] = ya + 0.5;
        (*measureline)[2] = xb + 0.5;
        (*measureline)[3] = yb + 0.5;
    }

    return dy;
}

static void crosshair(cv::Mat &img, cv::Point p, cv::Size sz, cv::Scalar color) {
    cv::line(img, cv::Point(p.x-sz.width/2, p.y), cv::Point(p.x+sz.width/2, p.y), color);
    cv::line(img, cv::Point(p.x, p.y-sz.height/2), cv::Point(p.x, p.y+sz.height/2), color);
}

void MainWindow::drawRuler(cv::Mat &img, cv::Scalar color, double kx, double ky) {
    int x0 = img.size[1] / 2;
    int y0 = img.size[0] / 2;
    double wmm = img.size[1] * kx;
    double hmm = img.size[0] * ky;

    // Decides which rulers to draw
    enum ruler_e {
        rmX=0,
        rmXY,
        rmMillCalibrationPatterns,
        rmOff,
        rmParallelLines,
        rmCopperDetector
    };
    bool drawXruler = true; // X ruler defaults to true
    bool drawYruler = false;
    bool drawMillCalibrationPatterns = false;
    bool drawParallel = false;
    bool drawCopper = false;

    switch (ui->cbRuler->currentIndex()) {
    case rmXY:
        drawYruler = true;
        break;

    case rmMillCalibrationPatterns:
        drawMillCalibrationPatterns = true;
        break;

    case rmParallelLines:
        drawParallel = true;
        break;

    case rmOff:
        drawXruler = false;
        break;

    case rmCopperDetector:
        drawCopper = true;
        drawParallel = true;
        break;
    }

    //
    cv::Mat m2 = img.clone();
    if (drawCopper) {
        copperdetect(m2);
        img = m2.clone();
    }

    //////////////////////////////
    /// Parallel line analysis ///
    //////////////////////////////
    if (drawParallel) {
        std::vector<cv::Vec4i> lines;
        cv::Mat dst;
        cv::Canny(m2, dst, 20, 235, 3);
//        cv::cvtColor(dst,img,CV_GRAY2BGR);
        cv::HoughLinesP(dst, lines, 1, CV_PI/180, 50, 320, 50); // rho theta thr srn stn
        for (size_t i = 0; i < lines.size(); i++) {
          cv::Vec4i l = lines[i];
          cv::line(img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,0,255), 1, CV_AA);
        }

        // Find dominant angle
        double theta = dominant_line_angle(lines);
        cv::Vec4i l;
        l[0] = 320 - 100 * cos(theta);
        l[1] = 240 - 100 * sin(theta);
        l[2] = 320 + 100 * cos(theta);
        l[3] = 240 + 100 * sin(theta);
        cv::line(img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,255,255), 1, CV_AA);
    }

    //////////////
    /// X Axis ///
    //////////////
    if (drawXruler) {
        // Draw 0.1mm ticks
        for (int i = -wmm/0.1; i<=+wmm/0.1; ++i) {
            int x = x0 + i*0.1/kx + 0.5;
            cv::line(img, cv::Point(x, y0-5), cv::Point(x, y0+5), color);
        }

        // Draw 0.5mm ticks
        for (int i = -wmm/0.5; i<=+wmm/0.5; ++i) {
            int x = x0 + i*0.5/kx + 0.5;
            cv::line(img, cv::Point(x, y0-10), cv::Point(x, y0+10), color);
        }

        // Draw 1.0mm ticks
        for (int i = -wmm/1.0; i<=+wmm/1.0; ++i) {
            int x = x0 + i*1.0/kx + 0.5;
            cv::line(img, cv::Point(x, y0-15), cv::Point(x, y0+15), color);
        }

        // Draw X axis
        cv::line(img, cv::Point(0,y0), cv::Point(2*x0+5,y0), color);
    }

    //////////////
    /// Y Axis ///
    //////////////

    if (drawYruler) {
        // Draw 0.1mm ticks
        for (int i = -hmm/0.1; i<=+hmm/0.1; ++i) {
            int y = y0 + i*0.1/ky + 0.5;
            cv::line(img, cv::Point(x0-5, y), cv::Point(x0+5, y), color);
        }

        // Draw 0.5mm ticks
        for (int i = -hmm/0.5; i<=+hmm/0.5; ++i) {
            int y = y0 + i*0.5/ky + 0.5;
            cv::line(img, cv::Point(x0-10, y), cv::Point(x0+10, y), color);
        }

        // Draw 1.0mm ticks
        for (int i = -hmm/1.0; i<=+hmm/1.0; ++i) {
            int y = y0 + i*1.0/ky + 0.5;
            cv::line(img, cv::Point(x0-15, y), cv::Point(x0+15, y), color);
        }

        // Draw Y axis
        cv::line(img, cv::Point(x0, 0), cv::Point(x0, 2*y0+5), color);
    }

    ///////////////////////////
    /// Calibration markers ///
    ///////////////////////////

    if (drawMillCalibrationPatterns) {
        // 0.2mm or 8mil
        int w_0p2 = 0.2/kx + 0.5;
        cv::line(img, cv::Point(x0/2 - w_0p2/2,         y0/2-5), cv::Point(x0/2 - w_0p2/2,         y0/2+5), color);
        cv::line(img, cv::Point(x0/2 - w_0p2/2 + w_0p2, y0/2-5), cv::Point(x0/2 - w_0p2/2 + w_0p2, y0/2+5), color);
        cv::line(img, cv::Point(x0/2 - w_0p2/2        , y0/2  ), cv::Point(x0/2 - w_0p2/2 + w_0p2, y0/2  ), color);

        // 0.25mm
        int w_0p25 = 0.25/kx + 0.5;
        cv::line(img, cv::Point(x0 - w_0p25/2,          y0/2-5), cv::Point(x0 - w_0p25/2,          y0/2+5), color);
        cv::line(img, cv::Point(x0 - w_0p25/2 + w_0p25, y0/2-5), cv::Point(x0 - w_0p25/2 + w_0p25, y0/2+5), color);
        cv::line(img, cv::Point(x0 - w_0p25/2,          y0/2  ), cv::Point(x0 - w_0p25/2 + w_0p25, y0/2  ), color);

        // 0.3mm or 12mil
        int w_0p3 = 0.3/kx + 0.5;
        cv::line(img, cv::Point(3*x0/2 - w_0p3/2,         y0/2-5), cv::Point(3*x0/2 - w_0p3/2,         y0/2+5), color);
        cv::line(img, cv::Point(3*x0/2 - w_0p3/2 + w_0p3, y0/2-5), cv::Point(3*x0/2 - w_0p3/2 + w_0p3, y0/2+5), color);
        cv::line(img, cv::Point(3*x0/2 - w_0p3/2,         y0/2  ), cv::Point(3*x0/2 - w_0p3/2 + w_0p3, y0/2  ), color);

    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    failedFramesCounter(0),
    calibrationTriggered(false)
{
    pCam = 0;
    ui->setupUi(this);
    ui->mainToolBar->setVisible(false);
    ui->statusBar->setVisible(false);
    ui->lblOriginal->setText("");
    showMaximized();

    // Detect and list video devices (linux specific)
    if (true) {
        QDirIterator dir("/dev/", QDir::System);
        QRegExp re("/dev/(video[0-9]+)");
        while (dir.hasNext()) {
            QString s = dir.next();
            if (re.exactMatch(s))
                ui->cbCameras->addItem(re.cap(1));
        }
    }

    // Add current working folder images for testing
    if (true) {
        qDebug() << "Loading images...";
        qDebug() << "  Current Directory: " << QDir::currentPath();
        QDirIterator dir(QDir::currentPath());
        QRegExp re(".*png");
        re.setCaseSensitivity(Qt::CaseInsensitive);
        while (dir.hasNext()) {
            QString s = dir.next();
            qDebug() << "  Image: " << s << "\n";
            if (re.exactMatch(s))
                ui->cbCameras->addItem(re.cap(0));
        }
    }
    on_cbCameras_currentIndexChanged(ui->cbCameras->currentText());

    // Create ann calibration data, if cal files are found
    if (mikroscope_ann_cal_builder("ann_cal_in.png", "ann_cal_out.png", "ann_cal_in.txt", "ann_cal_out.txt")) {
        // Maybe alert that calibration data was created?
        qDebug("ANN Calibration data created successfully.");
    } else {
        qDebug("ANN Calibration data creation failed/skipped.");
    }
}

void MainWindow::process_image() {
    if (cam.isOpened()) {
        cam.grab();
        cam.grab();
        cam.read(matOriginal);
    } else if (pCam) {
        matOriginal = pCam->clone();
    } else {
        // Error: no image source.
        timer.stop();
        return;
    }

    if (matOriginal.empty()) {
        ui->statusBar->showMessage("Failed grabbing frame (" + QString::number(++failedFramesCounter) + ").");
        ui->statusBar->setVisible(true);
        return;
    }

    if (calibrationTriggered) {
        // Convert color to RGB then blur
        cv::cvtColor(matOriginal, matOriginal, CV_BGR2RGB);
        cv::GaussianBlur(matOriginal, matOriginal, cv::Size(9,9), 2);

        // Convert blurred image to HSV, then check range
        cv::cvtColor(matOriginal, matOriginal, CV_RGB2HSV);
        cv::inRange(matOriginal, cv::Scalar(0,0,0), cv::Scalar(255,80,80), matOriginal);

        // Remove noise
        //cv::GaussianBlur(matInRange, matInRangeBlur, cv::Size(9,9), 2);
        cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5,5));
        cv::erode(matOriginal, matOriginal, erodeElement);
        cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(13,13));
        cv::dilate(matOriginal, matOriginal, dilateElement);

        // Find circles
/*        cv::HoughCircles(matOriginal, vecCircles, CV_HOUGH_GRADIENT, 2,
                         matOriginal.rows / 20, 200, 50,
                         matOriginal.rows / 20,
                         matOriginal.rows / 5); */
        find_center_of_mass(matOriginal, vecCircles);

        // Convert back to rgb
//        cv::threshold(matOriginal, matOriginal, 128, 255/4, cv::THRESH_BINARY);
        for (uchar *p=matOriginal.data; p!=matOriginal.dataend; ++p)
            *p = 255-*p/4;
        cv::cvtColor(matOriginal, matOriginal, CV_GRAY2RGB);

        // Draw circles
        for (auto i = vecCircles.begin(); i != vecCircles.end(); ++i) {
            cv::circle(matOriginal, cv::Point((*i)[0], (*i)[1]), (*i)[2], cv::Scalar(255,0,0));
        }

        cal.recalibrate(vecCircles);

        // Draw crosshair for calibration
        cal.drawNeighbors(matOriginal, cv::Scalar(128,0,0));
        for (int dx=-10; dx<=10; ++dx) {
            for (int dy=-10; dy<=10; ++dy) {
                crosshair(matOriginal, cal.getNode(dx,dy), cv::Size(10,10), cv::Scalar(0,128,128));
            }
        }
        crosshair(matOriginal, cal.dbgCenterLeft,  cv::Size(10,10), cv::Scalar(0,255,255));
        crosshair(matOriginal, cal.dbgCenterRight, cv::Size(10,10), cv::Scalar(255,255,0));
    } else {
        // Convert color
        cv::cvtColor(matOriginal, matOriginal, CV_BGR2RGB);

        // Draw Ruler
        drawRuler(matOriginal, cv::Scalar(255,0,0), cal.kx, cal.ky);
    }

    // Update display
    imgOriginal  = QImage((uchar*)matOriginal.data,  matOriginal.cols,  matOriginal.rows,  matOriginal.step,  QImage::Format_RGB888);

//    ui->lblOriginal->setPixmap(QPixmap::fromImage(imgOriginal));
    repaint();
}

MainWindow::~MainWindow()
{
    timer.stop();
    if (pCam) delete pCam, pCam=0;
    delete ui;
}

void MainWindow::on_btnCalibrate_clicked()
{
    calibrationTriggered = !calibrationTriggered;
    if (calibrationTriggered)
        ui->btnCalibrate->setText("Accept &Calibration");
    else
        ui->btnCalibrate->setText("Start &Calibration");
}

void MainWindow::on_cbCameras_currentIndexChanged(const QString &arg1)
{
    timer.stop();
    if (cam.isOpened()) cam.release();
    if (pCam) delete pCam, pCam=0;

    qDebug() << "Opening image souce: " << arg1;

    QRegExp reCamera("video([0-9]+)");
    if (reCamera.exactMatch(arg1)) {
        int id = reCamera.cap(1).toInt();

        cam.open(id);

        if (!cam.isOpened()) {
            ui->statusBar->setVisible(true);
            ui->statusBar->showMessage("Failed to access camera device "+arg1+".");
            timer.stop();
            return;
        }

        cam.set(CV_CAP_PROP_FRAME_WIDTH,  1920);
        cam.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);
    }

    QRegExp reImage(".*png");
    if (reImage.exactMatch(arg1)) {
        pCam = new cv::Mat();
        *pCam = cv::imread(arg1.toStdString().c_str());
    }

    timer.setInterval(1000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(process_image()));
    timer.start();
}

QRect MainWindow::place_image(QRect image, QRect displayport) {
    quint32 x,y,w,h;
    if (image.width() * displayport.height() > displayport.width() * image.height()) {
        h = displayport.width() * quint64(image.height()) / image.width();
        w = displayport.width();
    } else {
        h = displayport.height();
        w = displayport.height() * quint64(image.width()) / image.height();
    }

    //
    x = displayport.x() + ( displayport.width()  - w ) / 2;
    y = displayport.y() + ( displayport.height() - h ) / 2;

    return QRect(x,y,w,h);
}

void MainWindow::paintEvent(QPaintEvent *) {
    QPainter pnt(this);
    QRect image = imgOriginal.rect();
    if (image.isEmpty()) return;
    QRect port  = place_image(image, ui->lblOriginal->geometry());

    pnt.drawImage(port, imgOriginal, image);
}

void MainWindow::on_btnSnapshot_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, "Save Snapshot As");
    if (file.isEmpty()) return;

    imgOriginal.save(file);
}
