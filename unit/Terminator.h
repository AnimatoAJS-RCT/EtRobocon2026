/******************************************************************************
 *  Terminator.h (for SPIKE )
 *  Created on: 2025/01/05
 *  Definition of the Class Terminator
 *  Author: Kazuhiro Kawachi
 *  Modifier : Yuki Tsuchitoi
 *  Copyright (c) 2025 Embedded Technology Software Design Robot Contest
 *****************************************************************************/

#ifndef ETTR_UNIT_Terminator_H_
#define ETTR_UNIT_Terminator_H_

class Terminator {
   public:
    /**
     * コンストラクタ
     */
    Terminator();

    virtual void init();

    /**
     * @brief 実行を終了すべきか判定する抽象メソッド
     */
    virtual bool isToBeTerminate() = 0;
};

#endif  // ETTR_UNIT_Terminator_H_
