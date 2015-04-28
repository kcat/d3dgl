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
public:
    typedef std::atomic<ULONG> FlagType;

private:
    Command *mCommand;
    FlagType *mFlag;

public:
    CommandEvent(Command *command, FlagType *flag)
      : mCommand(command), mFlag(flag)
    { }

    virtual ULONG execute()
    {
        mCommand->execute();
        delete mCommand;
        mFlag->store(1);
        return sizeof(*this);
    }
};


class CommandQueue {
    static const size_t sQueueBits = 21;
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
    void doSizedSend(size_t size, Args...args)
    {
        ULONG head = mHead.load();
        while(1)
        {
            ULONG rem_size = sQueueSize - head;
            if(rem_size < size)
            {
                if(rem_size >= sizeof(CommandSkip))
                {
                    doSizedSend<CommandSkip>(rem_size, rem_size);
                    head = mHead.load();
                    rem_size = sQueueSize - head;
                }
                else do {
                    doSizedSend<CommandNoOp>(sizeof(CommandNoOp));
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
        TRACE("Sending %p\n", cmd);

        head += size;
        mHead.store(head&sQueueMask);
    }

public:
    CommandQueue();
    ~CommandQueue();

    bool init();
    void deinit();
    bool isActive() const { return mThreadHdl != nullptr; }

    void lock() { EnterCriticalSection(&mLock); }
    void unlock() { LeaveCriticalSection(&mLock); }

    template<typename T, typename ...Args>
    void doSend(Args...args)
    {
        static_assert(sizeof(T) >= sizeof(Command), "Type is too small!");
        static_assert((sizeof(T)%sizeof(Command)) == 0, "Type is not a multiple of Command!");
        static_assert(sizeof(T) < sQueueSize, "Type size is way too large!");

        doSizedSend<T,Args...>(sizeof(T), args...);
    }

    template<typename T, typename ...Args>
    void sendAndUnlock(Args...args)
    {
        doSend<T,Args...>(args...);
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
        CommandEvent::FlagType flag(0);
        send<CommandEvent>(new T(args...), &flag);
        while(!flag.load()) Sleep(1);
    }

    template<typename T, typename ...Args>
    void sendAndUnlockSync(Args...args)
    {
        CommandEvent::FlagType flag(0);
        sendAndUnlock<CommandEvent>(new T(args...), &flag);
        while(!flag.load()) Sleep(1);
    }
};

#endif /* COMMANDQUEUE_HPP */
