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

// Include libsolver for plate solving
#include <stellarsolver.h>
#include "INDIClient.h"
#include "StellarSolverInterface.h"

// Plate Solver panel
class PlateSolverPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit PlateSolverPanel(INDIClient *client, QWidget *parent = nullptr);
    ~PlateSolverPanel();
    
public slots:
    void solveImage(const QImage &image);
    void onSolverStatusUpdate(const QString &status);
    void onSolverFinished(const StellarSolverInterface::SolveResult &result);

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
    StellarSolver *m_solver;
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

    QDoubleSpinBox *searchRadiusSpinBox;
};
