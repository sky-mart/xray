#ifndef WIENER_H
#define WIENER_H

#include <QObject>
#include <QVector>
#include "opencv2/core/core.hpp"

using namespace cv;

class Wiener : public QObject
{
	Q_OBJECT
public:
	explicit Wiener(int psfSize, QObject *parent = 0);

	int getPsfSize();
	QPair<int, int> curShift();
	void addCurSample(const Mat & sample);

	void process();
	static int gcd(int a, int b);

private:
	int psfSize;
	QVector<QVector<Mat> > samples;
	Mat conv;
	Mat psf;
	Mat rest;

	int anchorX;
	int anchorY;

	int shiftX;
	int shiftY;

	void setPsfSize(int psfSize);
	void readSamples(const QString &path, const QString &baseName);
	void convFromSamples();
	void deconv(const Mat & c, const Mat & b, float snr, Mat &a);
	Size adjsize(const Size & N, const Size & K);
	void printMat(const Mat & M);
	void complexMatMul(const Mat & a, const Mat & b, Mat & c);
	void complexMatDiv(const Mat & a, const Mat & b, Mat & c);
};

#endif // WIENER_H
