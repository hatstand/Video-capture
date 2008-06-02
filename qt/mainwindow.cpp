#include "mainwindow.h"

#include "ffmpeg.h"

#include <QDebug>

MainWindow::MainWindow(Ffmpeg* grabber, QObject* parent) : grabber_(grabber) {
	ui_.setupUi(this);
	connect(grabber_, SIGNAL(frameAvailable()), SLOT(frameAvailable()));
	connect(this, SIGNAL(destroyed(QObject*)), grabber_, SLOT(stop()));
}

MainWindow::~MainWindow() {
}

void MainWindow::frameAvailable() {
	qDebug() << __PRETTY_FUNCTION__;

	ui_.video_label_->setPixmap(QPixmap::fromImage(grabber_->getImage()));
}
