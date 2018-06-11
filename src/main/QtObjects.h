#ifndef QT_OBJECTS_H
#define QT_OBJECTS_H

#include <QWidget>

class ImageViewer : public QWidget {
    Q_OBJECT
    QImage m_img;
    void paintEvent(QPaintEvent *);
public:
    ImageViewer(QWidget * parent = nullptr);
    Q_SLOT void setImage(const QImage & img); 
};

#endif // QT_OBJECTS_H
