#pragma once
#include <QString>
class QWidget;
namespace visionflow {
class ImageViewer;
void showRoiEditor(ImageViewer*,QWidget* parent,const QString& frameId);
}
