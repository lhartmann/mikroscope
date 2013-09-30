#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QDir>
#include <QDirIterator>
#include <QPainter>
#include "cv_center_of_mass.h"

static void crosshair(cv::Mat &img, cv::Point p, cv::Size sz, cv::Scalar color) {
    cv::line(img, cv::Point(p.x-sz.width/2, p.y), cv::Point(p.x+sz.width/2, p.y), color);
    cv::line(img, cv::Point(p.x, p.y-sz.height/2), cv::Point(p.x, p.y+sz.height/2), color);
}

static void drawRuler(cv::Mat &img, cv::Scalar color, double kx, double ky) {
    int x0 = img.size[1] / 2;
    int y0 = img.size[0] / 2;
    double wmm = img.size[1] * kx;

    // Draw 0.1mm ticks
    for (int i = -wmm/kx/0.1; i<=+wmm/kx/0.1; ++i) {
        int x = x0 + i*0.1/kx + 0.5;
        cv::line(img, cv::Point(x, y0-5), cv::Point(x, y0+5), color);
    }

    // Draw 0.5mm ticks
    for (int i = -wmm/kx/0.5; i<=+wmm/kx/0.5; ++i) {
        int x = x0 + i*0.5/kx + 0.5;
        cv::line(img, cv::Point(x, y0-10), cv::Point(x, y0+10), color);
    }

    // Draw 1.0mm ticks
    for (int i = -wmm/kx/1.0; i<=+wmm/kx/1.0; ++i) {
        int x = x0 + i*1.0/kx + 0.5;
        cv::line(img, cv::Point(x, y0-15), cv::Point(x, y0+15), color);
    }

    // Draw X axis
    cv::line(img, cv::Point(0,y0), cv::Point(2*x0+5,y0), color);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    failedFramesCounter(0),
    calibrationTriggered(false)
{
    ui->setupUi(this);
    ui->mainToolBar->setVisible(false);
    ui->statusBar->setVisible(false);
    ui->lblOriginal->setText("");
    showMaximized();

    // Detect and list video devices (linux specific)
    QDirIterator dir("/dev/", QDir::System);
    QRegExp re("/dev/(video[0-9]+)");
    while (dir.hasNext()) {
        QString s = dir.next();
        if (re.exactMatch(s))
            ui->cbCameras->addItem(re.cap(1));
    }
    on_cbCameras_currentIndexChanged(ui->cbCameras->currentText());
}

void MainWindow::process_image() {
    cam.read(matOriginal);
//    matOriginal = cv::imread("/home/lucas.hartmann/Recv/Bluetooth/01 - Padrão 1mm.png");

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
    if (cam.isOpened()) cam.release();

    QRegExp re(".*([0-9]+).*");
    if (!re.exactMatch(arg1)) return;
    int id = re.cap(1).toInt();

    cam.open(id);

    if (!cam.isOpened()) {
        ui->statusBar->setVisible(true);
        ui->statusBar->showMessage("Failed to access camera device "+arg1+".");
        timer.stop();
        return;
    }

    cam.set(CV_CAP_PROP_FRAME_WIDTH,  1920);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);

    timer.setInterval(20);
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
