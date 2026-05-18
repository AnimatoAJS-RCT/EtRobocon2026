/******************************************************************************
 *  DistanceTerminator.h (for SPIKE)
 *  Created on: 2025/08/23
 *  Definition of the Class DistanceTerminator
 *  Author: Your Name
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifndef ETTR_UNIT_DistanceTerminator_H_
#define ETTR_UNIT_DistanceTerminator_H_

#include "Terminator.h"
#include "Walker.h"

class DistanceTerminator : public Terminator {
public:
    /**
     * コンストラクタ
     * @param walker Walker
     * @param targetDistance 目標距離 (mm)
     */
    DistanceTerminator(Walker* walker, double targetDistance);

    bool isToBeTerminate();
    void init() override;

private:
    double calcCurrentDistance();

    Walker* mWalker;
    double mTargetDistance;
    double mInitialDistance;
    int mCheckCount;
    static const double TIRE_DIAMETER; // タイヤの直径 (mm)
    static const double PI;
};

#endif  // ETTR_UNIT_DistanceTerminator_H_
