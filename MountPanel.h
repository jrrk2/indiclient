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

#include "INDIClient.h"

// Mount control panel
class MountPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MountPanel(INDIClient *client, QWidget *parent = nullptr);
    ~MountPanel();
    QDoubleSpinBox *raSpinBox;
    QDoubleSpinBox *decSpinBox;
    
public slots:
    void updateDeviceList();
    void onDeviceConnected(const QString &deviceName);
    void onDeviceDisconnected(const QString &deviceName);
    void onMountPositionUpdated(const QString &deviceName, double ra, double dec);
    
private slots:
    void connectMount();
    void disconnectMount();
    void loadSelectedTarget();
    void gotoCoordinates();
    void syncCoordinates();
    void stopMount();
    void parkMount();
    void unparkMount();
    void homeMount();
    
signals:
    void logMessage(const QString &message);
    
private:
    void setupUI();
    
    INDIClient *m_client;
    
    QComboBox *mountComboBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    
    QComboBox *targetComboBox;
    QLabel *targetInfoLabel;
    
    QPushButton *gotoButton;
    QPushButton *syncButton;
    QPushButton *stopButton;
    QPushButton *parkButton;
    QPushButton *unparkButton;
    QPushButton *homeButton;
    
    QLabel *currentRaLabel;
    QLabel *currentDecLabel;
};
