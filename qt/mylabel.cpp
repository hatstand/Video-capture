#include "mylabel.h"

#include <QMouseEvent>
#include <QPainter>

#include <QtDebug>

int MyLabel::brush_alpha_ = 200;
QBrush MyLabel::green_brush_(QColor(0, 255, 0, brush_alpha_));
QBrush MyLabel::red_brush_(QColor(255, 0, 0, brush_alpha_));
QBrush MyLabel::yellow_brush_(QColor(255, 255, 0, brush_alpha_));
QBrush MyLabel::blue_brush_(QColor(0, 0, 255, brush_alpha_));
QBrush MyLabel::orange_brush_(QColor(255, 155, 0, brush_alpha_));
QBrush MyLabel::grey_brush_(QColor(0, 0, 0, brush_alpha_));

MyLabel::MyLabel(QWidget* parent) : QLabel(parent), mouse_down_(false) {
  last_frame_ = QTime::currentTime();
}

MyLabel::~MyLabel() {
}

void MyLabel::mousePressEvent(QMouseEvent* event) {
  mouse_down_ = true;
  box_.setTopLeft(QPoint(event->x(), event->y()));
  box_.setBottomRight(QPoint(event->x(), event->y()));
}

void MyLabel::mouseMoveEvent(QMouseEvent* event) {
  if (mouse_down_) {
    if (event->x() > box_.left())
      box_.setRight(event->x());
    else
      box_.setLeft(event->x());

    if (event->y() > box_.top())
      box_.setBottom(event->y());
    else
      box_.setTop(event->y());

    update();
  }
}

void MyLabel::mouseReleaseEvent(QMouseEvent*) {
  mouse_down_ = false;
}

void MyLabel::paintEvent(QPaintEvent* event) {
  QPainter p(this);
  p.drawImage(0, 0, image_);
  p.setPen(Qt::white);
  QFont font;
  font.setBold(true);
  p.setFont(font);
  p.drawText(30, 30, QString("%1 fps").arg(current_fps_, 4, 'g', 3));
}

void MyLabel::setBox(int colour) {
  boxes_.insert((Colour)colour, box_);
}

QBrush& MyLabel::getBrush(Colour c) {
  switch (c) {
    case Green:
      return green_brush_;
    case Red:
      return red_brush_;
    case Yellow:
      return yellow_brush_;
    case Blue:
      return blue_brush_;
    case Orange:
      return orange_brush_;
    default:
      return grey_brush_;
  }
}

void MyLabel::setImage(const QImage& image) {
  image_ = image;
  update();
  QTime now = QTime::currentTime();
  current_fps_ =  1000.0 / last_frame_.msecsTo(now);
  last_frame_ = now;
}
