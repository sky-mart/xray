#include "wiener.h"
#include <QPair>
#include <QDebug>
#include <QDir>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>

using namespace cv;

Wiener::Wiener(int psfSize, QObject *parent) : QObject(parent)
{
    setPsfSize(psfSize);

    curX = 0;
    curY = 0;

    curHorShift = curX - (psfSize - 1) / 2;
    curVerShift = curY - (psfSize - 1) / 2;
}

void Wiener::setPsfSize(int psfSize)
{
    this->psfSize = psfSize;
    samples = QVector<QVector<Mat> >(psfSize);
    for (int i = 0; i < psfSize; i++) {
        samples[i] = QVector<Mat>(psfSize);
    }
}

int Wiener::getPsfSize()
{
    return psfSize;
}

QPair<int, int> Wiener::curShift()
{
    return QPair<int, int>(curHorShift, curVerShift);
}

void Wiener::addCurSample(const Mat & sample)
{
    qDebug() << "added sample (" << curX << ", " << curY << ")" << endl;
    samples[curX][curY] = sample;

    curX++;
    if (curX == psfSize) {
        curX = 0;
        curHorShift = -(psfSize - 1);

        if (curY == psfSize - 1) {
            // finish
            curHorShift = 0;
            curVerShift = 0;

            process();
        } else {
            curY++;
            curVerShift = 1;
        }
    } else {
        curHorShift = 1;
        curVerShift = 0;
    }
}

void Wiener::process()
{
    readSamples(tr("."), tr("peka"));
    convFromSamples();
    imwrite("conv_peka.png", conv);
    Mat rest;
    Mat psf = Mat::ones(psfSize, psfSize, CV_32F) / (psfSize*psfSize);
    deconv(conv, psf, 100, rest);
    imwrite("rest_peka.png", rest);
}

void Wiener::readSamples(const QString & path, const QString & baseName)
{
    QDir dir(path);
    QStringList samples_paths;
    QRegExp rx(baseName + tr("_ay=[0-9]+_ax=[0-9]+.png"));

    foreach (QString entry, dir.entryList()) {
        if (rx.exactMatch(entry))
            samples_paths << entry;
    }
    setPsfSize(int(sqrt(samples_paths.size()))); // assume the correct number of samples in the folder

    foreach (QString sampleName, samples_paths) {
        int i1 = sampleName.indexOf("_ay=");
        int i2 = sampleName.indexOf("_ax=");
        int i3 = sampleName.indexOf(".png");
        int ay = sampleName.mid(i1 + 4, i2 - i1 - 4).toInt();
        int ax = sampleName.mid(i2 + 4, i3 - i2 - 4).toInt();

        QString fullPath = path + QDir::separator() + sampleName;
        Mat tmp = imread(fullPath.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
        tmp.convertTo(samples[ay][ax], CV_32F);
    }
}

void Wiener::convFromSamples()
{
    conv = Mat(psfSize * samples[0][0].rows, psfSize * samples[0][0].cols, CV_32FC1);

    for (int anchor_y = 0; anchor_y < psfSize; anchor_y++)
        for (int anchor_x = 0; anchor_x < psfSize; anchor_x++)
            for (int y = anchor_y; y < conv.rows; y += psfSize)
                for (int x = anchor_x; x < conv.cols; x += psfSize) {
                    int i = (y - anchor_y) / psfSize;
                    int j = (x - anchor_x) / psfSize;
                    conv.at<float>(y, x) = samples[anchor_y][anchor_x].at<float>(i, j);
                }
}

void Wiener::deconv(const Mat &c, const Mat &b, float snr, Mat & a)
{
    int outRows = c.rows - b.rows + 1;
    int outCols = c.cols - b.cols + 1;
    Size padSize = adjsize(c.size(), b.size());

    Mat cPadded, bPadded;
    copyMakeBorder(c, cPadded, 0, padSize.height - c.rows,
                   0, padSize.width - c.cols, BORDER_CONSTANT, Scalar::all(0));
    copyMakeBorder(b, bPadded, 0, padSize.height - b.rows,
                   0, padSize.width - b.cols, BORDER_CONSTANT, Scalar::all(0));

    printMat(cPadded);
    std::cout << endl;
    printMat(bPadded);

    Mat planes[2] = {Mat_<float>(cPadded), Mat::zeros(padSize, CV_32F)};

    Mat Fc, Fb;
    merge(planes, 2, Fc);
    dft(Fc, Fc); // Fc has spectre of c now

    planes[0] = Mat_<float>(bPadded);
    merge(planes, 2, Fb);
    dft(Fb, Fb); // Fb has spectre of b now


//    split(Fb, planes);
//    Mat bMag;
//    magnitude(planes[0], planes[1], bMag);
//    planes[0] = Mat_<float>(bMag);
//    planes[1] = Mat::zeros(bMag.size(), CV_32F);
//    merge(planes, 2, bMag); // bMag - complex number

//    Mat G = bMag / (bMag + 1.0 / snr) / Fb;
//    idft(G * Fc, a);
    idft(Fc / Fb, a);
    split(a, planes);
    a = planes[0](Range(0, outRows), Range(0, outCols)); // real part of necessary size
}

Size Wiener::adjsize(const Size & N0, const Size & K)
{
    Size N = N0;
    while (gcd(N.width, K.width) != 1)
        N = Size(N.width + 1, N.height);
    while (gcd(N.height, K.height) != 1)
        N = Size(N.width, N.height + 1);
    return N;
}

int Wiener::gcd(int a, int b)
{
    while (a != 0 && b != 0) {
        if (a > b)
            a = a % b;
        else
            b = b % a;
    }
    return a + b;
}

void Wiener::printMat(const Mat & M)
{
    for (int i = 0; i < M.rows; i++) {
        for (int j = 0; j < M.cols; j++) {
            std::cout << M.at<float>(i, j) << ' ';
        }
        std::cout << std::endl;
    }
}
