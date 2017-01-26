#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/qserialport.h>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <qfiledialog.h>
#include <qmessagebox.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"

#include "wiener.h"

using cv::Mat;
using cv::imread;

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(ui->shiftButton, SIGNAL(pressed()), SLOT(handleShiftButton()));
	connect(ui->snapshotButton, SIGNAL(pressed()), SLOT(handleSnapshotButton()));
	connect(ui->manualButton, SIGNAL(pressed()), SLOT(handleManualButton()));
	connect(ui->autoButton, SIGNAL(pressed()), SLOT(handleAutoButton()));
	connect(ui->testButton, SIGNAL(pressed()), SLOT(handleTestButton()));
	connect(ui->testDirButton, SIGNAL(pressed()), SLOT(handleTestDirButton()));
	connect(ui->testBaseNameButton, SIGNAL(pressed()), SLOT(handleTestBaseNameButton()));

	wiener = new Wiener(3);

	readSettings();

	pics = QDir(picsDirPath).entryList({ tr("*.png") }, QDir::Files, QDir::Time);

	watcher = new QFileSystemWatcher();
	watcher->addPath(picsDirPath);
	//connect(watcher, SIGNAL(directoryChanged(const QString &)), SLOT(handleDirectoryChanged(const QString &)));
	timer = new QTimer(this);
	timer->setInterval(10000);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(resnap()));

	serial = NULL;
	connectToDevice();

	firstPic = true;
}

MainWindow::~MainWindow()
{
	saveSettings();
	if (serial) {
		serial->close();
		delete serial;
	}
	delete watcher;
	delete wiener;
	delete ui;
	delete timer;
}

void MainWindow::readSettings()
{
	QSettings *settings = new QSettings("Dashboard.ini", QSettings::IniFormat);
	picsDirPath = settings->value("pics-dir-path").toString();
	qDebug() << picsDirPath << endl;
	comPort = settings->value("com-port").toString();

	QString testDirPath = settings->value("test-dir-path").toString();
	QString testBaseName = settings->value("test-base-name").toString();
	qDebug() << "testdir" << testDirPath << endl
		<< "testname" << testBaseName << endl;
	ui->testDirLine->setText(testDirPath);
	ui->testBaseNameLine->setText(testBaseName);
	wiener->setTestDirPath(testDirPath);
	wiener->setTestName(testBaseName);

	int psfSize = settings->value("psf-size").toInt();
	double noise_std = settings->value("noise-std").toDouble();
	wiener->setPsfSize(psfSize);
	wiener->setNoiseStd(noise_std);

	delete settings;
}

void MainWindow::saveSettings()
{
	QSettings *settings = new QSettings("Dashboard.ini", QSettings::IniFormat);
	settings->setValue("test-dir-path", QVariant(wiener->getTestDirPath()));
	settings->setValue("test-base-name", QVariant(wiener->getTestName()));
	delete settings;
}

void MainWindow::connectToDevice()
{
	serial = new QSerialPort(comPort);
	serial->setBaudRate(QSerialPort::Baud9600);
	serial->setParity(QSerialPort::NoParity);
	serial->setStopBits(QSerialPort::OneStop);

	if (serial->open(QIODevice::ReadWrite)) {
		qDebug() << tr("Successfully opened port %1").arg(serial->portName()) << endl;
	}
	else {
		qDebug() << tr("Failed to open port %1, error: %2").arg(serial->portName()).arg(serial->errorString()) << endl;
	}
}

void MainWindow::handleShiftButton()
{
	shift(ui->horSpin->value(), ui->verSpin->value());
}

void MainWindow::handleSnapshotButton()
{
	snapshot();
}

void MainWindow::handleDirectoryChanged(const QString &path)
{
	QStringList fresh = freshPics(path);
	foreach(QString pic, fresh) {
		qDebug() << pic << endl;
	}

	if (fresh.length() == 1) {
		// assume 1 image at a time for simplicity
		QString pic = fresh.back();
        QString picPath = picsDirPath + QDir::separator() + pic;
		pics << pic;

		ui->statusBar->showMessage("New image received: " + pic);
        Mat mat = imread(picPath.toStdString(), CV_LOAD_IMAGE_GRAYSCALE);
		timer->stop();

		if (firstPic) {
			QMessageBox mb(QMessageBox::Question,
				tr("First Picture"),
				tr("Is the first picture OK?"),
				QMessageBox::Yes | QMessageBox::No,
				this
			);
			if (mb.exec() == QMessageBox::Yes) {
				qDebug() << "Leave like this";
				firstPic = false;
				wiener->addCurSample(mat, true);
				handleAutoButton();
				firstAveragePixel = mean(mat)[0];
			}
			else {
				qDebug() << "Don not leave like this";
				resnap(true);
			}
		}
		else {
			double averagePixel = mean(mat)[0];
			if (abs(averagePixel - firstAveragePixel) / firstAveragePixel < 0.1) {
				mat *= (firstAveragePixel / averagePixel);
				wiener->addCurSample(mat);
				handleAutoButton();
			}
			else {
				resnap(true);
			}
			
		}
	}
}

QStringList MainWindow::freshPics(const QString &path)
{
	QStringList oldAndFresh = QDir(path).entryList({ tr("*.png") }, QDir::Files, QDir::Time);
	QStringList freshOnly;
	for (int i = 0; i < oldAndFresh.size() - pics.size(); i++) {
		freshOnly << oldAndFresh.front();
		oldAndFresh.pop_front();
	}
	return freshOnly;
}

void MainWindow::handleManualButton()
{
	disconnect(/*SIGNAL(directoryChanged(const QString &)),*/ this, SLOT(handleDirectoryChanged(const QString &)));
}

void MainWindow::handleAutoButton()
{
	connect(watcher, SIGNAL(directoryChanged(const QString &)), SLOT(handleDirectoryChanged(const QString &)));

	QPair<int, int> cur = wiener->curShift();
	if (cur.first == 0 && cur.second == 0) {
	    qDebug() << "finished" << endl;
		wiener->process();
		ui->statusBar->showMessage("Image restored");
		disconnect(this, SLOT(handleDirectoryChanged(const QString &)));
	}
	else {
		/*Дополнительное движение против люфтов */
		if (abs(cur.first) > 1) {
			if (abs(cur.second) > 1) {
				shift(2 * cur.first, 2 * cur.second);
				shift(-cur.first, -cur.second);
			}
			else {
				shift(2 * cur.first, cur.second);
				shift(-cur.first, 0);
			}
		}
		else {
			if (abs(cur.second) > 1) {
				shift(cur.first, 2 * cur.second);
				shift(0, -cur.second);
			}
			else {
				shift(cur.first, cur.second);
			}
		}

		resnap(true);
	}
}

void MainWindow::resnap(bool fromZero)
{
	if (fromZero) snapAttempts = 0;

	timer->start();
	snapAttempts++;

	if (snapAttempts >= 5) {
		ui->statusBar->showMessage("Can not take a picture!");
		timer->stop();
		return;
	} 
	snapshot();
}

void MainWindow::resnap()
{
	resnap(false);
}

void MainWindow::handleTestButton()
{
	connect(watcher, SIGNAL(directoryChanged(const QString &)), SLOT(handleDirectoryChanged(const QString &)));
	wiener->process(true);
}

void MainWindow::shift(int hor, int ver)
{
	qDebug() << "shift(x,y) " << hor << ' ' << ver << endl;
	serial->write(tr("s%1 %2")
		.arg(QString::number(hor))
		.arg(QString::number(ver))
		.toStdString().c_str());
}

void MainWindow::snapshot()
{
	serial->write("r\n");
}

void MainWindow::handleTestBaseNameButton()
{
	wiener->setTestName(ui->testBaseNameLine->text());
}

void MainWindow::handleTestDirButton()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
		".",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);
	ui->testDirLine->setText(dir);
	wiener->setTestDirPath(dir);
}
