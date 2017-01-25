#include "wiener.h"
#include <QPair>
#include <QDebug>
#include <QDir>
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <complex>

using namespace cv;

Wiener::Wiener(int psfSize, QObject *parent) : QObject(parent)
{
	// absolute coordinates on padded image
	anchorX = 0;
	anchorY = 0;

	setPsfSize(psfSize);
}

void Wiener::setPsfSize(int psfSize)
{
	this->psfSize = psfSize;
	samples = QVector<QVector<Mat> >(psfSize);
	for (int i = 0; i < psfSize; i++) {
		samples[i] = QVector<Mat>(psfSize);
	}

	// relative to previous step shift
	shiftX = anchorX - (psfSize - 1);
	shiftY = anchorY - (psfSize - 1);
}

int Wiener::getPsfSize()
{
	return psfSize;
}

QPair<int, int> Wiener::curShift()
{
	return QPair<int, int>(shiftX, shiftY);
}

/*
(anchorX, anchorY) - якорек на оригинальном изображении, относительно которого мы делаем очередной снимок
(shiftX, shiftY) - сдвиг, необходимый для следующего снимка
*/
void Wiener::addCurSample(const Mat & sample, bool firstPic)
{
	if (firstPic) {
		conv = Mat(psfSize * sample.rows, psfSize * sample.cols, CV_64F);
	}
	qDebug() << "added sample (" << anchorY << ", " << anchorX << ")" << endl;
	Mat convertedSample;
	sample.convertTo(convertedSample, CV_64F);
	convFromSample(convertedSample);

    anchorX++;
    if (anchorX == psfSize) {
        anchorX = 0;
		shiftX = -(psfSize - 1);

		if (anchorY == psfSize - 1) {
			// finish
			shiftX = 0;
			shiftY = 0;
		}
		else {
			anchorY++;
			shiftY = 1;
		}
	}
	else {
		shiftX = 1;
		shiftY = 0;
	}
}

void Wiener::process(bool test)
{
	if (test) {
		readSamples(testDirPath, testName);
		convFromSamples();
	}
	
	//imwrite((testDirPath + QDir::separator() + "conv_" + testName + ".png").toStdString(), conv);
	Mat rest;
	Mat psf = Mat::ones(psfSize, psfSize, CV_64F) / (psfSize*psfSize);

	Mat withoutBorders = conv(Range(psfSize, conv.rows - psfSize), Range(psfSize, conv.cols - psfSize));
	double snr = mean(withoutBorders)[0] / noise_std;

	deconv(conv, psf, snr, rest);
	imwrite((testDirPath + QDir::separator() + "rest_" + testName + ".png").toStdString(), rest);
}

void Wiener::readSamples(const QString & path, const QString & baseName)
{
	QDir dir(path);
	QStringList samples_paths;
	QRegExp rx(baseName + tr("_ay=[0-9]+_ax=[0-9]+.png"));

	foreach(QString entry, dir.entryList()) {
		if (rx.exactMatch(entry))
			samples_paths << entry;
	}
	setPsfSize(int(sqrt(samples_paths.size()))); // assume the correct number of samples in the folder

	foreach(QString sampleName, samples_paths) {
		int i1 = sampleName.indexOf("_ay=");
		int i2 = sampleName.indexOf("_ax=");
		int i3 = sampleName.indexOf(".png");
		int ay = sampleName.mid(i1 + 4, i2 - i1 - 4).toInt();
		int ax = sampleName.mid(i2 + 4, i3 - i2 - 4).toInt();

		QString fullPath = path + QDir::separator() + sampleName;
		Mat tmp = imread(fullPath.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
		tmp.convertTo(samples[ay][ax], CV_64F);
	}
}

void Wiener::convFromSamples()
{
	conv = Mat(psfSize * samples[0][0].rows, psfSize * samples[0][0].cols, CV_64F);

	for (int anchor_y = 0; anchor_y < psfSize; anchor_y++)
		for (int anchor_x = 0; anchor_x < psfSize; anchor_x++)
			for (int y = anchor_y; y < conv.rows; y += psfSize)
				for (int x = anchor_x; x < conv.cols; x += psfSize) {
					int i = (y - anchor_y) / psfSize;
					int j = (x - anchor_x) / psfSize;
					conv.at<double>(y, x) = samples[anchor_y][anchor_x].at<double>(i, j);
				}
}

void Wiener::convFromSample(const Mat &sample)
{
	for (int y = anchorX; y < conv.rows; y += psfSize)
		for (int x = anchorY; x < conv.cols; x += psfSize) {
			int i = (y - anchorY) / psfSize;
			int j = (x - anchorX) / psfSize;
			conv.at<double>(y, x) = sample.at<double>(i, j);
		}
}

void Wiener::deconv(const Mat &c, const Mat &b, double snr, Mat & a)
/*
	Wiener Deconvolution of 2D signals.
	Assumed that c(t) = (a * b)(t) + n(t), where n(t) is noise.
	Function calculates a(t), knowing signal-to-noise ratio.
*/
{
	// size of a
	int outRows = c.rows - b.rows + 1;
	int outCols = c.cols - b.cols + 1;

	// to avoid zeros in dft(b) we have to adjust sizes of both c and b
	Size padSize = adjsize(c.size(), b.size());
	Mat cPadded, bPadded;
	copyMakeBorder(c, cPadded, 0, padSize.height - c.rows,
		0, padSize.width - c.cols, BORDER_CONSTANT, Scalar::all(0));
	copyMakeBorder(b, bPadded, 0, padSize.height - b.rows,
		0, padSize.width - b.cols, BORDER_CONSTANT, Scalar::all(0));


	// Fc = dft(c), Fb = dft(b)
	Mat planes[2] = { Mat_<double>(cPadded), Mat::zeros(padSize, CV_64F) };
	Mat Fc, Fb;
	merge(planes, 2, Fc);
	dft(Fc, Fc); // Fc has spectre of c now
	planes[0] = Mat_<double>(bPadded);
	merge(planes, 2, Fb);
	dft(Fb, Fb); // Fb has spectre of b now

	// magnitudes of b for future calculations 
	split(Fb, planes);
	Mat bMag, bMag2;
	magnitude(planes[0], planes[1], bMag);
	bMag2 = bMag.mul(bMag);

	// a = idft(G * Fc), multiplication here, not convolution
	// G = Fb*Fb' / (Fb*Fb' + 1.0 / snr) / Fb
	//qDebug() << "numerator";
	//printMat(bMag2);
	//qDebug() << "denominator";
	//printMat(bMag2 + 1.0 / snr);
	Mat tmp = bMag2 / (bMag2 + 1.0 / snr);
	//qDebug() << "Fb*Fb' / (Fb*Fb' + 1.0 / snr)";
	//printMat(tmp);
	planes[0] = Mat_<double>(tmp);
	planes[1] = Mat::zeros(tmp.size(), CV_64F);
	merge(planes, 2, tmp); // tmp - complex number
	Mat G;
	complexMatDiv(tmp, Fb, G);
	//qDebug() << "G";
	//printMat(G);
	complexMatMul(G, Fc, tmp);
	//qDebug() << "G*Fc";
	//printMat(tmp);
	idft(tmp, a, DFT_SCALE);
	//qDebug() << "a";
	//printMat(a);
	
	// deconvolution without noise
	/*Mat x;
	complexMatDiv(Fc, Fb, x);
	idft(x, a, DFT_SCALE); */

	// real part of necessary size
	split(a, planes);
	a = planes[0](Range(0, outRows), Range(0, outCols)); 
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
	QDebug dbg = qDebug();
	for (int i = 0; i < M.rows / 10; i++) {
		for (int j = 0; j < M.cols / 10; j++) {
			if (M.channels() == 1) {
				dbg << M.at<double>(i, j) << ' ';
			} 
            else if (M.channels() == 2) {
				Vec2d el = M.at<Vec2d>(i, j);
				dbg << el[0] << "+" << el[1] << 'i';
			}	
		}
		dbg << endl;
	}
}

void Wiener::complexMatMul(const Mat & a, const Mat & b, Mat & c)
{
	c = Mat(a.rows, a.cols, CV_64FC2);
	for (int i = 0; i < a.rows; i++) {
		for (int j = 0; j < a.cols; j++) {
			Vec2d avec = a.at<Vec2d>(i, j);
			Vec2d bvec = b.at<Vec2d>(i, j);
			std::complex<double> ac(avec[0], avec[1]);
			std::complex<double> bc(bvec[0], bvec[1]);
			std::complex<double> cc = ac * bc;
			c.at<Vec2d>(i, j) = Vec2d(cc.real(), cc.imag());
		}
	}
}

void Wiener::complexMatDiv(const Mat & a, const Mat & b, Mat & c)
{
	c = Mat(a.rows, a.cols, CV_64FC2);
	for (int i = 0; i < a.rows; i++) {
		for (int j = 0; j < a.cols; j++) {
			Vec2d avec = a.at<Vec2d>(i, j);
			Vec2d bvec = b.at<Vec2d>(i, j);
			std::complex<double> ac(avec[0], avec[1]);
			std::complex<double> bc(bvec[0], bvec[1]);
			std::complex<double> cc = ac / bc;
			c.at<Vec2d>(i, j) = Vec2d(cc.real(), cc.imag());
		}
	}
}

void Wiener::setTestDirPath(const QString & dirPath)
{
	this->testDirPath = dirPath;
}

const QString & Wiener::getTestDirPath() const
{
	return testDirPath;
}

void Wiener::setTestName(const QString & name)
{
	this->testName = name;
}

const QString &  Wiener::getTestName() const
{
	return testName;
}

void Wiener::setNoiseStd(double noise_std)
{
	this->noise_std = noise_std;
}