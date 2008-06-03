#include "mainwindow.h"

#include "ffmpeg.h"

#include <QDebug>

MainWindow::MainWindow(Ffmpeg* grabber)
	: grabber_(grabber), button_mapper_(this) {
	ui_.setupUi(this);
	connect(grabber_, SIGNAL(frameAvailable()), SLOT(frameAvailable()));
	connect(qApp, SIGNAL(lastWindowClosed()), grabber_, SLOT(stop()));

	connect(ui_.green_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.green_button_, Green);
	connect(ui_.red_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.red_button_, Red);
	connect(ui_.yellow_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.yellow_button_, Yellow);
	connect(ui_.blue_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.blue_button_, Blue);
	connect(ui_.orange_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.orange_button_, Orange);

	connect(&button_mapper_, SIGNAL(mapped(int)), this, SLOT(setBox(int)));
}

MainWindow::~MainWindow() {
}

void MainWindow::frameAvailable() {
	ui_.video_label_->setPixmap(QPixmap::fromImage(grabber_->getImage()));
}

void MainWindow::setBox(int c) {
	qDebug() << __PRETTY_FUNCTION__ << c;
}
