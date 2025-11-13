// main.cpp
// Application entry point for INDI Test Client
// Author: Claude

#include "INDIClient.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>

int main(int argc, char *argv[])
{
    // Set application info
    QCoreApplication::setOrganizationName("AstroTools");
    QCoreApplication::setOrganizationDomain("astroimaging.org");
    QCoreApplication::setApplicationName("INDI Test Client");
    QCoreApplication::setApplicationVersion("1.0.0");
    
    // Create application
    QApplication app(argc, argv);
    
    // Process command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("INDI Test Client for mount/camera driver testing with plate solving and mount modeling");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add INDI server options
    QCommandLineOption serverOption(QStringList() << "s" << "server", 
                                  "INDI server hostname", "hostname", "localhost");
    QCommandLineOption portOption(QStringList() << "p" << "port", 
                                "INDI server port", "port", "7624");
    
    parser.addOption(serverOption);
    parser.addOption(portOption);
    
    // Process arguments
    parser.process(app);
    
    // Load default server settings from settings file
    QSettings settings;
    QString defaultServer = settings.value("server/hostname", "localhost").toString();
    int defaultPort = settings.value("server/port", 7624).toInt();
    
    // Get server and port from command line or settings
    QString server = parser.isSet(serverOption) ? parser.value(serverOption) : defaultServer;
    int port = parser.isSet(portOption) ? parser.value(portOption).toInt() : defaultPort;
    
    // Save settings
    settings.setValue("server/hostname", server);
    settings.setValue("server/port", port);
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // Set the server and port if provided
    if (parser.isSet(serverOption) || parser.isSet(portOption)) {
        mainWindow.serverHostEdit->setText(server);
        mainWindow.serverPortEdit->setValue(port);
        
        // Optionally auto-connect
        // mainWindow.connectToServer();
    }
    
    return app.exec();
}
