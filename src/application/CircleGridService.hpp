#pragma once
#include "CalibrationGrid.hpp"
#include <atomic>
#include <memory>
namespace visionflow {
class CircleGridService {
public:
    // Dark circular dots on a bright background, equal X/Y centre spacing.
    // Symmetry does not identify a physical orientation. No lens correction.
    static CalibrationGridObservation detect(const ImageFrame&,const CalibrationGridTarget&,
        const std::shared_ptr<std::atomic_bool>& cancel={});
};
}
