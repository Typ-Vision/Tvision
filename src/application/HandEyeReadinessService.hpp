#pragma once

#include "domain/HandEyeReadiness.hpp"

namespace visionflow {
// Pure value evaluator: no file, network, SDK, camera, registration, Run/Auto or
// activation side effect. It only classifies readiness for later COMM4 cards.
class HandEyeReadinessService {
public:
    [[nodiscard]] static HandEyeReadinessReport evaluate(const HandEyeReadinessInput& input);
};
} // namespace visionflow
