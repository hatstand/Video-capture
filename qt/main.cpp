#include <QApplication>

#include "ffmpeg.h"
#include "mainwindow.h"

int main (int argc, char** argv) {
  QApplication app(argc, argv);

  Ffmpeg grab;
  MainWindow w(&grab);

  w.show();

  grab.start();

  return app.exec();
}
