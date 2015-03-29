#ifndef COMMANDQUEUE_HPP
#define COMMANDQUEUE_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <new>

#include "trace.hpp"


class Command {
protected:
    virtual ~Command() { }
    friend class CommandEvent;
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

class CommandEvent : public Command {
    Command *mCommand;
    HANDLE mHandle;

public:
    CommandEvent(Command *command, HANDLE handle)
      : mCommand(command), mHandle(handle)
    { }

    virtual ULONG execute()
    {
        mCommand->execute();
        delete mCommand;
        SetEvent(mHandle);
        return sizeof(*this);
    }
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

    template<typename T, typename ...Args>
    void doSend(size_t size, Args...args)
    {
        ULONG head = mHead.load();
        while(1)
        {
            ULONG rem_size = sQueueSize - head;
            if(rem_size < size)
            {
                if(rem_size >= sizeof(CommandSkip))
                {
                    doSend<CommandSkip>(rem_size, rem_size);
                    head = mHead.load();
                    rem_size = sQueueSize - head;
                }
                else do {
                    doSend<CommandNoOp>(sizeof(CommandNoOp));
                    head = mHead.load();
                    rem_size = sQueueSize - head;
                } while(rem_size < size);
            }
            if(((mTail-head-1)&sQueueMask) >= size)
                break;

            ERR("CommandQueue is full!\n");
            SleepConditionVariableCS(&mCondVar, &mLock, INFINITE);
            head = mHead.load();
        }

        Command *cmd = new(&mQueueData[head]) T(args...);
        (void)cmd;

        head += size;
        mHead.store(head&sQueueMask);
    }

public:
    CommandQueue();
    ~CommandQueue();

    bool init(HWND window, HGLRC glctx);
    void deinit();

    void lock() { EnterCriticalSection(&mLock); }
    void unlock() { LeaveCriticalSection(&mLock); }
    template<typename T, typename ...Args>
    void sendAndUnlock(Args...args)
    {
        static_assert(sizeof(T) >= sizeof(Command), "Type is too small!");
        static_assert((sizeof(T)%sizeof(Command)) == 0, "Type is not a multiple of Command!");
        static_assert(sizeof(T) < sQueueSize, "Type size is way too large!");

        doSend<T,Args...>(sizeof(T), args...);
        unlock();
        WakeAllConditionVariable(&mCondVar);
    }

    template<typename T, typename ...Args>
    void send(Args...args)
    {
        lock();
        sendAndUnlock<T,Args...>(args...);
    }

    template<typename T, typename ...Args>
    void sendSync(Args...args)
    {
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        send<CommandEvent>(new T(args...), event);

        if(WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
            ERR("Failed to wait for sync event! Error: 0x%lx\n", GetLastError());
        CloseHandle(event);
    }

    template<typename T, typename ...Args>
    void sendAndUnlockSync(Args...args)
    {
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        sendAndUnlock<CommandEvent>(new T(args...), event);

        if(WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
            ERR("Failed to wait for sync event! Error: 0x%lx\n", GetLastError());
        CloseHandle(event);
    }
};

#endif /* COMMANDQUEUE_HPP */
