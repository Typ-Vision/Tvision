#pragma once
#include "vision/ImageFrame.hpp"
namespace visionflow {
class ImageFileService {
public:
    ImageFrame load(const QString& absolutePath) const;
};
}
