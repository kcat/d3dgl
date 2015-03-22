
#include "commandqueue.hpp"

#include <exception>

#include "trace.hpp"


static_assert(sizeof(Command) == sizeof(void*), "Command is larger than a pointer!");

static_assert(sizeof(Command) == sizeof(CommandNoOp), "CommandNoOp is too large!");


class CommandInitThrd : public Command {
    HWND mWindow;
    HGLRC mContext;
    HANDLE mEvent;

public:
    CommandInitThrd(HWND window, HGLRC glctx, HANDLE evt)
      : mWindow(window), mContext(glctx), mEvent(evt)
    { }

    virtual ULONG execute()
    {
        HDC hdc = GetDC(mWindow);
        if(!wglMakeCurrent(hdc, mContext))
        {
            ERR("Failed to make context current! Error: %lu\n", GetLastError());
            std::terminate();
        }
        ReleaseDC(mWindow, hdc);
        TRACE("Command thread init complete\n");

        SetEvent(mEvent);
        return sizeof(*this);
    }
};

class CommandQuitThrd : public Command {
public:
    virtual ULONG execute()
    {
        TRACE("Command thread shutting down\n");
        wglMakeCurrent(nullptr, nullptr);
        ExitThread(0);
        return sizeof(*this);
    }
};


CommandQueue::CommandQueue()
  : mHead(0)
  , mTail(0)
  , mThreadHdl(nullptr)
  , mThreadId(0)
{
    InitializeCriticalSection(&mLock);
    InitializeConditionVariable(&mCondVar);
}

CommandQueue::~CommandQueue()
{
    deinit();
    DeleteCriticalSection(&mLock);
}

bool CommandQueue::init(HWND window, HGLRC glctx)
{
    HANDLE thrdInitEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
    mHead = sQueueSize - sizeof(CommandInitThrd);
    mTail = sQueueSize - sizeof(CommandInitThrd);
    send<CommandInitThrd>(window, glctx, thrdInitEvt);

    mThreadHdl = CreateThread(nullptr, 1024*1024, thread_func, this, 0, &mThreadId);
    if(!mThreadHdl)
    {
        ERR("Failed to create background thread, error %lu\n", GetLastError());
        return false;
    }

    if(WaitForSingleObject(thrdInitEvt, INFINITE) != WAIT_OBJECT_0)
    {
        ERR("Background thread init failed! Error: %lu\n", GetLastError());
        return false;
    }

    CloseHandle(thrdInitEvt);
    return true;
}

void CommandQueue::deinit()
{
    if(mThreadHdl)
    {
        send<CommandQuitThrd>();
        WaitForSingleObject(mThreadHdl, 5000);
        CloseHandle(mThreadHdl);
        mThreadHdl = nullptr;
        mThreadId = 0;
    }
}



DWORD CommandQueue::run(void)
{
    TRACE("Starting command thread\n");
restart_loop:
    while(1)
    {
        ULONG tail = mTail.load();
        if(tail == mHead)
        {
            EnterCriticalSection(&mLock);
            WakeAllConditionVariable(&mCondVar);
            while(tail == mHead)
            {
                if(!SleepConditionVariableCS(&mCondVar, &mLock, INFINITE))
                {
                    ERR("SleepConditionVariableCS failed! Error: %lu\n", GetLastError());
                    LeaveCriticalSection(&mLock);
                    goto restart_loop;
                }
            }
            LeaveCriticalSection(&mLock);
        }

        Command *cmd = reinterpret_cast<Command*>(&mQueueData[tail]);
        ULONG size = cmd->execute();
        if(size < sizeof(Command))
        {
            ERR("Command returned too small size (%lu < %u)\n", size, sizeof(Command));
            std::terminate();
        }
        cmd->~Command();

        tail += size;
        mTail.store(tail&sQueueMask);
        WakeAllConditionVariable(&mCondVar);
    }
    ERR("Command thread loop broken\n");

    return 0;
}

