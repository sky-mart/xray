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

	const QString & getTestDirPath() const;
	void setTestDirPath(const QString & dirPath);
	const QString &  getTestName() const;
	void setTestName(const QString & name);

	int getPsfSize();
	QPair<int, int> curShift();
	void addCurSample(const Mat & sample);

	void process(bool test = false);
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

	QString testDirPath;
	QString testName;

	void setPsfSize(int psfSize);
	void readSamples(const QString &path, const QString &baseName);
	void convFromSamples();
	void deconv(const Mat & c, const Mat & b, double snr, Mat &a);
	Size adjsize(const Size & N, const Size & K);
	void printMat(const Mat & M);
	void complexMatMul(const Mat & a, const Mat & b, Mat & c);
	void complexMatDiv(const Mat & a, const Mat & b, Mat & c);
};

#endif // WIENER_H
