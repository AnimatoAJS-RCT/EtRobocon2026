/**
 * @file RallyRoute.cpp
 * @brief ETラリーの経路表現クラス実装
 */

#include "RallyRoute.h"

void RallyRoute::addStep(const RouteStep& step) {
    mSteps.push_back(step);
}

void RallyRoute::clear() {
    mSteps.clear();
}
