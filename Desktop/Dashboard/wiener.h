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

    void process();

private:
    int psfSize;
    QVector<QVector<Mat> > samples;
    Mat conv;

    int curX;
    int curY;

    int curHorShift;
    int curVerShift;

    void setPsfSize(int psfSize);
    void readSamples(const QString &path, const QString &baseName);
    void convFromSamples();
};

#endif // WIENER_H
