// CameraPanel.cpp
// Implementation of the Camera control panel
// Author: Claude

#include "CameraPanel.h"

CameraPanel::CameraPanel(INDIClient *client, QWidget *parent)
    : QWidget(parent),
      m_client(client),
      isContinuousCapture(false)
{
    // Set up the UI
    setupUI();
    
    // Create a timer for continuous capture
    captureTimer = new QTimer(this);
    connect(captureTimer, &QTimer::timeout, this, &CameraPanel::captureImage);
    
    // Initial update of device list
    updateDeviceList();
}

CameraPanel::~CameraPanel()
{
    if (captureTimer->isActive()) {
        captureTimer->stop();
    }
}

void CameraPanel::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Camera selection
    QGroupBox *cameraGroup = new QGroupBox("Camera Device", this);
    QHBoxLayout *cameraLayout = new QHBoxLayout(cameraGroup);
    
    cameraComboBox = new QComboBox(cameraGroup);
    connectButton = new QPushButton("Connect", cameraGroup);
    disconnectButton = new QPushButton("Disconnect", cameraGroup);
    disconnectButton->setEnabled(false);
    
    cameraLayout->addWidget(cameraComboBox);
    cameraLayout->addWidget(connectButton);
    cameraLayout->addWidget(disconnectButton);
    
    // Exposure controls
    QGroupBox *exposureGroup = new QGroupBox("Exposure Settings", this);
    QGridLayout *exposureLayout = new QGridLayout(exposureGroup);
    
    QLabel *exposureLabel = new QLabel("Exposure Time (s):", exposureGroup);
    exposureSpinBox = new QDoubleSpinBox(exposureGroup);
    exposureSpinBox->setRange(0.001, 3600.0);
    exposureSpinBox->setValue(1.0);
    exposureSpinBox->setDecimals(3);
    exposureSpinBox->setSingleStep(0.1);
    
    QLabel *gainLabel = new QLabel("Gain:", exposureGroup);
    gainSpinBox = new QSpinBox(exposureGroup);
    gainSpinBox->setRange(0, 100);
    gainSpinBox->setValue(50);
    
    QLabel *binningLabel = new QLabel("Binning:", exposureGroup);
    binningComboBox = new QComboBox(exposureGroup);
    binningComboBox->addItem("1x1", 1);
    binningComboBox->addItem("2x2", 2);
    binningComboBox->addItem("3x3", 3);
    binningComboBox->addItem("4x4", 4);
    
    exposureLayout->addWidget(exposureLabel, 0, 0);
    exposureLayout->addWidget(exposureSpinBox, 0, 1);
    exposureLayout->addWidget(gainLabel, 1, 0);
    exposureLayout->addWidget(gainSpinBox, 1, 1);
    exposureLayout->addWidget(binningLabel, 2, 0);
    exposureLayout->addWidget(binningComboBox, 2, 1);
    
    // Capture controls
    QGroupBox *captureGroup = new QGroupBox("Capture Controls", this);
    QHBoxLayout *captureLayout = new QHBoxLayout(captureGroup);
    
    captureButton = new QPushButton("Capture", captureGroup);
    abortButton = new QPushButton("Abort", captureGroup);
    continuousCaptureCheckBox = new QCheckBox("Continuous", captureGroup);
    
    captureLayout->addWidget(captureButton);
    captureLayout->addWidget(abortButton);
    captureLayout->addWidget(continuousCaptureCheckBox);
    
    // Add groups to main layout
    mainLayout->addWidget(cameraGroup);
    mainLayout->addWidget(exposureGroup);
    mainLayout->addWidget(captureGroup);
    mainLayout->addStretch();
    
    // Connect signals
    connect(connectButton, &QPushButton::clicked, this, &CameraPanel::connectCamera);
    connect(disconnectButton, &QPushButton::clicked, this, &CameraPanel::disconnectCamera);
    connect(captureButton, &QPushButton::clicked, this, &CameraPanel::captureImage);
    connect(abortButton, &QPushButton::clicked, this, &CameraPanel::abortCapture);
    connect(exposureSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &CameraPanel::updateExposure);
    connect(gainSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &CameraPanel::updateGain);
    connect(binningComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &CameraPanel::updateBinning);
    connect(continuousCaptureCheckBox, &QCheckBox::toggled, [this](bool checked) {
        isContinuousCapture = checked;
        if (checked && m_client->isConnected()) {
            captureTimer->start(100); // Start with a short delay
        } else {
            captureTimer->stop();
        }
    });
    
    // Store references to group boxes for easy enable/disable
    // Note: We don't use qobject_cast here, we keep direct pointers
    m_exposureGroup = exposureGroup;
    m_captureGroup = captureGroup;
    
    // Disable controls initially
    exposureGroup->setEnabled(false);
    captureGroup->setEnabled(false);
}

void CameraPanel::updateDeviceList()
{
    // Save current selection
    QString currentCamera = cameraComboBox->currentText();
    
    // Clear the list
    cameraComboBox->clear();
    
    // Add cameras from client
    if (m_client->isConnected()) {
        QStringList cameras = m_client->getCameraList();
        cameraComboBox->addItems(cameras);
        
        emit logMessage(QString("Updated camera list: %1 cameras found").arg(cameras.size()));
        
        // Restore previous selection if possible
        int index = cameraComboBox->findText(currentCamera);
        if (index >= 0) {
            cameraComboBox->setCurrentIndex(index);
        }
    }
    
    // Update button states
    connectButton->setEnabled(cameraComboBox->count() > 0 && m_client->isConnected());
    disconnectButton->setEnabled(false);
}

void CameraPanel::onDeviceConnected(const QString &deviceName)
{
    // Check if UI is fully initialized
    if (!cameraComboBox || !connectButton || !disconnectButton) {
        emit logMessage("onDeviceConnected called before UI setup complete, ignoring");
        return;
    }
    
    // Check if this is a camera device
    QStringList cameraList = m_client->getCameraList();
    if (cameraList.contains(deviceName)) {
        emit logMessage(QString("Camera connected event: %1").arg(deviceName));
        
        // Update device list
        updateDeviceList();
        
        // Select the connected camera
        int index = cameraComboBox->findText(deviceName);
        if (index >= 0) {
            cameraComboBox->setCurrentIndex(index);
        }
        
        // Update button states if this is the current camera
        if (cameraComboBox->currentText() == deviceName) {
            emit logMessage(QString("Enabling controls for camera: %1").arg(deviceName));
            
            connectButton->setEnabled(false);
            disconnectButton->setEnabled(true);
            
            // Enable exposure and capture controls
            if (m_exposureGroup) {
                m_exposureGroup->setEnabled(true);
                emit logMessage("Exposure group enabled");
            }
            if (m_captureGroup) {
                m_captureGroup->setEnabled(true);
                emit logMessage("Capture group enabled");
            }
        }
    }
}

void CameraPanel::onDeviceDisconnected(const QString &deviceName)
{
    emit logMessage(QString("Device disconnected event: %1").arg(deviceName));
    
    // Check if UI is fully initialized
    if (!cameraComboBox || !connectButton || !disconnectButton) {
        emit logMessage("onDeviceDisconnected called before UI setup complete, ignoring");
        return;
    }
    
    // Check if this is the current camera
    QString crnt = cameraComboBox->currentText();
    qDebug() << "current camera" << crnt;
    if (crnt == deviceName) {
        // Update button states
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        
        // Disable exposure and capture controls
        if (m_exposureGroup) {
            m_exposureGroup->setEnabled(false);
            emit logMessage("Exposure group disabled");
        }
        if (m_captureGroup) {
            m_captureGroup->setEnabled(false);
            emit logMessage("Capture group disabled");
        }
        
        // Stop continuous capture if active
        if (captureTimer && captureTimer->isActive()) {
            captureTimer->stop();
            if (continuousCaptureCheckBox) {
                continuousCaptureCheckBox->setChecked(false);
            }
        }
    }
}

void CameraPanel::connectCamera()
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty()) {
        emit logMessage("No camera selected");
        return;
    }
    
    emit logMessage(QString("Attempting to connect camera: %1").arg(cameraName));
    
    // Connect the camera
    if (m_client->connectDevice(cameraName)) {
        emit logMessage(QString("Connect request sent for camera %1").arg(cameraName));
        
        // Note: Don't enable controls here yet - wait for the deviceConnected signal
        // The INDI server will send a CONNECTION property update when connection succeeds
        connectButton->setEnabled(false);
    } else {
        emit logMessage(QString("Failed to send connect request for camera %1").arg(cameraName));
    }
}

void CameraPanel::disconnectCamera()
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty()) return;
    
    emit logMessage(QString("Attempting to disconnect camera: %1").arg(cameraName));
    
    // Disconnect the camera
    if (m_client->disconnectDevice(cameraName)) {
        emit logMessage(QString("Disconnect request sent for camera %1").arg(cameraName));
        
        // Disable controls immediately (don't wait for signal)
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        
        if (m_exposureGroup) m_exposureGroup->setEnabled(false);
        if (m_captureGroup) m_captureGroup->setEnabled(false);
        
        // Stop continuous capture if active
        if (captureTimer->isActive()) {
            captureTimer->stop();
            continuousCaptureCheckBox->setChecked(false);
        }
    } else {
        emit logMessage(QString("Failed to send disconnect request for camera %1").arg(cameraName));
    }
}

void CameraPanel::captureImage()
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty()) {
        emit logMessage("No camera selected for capture");
        return;
    }
    
    // Set exposure time
    double exposureTime = exposureSpinBox->value();
    if (!m_client->setCameraExposure(cameraName, exposureTime)) {
        emit logMessage(QString("Failed to set exposure time for %1").arg(cameraName));
        return;
    }
    
    // Start capture
    if (m_client->takeCameraExposure(cameraName)) {
        emit logMessage(QString("Starting %1s exposure with %2").arg(exposureTime).arg(cameraName));
        
        if (isContinuousCapture) {
            // Schedule next capture after current exposure plus a small buffer time
            captureTimer->setInterval((exposureTime * 1000) + 500);
        }
    } else {
        emit logMessage(QString("Failed to start exposure with %1").arg(cameraName));
        
        // Stop continuous capture if it failed
        if (isContinuousCapture) {
            captureTimer->stop();
            continuousCaptureCheckBox->setChecked(false);
            isContinuousCapture = false;
        }
    }
}

void CameraPanel::abortCapture()
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty()) return;
    
    // Abort current exposure
    if (m_client->abortCameraExposure(cameraName)) {
        emit logMessage(QString("Aborting exposure with %1").arg(cameraName));
        
        // Stop continuous capture
        if (isContinuousCapture) {
            captureTimer->stop();
            continuousCaptureCheckBox->setChecked(false);
            isContinuousCapture = false;
        }
    } else {
        emit logMessage(QString("Failed to abort exposure with %1").arg(cameraName));
    }
}

void CameraPanel::updateExposure(double value)
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty() || !m_client->isConnected()) return;
    
    // Update exposure time on camera if connected
    if (disconnectButton->isEnabled()) {
        m_client->setCameraExposure(cameraName, value);
        emit logMessage(QString("Updated exposure time: %1s").arg(value));
    }
}

void CameraPanel::updateGain(int value)
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty() || !m_client->isConnected()) return;
    
    // We would need to implement gain control in the INDI client
    // This is a placeholder
    emit logMessage(QString("Updated gain: %1").arg(value));
}

void CameraPanel::updateBinning(int index)
{
    QString cameraName = cameraComboBox->currentText();
    if (cameraName.isEmpty() || !m_client->isConnected() || index < 0) return;
    
    int binning = binningComboBox->itemData(index).toInt();
    
    // We would need to implement binning control in the INDI client
    // This is a placeholder
    emit logMessage(QString("Updated binning: %1x%1").arg(binning));
}
