#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QThread>
#include <QWidget>
#include <QScrollArea>

// Image Display panel
class ImagePanel : public QWidget {
    Q_OBJECT
    
public:
    explicit ImagePanel(QWidget *parent = nullptr);
    ~ImagePanel();
    
public slots:
    void displayImage(const QImage &image);
    void saveImage();
    
private:
    void setupUI();
    void updateZoom(int level);
    
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QSlider *zoomSlider;
    QPushButton *saveButton;
    
    QImage m_currentImage;
    double m_zoomFactor;
};
