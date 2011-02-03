#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>
#include <QMap>
#include <QRect>

class MyLabel : public QLabel {
Q_OBJECT
public:
  MyLabel(QWidget* parent = NULL);
  virtual ~MyLabel();

  QRect box() { return box_; }

  enum Colour {
    Green = 0,
    Red,
    Yellow,
    Blue,
    Orange
  };

  void setImage(const QImage& image)  { image_ = image; update(); }

public slots:
  void setBox(int colour);

private:
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);

  void paintEvent(QPaintEvent* event);

  static QBrush& getBrush(Colour c);

  QRect box_;
  bool mouse_down_;
  QMap<Colour, QRect> boxes_;

  QImage image_;

  static int brush_alpha_;
  static QBrush green_brush_;
  static QBrush red_brush_;
  static QBrush yellow_brush_;
  static QBrush blue_brush_;
  static QBrush orange_brush_;
  static QBrush grey_brush_;
};

#endif
