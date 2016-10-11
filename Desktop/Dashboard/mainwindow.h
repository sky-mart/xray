#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}
class QSerialPort;
class QFileSystemWatcher;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void handleShiftButton();
    void handleRelayButton();

    void handleDirectoryChanged(const QString &path);

private:
    void readSettings();
    void connectToDevice();
    QStringList freshPics(const QString &path);

    Ui::MainWindow *ui;
    QSerialPort *serial;
    QFileSystemWatcher *watcher;

    QStringList pics;

    QString comPort;
    QString picsDirPath;
};

#endif // MAINWINDOW_H
