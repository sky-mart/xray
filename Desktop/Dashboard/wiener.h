#ifndef WIENER_H
#define WIENER_H

#include <QObject>
#include <QVector>
#include "opencv2/core/core.hpp"

using cv::Mat;

class Wiener : public QObject
{
    Q_OBJECT
public:
    explicit Wiener(int psfSize, QObject *parent = 0);

    int getPsfSize();
    QPair<int, int> curShift();
    void addCurSample(const Mat & sample);

private:
    int psfSize;
    QVector<QVector<Mat> > samples;

    int curX;
    int curY;

    int curHorShift;
    int curVerShift;
};

#endif // WIENER_H
