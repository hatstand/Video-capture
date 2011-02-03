#include <QApplication>
#include <QtDebug>
#include "ffmpeg.h"
#include "mainwindow.h"

int main (int argc, char** argv) {
  QApplication app(argc, argv);

  if (argc < 2) {
    qWarning() << argv[0] << " filename";
    return -1;
  }

  Ffmpeg grab(argv[1]);
  MainWindow w(&grab);

  w.show();

  grab.start();

  return app.exec();
}
