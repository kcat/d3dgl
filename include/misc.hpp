#ifndef MISC_HPP
#define MISC_HPP

template<typename T>
class ref_ptr {
public:
    typedef T element_type;

private:
    element_type *mPtr;

    template<class Other>
    void assign(const ref_ptr<Other> &other)
    {
        if(mPtr == other.mPtr)
            return;

        element_type *tmp_ptr = mPtr;
        mPtr = other.mPtr;
        // unref second to prevent any deletion of any object which might
        // be referenced by the other object. i.e other is child of the
        // original _ptr.
        if(mPtr) mPtr->AddRef();
        if(tmp_ptr) tmp_ptr->Release();
    }
    template<class Other>
    friend class ref_ptr;

public:
    ref_ptr() : mPtr(nullptr) {}
    ref_ptr(element_type *ptr) : mPtr(ptr) { if(mPtr) mPtr->AddRef(); }
    ref_ptr(const ref_ptr &rhs) : mPtr(rhs.mPtr) { if(mPtr) mPtr->AddRef(); }
    ref_ptr(ref_ptr&& rhs) : mPtr(rhs.mPtr) { if(mPtr) mPtr->AddRef(); }
    template<class Other>
    ref_ptr(const ref_ptr<Other> &rhs) : mPtr(rhs.mPtr) { if(mPtr) mPtr->AddRef(); }
    template<class Other>
    ref_ptr(ref_ptr<Other>&& rhs) : mPtr(rhs.mPtr) { if(mPtr) mPtr->AddRef(); }
    ~ref_ptr()
    {
        if(mPtr)
            mPtr->Release();
        mPtr = nullptr;
    }

    ref_ptr& operator=(const ref_ptr &rhs)
    { assign(rhs); return *this; }
    ref_ptr& operator=(ref_ptr&& rhs)
    { assign(rhs); return *this; }

    template<class Other>
    ref_ptr& operator=(const ref_ptr<Other> &rhs)
    { assign(rhs); return *this; }
    template<class Other>
    ref_ptr& operator=(ref_ptr<Other>&& rhs)
    { assign(rhs); return *this; }

    ref_ptr& operator=(element_type *ptr)
    {
        if(mPtr == ptr)
            return *this;

        T *tmp_ptr = mPtr;
        mPtr = ptr;
        if(mPtr) mPtr->AddRef();
        if(tmp_ptr) tmp_ptr->Release();
        return *this;
    }

    // implicit output conversion
    operator element_type*() const { return mPtr; }

    element_type& operator*() const { return *mPtr; }
    element_type* operator->() const { return mPtr; }
    element_type* get() const { return mPtr; }

    bool operator!() const { return mPtr==nullptr; }
    bool valid() const     { return mPtr!=nullptr; }

    element_type* release()
    {
        element_type *tmp = mPtr;
        mPtr = nullptr;
        return tmp;
    }

    void swap(ref_ptr &rhs)
    {
        std::swap(mPtr, rhs.mPtr);
    }
};

#endif /* MISC_HPP */
