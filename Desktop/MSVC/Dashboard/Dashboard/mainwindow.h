#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
	class MainWindow;
}
class QSerialPort;
class QFileSystemWatcher;
class Wiener;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	private Q_SLOTS:
	void handleShiftButton();
	void handleSnapshotButton();
	void handleAutoButton();
	void handleTestButton();
	void handleTestBaseNameButton();
	void handleTestDirButton();

	void handleDirectoryChanged(const QString &path);

private:
	void shift(int hor, int ver);
	void snapshot();

	void readSettings();
	void saveSettings();
	void connectToDevice();
	QStringList freshPics(const QString &path);

	Ui::MainWindow *ui;
	QSerialPort *serial;
	QFileSystemWatcher *watcher;

	QStringList pics;
	Wiener *wiener;

	QString comPort;
	QString picsDirPath;
};

#endif // MAINWINDOW_H
