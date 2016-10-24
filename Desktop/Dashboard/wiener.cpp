#include "wiener.h"
#include <QPair>
#include <QDebug>

Wiener::Wiener(int psfSize, QObject *parent) : QObject(parent)
{
    this->psfSize = psfSize;
    samples = QVector<QVector<Mat> >(psfSize);
    for (int i = 0; i < psfSize; i++) {
        samples[i] = QVector<Mat>(psfSize);
    }

    curX = 0;
    curY = 0;

    curHorShift = curX - (psfSize - 1) / 2;
    curVerShift = curY - (psfSize - 1) / 2;
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
    // build convolved image
    int sampleRows = samples[0][0].rows;
    int sampleCols = samples[0][0].cols;
    int convRows = psfSize * sampleRows + psfSize - 1;
    int convCols = psfSize * sampleCols + psfSize - 1;
    Mat conv(convRows, convCols, samples[0][0].type());

    for (int x = 0; x < psfSize; x++) {
        for (int y = 0; y < psfSize; y++) {
            for (int i = 0; i < sampleRows; i++) {
                for (int j = 0; j < sampleCols; j++) {
                    conv[x + psfSize*i][y + psfSize*j] = samples[x][y][i][j];
                }
            }
        }
    }
    // deconvolve
}
