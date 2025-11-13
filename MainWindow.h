
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

#include "MountPanel.h"
#include "ImagePanel.h"
#include "PlateSolverPanel.h"
#include "MountModelPanel.h"


// Main application window
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QLineEdit *serverHostEdit;
    QSpinBox *serverPortEdit;

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
    
    QPushButton *connectButton;
    QPushButton *disconnectButton;
};
