// INDIClient.cpp
// Implementation of the INDI Client class
// Author: Claude

#include "INDITestClient.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QImageReader>

INDIClient::INDIClient(QObject *parent)
    : INDI::BaseClient(parent),
      m_isConnected(false)
{
    // Initialize lists
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();
}

INDIClient::~INDIClient()
{
    // Disconnect from server if still connected
    if (m_isConnected)
        disconnectServer();
}

void INDIClient::newDevice(INDI::BaseDevice *device)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    
    // Add to device list if not already present
    if (!m_deviceList.contains(deviceName))
        m_deviceList.append(deviceName);
    
    // Check device type and add to appropriate list
    if (device->getDriverInterface() & INDI::BaseDevice::CCD_INTERFACE) {
        if (!m_cameraList.contains(deviceName))
            m_cameraList.append(deviceName);
    }
    
    if (device->getDriverInterface() & INDI::BaseDevice::TELESCOPE_INTERFACE) {
        if (!m_mountList.contains(deviceName))
            m_mountList.append(deviceName);
    }
    
    // Emit signal
    emit deviceAdded(deviceName);
    emit message(QString("Device added: %1").arg(deviceName));
}

void INDIClient::removeDevice(INDI::BaseDevice *device)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    
    // Remove from lists
    m_deviceList.removeAll(deviceName);
    m_cameraList.removeAll(deviceName);
    m_mountList.removeAll(deviceName);
    
    // Emit signal
    emit deviceRemoved(deviceName);
    emit message(QString("Device removed: %1").arg(deviceName));
}

void INDIClient::newProperty(INDI::Property *property)
{
    if (!property)
        return;
    
    QString deviceName = property->getDeviceName();
    QString propertyName = property->getName();
    
    // Handle CONNECTION property
    if (propertyName == "CONNECTION") {
        ISwitchVectorProperty *svp = property->getSwitch();
        if (svp) {
            ISwitch *connectSwitch = IUFindSwitch(svp, "CONNECT");
            if (connectSwitch && connectSwitch->s == ISS_ON) {
                emit deviceConnected(deviceName);
                emit message(QString("Device connected: %1").arg(deviceName));
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::removeProperty(INDI::Property *property)
{
    if (!property)
        return;
    
    // Not much to do here for our simple client
}

void INDIClient::newBLOB(IBLOB *bp)
{
    if (!bp)
        return;
    
    QString deviceName = bp->bvp->device;
    QString propertyName = bp->bvp->name;
    QString blobName = bp->name;
    
    emit message(QString("Received BLOB: %1.%2.%3").arg(deviceName, propertyName, blobName));
    
    // Process image data
    if (propertyName == "CCD1" || propertyName.contains("IMAGE")) {
        QImage image = processImageData(bp);
        if (!image.isNull()) {
            emit newImage(deviceName, image);
        }
    }
}

void INDIClient::newSwitch(ISwitchVectorProperty *svp)
{
    if (!svp)
        return;
    
    QString deviceName = svp->device;
    QString propertyName = svp->name;
    
    // Handle CONNECTION property changes
    if (propertyName == "CONNECTION") {
        ISwitch *connectSwitch = IUFindSwitch(svp, "CONNECT");
        ISwitch *disconnectSwitch = IUFindSwitch(svp, "DISCONNECT");
        
        if (connectSwitch && disconnectSwitch) {
            if (connectSwitch->s == ISS_ON) {
                emit deviceConnected(deviceName);
                emit message(QString("Device connected: %1").arg(deviceName));
            } else if (disconnectSwitch->s == ISS_ON) {
                emit deviceDisconnected(deviceName);
                emit message(QString("Device disconnected: %1").arg(deviceName));
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newNumber(INumberVectorProperty *nvp)
{
    if (!nvp)
        return;
    
    QString deviceName = nvp->device;
    QString propertyName = nvp->name;
    
    // Handle mount position updates
    if (propertyName == "EQUATORIAL_EOD_COORD" || 
        propertyName == "EQUATORIAL_COORD" || 
        propertyName == "HORIZONTAL_COORD") {
        
        if (propertyName == "EQUATORIAL_EOD_COORD" || propertyName == "EQUATORIAL_COORD") {
            double ra = 0, dec = 0;
            
            INumber *raElement = IUFindNumber(nvp, "RA");
            INumber *decElement = IUFindNumber(nvp, "DEC");
            
            if (raElement && decElement) {
                ra = raElement->value;
                dec = decElement->value;
                emit mountPositionUpdated(deviceName, ra, dec);
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newText(ITextVectorProperty *tvp)
{
    if (!tvp)
        return;
    
    QString deviceName = tvp->device;
    QString propertyName = tvp->name;
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newLight(ILightVectorProperty *lvp)
{
    if (!lvp)
        return;
    
    QString deviceName = lvp->device;
    QString propertyName = lvp->name;
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newMessage(INDI::BaseDevice *device, int messageID)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    QString message = QString::fromStdString(device->messageQueue(messageID));
    
    emit this->message(QString("[%1]: %2").arg(deviceName, message));
}

void INDIClient::serverConnected()
{
    m_isConnected = true;
    emit message("Connected to INDI server");
    emit this->serverConnected();
}

void INDIClient::serverDisconnected(int exitCode)
{
    m_isConnected = false;
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();
    
    emit message(QString("Disconnected from INDI server (exit code: %1)").arg(exitCode));
    emit this->serverDisconnected();
}

QImage INDIClient::processImageData(IBLOB *bp)
{
    if (!bp || bp->size <= 0 || !bp->blob)
        return QImage();
    
    QImage image;
    QString format = QString(bp->format).toLower();
    
    if (format == ".fits" || format == "fits") {
        // Process FITS data - this is simplified, real implementation would use cfitsio
        emit message("Processing FITS image data");
        // TODO: Implement FITS processing using cfitsio
        // For now, we'll create a placeholder image
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
        return image;
    } 
    else if (format == ".jpg" || format == "jpg" || format == "jpeg") {
        QByteArray buffer((const char *)bp->blob, bp->size);
        image.loadFromData(buffer, "JPG");
    } 
    else if (format == ".png" || format == "png") {
        QByteArray buffer((const char *)bp->blob, bp->size);
        image.loadFromData(buffer, "PNG");
    } 
    else if (format == ".raw" || format == "raw") {
        // Process RAW data - very simplified
        emit message("Processing RAW image data");
        // TODO: Implement RAW processing
        // For now, create a placeholder
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
    } 
    else {
        // Try to load using Qt's image reader
        QByteArray buffer((const char *)bp->blob, bp->size);
        QBuffer imageBuffer(&buffer);
        imageBuffer.open(QIODevice::ReadOnly);
        
        QImageReader reader(&imageBuffer);
        image = reader.read();
        
        if (image.isNull()) {
            emit message(QString("Unsupported image format: %1").arg(format));
        }
    }
    
    return image;
}

bool INDIClient::connectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(deviceName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *connectionSP = device->getSwitch("CONNECTION");
    if (!connectionSP)
        return false;
    
    ISwitch *connectSwitch = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");
    
    if (!connectSwitch || !disconnectSwitch)
        return false;
    
    connectSwitch->s = ISS_ON;
    disconnectSwitch->s = ISS_OFF;
    
    sendNewSwitch(connectionSP);
    emit message(QString("Connecting device: %1").arg(deviceName));
    
    return true;
}

bool INDIClient::disconnectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(deviceName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *connectionSP = device->getSwitch("CONNECTION");
    if (!connectionSP)
        return false;
    
    ISwitch *connectSwitch = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");
    
    if (!connectSwitch || !disconnectSwitch)
        return false;
    
    connectSwitch->s = ISS_OFF;
    disconnectSwitch->s = ISS_ON;
    
    sendNewSwitch(connectionSP);
    emit message(QString("Disconnecting device: %1").arg(deviceName));
    
    return true;
}

bool INDIClient::setCameraExposure(const QString &cameraName, double exposureTime)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *exposureNP = device->getNumber("CCD_EXPOSURE");
    if (!exposureNP)
        return false;
    
    INumber *exposureN = IUFindNumber(exposureNP, "CCD_EXPOSURE_VALUE");
    if (!exposureN)
        return false;
    
    exposureN->value = exposureTime;
    
    sendNewNumber(exposureNP);
    emit message(QString("Setting exposure time for %1: %2s").arg(cameraName).arg(exposureTime));
    
    return true;
}

bool INDIClient::takeCameraExposure(const QString &cameraName)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *exposureNP = device->getNumber("CCD_EXPOSURE");
    if (!exposureNP)
        return false;
    
    sendNewNumber(exposureNP);
    emit message(QString("Starting exposure with %1").arg(cameraName));
    
    return true;
}

bool INDIClient::abortCameraExposure(const QString &cameraName)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *abortSP = device->getSwitch("CCD_ABORT_EXPOSURE");
    if (!abortSP)
        return false;
    
    ISwitch *abortS = IUFindSwitch(abortSP, "ABORT");
    if (!abortS)
        return false;
    
    abortS->s = ISS_ON;
    
    sendNewSwitch(abortSP);
    emit message(QString("Aborting exposure with %1").arg(cameraName));
    
    return true;
}

bool INDIClient::moveMountTo(const QString &mountName, double ra, double dec)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *coordNP = device->getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device->getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;
    
    INumber *raNumber = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    
    if (!raNumber || !decNumber)
        return false;
    
    raNumber->value = ra;
    decNumber->value = dec;
    
    sendNewNumber(coordNP);
    emit message(QString("Moving %1 to RA: %2 DEC: %3").arg(mountName).arg(ra).arg(dec));
    
    return true;
}

bool INDIClient::stopMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *motionSP = device->getSwitch("TELESCOPE_ABORT_MOTION");
    if (!motionSP)
        return false;
    
    ISwitch *abortS = IUFindSwitch(motionSP, "ABORT");
    if (!abortS)
        abortS = IUFindSwitch(motionSP, "ABORT_MOTION");
    if (!abortS)
        return false;
    
    abortS->s = ISS_ON;
    
    sendNewSwitch(motionSP);
    emit message(QString("Stopping %1").arg(mountName));
    
    return true;
}

bool INDIClient::syncMountTo(const QString &mountName, double ra, double dec)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    // Set the TRACK_MODE to TRACK_SIDEREAL first
    ISwitchVectorProperty *trackModeSP = device->getSwitch("TELESCOPE_TRACK_MODE");
    if (trackModeSP) {
        ISwitch *siderealS = IUFindSwitch(trackModeSP, "TRACK_SIDEREAL");
        if (siderealS) {
            IUResetSwitch(trackModeSP);
            siderealS->s = ISS_ON;
            sendNewSwitch(trackModeSP);
        }
    }
    
    // Now do the sync
    ISwitchVectorProperty *motionSP = device->getSwitch("ON_COORD_SET");
    if (!motionSP)
        return false;
    
    ISwitch *syncS = IUFindSwitch(motionSP, "SYNC");
    if (!syncS)
        return false;
    
    IUResetSwitch(motionSP);
    syncS->s = ISS_ON;
    
    sendNewSwitch(motionSP);
    
    // Set the coordinates
    INumberVectorProperty *coordNP = device->getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device->getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;
    
    INumber *raNumber = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    
    if (!raNumber || !decNumber)
        return false;
    
    raNumber->value = ra;
    decNumber->value = dec;
    
    sendNewNumber(coordNP);
    emit message(QString("Syncing %1 to RA: %2 DEC: %3").arg(mountName).arg(ra).arg(dec));
    
    return true;
}

bool INDIClient::parkMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *parkSP = device->getSwitch("TELESCOPE_PARK");
    if (!parkSP)
        return false;
    
    ISwitch *parkS = IUFindSwitch(parkSP, "PARK");
    if (!parkS)
        return false;
    
    IUResetSwitch(parkSP);
    parkS->s = ISS_ON;
    
    sendNewSwitch(parkSP);
    emit message(QString("Parking %1").arg(mountName));
    
    return true;
}

bool INDIClient::unparkMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *parkSP = device->getSwitch("TELESCOPE_PARK");
    if (!parkSP)
        return false;
    
    ISwitch *unparkS = IUFindSwitch(parkSP, "UNPARK");
    if (!unparkS)
        return false;
    
    IUResetSwitch(parkSP);
    unparkS->s = ISS_ON;
    
    sendNewSwitch(parkSP);
    emit message(QString("Unparking %1").arg(mountName));
    
    return true;
}

bool INDIClient::homeMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *homeSP = device->getSwitch("TELESCOPE_HOME");
    if (!homeSP)
        return false;
    
    ISwitch *homeS = IUFindSwitch(homeSP, "HOME");
    if (!homeS)
        return false;
    
    IUResetSwitch(homeSP);
    homeS->s = ISS_ON;
    
    sendNewSwitch(homeSP);
    emit message(QString("Homing %1").arg(mountName));
    
    return true;
}bool INDIClient::parkMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *parkSP = device->getSwitch("TELESCOPE_PARK");
    if (!parkSP)
        return false;
    
    ISwitch *parkS = IUFindSwitch(parkSP, "PARK");
    if (!parkS)
        return false;
    
    IUResetSwitch(parkSP);
    parkS->s = ISS_ON;
    
    sendNewSwitch(parkSP);
    emit message(QString("Parking %1").arg(mountName));
    
    return true;
}

bool INDIClient::unparkMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *parkSP = device->getSwitch("TELESCOPE_PARK");
    if (!parkSP)
        return false;
    
    ISwitch *unparkS = IUFindSwitch(parkSP, "UNPARK");
    if (!unparkS)
        return false;
    
    IUResetSwitch(parkSP);
    unparkS->s = ISS_ON;
    
    sendNewSwitch(parkSP);
    emit message(QString("Unparking %1").arg(mountName));
    
    return true;
}

bool INDIClient::homeMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *homeSP = device->getSwitch("TELESCOPE_HOME");
    if (!homeSP)
        return false;
    
    ISwitch *homeS = IUFindSwitch(homeSP, "HOME");
    if (!homeS)
        return false;
    
    IUResetSwitch(homeSP);
    homeS->s = ISS_ON;
    
    sendNewSwitch(homeSP);
    emit message(QString("Homing %1").arg(mountName));
    
    return true;
}bool INDIClient::setCameraExposure(const QString &cameraName, double exposureTime)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *exposureNP = device->getNumber("CCD_EXPOSURE");
    if (!exposureNP)
        return false;
    
    INumber *exposureN = IUFindNumber(exposureNP, "CCD_EXPOSURE_VALUE");
    if (!exposureN)
        return false;
    
    exposureN->value = exposureTime;
    
    sendNewNumber(exposureNP);
    emit message(QString("Setting exposure time for %1: %2s").arg(cameraName).arg(exposureTime));
    
    return true;
}

bool INDIClient::takeCameraExposure(const QString &cameraName)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *exposureNP = device->getNumber("CCD_EXPOSURE");
    if (!exposureNP)
        return false;
    
    sendNewNumber(exposureNP);
    emit message(QString("Starting exposure with %1").arg(cameraName));
    
    return true;
}

bool INDIClient::abortCameraExposure(const QString &cameraName)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(cameraName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *abortSP = device->getSwitch("CCD_ABORT_EXPOSURE");
    if (!abortSP)
        return false;
    
    ISwitch *abortS = IUFindSwitch(abortSP, "ABORT");
    if (!abortS)
        return false;
    
    abortS->s = ISS_ON;
    
    sendNewSwitch(abortSP);
    emit message(QString("Aborting exposure with %1").arg(cameraName));
    
    return true;
}

bool INDIClient::moveMountTo(const QString &mountName, double ra, double dec)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    INumberVectorProperty *coordNP = device->getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device->getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;
    
    INumber *raNumber = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    
    if (!raNumber || !decNumber)
        return false;
    
    raNumber->value = ra;
    decNumber->value = dec;
    
    sendNewNumber(coordNP);
    emit message(QString("Moving %1 to RA: %2 DEC: %3").arg(mountName).arg(ra).arg(dec));
    
    return true;
}

bool INDIClient::stopMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *motionSP = device->getSwitch("TELESCOPE_ABORT_MOTION");
    if (!motionSP)
        return false;
    
    ISwitch *abortS = IUFindSwitch(motionSP, "ABORT");
    if (!abortS)
        abortS = IUFindSwitch(motionSP, "ABORT_MOTION");
    if (!abortS)
        return false;
    
    abortS->s = ISS_ON;
    
    sendNewSwitch(motionSP);
    emit message(QString("Stopping %1").arg(mountName));
    
    return true;
}

bool INDIClient::syncMountTo(const QString &mountName, double ra, double dec)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(mountName.toStdString().c_str());
    if (!device)
        return false;
    
    // Set the TRACK_MODE to TRACK_SIDEREAL first
    ISwitchVectorProperty *trackModeSP = device->getSwitch("TELESCOPE_TRACK_MODE");
    if (trackModeSP) {
        ISwitch *siderealS = IUFindSwitch(trackModeSP, "TRACK_SIDEREAL");
        if (siderealS) {
            IUResetSwitch(trackModeSP);
            siderealS->s = ISS_ON;
            sendNewSwitch(trackModeSP);
        }
    }
    
    // Now do the sync
    ISwitchVectorProperty *motionSP = device->getSwitch("ON_COORD_SET");
    if (!motionSP)
        return false;
    
    ISwitch *syncS = IUFindSwitch(motionSP, "SYNC");
    if (!syncS)
        return false;
    
    IUResetSwitch(motionSP);
    syncS->s = ISS_ON;
    
    sendNewSwitch(motionSP);
    
    // Set the coordinates
    INumberVectorProperty *coordNP = device->getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device->getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;
    
    INumber *raNumber = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    
    if (!raNumber || !decNumber)
        return false;
    
    raNumber->value = ra;
    decNumber->value = dec;
    
    sendNewNumber(coordNP);
    emit message(QString("Syncing %1 to RA: %2 DEC: %3").arg(mountName).arg(ra).arg(dec));
    
    return true;
}bool INDIClient::connectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(deviceName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *connectionSP = device->getSwitch("CONNECTION");
    if (!connectionSP)
        return false;
    
    ISwitch *connectSwitch = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");
    
    if (!connectSwitch || !disconnectSwitch)
        return false;
    
    connectSwitch->s = ISS_ON;
    disconnectSwitch->s = ISS_OFF;
    
    sendNewSwitch(connectionSP);
    emit message(QString("Connecting device: %1").arg(deviceName));
    
    return true;
}

bool INDIClient::disconnectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;
    
    INDI::BaseDevice *device = getDevice(deviceName.toStdString().c_str());
    if (!device)
        return false;
    
    ISwitchVectorProperty *connectionSP = device->getSwitch("CONNECTION");
    if (!connectionSP)
        return false;
    
    ISwitch *connectSwitch = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");
    
    if (!connectSwitch || !disconnectSwitch)
        return false;
    
    connectSwitch->s = ISS_OFF;
    disconnectSwitch->s = ISS_ON;
    
    sendNewSwitch(connectionSP);
    emit message(QString("Disconnecting device: %1").arg(deviceName));
    
    return true;
}QImage INDIClient::processImageData(IBLOB *bp)
{
    if (!bp || bp->size <= 0 || !bp->blob)
        return QImage();
    
    QImage image;
    QString format = QString(bp->format).toLower();
    
    if (format == ".fits" || format == "fits") {
        // Process FITS data - this is simplified, real implementation would use cfitsio
        emit message("Processing FITS image data");
        // TODO: Implement FITS processing using cfitsio
        // For now, we'll create a placeholder image
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
        return image;
    } 
    else if (format == ".jpg" || format == "jpg" || format == "jpeg") {
        QByteArray buffer((const char *)bp->blob, bp->size);
        image.loadFromData(buffer, "JPG");
    } 
    else if (format == ".png" || format == "png") {
        QByteArray buffer((const char *)bp->blob, bp->size);
        image.loadFromData(buffer, "PNG");
    } 
    else if (format == ".raw" || format == "raw") {
        // Process RAW data - very simplified
        emit message("Processing RAW image data");
        // TODO: Implement RAW processing
        // For now, create a placeholder
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
    } 
    else {
        // Try to load using Qt's image reader
        QByteArray buffer((const char *)bp->blob, bp->size);
        QBuffer imageBuffer(&buffer);
        imageBuffer.open(QIODevice::ReadOnly);
        
        QImageReader reader(&imageBuffer);
        image = reader.read();
        
        if (image.isNull()) {
            emit message(QString("Unsupported image format: %1").arg(format));
        }
    }
    
    return image;
}// INDIClient.cpp
// Implementation of the INDI Client class
// Author: Claude

#include "INDITestClient.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QImageReader>

INDIClient::INDIClient(QObject *parent)
    : INDI::BaseClient(parent),
      m_isConnected(false)
{
    // Initialize lists
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();
}

INDIClient::~INDIClient()
{
    // Disconnect from server if still connected
    if (m_isConnected)
        disconnectServer();
}

void INDIClient::newDevice(INDI::BaseDevice *device)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    
    // Add to device list if not already present
    if (!m_deviceList.contains(deviceName))
        m_deviceList.append(deviceName);
    
    // Check device type and add to appropriate list
    if (device->getDriverInterface() & INDI::BaseDevice::CCD_INTERFACE) {
        if (!m_cameraList.contains(deviceName))
            m_cameraList.append(deviceName);
    }
    
    if (device->getDriverInterface() & INDI::BaseDevice::TELESCOPE_INTERFACE) {
        if (!m_mountList.contains(deviceName))
            m_mountList.append(deviceName);
    }
    
    // Emit signal
    emit deviceAdded(deviceName);
    emit message(QString("Device added: %1").arg(deviceName));
}

void INDIClient::removeDevice(INDI::BaseDevice *device)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    
    // Remove from lists
    m_deviceList.removeAll(deviceName);
    m_cameraList.removeAll(deviceName);
    m_mountList.removeAll(deviceName);
    
    // Emit signal
    emit deviceRemoved(deviceName);
    emit message(QString("Device removed: %1").arg(deviceName));
}

void INDIClient::newProperty(INDI::Property *property)
{
    if (!property)
        return;
    
    QString deviceName = property->getDeviceName();
    QString propertyName = property->getName();
    
    // Handle CONNECTION property
    if (propertyName == "CONNECTION") {
        ISwitchVectorProperty *svp = property->getSwitch();
        if (svp) {
            ISwitch *connectSwitch = IUFindSwitch(svp, "CONNECT");
            if (connectSwitch && connectSwitch->s == ISS_ON) {
                emit deviceConnected(deviceName);
                emit message(QString("Device connected: %1").arg(deviceName));
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::removeProperty(INDI::Property *property)
{
    if (!property)
        return;
    
    // Not much to do here for our simple client
}

void INDIClient::newBLOB(IBLOB *bp)
{
    if (!bp)
        return;
    
    QString deviceName = bp->bvp->device;
    QString propertyName = bp->bvp->name;
    QString blobName = bp->name;
    
    emit message(QString("Received BLOB: %1.%2.%3").arg(deviceName, propertyName, blobName));
    
    // Process image data
    if (propertyName == "CCD1" || propertyName.contains("IMAGE")) {
        QImage image = processImageData(bp);
        if (!image.isNull()) {
            emit newImage(deviceName, image);
        }
    }
}

void INDIClient::newSwitch(ISwitchVectorProperty *svp)
{
    if (!svp)
        return;
    
    QString deviceName = svp->device;
    QString propertyName = svp->name;
    
    // Handle CONNECTION property changes
    if (propertyName == "CONNECTION") {
        ISwitch *connectSwitch = IUFindSwitch(svp, "CONNECT");
        ISwitch *disconnectSwitch = IUFindSwitch(svp, "DISCONNECT");
        
        if (connectSwitch && disconnectSwitch) {
            if (connectSwitch->s == ISS_ON) {
                emit deviceConnected(deviceName);
                emit message(QString("Device connected: %1").arg(deviceName));
            } else if (disconnectSwitch->s == ISS_ON) {
                emit deviceDisconnected(deviceName);
                emit message(QString("Device disconnected: %1").arg(deviceName));
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newNumber(INumberVectorProperty *nvp)
{
    if (!nvp)
        return;
    
    QString deviceName = nvp->device;
    QString propertyName = nvp->name;
    
    // Handle mount position updates
    if (propertyName == "EQUATORIAL_EOD_COORD" || 
        propertyName == "EQUATORIAL_COORD" || 
        propertyName == "HORIZONTAL_COORD") {
        
        if (propertyName == "EQUATORIAL_EOD_COORD" || propertyName == "EQUATORIAL_COORD") {
            double ra = 0, dec = 0;
            
            INumber *raElement = IUFindNumber(nvp, "RA");
            INumber *decElement = IUFindNumber(nvp, "DEC");
            
            if (raElement && decElement) {
                ra = raElement->value;
                dec = decElement->value;
                emit mountPositionUpdated(deviceName, ra, dec);
            }
        }
    }
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newText(ITextVectorProperty *tvp)
{
    if (!tvp)
        return;
    
    QString deviceName = tvp->device;
    QString propertyName = tvp->name;
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newLight(ILightVectorProperty *lvp)
{
    if (!lvp)
        return;
    
    QString deviceName = lvp->device;
    QString propertyName = lvp->name;
    
    // Emit property update signal
    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newMessage(INDI::BaseDevice *device, int messageID)
{
    if (!device)
        return;
    
    QString deviceName = device->getDeviceName();
    QString message = QString::fromStdString(device->messageQueue(messageID));
    
    emit this->message(QString("[%1]: %2").arg(deviceName, message));
}

void INDIClient::serverConnected()
{
    m_isConnected = true;
    emit message("Connected to INDI server");
    emit this->serverConnected();
}

void INDIClient::serverDisconnected(int exitCode)
{
    m_isConnected = false;
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();
    
    emit message(QString("Disconnected from INDI server (exit code: %1)").arg(exitCode));
    emit this->serverDisconnected();
}