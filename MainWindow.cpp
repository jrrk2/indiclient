// MainWindow.cpp
// Implementation of the main application window
// Author: Claude

#include "INDITestClient.h"

#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QSplitter>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      serverHost("localhost"),
      serverPort(7624)
{
    // Create INDI client
    indiClient = new INDIClient(this);
    
    // Connect signals from INDI client
    connect(indiClient, &INDIClient::serverConnected, this, &MainWindow::serverConnected);
    connect(indiClient, &INDIClient::serverDisconnected, this, &MainWindow::serverDisconnected);
    connect(indiClient, &INDIClient::deviceConnected, this, &MainWindow::deviceConnected);
    connect(indiClient, &INDIClient::deviceDisconnected, this, &MainWindow::deviceDisconnected);
    connect(indiClient, &INDIClient::message, this, &MainWindow::logMessage);
    
    // Set up the UI
    setupUI();
    createMenus();
    
    // Set window title and size
    setWindowTitle("INDI Test Client");
    resize(1024, 768);
    
    // Load settings
    QSettings settings("AstroTools", "INDITestClient");
    serverHost = settings.value("server/host", "localhost").toString();
    serverPort = settings.value("server/port", 7624).toInt();
    
    serverHostEdit->setText(serverHost);
    serverPortEdit->setValue(serverPort);
    
    // Update status
    updateStatus();
    
    // Log startup
    logMessage("INDI Test Client started");
}

MainWindow::~MainWindow()
{
    // Save settings
    QSettings settings("AstroTools", "INDITestClient");
    settings.setValue("server/host", serverHost);
    settings.setValue("server/port", serverPort);
    
    // Disconnect from server if connected
    if (indiClient->isConnected()) {
        indiClient->disconnectServer();
    }
}

void MainWindow::setupUI()
{
    // Create central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Server connection controls
    QGroupBox *serverGroup = new QGroupBox("INDI Server Connection", centralWidget);
    QHBoxLayout *serverLayout = new QHBoxLayout(serverGroup);
    
    QLabel *hostLabel = new QLabel("Host:", serverGroup);
    serverHostEdit = new QLineEdit(serverHost, serverGroup);
    
    QLabel *portLabel = new QLabel("Port:", serverGroup);
    serverPortEdit = new QSpinBox(serverGroup);
    serverPortEdit->setRange(1, 65535);
    serverPortEdit->setValue(serverPort);
    
    connectButton = new QPushButton("Connect", serverGroup);
    disconnectButton = new QPushButton("Disconnect", serverGroup);
    disconnectButton->setEnabled(false);
    
    serverLayout->addWidget(hostLabel);
    serverLayout->addWidget(serverHostEdit);
    serverLayout->addWidget(portLabel);
    serverLayout->addWidget(serverPortEdit);
    serverLayout->addWidget(connectButton);
    serverLayout->addWidget(disconnectButton);
    
    // Tab widget for different panels
    tabWidget = new QTabWidget(centralWidget);
    
    // Create the panels
    cameraPanel = new CameraPanel(indiClient, tabWidget);
    mountPanel = new MountPanel(indiClient, tabWidget);
    plateSolverPanel = new PlateSolverPanel(indiClient, tabWidget);
    imagePanel = new ImagePanel(tabWidget);
    
    // Add panels to tab widget
    tabWidget->addTab(cameraPanel, "Camera Control");
    tabWidget->addTab(mountPanel, "Mount Control");
    tabWidget->addTab(plateSolverPanel, "Plate Solver");
    
    // Create mount model panel after others are created (it needs references to other panels)
    mountModelPanel = new MountModelPanel(indiClient, mountPanel, plateSolverPanel, cameraPanel, tabWidget);
    tabWidget->addTab(mountModelPanel, "Mount Modeling");
    tabWidget->addTab(imagePanel, "Image View");
    
    // Log text edit
    QGroupBox *logGroup = new QGroupBox("Log", centralWidget);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logTextEdit = new QTextEdit(logGroup);
    logTextEdit->setReadOnly(true);
    logLayout->addWidget(logTextEdit);
    
    // Create a splitter for the main content and log
    QSplitter *splitter = new QSplitter(Qt::Vertical, centralWidget);
    splitter->addWidget(tabWidget);
    splitter->addWidget(logGroup);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    
    // Add everything to main layout
    mainLayout->addWidget(serverGroup);
    mainLayout->addWidget(splitter);
    
    // Set central widget
    setCentralWidget(centralWidget);
    
    // Create status bar
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    
    // Connect signals
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);
    
    // Connect signals between panels
    connect(cameraPanel, &CameraPanel::logMessage, this, &MainWindow::logMessage);
    connect(mountPanel, &MountPanel::logMessage, this, &MainWindow::logMessage);
    connect(plateSolverPanel, &PlateSolverPanel::logMessage, this, &MainWindow::logMessage);
    connect(mountModelPanel, &MountModelPanel::logMessage, this, &MainWindow::logMessage);
    
    // Connect image signals
    connect(cameraPanel, &CameraPanel::newImage, imagePanel, &ImagePanel::displayImage);
    connect(indiClient, &INDIClient::newImage, imagePanel, &ImagePanel::displayImage);
    connect(cameraPanel, &CameraPanel::newImage, plateSolverPanel, &PlateSolverPanel::solveImage);
    
    // Connect plate solver signals
    connect(plateSolverPanel, &PlateSolverPanel::solutionFound, 
            mountModelPanel, &MountModelPanel::onSolutionFound);
}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("File");
    
    QAction *saveLogAction = new QAction("Save Log...", this);
    connect(saveLogAction, &QAction::triggered, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Log", 
                                                     QDir::homePath(), 
                                                     "Log Files (*.log);;All Files (*)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << logTextEdit->toPlainText();
                file.close();
                logMessage(QString("Log saved to %1").arg(fileName));
            } else {
                QMessageBox::warning(this, "Error", QString("Could not save log to %1").arg(fileName));
            }
        }
    });
    
    QAction *exitAction = new QAction("Exit", this);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    fileMenu->addAction(saveLogAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("Help");
    
    QAction *aboutAction = new QAction("About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About INDI Test Client",
                         "INDI Test Client\n\n"
                         "A Qt-based INDI client for testing mount/camera functionality\n"
                         "with plate solving and mount modeling.\n\n"
                         "This application allows you to test all the functionality of\n"
                         "your mount/camera INDI drivers including plate solving using\n"
                         "libsolver and mount modeling using the align library of Ekos.");
    });
    
    helpMenu->addAction(aboutAction);
}

void MainWindow::connectToServer()
{
    serverHost = serverHostEdit->text();
    serverPort = serverPortEdit->value();
    
    // Save settings
    QSettings settings("AstroTools", "INDITestClient");
    settings.setValue("server/host", serverHost);
    settings.setValue("server/port", serverPort);
    
    // Connect to server
    logMessage(QString("Connecting to INDI server at %1:%2").arg(serverHost).arg(serverPort));
    indiClient->setServer(serverHost.toStdString().c_str(), serverPort);
    
    if (!indiClient->connectServer()) {
        QMessageBox::critical(this, "Connection Error", 
                            QString("Could not connect to INDI server at %1:%2").arg(serverHost).arg(serverPort));
        return;
    }
}

void MainWindow::disconnectFromServer()
{
    if (indiClient->isConnected()) {
        logMessage("Disconnecting from INDI server");
        indiClient->disconnectServer();
    }
}

void MainWindow::serverConnected()
{
    connectButton->setEnabled(false);
    disconnectButton->setEnabled(true);
    
    // Update status
    updateStatus();
    
    // Update device lists in panels
    cameraPanel->updateDeviceList();
    mountPanel->updateDeviceList();
}

void MainWindow::serverDisconnected()
{
    connectButton->setEnabled(true);
    disconnectButton->setEnabled(false);
    
    // Update status
    updateStatus();
    
    // Update device lists in panels
    cameraPanel->updateDeviceList();
    mountPanel->updateDeviceList();
}

void MainWindow::deviceConnected(const QString &deviceName)
{
    logMessage(QString("Device connected: %1").arg(deviceName));
    
    // Notify panels
    cameraPanel->onDeviceConnected(deviceName);
    mountPanel->onDeviceConnected(deviceName);
}

void MainWindow::deviceDisconnected(const QString &deviceName)
{
    logMessage(QString("Device disconnected: %1").arg(deviceName));
    
    // Notify panels
    cameraPanel->onDeviceDisconnected(deviceName);
    mountPanel->onDeviceDisconnected(deviceName);
}

void MainWindow::logMessage(const QString &message)
{
    // Get current timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    
    // Add log entry with timestamp
    logTextEdit->append(QString("[%1] %2").arg(timestamp, message));
}

void MainWindow::updateStatus()
{
    if (indiClient->isConnected()) {
        statusBar->showMessage(QString("Connected to INDI server at %1:%2").arg(serverHost).arg(serverPort));
    } else {
        statusBar->showMessage("Disconnected from INDI server");
    }
}