#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "microscope.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    cv::VideoCapture cam;
    cv::Mat matOriginal;
    QImage  imgOriginal;
    std::vector<cv::Vec3f> vecCircles;
    std::vector<cv::Vec3f>::iterator itrCircles;
    QTimer timer;
    unsigned failedFramesCounter;
    bool calibrationTriggered;
    cv::Scalar rangeMinimum, rangeMaximum;
    microscope_calibration cal;

    void paintEvent(QPaintEvent *e);
    static QRect place_image(QRect image, QRect displayport);
    void drawRuler(cv::Mat &img, cv::Scalar color, double kx, double ky);

public slots:
    void process_image();

private slots:
    void on_btnCalibrate_clicked();
    void on_cbCameras_currentIndexChanged(const QString &arg1);
    void on_btnSnapshot_clicked();
};

#endif // MAINWINDOW_H
