#include "StellarSolverInterface.h"
#include <QDebug>
#include <fitsio.h>

StellarSolverInterface::StellarSolverInterface(QObject *parent)
    : QObject(parent)
{
    // Find index files
    m_indexPaths = findIndexFiles();
    if (m_indexPaths.isEmpty()) {
        qWarning() << "No astrometry index files found. Plate solving may fail.";
    }
    
    // Setup default parameters
    setupParameters();
}

StellarSolverInterface::~StellarSolverInterface()
{
    abort();
}

void StellarSolverInterface::solveImage(const QImage &image, double raHint, double decHint,
                                       double fovMin, double fovMax)
{
    QMutexLocker locker(&m_mutex);
    
    // Update hints if provided
    if (raHint > -1) m_raHint = raHint;
    if (decHint > -91) m_decHint = decHint;
    if (fovMin > 0) m_fovMin = fovMin;
    if (fovMax > 0) m_fovMax = fovMax;
    
    // Clean up previous solver if still running
    abort();
    
    // Create new solver and thread
    m_solver = new StellarSolver(this);
    m_solverThread = new QThread();
    m_solver->moveToThread(m_solverThread);
    
    // Connect signals
    connect(m_solver, &StellarSolver::finished, this, &StellarSolverInterface::handleSolverFinished);
    connect(m_solverThread, &QThread::finished, m_solver, &QObject::deleteLater);
    
    // Configure solver
    m_solver->setProperty("ProcessType", SSolver::SOLVE);
    m_solver->setProperty("ExtractorType", SSolver::EXTRACTOR_INTERNAL);
    m_solver->setProperty("SolverType", SSolver::SOLVER_STELLARSOLVER);
    m_solver->setIndexFolderPaths(m_indexPaths);
    m_solver->setParameters(m_params);
    
    // Set search parameters
    if (m_raHint > -1 && m_decHint > -91) {
        m_solver->setSearchPositionInDegrees(m_raHint, m_decHint);
        emit statusUpdate(QString("Searching around RA=%1°, Dec=%2°")
                         .arg(m_raHint, 0, 'f', 6)
                         .arg(m_decHint, 0, 'f', 6));
    }
    
    // Prepare the image
    if (!prepareImageForSolver(image)) {
        emit statusUpdate("Failed to prepare image for solving");
        delete m_solver;
        delete m_solverThread;
        m_solver = nullptr;
        m_solverThread = nullptr;
        return;
    }
    
    // Start timing
    m_startTime = QDateTime::currentDateTime();
    m_isSolving = true;
    
    // Start the thread and solver
    m_solverThread->start();
    emit statusUpdate("Starting plate solver...");
    QMetaObject::invokeMethod(m_solver, "start", Qt::QueuedConnection);
}

void StellarSolverInterface::abort()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_solver && m_isSolving) {
        m_solver->abort();
        m_isSolving = false;
    }
    
    if (m_solverThread) {
        m_solverThread->quit();
        m_solverThread->wait(1000);  // Give it 1 second to shut down
        
        // Force termination if needed
        if (m_solverThread->isRunning()) {
            m_solverThread->terminate();
            m_solverThread->wait();
        }
        
        delete m_solverThread;
        m_solverThread = nullptr;
    }
    
    m_solver = nullptr;  // Will be deleted via parent-child relationship
}

bool StellarSolverInterface::isSolving() const
{
    QMutexLocker locker(&const_cast<QMutex&>(m_mutex));
    return m_isSolving;
}

void StellarSolverInterface::handleSolverFinished()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_solver) {
        return;
    }
    
    // Calculate solving time
    qint64 totalMs = m_startTime.msecsTo(QDateTime::currentDateTime());
    double solveTimeSeconds = totalMs / 1000.0;
    
    // Create result structure
    SolveResult result;
    result.solveTime = solveTimeSeconds;
    
    if (m_solver->solvingDone() && m_solver->hasWCSData()) {
        FITSImage::Solution solution = m_solver->getSolution();
        
        result.success = true;
        result.ra = solution.ra;
        result.dec = solution.dec;
        result.pixelScale = solution.pixscale;  // arcsec/pixel
        result.orientation = solution.orientation;
        result.fieldWidth = solution.fieldWidth;
        result.fieldHeight = solution.fieldHeight;
        
        result.statusMessage = QString("Solved: RA=%1°, Dec=%2°, Scale=%3\"/px, Angle=%4°")
                              .arg(solution.ra, 0, 'f', 6)
                              .arg(solution.dec, 0, 'f', 6)
                              .arg(solution.pixscale, 0, 'f', 3)
                              .arg(solution.orientation, 0, 'f', 2);
        
        emit statusUpdate(result.statusMessage);
    } else {
        result.success = false;
        
        if (m_solver->failed()) {
            result.statusMessage = "Solving failed";
        } else if (!m_solver->solvingDone()) {
            result.statusMessage = "Solving incomplete";
        } else {
            result.statusMessage = "No WCS data available";
        }
        
        emit statusUpdate(result.statusMessage);
    }
    
    // Reset state
    m_isSolving = false;
    
    // Clean up
    if (m_solverThread) {
        m_solverThread->quit();
        m_solverThread->wait();
        delete m_solverThread;
        m_solverThread = nullptr;
    }
    
    m_solver = nullptr;
    
    // Emit the result
    emit solveComplete(result);
}

void StellarSolverInterface::setupParameters()
{
    // Get built-in profiles
    QList<Parameters> profiles = StellarSolver::getBuiltInProfiles();
    if (profiles.isEmpty()) {
        qWarning() << "No parameter profiles available for StellarSolver";
        return;
    }
    
    m_params = profiles.at(0); // Use first profile as base
    
    // Configure parameters
    m_params.multiAlgorithm = SSolver::MULTI_AUTO;
    m_params.search_radius = m_searchRadius;
    m_params.minwidth = m_fovMin;
    m_params.maxwidth = m_fovMax;
    m_params.resort = true;
    m_params.autoDownsample = true;
    m_params.inParallel = true;
    m_params.solverTimeLimit = 60;  // 60-second timeout
    
    // Star extraction parameters
    m_params.initialKeep = 1000;
    m_params.keepNum = 500;
    m_params.r_min = 1.0;
    m_params.removeBrightest = 0;
    m_params.removeDimmest = 20;
    m_params.saturationLimit = 65000;
    m_params.minarea = 5;
    m_params.threshold_offset = 0;
    m_params.threshold_bg_multiple = 2.0;
}

QStringList StellarSolverInterface::findIndexFiles()
{
    QStringList indexPaths;
    QStringList searchPaths = {
        "/usr/local/astrometry/data",
        "/opt/homebrew/share/astrometry",
        "/usr/local/share/astrometry",
        "/usr/share/astrometry"
    };
    
    for (const QString &path : searchPaths) {
        QDir indexDir(path);
        if (indexDir.exists()) {
            QStringList filters;
            filters << "index-*.fits";
            QFileInfoList indexFiles = indexDir.entryInfoList(filters, QDir::Files);
            
            if (!indexFiles.isEmpty()) {
                indexPaths.append(path);
                qDebug() << "Found astrometry index files in:" << path;
                qDebug() << "  First index file:" << indexFiles.first().fileName();
                break;
            }
        }
    }
    
    return indexPaths;
}

bool StellarSolverInterface::prepareImageForSolver(const QImage &image)
{
    if (image.isNull()) {
        return false;
    }
    
    // Convert to grayscale if needed
    QImage grayscaleImage;
    if (image.format() == QImage::Format_Grayscale8) {
        grayscaleImage = image;
    } else {
        grayscaleImage = image.convertToFormat(QImage::Format_Grayscale8);
    }
    
    int width = grayscaleImage.width();
    int height = grayscaleImage.height();
    
    // Create statistics for the image
    FITSImage::Statistic stats{};
    stats.width = width;
    stats.height = height;
    stats.channels = 1;
    stats.dataType = TBYTE;
    stats.bytesPerPixel = 1;
    
    // Copy the image data
    m_imageBuffer.resize(width * height);
    
    // Copy data from QImage to our buffer
    for (int y = 0; y < height; ++y) {
        const uchar *scanLine = grayscaleImage.scanLine(y);
        for (int x = 0; x < width; ++x) {
            m_imageBuffer[y * width + x] = scanLine[x];
        }
    }
    
    // Calculate basic statistics for the image
    uchar minVal = 255, maxVal = 0;
    double sum = 0;
    
    for (const uchar &pixel : m_imageBuffer) {
        minVal = std::min(minVal, pixel);
        maxVal = std::max(maxVal, pixel);
        sum += pixel;
    }
    
    double mean = sum / m_imageBuffer.size();
    
    for (int i = 0; i < 3; i++) {
        stats.min[i] = (i == 0) ? minVal : 0.0;
        stats.max[i] = (i == 0) ? maxVal : 0.0;
        stats.mean[i] = (i == 0) ? mean : 0.0;
        stats.stddev[i] = 0.0;
        stats.median[i] = (i == 0) ? mean : 0.0;
    }
    
    stats.SNR = 1.0;
    
    // Load the image into the solver
    return m_solver->loadNewImageBuffer(stats, m_imageBuffer.data());
}
