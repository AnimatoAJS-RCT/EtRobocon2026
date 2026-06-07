#ifndef TRACER_H
#define TRACER_H

#include "Starter.h"
#include "Terminator.h"

#include <vector>

class Tracer {
   protected:
    enum State { UNDEFINED, WAITING_FOR_START, WALKING, TERMINATED };
    std::vector<Starter*> mStarterList;
    std::vector<Terminator*> mTerminatorList;

    void execUndefined();
    void execWaitingForStart();
    void execWalking();

   public:
    /**
     * コンストラクタ
     */
    Tracer();
    State mState;

    /**
     * @brief 走行する抽象メソッド
     */
    virtual void run() = 0;
        virtual ~Tracer() = default;

    void addStarter(Starter* starter) { mStarterList.push_back(starter); };
    void addTerminator(Terminator* terminator) { mTerminatorList.push_back(terminator); };
    bool isTerminated() const { return mState == TERMINATED; };
};

#endif