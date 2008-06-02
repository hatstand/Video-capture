#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include <QMainWindow>

class Ffmpeg;

class MainWindow : public QMainWindow {
Q_OBJECT
public:
	MainWindow(Ffmpeg* grabber, QObject* parent = NULL);
	virtual ~MainWindow();

private:
	Ui_MainWindow ui_;
	Ffmpeg* grabber_;

private slots:
	void frameAvailable();

};

#endif
