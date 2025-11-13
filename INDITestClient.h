// INDITestClient.h
// Header file for the Qt-based INDI client for testing mount/camera functionality
// Author: Claude

#ifndef INDI_TEST_CLIENT_H
#define INDI_TEST_CLIENT_H

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
#include <QTimer>
#include <QScrollArea>
#include <QSlider>
#include <QStringList>
#include <QImage>

// INDI Client Library
#include <libindi/baseclient.h>
#include <libindi/basedevice.h>
#include <libindi/indiproperty.h>

// Forward declarations for Ekos and solver libraries
namespace Ekos {
    class Align;
}

class StarSolver;

// INDI Client implementation
class INDIClient : public INDI::BaseClient {
    Q_OBJECT
    
public:
    explicit INDIClient(QObject *parent = nullptr);
    ~INDIClient() override;
    
    bool isConnected() const;
    QStringList getDeviceList() const;
    QStringList getCameraList() const;
    QStringList getMountList() const;
    
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
    void serverConnected();
    void serverDisconnected();
    void deviceAdded(const QString &deviceName);
    void deviceRemoved(const QString &deviceName);
    void deviceConnected(const QString &deviceName);
    void deviceDisconnected(const QString &deviceName);
    void newImage(const QString &deviceName, const QImage &image);
    void propertyUpdated(const QString &deviceName, const QString &propertyName);
    void mountPositionUpdated(const QString &deviceName, double ra, double dec);
    void message(const QString &msg);
    
protected:
    void newDevice(INDI::BaseDevice *device) override;
    void removeDevice(INDI::BaseDevice *device) override;
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property *property) override;
    void newBLOB(IBLOB *bp) override;
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override;
    void newMessage(INDI::BaseDevice *device, int messageID) override;
    void serverConnected() override;
    void serverDisconnected(int exit_code) override;
    
private:
    bool m_isConnected;
    QStringList m_deviceList;
    QStringList m_cameraList;
    QStringList m_mountList;
    
    QImage processImageData(IBLOB *bp);
};

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

// Mount control panel
class MountPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MountPanel(INDIClient *client, QWidget *parent = nullptr);
    ~MountPanel();
    
public slots:
    void updateDeviceList();
    void onDeviceConnected(const QString &deviceName);
    void onDeviceDisconnected(const QString &deviceName);
    void onMountPositionUpdated(const QString &deviceName, double ra, double dec);
    
private slots:
    void connectMount();
    void disconnectMount();
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
    
    QDoubleSpinBox *raSpinBox;
    QDoubleSpinBox *decSpinBox;
    QPushButton *gotoButton;
    QPushButton *syncButton;
    QPushButton *stopButton;
    QPushButton *parkButton;
    QPushButton *unparkButton;
    QPushButton *homeButton;
    
    QLabel *currentRaLabel;
    QLabel *currentDecLabel;
};

// Plate Solver panel
class PlateSolverPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit PlateSolverPanel(INDIClient *client, QWidget *parent = nullptr);
    ~PlateSolverPanel();
    
public slots:
    void solveImage(const QImage &image);
    
private slots:
    void loadImage();
    void solve();
    void abortSolve();
    void solverFinished(bool success);
    void updateSettings();
    
signals:
    void logMessage(const QString &message);
    void solutionFound(double ra, double dec, double pixscale, double angle);
    
private:
    void setupUI();
    void startSolver();
    
    INDIClient *m_client;
    StarSolver *m_solver;
    QThread *m_solverThread;
    
    QString m_currentImagePath;
    QImage m_currentImage;
    
    QGroupBox *settingsGroup;
    QDoubleSpinBox *fovLowSpinBox;
    QDoubleSpinBox *fovHighSpinBox;
    QLineEdit *catalogPathEdit;
    QPushButton *browseButton;
    QCheckBox *useOnlineCheckBox;
    
    QPushButton *loadImageButton;
    QPushButton *solveButton;
    QPushButton *abortButton;
    
    QLabel *statusLabel;
    QLabel *coordinatesLabel;
    QLabel *pixscaleLabel;
    QLabel *angleLabel;
};

// Mount Modeling panel
class MountModelPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MountModelPanel(INDIClient *client, MountPanel *mountPanel, 
                           PlateSolverPanel *solverPanel, CameraPanel *cameraPanel, 
                           QWidget *parent = nullptr);
    ~MountModelPanel();
    
private slots:
    void startModeling();
    void stopModeling();
    void clearModel();
    void addPoint();
    void removeSelectedPoint();
    void updateModelDisplay();
    void onSolutionFound(double ra, double dec, double pixscale, double angle);
    
signals:
    void logMessage(const QString &message);
    
private:
    void setupUI();
    
    INDIClient *m_client;
    MountPanel *m_mountPanel;
    PlateSolverPanel *m_solverPanel;
    CameraPanel *m_cameraPanel;
    
    Ekos::Align *m_alignModule;
    
    QSpinBox *pointsSpinBox;
    QCheckBox *autoAddPointsCheckBox;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *clearButton;
    QPushButton *addPointButton;
    QPushButton *removePointButton;
    
    QTextEdit *modelPointsTextEdit;
    QLabel *rmsErrorLabel;
    QLabel *statusLabel;
};

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

// Main application window
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectToServer();
    void disconnectFromServer();
    void serverConnected();
    void serverDisconnected();
    void deviceConnected(const QString &deviceName);
    void deviceDisconnected(const QString &deviceName);
    void logMessage(const QString &message);
    
private:
    void setupUI();
    void createMenus();
    void updateStatus();
    
    QTabWidget *tabWidget;
    CameraPanel *cameraPanel;
    MountPanel *mountPanel;
    PlateSolverPanel *plateSolverPanel;
    MountModelPanel *mountModelPanel;
    ImagePanel *imagePanel;
    
    QTextEdit *logTextEdit;
    QStatusBar *statusBar;
    
    INDIClient *indiClient;
    QString serverHost;
    int serverPort;
    
    QLineEdit *serverHostEdit;
    QSpinBox *serverPortEdit;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
};

#endif // INDI_TEST_CLIENT_H
