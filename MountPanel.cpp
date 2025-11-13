// MountPanel.cpp
// Implementation of the Mount control panel
// Author: Claude

#include "MountPanel.h"

MountPanel::MountPanel(INDIClient *client, QWidget *parent)
    : QWidget(parent),
      m_client(client)
{
    // Set up the UI
    setupUI();
    
    // Initial update of device list
    updateDeviceList();
}

MountPanel::~MountPanel()
{
    // Nothing to clean up
}

void MountPanel::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Mount selection
    QGroupBox *mountGroup = new QGroupBox("Mount Device", this);
    QHBoxLayout *mountLayout = new QHBoxLayout(mountGroup);
    
    mountComboBox = new QComboBox(mountGroup);
    connectButton = new QPushButton("Connect", mountGroup);
    disconnectButton = new QPushButton("Disconnect", mountGroup);
    disconnectButton->setEnabled(false);
    
    mountLayout->addWidget(mountComboBox);
    mountLayout->addWidget(connectButton);
    mountLayout->addWidget(disconnectButton);
    
    // Coordinates group
    QGroupBox *coordGroup = new QGroupBox("Coordinates", this);
    QGridLayout *coordLayout = new QGridLayout(coordGroup);
    
    QLabel *raLabel = new QLabel("RA (hours):", coordGroup);
    raSpinBox = new QDoubleSpinBox(coordGroup);
    raSpinBox->setRange(0.0, 24.0);
    raSpinBox->setValue(0.0);
    raSpinBox->setDecimals(6);
    raSpinBox->setSingleStep(0.1);
    
    QLabel *decLabel = new QLabel("Dec (degrees):", coordGroup);
    decSpinBox = new QDoubleSpinBox(coordGroup);
    decSpinBox->setRange(-90.0, 90.0);
    decSpinBox->setValue(0.0);
    decSpinBox->setDecimals(6);
    decSpinBox->setSingleStep(0.1);
    
    gotoButton = new QPushButton("Goto", coordGroup);
    syncButton = new QPushButton("Sync", coordGroup);
    
    coordLayout->addWidget(raLabel, 0, 0);
    coordLayout->addWidget(raSpinBox, 0, 1);
    coordLayout->addWidget(decLabel, 1, 0);
    coordLayout->addWidget(decSpinBox, 1, 1);
    coordLayout->addWidget(gotoButton, 2, 0);
    coordLayout->addWidget(syncButton, 2, 1);
    
    // Current position group
    QGroupBox *positionGroup = new QGroupBox("Current Position", this);
    QGridLayout *positionLayout = new QGridLayout(positionGroup);
    
    QLabel *currentRaTextLabel = new QLabel("RA:", positionGroup);
    currentRaLabel = new QLabel("00h 00m 00s", positionGroup);
    currentRaLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    currentRaLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    QLabel *currentDecTextLabel = new QLabel("Dec:", positionGroup);
    currentDecLabel = new QLabel("+00° 00' 00\"", positionGroup);
    currentDecLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    currentDecLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    
    positionLayout->addWidget(currentRaTextLabel, 0, 0);
    positionLayout->addWidget(currentRaLabel, 0, 1);
    positionLayout->addWidget(currentDecTextLabel, 1, 0);
    positionLayout->addWidget(currentDecLabel, 1, 1);
    
    // Control buttons group
    QGroupBox *controlGroup = new QGroupBox("Mount Control", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    
    stopButton = new QPushButton("Stop", controlGroup);
    parkButton = new QPushButton("Park", controlGroup);
    unparkButton = new QPushButton("Unpark", controlGroup);
    homeButton = new QPushButton("Home", controlGroup);
    
    controlLayout->addWidget(stopButton);
    controlLayout->addWidget(parkButton);
    controlLayout->addWidget(unparkButton);
    controlLayout->addWidget(homeButton);
    
    // Add groups to main layout
    mainLayout->addWidget(mountGroup);
    mainLayout->addWidget(coordGroup);
    mainLayout->addWidget(positionGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addStretch();
    
    // Connect signals
    connect(connectButton, &QPushButton::clicked, this, &MountPanel::connectMount);
    connect(disconnectButton, &QPushButton::clicked, this, &MountPanel::disconnectMount);
    connect(gotoButton, &QPushButton::clicked, this, &MountPanel::gotoCoordinates);
    connect(syncButton, &QPushButton::clicked, this, &MountPanel::syncCoordinates);
    connect(stopButton, &QPushButton::clicked, this, &MountPanel::stopMount);
    connect(parkButton, &QPushButton::clicked, this, &MountPanel::parkMount);
    connect(unparkButton, &QPushButton::clicked, this, &MountPanel::unparkMount);
    connect(homeButton, &QPushButton::clicked, this, &MountPanel::homeMount);
    
    // Disable controls initially
    coordGroup->setEnabled(false);
    positionGroup->setEnabled(false);
    controlGroup->setEnabled(false);
}

void MountPanel::updateDeviceList()
{
    // Save current selection
    QString currentMount = mountComboBox->currentText();
    
    // Clear the list
    mountComboBox->clear();
    
    // Add mounts from client
    if (m_client->isConnected()) {
        mountComboBox->addItems(m_client->getMountList());
        
        // Restore previous selection if possible
        int index = mountComboBox->findText(currentMount);
        if (index >= 0) {
            mountComboBox->setCurrentIndex(index);
        }
    }
    
    // Update button states
    connectButton->setEnabled(mountComboBox->count() > 0 && m_client->isConnected());
    disconnectButton->setEnabled(false);
}

void MountPanel::onDeviceConnected(const QString &deviceName)
{
    // Check if this is a mount device
    QStringList mountList = m_client->getMountList();
    if (mountList.contains(deviceName)) {
        // Update device list
        updateDeviceList();
        
        // Select the connected mount
        int index = mountComboBox->findText(deviceName);
        if (index >= 0) {
            mountComboBox->setCurrentIndex(index);
        }
        
        // Update button states if this is the current mount
        if (mountComboBox->currentText() == deviceName) {
            connectButton->setEnabled(false);
            disconnectButton->setEnabled(true);
            
            // Enable control groups
            QGroupBox *coordGroup = qobject_cast<QGroupBox*>(raSpinBox->parent());
            QGroupBox *positionGroup = qobject_cast<QGroupBox*>(currentRaLabel->parent());
            QGroupBox *controlGroup = qobject_cast<QGroupBox*>(stopButton->parent());
            
            if (coordGroup) coordGroup->setEnabled(true);
            if (positionGroup) positionGroup->setEnabled(true);
            if (controlGroup) controlGroup->setEnabled(true);
        }
    }
}

void MountPanel::onDeviceDisconnected(const QString &deviceName)
{
    // Check if this is the current mount
    if (mountComboBox->currentText() == deviceName) {
        // Update button states
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        
        // Disable control groups
        QGroupBox *coordGroup = qobject_cast<QGroupBox*>(raSpinBox->parent());
        QGroupBox *positionGroup = qobject_cast<QGroupBox*>(currentRaLabel->parent());
        QGroupBox *controlGroup = qobject_cast<QGroupBox*>(stopButton->parent());
        
        if (coordGroup) coordGroup->setEnabled(false);
        if (positionGroup) positionGroup->setEnabled(false);
        if (controlGroup) controlGroup->setEnabled(false);
    }
}

void MountPanel::onMountPositionUpdated(const QString &deviceName, double ra, double dec)
{
    // Check if this is the current mount
    if (mountComboBox->currentText() == deviceName) {
        // Update displayed coordinates
        
        // Format RA as HH:MM:SS
        int raHours = static_cast<int>(ra);
        int raMinutes = static_cast<int>((ra - raHours) * 60);
        double raSeconds = (((ra - raHours) * 60) - raMinutes) * 60;
        
        // Format Dec as DD:MM:SS
        int decDegrees = static_cast<int>(dec);
        int decMinutes = static_cast<int>(std::abs((dec - decDegrees) * 60));
        double decSeconds = (std::abs((dec - decDegrees) * 60) - decMinutes) * 60;
        
        // Update labels
        currentRaLabel->setText(QString("%1h %2m %3s")
            .arg(raHours, 2, 10, QChar('0'))
            .arg(raMinutes, 2, 10, QChar('0'))
            .arg(raSeconds, 2, 'f', 1, QChar('0')));
        
        currentDecLabel->setText(QString("%1° %2' %3\"")
            .arg(decDegrees, 2, 10, QChar('0'))
            .arg(decMinutes, 2, 10, QChar('0'))
            .arg(decSeconds, 2, 'f', 1, QChar('0')));
    }
}

void MountPanel::connectMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Connect the mount
    if (m_client->connectDevice(mountName)) {
        emit logMessage(QString("Connecting to mount %1").arg(mountName));
        
        // Update UI
        connectButton->setEnabled(false);
        disconnectButton->setEnabled(true);
    }
}

void MountPanel::disconnectMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Disconnect the mount
    if (m_client->disconnectDevice(mountName)) {
        emit logMessage(QString("Disconnecting from mount %1").arg(mountName));
        
        // Update UI
        connectButton->setEnabled(true);
        disconnectButton->setEnabled(false);
        
        // Disable control groups
        QGroupBox *coordGroup = qobject_cast<QGroupBox*>(raSpinBox->parent());
        QGroupBox *positionGroup = qobject_cast<QGroupBox*>(currentRaLabel->parent());
        QGroupBox *controlGroup = qobject_cast<QGroupBox*>(stopButton->parent());
        
        if (coordGroup) coordGroup->setEnabled(false);
        if (positionGroup) positionGroup->setEnabled(false);
        if (controlGroup) controlGroup->setEnabled(false);
    }
}

void MountPanel::gotoCoordinates()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    double ra = raSpinBox->value();
    double dec = decSpinBox->value();
    
    // Move the mount
    if (m_client->moveMountTo(mountName, ra, dec)) {
        emit logMessage(QString("Moving %1 to RA: %2 DEC: %3")
                      .arg(mountName)
                      .arg(ra, 0, 'f', 6)
                      .arg(dec, 0, 'f', 6));
    } else {
        emit logMessage(QString("Failed to move %1").arg(mountName));
    }
}

void MountPanel::syncCoordinates()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    double ra = raSpinBox->value();
    double dec = decSpinBox->value();
    
    // Sync the mount
    if (m_client->syncMountTo(mountName, ra, dec)) {
        emit logMessage(QString("Syncing %1 to RA: %2 DEC: %3")
                      .arg(mountName)
                      .arg(ra, 0, 'f', 6)
                      .arg(dec, 0, 'f', 6));
    } else {
        emit logMessage(QString("Failed to sync %1").arg(mountName));
    }
}

void MountPanel::stopMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Stop the mount
    if (m_client->stopMount(mountName)) {
        emit logMessage(QString("Stopping %1").arg(mountName));
    } else {
        emit logMessage(QString("Failed to stop %1").arg(mountName));
    }
}

void MountPanel::parkMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Park the mount
    if (m_client->parkMount(mountName)) {
        emit logMessage(QString("Parking %1").arg(mountName));
    } else {
        emit logMessage(QString("Failed to park %1").arg(mountName));
    }
}

void MountPanel::unparkMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Unpark the mount
    if (m_client->unparkMount(mountName)) {
        emit logMessage(QString("Unparking %1").arg(mountName));
    } else {
        emit logMessage(QString("Failed to unpark %1").arg(mountName));
    }
}

void MountPanel::homeMount()
{
    QString mountName = mountComboBox->currentText();
    if (mountName.isEmpty()) return;
    
    // Home the mount
    if (m_client->homeMount(mountName)) {
        emit logMessage(QString("Homing %1").arg(mountName));
    } else {
        emit logMessage(QString("Failed to home %1").arg(mountName));
    }
}
