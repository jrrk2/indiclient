// PlateSolverPanel.cpp
// Implementation of the plate solver panel
// Author: Claude

#include "INDITestClient.h"

#include <QFileDialog>
#include <QMessageBox>

// Include headers for libsolver - this is a simplified version
// In a real implementation, we would need to include the actual libsolver headers
namespace {
    // Forward declaration of StarSolver for this example
    class StarSolver : public QObject {
        Q_OBJECT
    public:
        explicit StarSolver(QObject *parent = nullptr) : QObject(parent) {}
        
        void setImage(const QImage &image) { m_image = image; }
        void setImagePath(const QString &path) { m_imagePath = path; }
        void setFOVRange(double low, double high) { m_fovLow = low; m_fovHigh = high; }
        void setCatalogPath(const QString &path) { m_catalogPath = path; }
        void setUseOnlineService(bool use) { m_useOnline = use; }
        
        void abort() { m_aborted = true; emit finished(false); }
        void solve() { 
            // Simulated solving process
            QTimer::singleShot(2000, this, [this]() { 
                if (!m_aborted) {
                    m_ra = 12.345;
                    m_dec = 45.678;
                    m_pixscale = 1.23;
                    m_angle = 90.0;
                    emit finished(true); 
                }
            }); 
        }
        
        double getRA() const { return m_ra; }
        double getDec() const { return m_dec; }
        double getPixScale() const { return m_pixscale; }
        double getAngle() const { return m_angle; }
        
    signals:
        void finished(bool success);
        
    private:
        QImage m_image;
        QString m_imagePath;
        double m_fovLow = 0.1;
        double m_fovHigh = 10.0;
        QString m_catalogPath;
        bool m_useOnline = false;
        bool m_aborted = false;
        
        // Solution results
        double m_ra = 0.0;
        double m_dec = 0.0;
        double m_pixscale = 0.0;
        double m_angle = 0.0;
    };
}

PlateSolverPanel::PlateSolverPanel(INDIClient *client, QWidget *parent)
    : QWidget(parent),
      m_client(client),
      m_solver(nullptr),
      m_solverThread(nullptr)
{
    // Set up the UI
    setupUI();
    
    // Create the solver and thread
    m_solver = new StarSolver();
    m_solverThread = new QThread(this);
    m_solver->moveToThread(m_solverThread);
    
    // Connect signals
    connect(m_solver, &StarSolver::finished, this, &PlateSolverPanel::solverFinished);
    connect(m_solverThread, &QThread::finished, m_solver, &QObject::deleteLater);
    
    // Start the thread
    m_solverThread->start();
}

PlateSolverPanel::~PlateSolverPanel()
{
    // Clean up thread
    if (m_solverThread) {
        m_solverThread->quit();
        m_solverThread->wait();
    }
}

void PlateSolverPanel::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Settings group
    settingsGroup = new QGroupBox("Solver Settings", this);
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);
    
    QLabel *fovLabel = new QLabel("FOV Range (degrees):", settingsGroup);
    fovLowSpinBox = new QDoubleSpinBox(settingsGroup);
    fovLowSpinBox->setRange(0.1, 10.0);
    fovLowSpinBox->setValue(0.5);
    fovLowSpinBox->setSingleStep(0.1);
    
    fovHighSpinBox = new QDoubleSpinBox(settingsGroup);
    fovHighSpinBox->setRange(0.1, 180.0);
    fovHighSpinBox->setValue(5.0);
    fovHighSpinBox->setSingleStep(0.5);
    
    QLabel *catalogLabel = new QLabel("Catalog Path:", settingsGroup);
    catalogPathEdit = new QLineEdit(settingsGroup);
    catalogPathEdit->setReadOnly(true);
    
    browseButton = new QPushButton("Browse", settingsGroup);
    
    useOnlineCheckBox = new QCheckBox("Use Online Service", settingsGroup);
    useOnlineCheckBox->setChecked(true);
    
    QPushButton *updateButton = new QPushButton("Update Settings", settingsGroup);
    
    settingsLayout->addWidget(fovLabel, 0, 0);
    settingsLayout->addWidget(fovLowSpinBox, 0, 1);
    settingsLayout->addWidget(new QLabel("to", settingsGroup), 0, 2);
    settingsLayout->addWidget(fovHighSpinBox, 0, 3);
    settingsLayout->addWidget(catalogLabel, 1, 0);
    settingsLayout->addWidget(catalogPathEdit, 1, 1, 1, 2);
    settingsLayout->addWidget(browseButton, 1, 3);
    settingsLayout->addWidget(useOnlineCheckBox, 2, 0, 1, 2);
    settingsLayout->addWidget(updateButton, 2, 2, 1, 2);
    
    // Control group
    QGroupBox *controlGroup = new QGroupBox("Image Control", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    
    loadImageButton = new QPushButton("Load Image", controlGroup);
    solveButton = new QPushButton("Solve", controlGroup);
    abortButton = new QPushButton("Abort", controlGroup);
    
    controlLayout->addWidget(loadImageButton);
    controlLayout->addWidget(solveButton);
    controlLayout->addWidget(abortButton);
    
    // Status group
    QGroupBox *statusGroup = new QGroupBox("Solution Status", this);
    QGridLayout *statusLayout = new QGridLayout(statusGroup);
    
    statusLabel = new QLabel("Ready", statusGroup);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    QLabel *coordTextLabel = new QLabel("Coordinates:", statusGroup);
    coordinatesLabel = new QLabel("Not solved", statusGroup);
    coordinatesLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    QLabel *pixscaleTextLabel = new QLabel("Pixel Scale:", statusGroup);
    pixscaleLabel = new QLabel("Unknown", statusGroup);
    pixscaleLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    QLabel *angleTextLabel = new QLabel("Position Angle:", statusGroup);
    angleLabel = new QLabel("Unknown", statusGroup);
    angleLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    statusLayout->addWidget(statusLabel, 0, 0, 1, 2);
    statusLayout->addWidget(coordTextLabel, 1, 0);
    statusLayout->addWidget(coordinatesLabel, 1, 1);
    statusLayout->addWidget(pixscaleTextLabel, 2, 0);
    statusLayout->addWidget(pixscaleLabel, 2, 1);
    statusLayout->addWidget(angleTextLabel, 3, 0);
    statusLayout->addWidget(angleLabel, 3, 1);
    
    // Add groups to main layout
    mainLayout->addWidget(settingsGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();
    
    // Connect signals
    connect(loadImageButton, &QPushButton::clicked, this, &PlateSolverPanel::loadImage);
    connect(solveButton, &QPushButton::clicked, this, &PlateSolverPanel::solve);
    connect(abortButton, &QPushButton::clicked, this, &PlateSolverPanel::abortSolve);
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Catalog Directory", QDir::homePath());
        if (!path.isEmpty()) {
            catalogPathEdit->setText(path);
        }
    });
    connect(updateButton, &QPushButton::clicked, this, &PlateSolverPanel::updateSettings);
    
    // Initial state
    abortButton->setEnabled(false);
}

void PlateSolverPanel::loadImage()
{
    // Open file dialog to select image
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QDir::homePath(),
        "Image Files (*.fits *.fit *.fts *.jpg *.jpeg *.png);;All Files (*)"
    );
    
    if (filePath.isEmpty())
        return;
    
    // Try to load the image
    QImage image;
    if (filePath.endsWith(".fits", Qt::CaseInsensitive) ||
        filePath.endsWith(".fit", Qt::CaseInsensitive) ||
        filePath.endsWith(".fts", Qt::CaseInsensitive)) {
        // For FITS files, we'd need a proper FITS reader
        // This is a placeholder
        emit logMessage("FITS loading not implemented in this example");
        
        // Create a dummy image
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
    } else {
        // For normal image formats
        if (!image.load(filePath)) {
            QMessageBox::warning(this, "Error", "Failed to load image");
            return;
        }
    }
    
    // Store image and path
    m_currentImage = image;
    m_currentImagePath = filePath;
    
    // Update UI
    emit logMessage(QString("Loaded image: %1").arg(filePath));
    statusLabel->setText("Image loaded");
    
    // Enable solve button
    solveButton->setEnabled(true);
}

void PlateSolverPanel::solveImage(const QImage &image)
{
    // Store image
    m_currentImage = image;
    m_currentImagePath = ""; // From camera, no file path
    
    // Update UI
    emit logMessage("Received image from camera");
    statusLabel->setText("Camera image received");
    
    // Enable solve button
    solveButton->setEnabled(true);
}

void PlateSolverPanel::solve()
{
    if (m_currentImage.isNull()) {
        QMessageBox::warning(this, "Error", "No image loaded");
        return;
    }
    
    // Update UI
    statusLabel->setText("Solving...");
    solveButton->setEnabled(false);
    abortButton->setEnabled(true);
    loadImageButton->setEnabled(false);
    
    // Start solver
    emit logMessage("Starting plate solver...");
    startSolver();
}

void PlateSolverPanel::abortSolve()
{
    if (m_solver) {
        m_solver->abort();
        emit logMessage("Aborting plate solve");
        
        // Update UI
        statusLabel->setText("Aborted");
        solveButton->setEnabled(true);
        abortButton->setEnabled(false);
        loadImageButton->setEnabled(true);
    }
}

void PlateSolverPanel::updateSettings()
{
    emit logMessage("Updating plate solver settings");
    
    // In a real implementation, we would update the solver configuration
    // This is a placeholder
}

void PlateSolverPanel::startSolver()
{
    // Configure solver
    m_solver->setImage(m_currentImage);
    if (!m_currentImagePath.isEmpty()) {
        m_solver->setImagePath(m_currentImagePath);
    }
    
    m_solver->setFOVRange(fovLowSpinBox->value(), fovHighSpinBox->value());
    m_solver->setCatalogPath(catalogPathEdit->text());
    m_solver->setUseOnlineService(useOnlineCheckBox->isChecked());
    
    // Start solving
    QTimer::singleShot(0, m_solver, &StarSolver::solve);
}

void PlateSolverPanel::solverFinished(bool success)
{
    // Update UI
    solveButton->setEnabled(true);
    abortButton->setEnabled(false);
    loadImageButton->setEnabled(true);
    
    if (success) {
        double ra = m_solver->getRA();
        double dec = m_solver->getDec();
        double pixscale = m_solver->getPixScale();
        double angle = m_solver->getAngle();
        
        // Update labels
        statusLabel->setText("Solved successfully");
        coordinatesLabel->setText(QString("RA: %1, Dec: %2").arg(ra, 0, 'f', 6).arg(dec, 0, 'f', 6));
        pixscaleLabel->setText(QString("%1 arcsec/pixel").arg(pixscale, 0, 'f', 3));
        angleLabel->setText(QString("%1 degrees").arg(angle, 0, 'f', 2));
        
        // Emit solution found signal
        emit solutionFound(ra, dec, pixscale, angle);
        emit logMessage(QString("Solution found: RA=%1, Dec=%2, Scale=%3 \"/px, Angle=%4°")
                      .arg(ra, 0, 'f', 6)
                      .arg(dec, 0, 'f', 6)
                      .arg(pixscale, 0, 'f', 3)
                      .arg(angle, 0, 'f', 2));
    } else {
        statusLabel->setText("Solving failed");
        emit logMessage("Plate solving failed");
    }
}

// Include the QObject meta-object stuff for our StarSolver mock class
#include "PlateSolverPanel.moc"