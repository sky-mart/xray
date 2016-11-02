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
	setPsfSize(psfSize);

    // absolute coordinates on padded image
	anchorX = 0;
	anchorY = 0;

    // relative to previous step shift
    shiftX = anchorX - (psfSize - 1);
	shiftY = anchorY - (psfSize - 1);
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
	return QPair<int, int>(shiftX, shiftY);
}

void Wiener::addCurSample(const Mat & sample)
{
	qDebug() << "added sample (" << anchorY << ", " << anchorX << ")" << endl;
    sample.convertTo(samples[anchorY][anchorX], CV_32F);

    anchorX++;
    if (anchorX == psfSize) {
        anchorX = 0;
		shiftX = -(psfSize - 1);

		if (anchorY == psfSize - 1) {
			// finish
			shiftX = 0;
			shiftY = 0;

			process();
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

void Wiener::process()
{
	//readSamples(tr("D:\\Vlad\\Projects\\xray\\images"), tr("peka"));
	convFromSamples();
	//imwrite("D:\\Vlad\\Projects\\xray\\images\\conv_peka.png", conv);
	Mat rest;
	Mat psf = Mat::ones(psfSize, psfSize, CV_32F) / (psfSize*psfSize);

	float sigma = 0.1;
	Mat withoutBorders = conv(Range(psfSize, conv.rows - psfSize), Range(psfSize, conv.cols - psfSize));
	float snr = mean(withoutBorders)[0] / sigma;

	deconv(conv, psf, snr, rest);
	imwrite("D:\\Vlad\\Projects\\xray\\images\\rest_peka.png", rest);
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
	Mat planes[2] = { Mat_<float>(cPadded), Mat::zeros(padSize, CV_32F) };
	Mat Fc, Fb;
	merge(planes, 2, Fc);
	dft(Fc, Fc); // Fc has spectre of c now
	planes[0] = Mat_<float>(bPadded);
	merge(planes, 2, Fb);
	dft(Fb, Fb); // Fb has spectre of b now

	// magnitudes of b for future calculations 
	split(Fb, planes);
	Mat bMag;
	magnitude(planes[0], planes[1], bMag);

	// a = idft(G * Fc), multiplication here, not convolution
	// G = Fb*Fb' / (Fb*Fb' + 1.0 / snr) / Fb
	Mat tmp = bMag / (bMag + 1.0 / snr);
	planes[0] = Mat_<float>(tmp);
	planes[1] = Mat::zeros(tmp.size(), CV_32F);
	merge(planes, 2, tmp); // tmp - complex number
	Mat G;
	complexMatDiv(tmp, Fb, G);
	complexMatMul(G, Fc, tmp);
	idft(tmp, a, DFT_SCALE);
	
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
	for (int i = 0; i < M.rows; i++) {
		for (int j = 0; j < M.cols; j++) {
			if (M.channels() == 1) {
				dbg << M.at<float>(i, j) << ' ';
			} 
            else if (M.channels() == 2) {
				Vec2f el = M.at<Vec2f>(i, j);
				dbg << el[0] << "+" << el[1] << 'i';
			}	
		}
		dbg << '\n';
	}
}

void Wiener::complexMatMul(const Mat & a, const Mat & b, Mat & c)
{
	c = Mat(a.rows, a.cols, CV_32FC2);
	for (int i = 0; i < a.rows; i++) {
		for (int j = 0; j < a.cols; j++) {
			Vec2f avec = a.at<Vec2f>(i, j);
			Vec2f bvec = b.at<Vec2f>(i, j);
			std::complex<float> ac(avec[0], avec[1]);
			std::complex<float> bc(bvec[0], bvec[1]);
			std::complex<float> cc = ac * bc;
			c.at<Vec2f>(i, j) = Vec2f(cc.real(), cc.imag());
		}
	}
}

void Wiener::complexMatDiv(const Mat & a, const Mat & b, Mat & c)
{
	c = Mat(a.rows, a.cols, CV_32FC2);
	for (int i = 0; i < a.rows; i++) {
		for (int j = 0; j < a.cols; j++) {
			Vec2f avec = a.at<Vec2f>(i, j);
			Vec2f bvec = b.at<Vec2f>(i, j);
			std::complex<float> ac(avec[0], avec[1]);
			std::complex<float> bc(bvec[0], bvec[1]);
			std::complex<float> cc = ac / bc;
			c.at<Vec2f>(i, j) = Vec2f(cc.real(), cc.imag());
		}
	}
}
