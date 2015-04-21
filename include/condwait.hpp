#ifndef CONDWAIT_HPP
#define CONDWAIT_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class ConditionWaiter {
    CRITICAL_SECTION mCritSect;
    CONDITION_VARIABLE mCondVar;

    // Non-copyable
    ConditionWaiter(const ConditionWaiter&) = delete;
    ConditionWaiter& operator=(const ConditionWaiter&) = delete;

public:
    ConditionWaiter()
    {
        InitializeCriticalSection(&mCritSect);
        InitializeConditionVariable(&mCondVar);
    }
    ~ConditionWaiter()
    { DeleteCriticalSection(&mCritSect); }

    void beginWait() { EnterCriticalSection(&mCritSect); }
    void endWait() { LeaveCriticalSection(&mCritSect); }
    void wait(DWORD time=INFINITE) { SleepConditionVariableCS(&mCondVar, &mCritSect, time); }

    void prepareSignal() { EnterCriticalSection(&mCritSect); }
    void sendSignal()
    {
        LeaveCriticalSection(&mCritSect);
        WakeAllConditionVariable(&mCondVar);
    }
};

#endif /* CONDWAIT_HPP */
