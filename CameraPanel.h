#pragma once
#include <QCoreApplication>
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
/*
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QDateTime>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QThread>
*/

#include "INDIClient.h"

// Camera control panel
class CameraPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit CameraPanel(INDIClient *client, QWidget *parent = nullptr);
    ~CameraPanel();
    
public slots:
    void updateDeviceList();
    void onDeviceConnected(const QString &deviceName);
    void onDeviceDisconnected(const QString &deviceName);
    
private slots:
    void connectCamera();
    void disconnectCamera();
    void captureImage();
    void abortCapture();
    void updateExposure(double value);
    void updateGain(int value);
    void updateBinning(int index);
    
signals:
    void newImage(const QImage &image);
    void logMessage(const QString &message);
    
private:
    void setupUI();
    
    INDIClient *m_client;
    
    QComboBox *cameraComboBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QDoubleSpinBox *exposureSpinBox;
    QSpinBox *gainSpinBox;
    QComboBox *binningComboBox;
    QPushButton *captureButton;
    QPushButton *abortButton;
    QCheckBox *continuousCaptureCheckBox;
    
    QTimer *captureTimer;
    bool isContinuousCapture;
};
