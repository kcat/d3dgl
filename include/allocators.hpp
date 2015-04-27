#ifndef ALLOCATORS_HPP
#define ALLOCATORS_HPP

#include <new>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


enum class Alignment : size_t {
    Normal = sizeof(void*),
    SSE    = 16,
    AVX    = 32,
};


template<typename T, Alignment Align=Alignment::AVX>
class AlignedAllocator;

template<Alignment Align>
class AlignedAllocator<void, Align> {
public:
    typedef void       *pointer;
    typedef const void *const_pointer;
    typedef void        value_type;

    template<class U>
    struct rebind { typedef AlignedAllocator<U, Align> other; };
};


template<typename T, Alignment Align>
class AlignedAllocator {
public:
    typedef T         value_type;
    typedef T        *pointer;
    typedef const T  *const_pointer;
    typedef T&        reference;
    typedef const T  &const_reference;
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;

    typedef std::true_type propagate_on_container_move_assignment;

    template<class U>
    struct rebind { typedef AlignedAllocator<U, Align> other; };

public:
    AlignedAllocator() noexcept { }

    template<class U>
    AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept { }

    size_type max_size() const noexcept
    { return (size_type(~0) - size_type(Align)) / sizeof(T); }

    pointer address(reference x) const noexcept
    { return std::addressof(x); }

    const_pointer address(const_reference x) const noexcept
    { return std::addressof(x); }

    pointer allocate(size_type n, typename AlignedAllocator<void, Align>::const_pointer = 0)
    {
        const size_type alignment = static_cast<size_type>(Align);
        void *ptr = _aligned_malloc(n * sizeof(T), alignment);
        if(ptr == nullptr) throw std::bad_alloc();
        return reinterpret_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_type) noexcept
    { return _aligned_free(p); }

    template<class U, class ...Args>
    void construct(U *p, Args&&... args)
    { ::new(reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...); }

    void destroy(pointer p)
    { p->~T(); }
};


template<typename T, Alignment Align>
class AlignedAllocator<const T, Align>
{
public:
    typedef T         value_type;
    typedef const T  *pointer;
    typedef const T  *const_pointer;
    typedef const T  &reference;
    typedef const T  &const_reference;
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;

    typedef std::true_type propagate_on_container_move_assignment;

    template<class U>
    struct rebind { typedef AlignedAllocator<U, Align> other; };

public:
    AlignedAllocator() noexcept { }

    template<class U>
    AlignedAllocator(const AlignedAllocator<U, Align>&) noexcept { }

    size_type max_size() const noexcept
    { return (size_type(~0) - size_type(Align)) / sizeof(T); }

    const_pointer address(const_reference x) const noexcept
    { return std::addressof(x); }

    pointer allocate(size_type n, typename AlignedAllocator<void, Align>::const_pointer = 0)
    {
        const size_type alignment = static_cast<size_type>( Align );
        void *ptr = _aligned_malloc(n * sizeof(T), alignment);
        if(ptr == nullptr) throw std::bad_alloc();
        return reinterpret_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_type) noexcept
    { return _aligned_free(p); }

    template<class U, class ...Args>
    void construct(U *p, Args&&... args)
    { ::new(reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...); }

    void destroy(pointer p)
    { p->~T(); }
};

template<typename T, Alignment TAlign, typename U, Alignment UAlign>
inline bool operator==(const AlignedAllocator<T,TAlign>&, const AlignedAllocator<U, UAlign>&) noexcept
{ return TAlign == UAlign; }

template <typename T, Alignment TAlign, typename U, Alignment UAlign>
inline bool operator!=(const AlignedAllocator<T,TAlign>&, const AlignedAllocator<U, UAlign>&) noexcept
{ return TAlign != UAlign; }

#endif /* ALLOCATORS_HPP */
