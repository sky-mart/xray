#include "wiener.h"
#include <QPair>
#include <QDebug>
#include <QDir>
#include "opencv2/highgui.hpp"

//using cv::imread;
//using cv::imwrite;
//using cv::Point;
//using cv::Size;
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
        tmp.convertTo(samples[ay][ax], CV_32FC1);
        qDebug() << samples[ay][ax].type() << endl;
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
