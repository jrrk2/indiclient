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

#include "CameraPanel.h"
#include "MountPanel.h"
#include "PlateSolverPanel.h"

// Include Ekos align library for mount modeling
// #include <ekos/align/align.h>

// Mount Modeling panel
class MountModelPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MountModelPanel(INDIClient *client, MountPanel *mountPanel, PlateSolverPanel *solverPanel, CameraPanel *cameraPanel, QWidget *parent = nullptr);
    ~MountModelPanel();

public slots:
    void onSolutionFound(double ra, double dec, double pixscale, double angle);
      
private slots:
    void startModeling();
    void stopModeling();
    void clearModel();
    void addPoint();
    void removeSelectedPoint();
    void updateModelDisplay();
    
signals:
    void logMessage(const QString &message);
    
private:
    void setupUI();
    
    INDIClient *m_client;
    MountPanel *m_mountPanel;
    PlateSolverPanel *m_solverPanel;
    CameraPanel *m_cameraPanel;
    
    //    Ekos::Align *m_alignModule;
    
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
