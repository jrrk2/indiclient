// INDIClient.cpp
// Implementation of the INDI Client class

#include "INDIClient.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QImageReader>

// ======================
// Constructor / Destructor
// ======================

INDIClient::INDIClient(QObject *parent)
    : QObject(parent)
    , INDI::BaseClient()
    , m_isConnected(false)
{
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();
}

INDIClient::~INDIClient()
{
    if (m_isConnected)
        disconnectServer();
}

// ======================
// BaseClient virtuals: devices & properties
// ======================

void INDIClient::newDevice(INDI::BaseDevice device)
{
    QString deviceName = QString::fromStdString(device.getDeviceName());

    if (!m_deviceList.contains(deviceName))
        m_deviceList.append(deviceName);

    // Enable BLOB mode for all devices to ensure we get all property updates
    setBLOBMode(B_ALSO, deviceName.toStdString().c_str(), nullptr);
    
    // NOTE: We can't determine the device interface yet!
    // The interface is only available AFTER DRIVER_INFO property is defined
    // See newProperty() below where we check for DRIVER_INFO

    emit deviceAdded(deviceName);
    emit message(QString("Device added: %1").arg(deviceName));
}

void INDIClient::removeDevice(INDI::BaseDevice device)
{
    QString deviceName = QString::fromStdString(device.getDeviceName());

    m_deviceList.removeAll(deviceName);
    m_cameraList.removeAll(deviceName);
    m_mountList.removeAll(deviceName);

    emit deviceRemoved(deviceName);
    emit message(QString("Device removed: %1").arg(deviceName));
}

void INDIClient::newProperty(INDI::Property property)
{
    QString deviceName   = QString::fromStdString(property.getDeviceName());
    QString propertyName = QString::fromStdString(property.getName());

    // Debug: Log ALL newProperty calls to see what's arriving
    emit message(QString("newProperty: %1.%2").arg(deviceName, propertyName));

    // WORKAROUND: If we see CCD-specific properties being defined, the camera must be connected
    // The INDI server doesn't always re-send CONNECTION property on reconnect
    // Only trigger once per device by checking if it's the first CCD property we see
    if (propertyName == "CCD_EXPOSURE")
    {
        if (m_cameraList.contains(deviceName))
        {
            emit message(QString("CCD_EXPOSURE property detected - %1 is CONNECTED").arg(deviceName));
            emit deviceConnected(deviceName);
        }
    }
    
    // Similarly for telescope properties - use first coord property
    if (propertyName == "EQUATORIAL_EOD_COORD")
    {
        if (m_mountList.contains(deviceName))
        {
            emit message(QString("EQUATORIAL_EOD_COORD property detected - %1 is CONNECTED").arg(deviceName));
            emit deviceConnected(deviceName);
        }
    }

    // CRITICAL: Check for DRIVER_INFO property to determine device interface
    if (propertyName == "DRIVER_INFO")
    {
        INDI::BaseDevice device = property.getBaseDevice();
        uint16_t interface = device.getDriverInterface();
        
        emit message(QString("Device %1 interface: 0x%2").arg(deviceName).arg(interface, 0, 16));
        
        // Check if this is a CCD/Camera
        if (interface & INDI::BaseDevice::CCD_INTERFACE)
        {
            if (!m_cameraList.contains(deviceName))
            {
                m_cameraList.append(deviceName);
                emit message(QString("Camera detected: %1").arg(deviceName));
                // Don't emit deviceAdded here - camera panel will update from the list
            }
        }
        
        // Check if this is a Telescope/Mount
        if (interface & INDI::BaseDevice::TELESCOPE_INTERFACE)
        {
            if (!m_mountList.contains(deviceName))
            {
                m_mountList.append(deviceName);
                emit message(QString("Mount detected: %1").arg(deviceName));
                // Don't emit deviceAdded here - mount panel will update from the list
            }
        }
    }

    // Handle CONNECTION property - this is the critical one for enabling controls!
    if (propertyName == "CONNECTION")
    {
        auto svp = property.getSwitch();
        if (svp)
        {
            ISwitch *connectSwitch = IUFindSwitch(svp, "CONNECT");
            ISwitch *disconnectSwitch = IUFindSwitch(svp, "DISCONNECT");
            
            emit message(QString("CONNECTION property state: %1 CONNECT=%2 DISCONNECT=%3")
                        .arg(deviceName)
                        .arg(connectSwitch ? (connectSwitch->s == ISS_ON ? "ON" : "OFF") : "NULL")
                        .arg(disconnectSwitch ? (disconnectSwitch->s == ISS_ON ? "ON" : "OFF") : "NULL"));
            
            if (connectSwitch && disconnectSwitch)
            {
                if (connectSwitch->s == ISS_ON)
                {
                    emit message(QString("CONNECTION property: %1 is CONNECTED").arg(deviceName));
                    emit deviceConnected(deviceName);
                }
                else if (disconnectSwitch->s == ISS_ON)
                {
                    emit message(QString("CONNECTION property: %1 is DISCONNECTED").arg(deviceName));
                    emit deviceDisconnected(deviceName);
                }
            }
        }
    }

    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::removeProperty(INDI::Property /*property*/)
{
    // No-op for this simple client
}

// ======================
// BaseClient virtuals: BLOB / switch / number / text / light
// ======================

void INDIClient::newBLOB(IBLOB *bp)
{
    if (!bp)
        return;

    QString deviceName   = QString::fromStdString(bp->bvp->device);
    QString propertyName = QString::fromStdString(bp->bvp->name);
    QString blobName     = QString::fromStdString(bp->name);

    emit message(QString("Received BLOB: %1.%2.%3").arg(deviceName, propertyName, blobName));

    // Very simple heuristic: treat CCD1/IMAGE-like properties as images
    if (propertyName == "CCD1" || propertyName.contains("IMAGE"))
    {
        QImage image = processImageData(bp);
        if (!image.isNull())
            emit newImage(deviceName, image);
    }
}

void INDIClient::newSwitch(ISwitchVectorProperty *svp)
{
    if (!svp)
        return;

    QString deviceName   = QString::fromStdString(svp->device);
    QString propertyName = QString::fromStdString(svp->name);

    // Debug: Log all switch updates
    emit message(QString("newSwitch: %1.%2").arg(deviceName, propertyName));

    if (propertyName == "CONNECTION")
    {
        ISwitch *connectSwitch    = IUFindSwitch(svp, "CONNECT");
        ISwitch *disconnectSwitch = IUFindSwitch(svp, "DISCONNECT");

        emit message(QString("newSwitch CONNECTION: %1 CONNECT=%2 DISCONNECT=%3")
                    .arg(deviceName)
                    .arg(connectSwitch ? (connectSwitch->s == ISS_ON ? "ON" : "OFF") : "NULL")
                    .arg(disconnectSwitch ? (disconnectSwitch->s == ISS_ON ? "ON" : "OFF") : "NULL"));

        if (connectSwitch && disconnectSwitch)
        {
            if (connectSwitch->s == ISS_ON)
            {
                emit message(QString("newSwitch: %1 CONNECTION state is CONNECT").arg(deviceName));
                emit deviceConnected(deviceName);
            }
            else if (disconnectSwitch->s == ISS_ON)
            {
                emit message(QString("newSwitch: %1 CONNECTION state is DISCONNECT").arg(deviceName));
                emit deviceDisconnected(deviceName);
            }
        }
    }

    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newNumber(INumberVectorProperty *nvp)
{
    if (!nvp)
        return;

    QString deviceName   = QString::fromStdString(nvp->device);
    QString propertyName = QString::fromStdString(nvp->name);

    // Debug: Log received number property
    // emit message(QString("Received number property: %1.%2").arg(deviceName, propertyName));

    // Mount position updates
    if (propertyName == "EQUATORIAL_EOD_COORD" ||
        propertyName == "EQUATORIAL_COORD" ||
        propertyName == "HORIZONTAL_COORD")
    {
        if (propertyName == "EQUATORIAL_EOD_COORD" ||
            propertyName == "EQUATORIAL_COORD")
        {
            INumber *raElement  = IUFindNumber(nvp, "RA");
            INumber *decElement = IUFindNumber(nvp, "DEC");

            if (raElement && decElement)
            {
                double ra  = raElement->value;
                double dec = decElement->value;
                emit mountPositionUpdated(deviceName, ra, dec);
            }
        }
    }

    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newText(ITextVectorProperty *tvp)
{
    if (!tvp)
        return;

    QString deviceName   = QString::fromStdString(tvp->device);
    QString propertyName = QString::fromStdString(tvp->name);

    emit propertyUpdated(deviceName, propertyName);
}

void INDIClient::newLight(ILightVectorProperty *lvp)
{
    if (!lvp)
        return;

    QString deviceName   = QString::fromStdString(lvp->device);
    QString propertyName = QString::fromStdString(lvp->name);

    emit propertyUpdated(deviceName, propertyName);
}

// ======================
// BaseClient virtuals: messages & server connection
// ======================

void INDIClient::newMessage(INDI::BaseDevice device, int messageID)
{
    QString deviceName = QString::fromStdString(device.getDeviceName());

    // messageQueue now returns a std::string (or something compatible)
    auto msgStd = device.messageQueue(messageID);
    QString msg = QString::fromStdString(msgStd);

    emit this->message(QString("[%1]: %2").arg(deviceName, msg));
}

void INDIClient::serverConnected()
{
    m_isConnected = true;
    emit message("Connected to INDI server");
    emit serverConnectedSignal();
}

void INDIClient::serverDisconnected(int exitCode)
{
    m_isConnected = false;
    m_deviceList.clear();
    m_cameraList.clear();
    m_mountList.clear();

    emit message(QString("Disconnected from INDI server (exit code: %1)").arg(exitCode));
    emit serverDisconnectedSignal();
}

// ======================
// Image handling
// ======================

QImage INDIClient::processImageData(IBLOB *bp)
{
    if (!bp || bp->size <= 0 || !bp->blob)
        return QImage();

    QImage image;
    QString format = QString::fromStdString(bp->format).toLower();

    if (format == ".fits" || format == "fits")
    {
        emit message("Processing FITS image data (placeholder)");
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
        return image;
    }
    else if (format == ".jpg" || format == "jpg" || format == "jpeg")
    {
        QByteArray buffer(static_cast<const char *>(bp->blob), static_cast<int>(bp->size));
        image.loadFromData(buffer, "JPG");
    }
    else if (format == ".png" || format == "png")
    {
        QByteArray buffer(static_cast<const char *>(bp->blob), static_cast<int>(bp->size));
        image.loadFromData(buffer, "PNG");
    }
    else if (format == ".raw" || format == "raw")
    {
        emit message("Processing RAW image data (placeholder)");
        image = QImage(512, 512, QImage::Format_Grayscale8);
        image.fill(Qt::black);
    }
    else
    {
        QByteArray buffer(static_cast<const char *>(bp->blob), static_cast<int>(bp->size));
        QBuffer imageBuffer(&buffer);
        imageBuffer.open(QIODevice::ReadOnly);

        QImageReader reader(&imageBuffer);
        image = reader.read();

        if (image.isNull())
            emit message(QString("Unsupported image format: %1").arg(format));
    }

    return image;
}

// ======================
// High-level device control helpers
// ======================

bool INDIClient::connectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(deviceName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto connectionSP = device.getSwitch("CONNECTION");
    if (!connectionSP)
        return false;

    ISwitch *connectSwitch    = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");

    if (!connectSwitch || !disconnectSwitch)
        return false;

    connectSwitch->s    = ISS_ON;
    disconnectSwitch->s = ISS_OFF;

    sendNewSwitch(connectionSP);
    emit message(QString("Connecting device: %1").arg(deviceName));
    return true;
}

bool INDIClient::disconnectDevice(const QString &deviceName)
{
    if (!m_isConnected || deviceName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(deviceName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto connectionSP = device.getSwitch("CONNECTION");
    if (!connectionSP)
        return false;

    ISwitch *connectSwitch    = IUFindSwitch(connectionSP, "CONNECT");
    ISwitch *disconnectSwitch = IUFindSwitch(connectionSP, "DISCONNECT");

    if (!connectSwitch || !disconnectSwitch)
        return false;

    connectSwitch->s    = ISS_OFF;
    disconnectSwitch->s = ISS_ON;

    sendNewSwitch(connectionSP);
    emit message(QString("Disconnecting device: %1").arg(deviceName));
    return true;
}

bool INDIClient::setCameraExposure(const QString &cameraName, double exposureTime)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(cameraName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto exposureNP = device.getNumber("CCD_EXPOSURE");
    if (!exposureNP)
        return false;

    INumber *exposureN = IUFindNumber(exposureNP, "CCD_EXPOSURE_VALUE");
    if (!exposureN)
        return false;

    exposureN->value = exposureTime;

    sendNewNumber(exposureNP);
    emit message(QString("Setting exposure time for %1: %2 s")
                     .arg(cameraName)
                     .arg(exposureTime));
    return true;
}

bool INDIClient::takeCameraExposure(const QString &cameraName)
{
    if (!m_isConnected || cameraName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(cameraName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto exposureNP = device.getNumber("CCD_EXPOSURE");
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

    INDI::BaseDevice device = getDevice(cameraName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto abortSP = device.getSwitch("CCD_ABORT_EXPOSURE");
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

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto coordNP = device.getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device.getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;

    INumber *raNumber  = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    if (!raNumber || !decNumber)
        return false;

    raNumber->value = ra;
    decNumber->value = dec;

    sendNewNumber(coordNP);
    emit message(QString("Moving %1 to RA: %2 DEC: %3")
                     .arg(mountName)
                     .arg(ra)
                     .arg(dec));
    return true;
}

bool INDIClient::stopMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto motionSP = device.getSwitch("TELESCOPE_ABORT_MOTION");
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

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    // First ensure sidereal tracking
    auto trackModeSP = device.getSwitch("TELESCOPE_TRACK_MODE");
    if (trackModeSP)
    {
        ISwitch *siderealS = IUFindSwitch(trackModeSP, "TRACK_SIDEREAL");
        if (siderealS)
        {
            IUResetSwitch(trackModeSP);
            siderealS->s = ISS_ON;
            sendNewSwitch(trackModeSP);
        }
    }

    // Now do the sync
    auto motionSP = device.getSwitch("ON_COORD_SET");
    if (!motionSP)
        return false;

    ISwitch *syncS = IUFindSwitch(motionSP, "SYNC");
    if (!syncS)
        return false;

    IUResetSwitch(motionSP);
    syncS->s = ISS_ON;
    sendNewSwitch(motionSP);

    // Set coordinates
    auto coordNP = device.getNumber("EQUATORIAL_EOD_COORD");
    if (!coordNP)
        coordNP = device.getNumber("EQUATORIAL_COORD");
    if (!coordNP)
        return false;

    INumber *raNumber  = IUFindNumber(coordNP, "RA");
    INumber *decNumber = IUFindNumber(coordNP, "DEC");
    if (!raNumber || !decNumber)
        return false;

    raNumber->value = ra;
    decNumber->value = dec;

    sendNewNumber(coordNP);
    emit message(QString("Syncing %1 to RA: %2 DEC: %3")
                     .arg(mountName)
                     .arg(ra)
                     .arg(dec));
    return true;
}

bool INDIClient::parkMount(const QString &mountName)
{
    if (!m_isConnected || mountName.isEmpty())
        return false;

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto parkSP = device.getSwitch("TELESCOPE_PARK");
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

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto parkSP = device.getSwitch("TELESCOPE_PARK");
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

    INDI::BaseDevice device = getDevice(mountName.toStdString().c_str());
    if (!device.isValid())
        return false;

    auto homeSP = device.getSwitch("TELESCOPE_HOME");
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
}
