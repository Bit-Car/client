#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QSize>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    isCameraRunning = false;
    isCalibrate = false;
    boardSize.width = 8;
    boardSize.height = 6;
    numSeq = 0;
    numRequiredSnapshot = 20;

    // open camera stream
    capture.open(1); // default: 0

    if(!capture.isOpened())
        capture.open(CV_CAP_ANY); // default: 0

    if(!capture.isOpened())
        return;

    // set the acquired frame size to the size of its container
    capture.set(CV_CAP_PROP_FRAME_WIDTH, ui->before_img->size().width());
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, ui->before_img->size().height());

    isCameraRunning = true;

    // start timer for acquiring the video
    cameraTimer.start(33); // 33 ms = 30 fps
    // at the timeout() event, execute the cameraTimerTimeout() method
    connect(&cameraTimer, SIGNAL(timeout()), this, SLOT(cameraTimerTimeout()));

    QString imgPath = "/home/alice/client/smile.png";
    QImage *img = new QImage();
    bool loaded = img->load(imgPath);

    QTableWidgetItem *thumbnail = new QTableWidgetItem;
    thumbnail->setData(Qt::DecorationRole, QPixmap::fromImage(*img).scaled(100, 100));
    thumbnail->setSizeHint(QSize(100, 100));
    this->ui->table->setColumnCount(5);
    this->ui->table->setRowCount(1);
    this->ui->table->setItem(0, 0, thumbnail);

    this->ui->table->horizontalHeader()->setDefaultSectionSize(100);
    this->ui->table->verticalHeader()->setDefaultSectionSize(100);
    this->ui->table->horizontalHeader()->setResizeMode(QHeaderView::Fixed);
    this->ui->table->verticalHeader()->setResizeMode(QHeaderView::Fixed);

    ui->tabWidget->setCurrentIndex(0);


    Mat img_c_cv = imread("/home/alice/client/under_c.png", CV_LOAD_IMAGE_COLOR);
    cv::resize(img_c_cv, img_c_cv, Size(ui->img_construction->geometry().width(), ui->img_construction->geometry().height()), 1, 1, INTER_AREA);
    cvtColor(img_c_cv, img_c_cv, CV_BGR2RGB);
    QImage img_constr = QImage((const unsigned char*)(img_c_cv.data), img_c_cv.cols, img_c_cv.rows, img_c_cv.step, QImage::Format_RGB888);
    ui->img_construction->setPixmap(QPixmap::fromImage(img_constr));
    ui->tabWidget->setTabEnabled(2, false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_commandLinkButton_clicked() {
    ui->tabWidget->setCurrentIndex(1);
}



void MainWindow::cameraTimerTimeout()
{
    if(isCameraRunning && capture.isOpened())
    {
        // store the frame to show in a Qt window
        QImage frameToShow, frameUndistorted;

        // get the current frame from the video stream
        capture >> image;


        // Load Face cascade (.xml file)
        CascadeClassifier fface_cascade;
        fface_cascade.load( "/home/alice/client/haarcascades/haarcascade_frontalface_alt.xml" );

        CascadeClassifier face_cascade;
        face_cascade.load( "/home/alice/client/haarcascades/haarcascade_profileface.xml" );

        CascadeClassifier plate_cascade;
        plate_cascade.load( "/home/alice/client/haarcascades/haarcascade_russian_plate_number.xml" );

        if(face_cascade.empty())
        {
            cerr<<"Error Loading XML file"<<endl;
            return;
        }

        if(plate_cascade.empty())
        {
            cerr<<"Error Loading XML file"<<endl;
            return;
        }

        std::vector<Rect> ffaces;
        fface_cascade.detectMultiScale( image, ffaces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

        std::vector<Rect> faces;
        face_cascade.detectMultiScale( image, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );

        std::vector<Rect> plates;
        plate_cascade.detectMultiScale( image, plates, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(32, 8) );

        Mat smile_img = imread("/home/alice/client/smile.png", CV_LOAD_IMAGE_COLOR);

        // Draw circles on the detected faces
        for( int i = 0; i < faces.size(); i++ ) {
            //Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
            //ellipse( image, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );
            Mat scaled_smile;
            int size_sc = std::min(faces[i].width, faces[i].height) * 0.9;
            cv::resize(smile_img, scaled_smile, Size(size_sc, size_sc),1,1,INTER_AREA);
            scaled_smile.copyTo(image(cv::Rect(faces[i].x, faces[i].y, scaled_smile.cols, scaled_smile.rows)));
        }

        for( int i = 0; i < ffaces.size(); i++ ) {
            Mat scaled_smile;
            int size_sc = std::min(ffaces[i].width, ffaces[i].height) * 0.9;
            cv::resize(smile_img, scaled_smile, Size(size_sc, size_sc),1,1,INTER_AREA);
            scaled_smile.copyTo(image(cv::Rect(ffaces[i].x, ffaces[i].y, scaled_smile.cols, scaled_smile.rows)));
        }


        for( int i = 0; i < plates.size(); i++ ) {
            Mat scaled_smile;
            int size_sc = std::min(plates[i].width, plates[i].height) * 0.9;
            cv::resize(smile_img, scaled_smile, Size(size_sc, size_sc),1,1,INTER_AREA);
            for (int j = 0; j < plates[i].width / size_sc; ++j)
                scaled_smile.copyTo(image(cv::Rect(plates[i].x + j * size_sc, plates[i].y, scaled_smile.cols, scaled_smile.rows)));
        }

        // prepare the image for the Qt format...
        // ... change color channel ordering (from BGR in our Mat to RGB in QImage)
        cvtColor(image, image, CV_BGR2RGB);
        cv::resize(image, image, Size(ui->before_img->geometry().width(), ui->before_img->geometry().height()), 1, 1, INTER_AREA);


        // Qt image
        // image.step is needed to properly show some images (due to padding byte added in the Mat creation)
        frameToShow = QImage((const unsigned char*)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);

        // display on label
        ui->before_img->setPixmap(QPixmap::fromImage(frameToShow));
    }
}

void MainWindow::findAndDrawPoints()
{
    /*
    std::vector<Point2f> imageCorners;
    bool found = findChessboardCorners(image, boardSize, imageCorners);
    // store the image to be used for the calibration process
    if(found)
        image.copyTo(imageSaved);
    // show the found corners on screen, if any
    drawChessboardCorners(image, boardSize, imageCorners, found);
    */
}

void MainWindow::on_takeSnaphotButton_clicked()
{
    if (isCameraRunning && imageSaved.data)
    {
        // store the image, if valid
        imageList.push_back(imageSaved);
        numSeq++;
    }

    QImage frameToShow = QImage((const unsigned char*)(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
    ui->img_to_send->setPixmap(QPixmap::fromImage(frameToShow));
    ui->tabWidget->setTabEnabled(2, true);
    ui->tabWidget->setCurrentIndex(2);
}

// start the calibration process
void MainWindow::startCalibration()
{
    if(numSeq >= numRequiredSnapshot)
    {
        ui->takeSnaphotButton->setEnabled(false);

        // open chessboard images and extract corner points
        successes = cameraCalib.addChessboardPoints(imageList,boardSize);

        // calibrate the camera frames
        //Size calibSize = Size(ui->after_img->size().width(), ui->after_img->size().height());
        //cameraCalib.calibrate(calibSize);

        isCalibrate = true;
        ui->success_label->setText("Successful images used: " + QString::number(successes));
    }
}
