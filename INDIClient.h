#pragma once

#include <QCoreApplication>
/*
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
*/

// INDI Client Library
#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>
#include <libindi/indidevapi.h>

// INDI Client implementation
class INDIClient : public QObject, public INDI::BaseClient {
    Q_OBJECT
    
public:
    explicit INDIClient(QObject *parent = nullptr);
    ~INDIClient() override;
    
    bool isConnected() const { return m_isConnected; }
    QStringList getDeviceList() const { return m_deviceList; }
    QStringList getCameraList() const { return m_cameraList; }
    QStringList getMountList() const { return m_mountList; }
    
    // Device control methods
    bool connectDevice(const QString &deviceName);
    bool disconnectDevice(const QString &deviceName);
    bool setCameraExposure(const QString &cameraName, double exposureTime);
    bool takeCameraExposure(const QString &cameraName);
    bool abortCameraExposure(const QString &cameraName);
    bool moveMountTo(const QString &mountName, double ra, double dec);
    bool stopMount(const QString &mountName);
    bool syncMountTo(const QString &mountName, double ra, double dec);
    bool parkMount(const QString &mountName);
    bool unparkMount(const QString &mountName);
    bool homeMount(const QString &mountName);
    
signals:
    void serverConnectedSignal();
    void serverDisconnectedSignal();
    void deviceAdded(const QString &deviceName);
    void deviceRemoved(const QString &deviceName);
    void deviceConnected(const QString &deviceName);
    void deviceDisconnected(const QString &deviceName);
    void newImage(const QString &deviceName, const QImage &image);
    void propertyUpdated(const QString &deviceName, const QString &propertyName);
    void mountPositionUpdated(const QString &deviceName, double ra, double dec);
    void message(const QString &msg);
    
protected:
    void newDevice(INDI::BaseDevice device) override;
    void removeDevice(INDI::BaseDevice device) override;
    void newProperty(INDI::Property property) override;
    void removeProperty(INDI::Property property) override;
    void newBLOB(IBLOB *bp);
    void newSwitch(ISwitchVectorProperty *svp);
    void newNumber(INumberVectorProperty *nvp);
    void newText(ITextVectorProperty *tvp);
    void newLight(ILightVectorProperty *lvp);
    void newMessage(INDI::BaseDevice device, int messageID) override;
    void serverConnected() override;
    void serverDisconnected(int exit_code) override;
    
private:
    bool m_isConnected;
    QStringList m_deviceList;
    QStringList m_cameraList;
    QStringList m_mountList;
    
    QImage processImageData(IBLOB *bp);
};
