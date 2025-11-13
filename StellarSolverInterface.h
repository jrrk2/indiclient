#ifndef STELLAR_SOLVER_INTERFACE_H
#define STELLAR_SOLVER_INTERFACE_H

#include <QObject>
#include <QImage>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QFileInfo>
#include <QDir>

// StellarSolver includes
#include <stellarsolver.h>
#include <parameters.h>
#include <structuredefinitions.h>

class StellarSolverInterface : public QObject
{
    Q_OBJECT

public:
    // Result structure for plate solving
    struct SolveResult {
        bool success = false;
        double ra = -1;
        double dec = -91;
        double pixelScale = 0;
        double orientation = 0;
        double fieldWidth = 0;
        double fieldHeight = 0;
        double solveTime = 0;
        QString statusMessage;
    };

    explicit StellarSolverInterface(QObject *parent = nullptr);
    ~StellarSolverInterface();

    // Main method to solve an image
    void solveImage(const QImage &image, double raHint = -1, double decHint = -91,
                   double fovMin = 0.1, double fovMax = 10.0);

    // Set the search radius in degrees
    void setSearchRadius(double radius) { m_searchRadius = radius; }

    // Set position hints
    void setPositionHint(double ra, double dec) { 
        m_raHint = ra;
        m_decHint = dec;
    }

    // Set FOV range
    void setFovRange(double minFov, double maxFov) {
        m_fovMin = minFov;
        m_fovMax = maxFov;
    }

    // Abort current solving
    void abort();

    // Check if solving is in progress
    bool isSolving() const;

signals:
    void solveComplete(const StellarSolverInterface::SolveResult &result);
    void statusUpdate(const QString &status);
    void progressUpdate(int percent);

private slots:
    void handleSolverFinished();

private:
    // Setup parameters for StellarSolver
    void setupParameters();
    
    // Find astrometry index files
    QStringList findIndexFiles();

    // Convert QImage to a format StellarSolver can use
    bool prepareImageForSolver(const QImage &image);

    // Member variables
    StellarSolver *m_solver = nullptr;
    QThread *m_solverThread = nullptr;
    Parameters m_params;
    QStringList m_indexPaths;

    double m_searchRadius = 2.0;
    double m_raHint = -1;
    double m_decHint = -91;
    double m_fovMin = 0.1;
    double m_fovMax = 10.0;

    QDateTime m_startTime;
    std::vector<uint8_t> m_imageBuffer;

    bool m_isSolving = false;
    QMutex m_mutex;
};

#endif // STELLAR_SOLVER_INTERFACE_H
