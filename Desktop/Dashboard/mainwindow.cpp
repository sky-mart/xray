#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort/qserialport.h>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QDir>
#include <QSettings>

//TODO: настройки компорта, папки с фотками

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->shiftButton, SIGNAL(pressed()), SLOT(handleShiftButton()));
    connect(ui->relayButton, SIGNAL(pressed()), SLOT(handleRelayButton()));

    readSettings();

    pics = QDir(picsDirPath).entryList({tr("*.png")}, QDir::Files, QDir::Time);

    watcher = new QFileSystemWatcher();
    watcher->addPath(picsDirPath);
    connect(watcher, SIGNAL(directoryChanged(const QString &)), SLOT(handleDirectoryChanged(const QString &)));

    serial = NULL;
    connectToDevice();
}

MainWindow::~MainWindow()
{
    if (serial) {
        serial->close();
        delete serial;
    }
    delete watcher;
    delete ui;
}

void MainWindow::readSettings()
{
    QSettings *settings = new QSettings("Dashboard.ini", QSettings::IniFormat);
    picsDirPath = settings->value("pics-dir-path").toString();
    comPort = settings->value("com-port").toString();
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
    } else {
        qDebug() << tr("Failed to open port %1, error: %2").arg(serial->portName()).arg(serial->errorString()) << endl;
    }
}

void MainWindow::handleShiftButton()
{
    serial->write(tr("s%1 %2")
                  .arg(QString::number(ui->horSpin->value()))
                  .arg(QString::number(ui->verSpin->value()))
                  .toStdString().c_str());
}

void MainWindow::handleRelayButton() {
}

void MainWindow::handleDirectoryChanged(const QString &path)
{
    QStringList fresh = freshPics(path);
    foreach (QString pic, fresh) {
        qDebug() << pic << endl;
        pics << pic;
    }
}

QStringList MainWindow::freshPics(const QString &path)
{
    QStringList oldAndFresh = QDir(path).entryList({tr("*.png")}, QDir::Files, QDir::Time);
    QStringList freshOnly;
    for (int i = 0; i < oldAndFresh.size() - pics.size(); i++) {
        freshOnly << oldAndFresh.front();
        oldAndFresh.pop_front();
    }
    return freshOnly;
}
