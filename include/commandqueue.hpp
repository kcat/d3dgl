#ifndef COMMANDQUEUE_HPP
#define COMMANDQUEUE_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <new>


class Command {
protected:
    virtual ~Command() { }
    friend class CommandQueue;

public:
    virtual ULONG execute() = 0;
};

class CommandNoOp : public Command {
public:
    virtual ULONG execute() { return sizeof(*this); }
};

class CommandSkip : public Command {
    ULONG mSkipAmt;

public:
    CommandSkip(ULONG amt) : mSkipAmt(amt) { }

    virtual ULONG execute() { return mSkipAmt; }
};


class CommandQueue {
    static const size_t sQueueBits = 20;
    static const size_t sQueueSize = 1<<sQueueBits;
    static const size_t sQueueMask = sQueueSize-1;

    std::atomic<ULONG> mHead, mTail;
    char mQueueData[sQueueSize];
    CRITICAL_SECTION mLock;
    CONDITION_VARIABLE mCondVar;

    HANDLE mThreadHdl;
    DWORD mThreadId;

    DWORD CALLBACK run(void);
    static DWORD CALLBACK thread_func(void *arg)
    { return reinterpret_cast<CommandQueue*>(arg)->run(); }

public:
    CommandQueue();
    ~CommandQueue();

    bool init(HWND window, HGLRC glctx);
    void deinit();

    template<typename T, typename ...Args>
    void prepareMsg(Args...args)
    {
        static_assert(sizeof(T) >= sizeof(Command), "Type is too small!");
        static_assert((sizeof(T)%sizeof(Command)) == 0, "Type is not a multiple of Command!");
        static_assert(sizeof(T) < sQueueSize, "Type size is way too large!");

        EnterCriticalSection(&mLock);
        ULONG head = mHead.load();
        while(1)
        {
            ULONG rem_size = sQueueSize - head;
            if(rem_size < sizeof(T))
            {
                if(rem_size >= sizeof(CommandSkip))
                {
                    prepareMsg<CommandSkip>(rem_size);
                    head = mHead.load();
                    rem_size = sQueueSize - head;
                }
                else do {
                    prepareMsg<CommandNoOp>();
                    head = mHead.load();
                    rem_size = sQueueSize - head;
                } while(rem_size < sizeof(T));
            }
            ULONG tail = mTail;
            if(((tail-head-1)&sQueueMask) >= sizeof(T))
                break;

            SleepConditionVariableCS(&mCondVar, &mLock, INFINITE);
            head = mHead.load();
        }

        Command *cmd = new(&mQueueData[head]) T(args...);
        (void)cmd;

        head += sizeof(T);
        mHead.store(head&sQueueMask);
        LeaveCriticalSection(&mLock);
        WakeAllConditionVariable(&mCondVar);
    }
};

#endif /* COMMANDQUEUE_HPP */
