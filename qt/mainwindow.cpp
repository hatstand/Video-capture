#include "mainwindow.h"

#include "ffmpeg.h"

#include <QDebug>

MainWindow::MainWindow(Ffmpeg* grabber)
	: grabber_(grabber), button_mapper_(this) {
	ui_.setupUi(this);
	connect(grabber_, SIGNAL(frameAvailable()), SLOT(frameAvailable()));
	connect(qApp, SIGNAL(lastWindowClosed()), grabber_, SLOT(stop()));

	connect(ui_.green_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.green_button_, MyLabel::Green);
	connect(ui_.red_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.red_button_, MyLabel::Red);
	connect(ui_.yellow_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.yellow_button_, MyLabel::Yellow);
	connect(ui_.blue_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.blue_button_, MyLabel::Blue);
	connect(ui_.orange_button_, SIGNAL(clicked()), &button_mapper_, SLOT(map()));
	button_mapper_.setMapping(ui_.orange_button_, MyLabel::Orange);

	connect(&button_mapper_, SIGNAL(mapped(int)), ui_.video_label_, SLOT(setBox(int)));
}

MainWindow::~MainWindow() {
	grabber_->stop();
	grabber_->wait(5000);
}

void MainWindow::frameAvailable() {
	ui_.video_label_->setPixmap(QPixmap::fromImage(grabber_->getImage()));
}
