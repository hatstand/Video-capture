#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QMainWindow>
#include <QRect>
#include <QSignalMapper>

class Ffmpeg;

class MainWindow : public QMainWindow {
Q_OBJECT
public:
	MainWindow(Ffmpeg* grabber);
	virtual ~MainWindow();

private:
	Ui_MainWindow ui_;
	Ffmpeg* grabber_;
	QSignalMapper button_mapper_;

private slots:
	void frameAvailable();
};

#endif
