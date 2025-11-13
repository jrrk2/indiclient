// ImagePanel.cpp
// Implementation of the image display panel
// Author: Claude

#include "INDITestClient.h"

#include <QScrollBar>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>

ImagePanel::ImagePanel(QWidget *parent)
    : QWidget(parent),
      m_zoomFactor(1.0)
{
    // Set up the UI
    setupUI();
}

ImagePanel::~ImagePanel()
{
    // Nothing to clean up
}

void ImagePanel::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Image display
    scrollArea = new QScrollArea(this);
    imageLabel = new QLabel(scrollArea);
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setScaledContents(true);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setAlignment(Qt::AlignCenter);
    
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setWidgetResizable(true);
    
    // Controls
    QGroupBox *controlsGroup = new QGroupBox("Image Controls", this);
    QHBoxLayout *controlsLayout = new QHBoxLayout(controlsGroup);
    
    QLabel *zoomLabel = new QLabel("Zoom:", controlsGroup);
    zoomSlider = new QSlider(Qt::Horizontal, controlsGroup);
    zoomSlider->setRange(10, 500);
    zoomSlider->setValue(100);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    zoomSlider->setTickInterval(50);
    
    saveButton = new QPushButton("Save Image", controlsGroup);
    saveButton->setEnabled(false);
    
    controlsLayout->addWidget(zoomLabel);
    controlsLayout->addWidget(zoomSlider, 1);
    controlsLayout->addWidget(saveButton);
    
    // Add to main layout
    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(controlsGroup);
    
    // Connect signals
    connect(zoomSlider, &QSlider::valueChanged, [this](int value) {
        updateZoom(value);
    });
    
    connect(saveButton, &QPushButton::clicked, this, &ImagePanel::saveImage);
}

void ImagePanel::displayImage(const QImage &image)
{
    if (image.isNull())
        return;
    
    m_currentImage = image;
    
    // Reset zoom
    zoomSlider->setValue(100);
    updateZoom(100);
    
    // Enable save button
    saveButton->setEnabled(true);
}

void ImagePanel::updateZoom(int level)
{
    m_zoomFactor = level / 100.0;
    
    if (!m_currentImage.isNull()) {
        int w = m_currentImage.width() * m_zoomFactor;
        int h = m_currentImage.height() * m_zoomFactor;
        
        imageLabel->setFixedSize(w, h);
        imageLabel->setPixmap(QPixmap::fromImage(m_currentImage).scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ImagePanel::saveImage()
{
    if (m_currentImage.isNull())
        return;
    
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Image",
        QDir::homePath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tif);;All Files (*)"
    );
    
    if (filePath.isEmpty())
        return;
    
    // Ensure file has extension
    QFileInfo fileInfo(filePath);
    if (fileInfo.suffix().isEmpty()) {
        filePath += ".png"; // Default to PNG
    }
    
    // Save the image
    QImageWriter writer(filePath);
    if (!writer.write(m_currentImage)) {
        QMessageBox::warning(
            this,
            "Error",
            QString("Failed to save image: %1").arg(writer.errorString())
        );
    }
}