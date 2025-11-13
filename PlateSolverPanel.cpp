// PlateSolverPanel.cpp
// Implementation of the plate solver panel using StellarSolver
// Author: Claude

#include "PlateSolverPanel.h"

#include <QFileDialog>
#include <QMessageBox>

PlateSolverPanel::PlateSolverPanel(INDIClient *client, QWidget *parent)
    : QWidget(parent),
      m_client(client)
{
    // Create StellarSolver interface
//    m_solver = new StellarSolverInterface(this);
    
    // Set up the UI
    setupUI();
    /*    
    // Connect signals from solver
    connect(m_solver, &StellarSolverInterface::solveComplete, 
            this, &PlateSolverPanel::onSolverFinished);
    connect(m_solver, &StellarSolverInterface::statusUpdate, 
            this, &PlateSolverPanel::onSolverStatusUpdate);
    */
}

PlateSolverPanel::~PlateSolverPanel()
{
  /*
    // Clean up solver (if needed)
    if (m_solver && m_solver->isSolving()) {
        m_solver->abort();
    }
  */
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
    
    QLabel *searchRadiusLabel = new QLabel("Search Radius (degrees):", settingsGroup);
    searchRadiusSpinBox = new QDoubleSpinBox(settingsGroup);
    searchRadiusSpinBox->setRange(0.5, 180.0);
    searchRadiusSpinBox->setValue(2.0);
    searchRadiusSpinBox->setSingleStep(0.5);
    
    QLabel *catalogLabel = new QLabel("Index Files:", settingsGroup);
    catalogPathEdit = new QLineEdit(settingsGroup);
    catalogPathEdit->setReadOnly(true);
    
    browseButton = new QPushButton("Browse", settingsGroup);
    
    QPushButton *updateButton = new QPushButton("Update Settings", settingsGroup);
    
    settingsLayout->addWidget(fovLabel, 0, 0);
    settingsLayout->addWidget(fovLowSpinBox, 0, 1);
    settingsLayout->addWidget(new QLabel("to", settingsGroup), 0, 2);
    settingsLayout->addWidget(fovHighSpinBox, 0, 3);
    settingsLayout->addWidget(searchRadiusLabel, 1, 0);
    settingsLayout->addWidget(searchRadiusSpinBox, 1, 1, 1, 3);
    settingsLayout->addWidget(catalogLabel, 2, 0);
    settingsLayout->addWidget(catalogPathEdit, 2, 1, 1, 2);
    settingsLayout->addWidget(browseButton, 2, 3);
    settingsLayout->addWidget(updateButton, 3, 0, 1, 4);
    
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
    connect(updateButton, &QPushButton::clicked, this, &PlateSolverPanel::updateSettings);
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Astrometry Index Directory", QDir::homePath());
        if (!path.isEmpty()) {
            catalogPathEdit->setText(path);
        }
    });
    
    // Initial state
    solveButton->setEnabled(false);
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
        
        // For FITS files, we'd ideally use CFITSIO directly
        // But for simplicity, let's create a placeholder
        emit logMessage("FITS loading would require CFITSIO integration");
        QMessageBox::information(this, "FITS Loading", 
            "FITS file detected. In a complete implementation, this would use CFITSIO to load the file properly.");
        
        // Create a dummy image for demonstration
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
    
    // Configure solver with current settings
    updateSettings();
    
    // Start solving
    //    m_solver->solveImage(m_currentImage);
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
  /*
  // Update solver settings
    m_solver->setFovRange(fovLowSpinBox->value(), fovHighSpinBox->value());
    m_solver->setSearchRadius(searchRadiusSpinBox->value());
    
    // If a custom catalog path was provided, we would set it here
    // But this requires modifications to the StellarSolverInterface
    */    
    emit logMessage("Updated plate solver settings");
}

void PlateSolverPanel::onSolverStatusUpdate(const QString &status)
{
    statusLabel->setText(status);
    emit logMessage(status);
}

void PlateSolverPanel::onSolverFinished(const StellarSolverInterface::SolveResult &result)
{
    // Update UI
    solveButton->setEnabled(true);
    abortButton->setEnabled(false);
    loadImageButton->setEnabled(true);
    
    if (result.success) {
        // Update labels
        coordinatesLabel->setText(QString("RA: %1°, Dec: %2°")
                                .arg(result.ra, 0, 'f', 6)
                                .arg(result.dec, 0, 'f', 6));
        
        pixscaleLabel->setText(QString("%1 arcsec/pixel")
                             .arg(result.pixelScale, 0, 'f', 3));
        
        angleLabel->setText(QString("%1°")
                          .arg(result.orientation, 0, 'f', 2));
        
        // Emit solution found signal for other panels to use
        emit solutionFound(result.ra, result.dec, result.pixelScale, result.orientation);
        
        emit logMessage(QString("Solution found: RA=%1°, Dec=%2°, Scale=%3\"/px, Angle=%4°")
                      .arg(result.ra, 0, 'f', 6)
                      .arg(result.dec, 0, 'f', 6)
                      .arg(result.pixelScale, 0, 'f', 3)
                      .arg(result.orientation, 0, 'f', 2));
    } else {
        emit logMessage("Plate solving failed: " + result.statusMessage);
    }
}

void PlateSolverPanel::solverFinished(bool success)
{

}

