#ifndef COMMANDQUEUE_HPP
#define COMMANDQUEUE_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <new>

#include "trace.hpp"


class CommandQueue;


template<typename T>
struct ref_holder {
    typedef T value_type;
    value_type &mValue;

    ref_holder(ref_holder<T> &rhs) : mValue(rhs.mValue) { }
    ref_holder(value_type &value) : mValue(value) { }

    ref_holder<T>& operator=(ref_holder<T>&) = delete;

    operator value_type&() { return mValue; }
    operator const value_type&() const { return mValue; }
};
template<typename T>
ref_holder<T> make_ref(T &value) { return ref_holder<T>(value); };


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

template<typename T>
class CommandSync : public Command {
public:
    typedef std::atomic<ULONG> FlagType;

private:
    CommandQueue &mQueue;
    FlagType &mFlag;
    T mCommand;

public:
    template<typename ...Args>
    CommandSync(CommandQueue &queue, FlagType &flag, Args...args)
      : mQueue(queue), mFlag(flag), mCommand(args...)
    { }

    virtual ULONG execute();
};

class FlushGLCmd : public Command {
public:
    FlushGLCmd() { }

    virtual ULONG execute();
};


class CommandQueue {
    static const size_t sQueueBits = 21;
    static const size_t sQueueSize = 1<<sQueueBits;
    static const size_t sQueueMask = sQueueSize-1;

    std::atomic<ULONG> mHead, mTail;
    char mQueueData[sQueueSize];
    CRITICAL_SECTION mLock;
    CONDITION_VARIABLE mCondVar;
    std::atomic<ULONG> mSpinLock;

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

            EnterCriticalSection(&mLock);
            WakeAllConditionVariable(&mCondVar);
            SleepConditionVariableCS(&mCondVar, &mLock, INFINITE);
            LeaveCriticalSection(&mLock);
            head = mHead.load();
        }

        Command *cmd = new(&mQueueData[head]) T(args...);
        TRACE("Sending %p\n", cmd);

        head += size;
        mHead.store(head&sQueueMask);
    }

    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

public:
    CommandQueue();
    ~CommandQueue();

    bool init();
    void deinit();
    bool isActive() const { return mThreadHdl != nullptr; }


    void beginWait() { EnterCriticalSection(&mLock); }
    void endWait() { LeaveCriticalSection(&mLock); }
    void wait(DWORD time_ms=INFINITE)
    {
        wake();
        SleepConditionVariableCS(&mCondVar, &mLock, time_ms);
    }


    void lock()
    {
        while(mSpinLock.exchange(true) == true)
            SwitchToThread();
    }
    void unlock() { mSpinLock = false; }
    void wake() { WakeAllConditionVariable(&mCondVar); }

    void wakeAndSleep()
    {
        wake();
        Sleep(1);
    }

    template<typename T, typename ...Args>
    void doSend(Args...args)
    {
        static_assert(sizeof(T) >= sizeof(Command), "Type is too small!");
        static_assert((sizeof(T)%sizeof(Command)) == 0, "Type is not a multiple of Command!");
        static_assert(sizeof(T) < sQueueSize, "Type size is way too large!");

        doSizedSend<T,Args...>(sizeof(T), args...);
    }

    template<typename T, typename ...Args>
    void send(Args...args)
    {
        lock();
        doSend<T,Args...>(args...);
        unlock();
    }

    template<typename T, typename ...Args>
    void sendSync(Args...args)
    {
        typename CommandSync<T>::FlagType flag(0);
        send<CommandSync<T>>(make_ref(*this), make_ref(flag), args...);

        beginWait();
        while(!flag) wait();
        endWait();
    }

    template<typename T, typename ...Args>
    void sendFlush(Args...args)
    {
        EnterCriticalSection(&mLock);
        send<T,Args...>(args...);
        LeaveCriticalSection(&mLock);
        wake();
    }


    void flush()
    {
        // FIXME: On Wine, a glFlush may cause a repaint of the window from the
        // GL's front buffer, which is fine except for the added processing we
        // don't want. However, it would be nice to ensure OpenGL is processing
        // all the commands that got sent to it up to this point.
        //sendFlush<FlushGLCmd>();
        EnterCriticalSection(&mLock);
        LeaveCriticalSection(&mLock);
        wake();
    }
};


template<typename T>
inline ULONG CommandSync<T>::execute()
{
    mCommand.execute();

    mQueue.beginWait();
    mFlag = 1;
    mQueue.endWait();

    return sizeof(*this);
}

#endif /* COMMANDQUEUE_HPP */
