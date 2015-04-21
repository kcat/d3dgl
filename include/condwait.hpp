#ifndef CONDWAIT_HPP
#define CONDWAIT_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class ConditionWaiter {
    CRITICAL_SECTION mCritSect;
    CONDITION_VARIABLE mCondVar;

public:
    ConditionWaiter()
    {
        InitializeCriticalSection(&mCritSect);
        InitializeConditionVariable(&mCondVar);
    }
    ~ConditionWaiter()
    { DeleteCriticalSection(&mCritSect); }

    void signal()
    {
        EnterCriticalSection(&mCritSect);
        LeaveCriticalSection(&mCritSect);
        WakeAllConditionVariable(&mCondVar);
    }

    void beginWait() { EnterCriticalSection(&mCritSect); }
    void endWait() { LeaveCriticalSection(&mCritSect); }
    void wait(DWORD time=INFINITE) { SleepConditionVariableCS(&mCondVar, &mCritSect, time); }
};

#endif /* CONDWAIT_HPP */
