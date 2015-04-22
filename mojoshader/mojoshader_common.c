#define __MOJOSHADER_INTERNAL__ 1
#include "mojoshader_internal.h"

// Convenience functions for allocators...
#if !MOJOSHADER_FORCE_ALLOCATOR
void *MOJOSHADER_internal_malloc(int bytes, void *d) { return malloc(bytes); }
void MOJOSHADER_internal_free(void *ptr, void *d) { free(ptr); }
#endif


// We chain errors as a linked list with a head/tail for easy appending.
//  These get flattened before passing to the application.
typedef struct ErrorItem
{
    MOJOSHADER_error error;
    struct ErrorItem *next;
} ErrorItem;

struct ErrorList
{
    ErrorItem head;
    ErrorItem *tail;
    int count;
    MOJOSHADER_malloc m;
    MOJOSHADER_free f;
    void *d;
};

ErrorList *errorlist_create(MOJOSHADER_malloc m, MOJOSHADER_free f, void *d)
{
    ErrorList *retval = (ErrorList *) m(sizeof (ErrorList), d);
    if (retval != NULL)
    {
        memset(retval, '\0', sizeof (ErrorList));
        retval->tail = &retval->head;
        retval->m = m;
        retval->f = f;
        retval->d = d;
    } // if

    return retval;
} // errorlist_create


int errorlist_add(ErrorList *list, const char *fname,
                  const int errpos, const char *str)
{
    return errorlist_add_fmt(list, fname, errpos, "%s", str);
} // errorlist_add


int errorlist_add_fmt(ErrorList *list, const char *fname,
                      const int errpos, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = errorlist_add_va(list, fname, errpos, fmt, ap);
    va_end(ap);
    return retval;
} // errorlist_add_fmt


int errorlist_add_va(ErrorList *list, const char *_fname,
                     const int errpos, const char *fmt, va_list va)
{
    ErrorItem *error = (ErrorItem *) list->m(sizeof (ErrorItem), list->d);
    if (error == NULL)
        return 0;

    char *fname = NULL;
    if (_fname != NULL)
    {
        fname = (char *) list->m(strlen(_fname) + 1, list->d);
        if (fname == NULL)
        {
            list->f(error, list->d);
            return 0;
        } // if
        strcpy(fname, _fname);
    } // if

    char scratch[128];
    va_list ap;
    va_copy(ap, va);
    const uint len = vsnprintf(scratch, sizeof (scratch), fmt, ap);
    va_end(ap);

    char *failstr = (char *) list->m(len + 1, list->d);

    // If we overflowed our scratch buffer, that's okay. We were going to
    //  allocate anyhow...the scratch buffer just lets us avoid a second
    //  run of vsnprintf().
    if (len < sizeof (scratch))
        strcpy(failstr, scratch);  // copy it over.
    else
    {
        va_copy(ap, va);
        vsnprintf(failstr, len + 1, fmt, ap);  // rebuild it.
        va_end(ap);
    } // else

    error->error.error = failstr;
    error->error.filename = fname;
    error->error.error_position = errpos;
    error->next = NULL;

    list->tail->next = error;
    list->tail = error;

    list->count++;
    return 1;
} // errorlist_add_va


int errorlist_count(ErrorList *list)
{
    return list->count;
} // errorlist_count


MOJOSHADER_error *errorlist_flatten(ErrorList *list)
{
    if (list->count == 0)
        return NULL;

    int total = 0;
    MOJOSHADER_error *retval = (MOJOSHADER_error *)
            list->m(sizeof (MOJOSHADER_error) * list->count, list->d);
    if (retval == NULL)
        return NULL;

    ErrorItem *item = list->head.next;
    while (item != NULL)
    {
        ErrorItem *next = item->next;
        // reuse the string allocations
        memcpy(&retval[total], &item->error, sizeof (MOJOSHADER_error));
        list->f(item, list->d);
        item = next;
        total++;
    } // while

    assert(total == list->count);
    list->count = 0;
    list->head.next = NULL;
    list->tail = &list->head;
    return retval;
} // errorlist_flatten


void errorlist_destroy(ErrorList *list)
{
    if (list == NULL)
        return;

    MOJOSHADER_free f = list->f;
    void *d = list->d;
    ErrorItem *item = list->head.next;
    while (item != NULL)
    {
        ErrorItem *next = item->next;
        f((void *) item->error.error, d);
        f((void *) item->error.filename, d);
        f(item, d);
        item = next;
    } // while
    f(list, d);
} // errorlist_destroy


typedef struct BufferBlock
{
    uint8 *data;
    size_t bytes;
    struct BufferBlock *next;
} BufferBlock;

struct Buffer
{
    size_t total_bytes;
    BufferBlock *head;
    BufferBlock *tail;
    size_t block_size;
    MOJOSHADER_malloc m;
    MOJOSHADER_free f;
    void *d;
};

Buffer *buffer_create(size_t blksz, MOJOSHADER_malloc m,
                      MOJOSHADER_free f, void *d)
{
    Buffer *buffer = (Buffer *) m(sizeof (Buffer), d);
    if (buffer != NULL)
    {
        memset(buffer, '\0', sizeof (Buffer));
        buffer->block_size = blksz;
        buffer->m = m;
        buffer->f = f;
        buffer->d = d;
    } // if
    return buffer;
} // buffer_create

char *buffer_reserve(Buffer *buffer, const size_t len)
{
    // note that we make the blocks bigger than blocksize when we have enough
    //  data to overfill a fresh block, to reduce allocations.
    const size_t blocksize = buffer->block_size;

    if (len == 0)
        return NULL;

    if (buffer->tail != NULL)
    {
        const size_t tailbytes = buffer->tail->bytes;
        const size_t avail = (tailbytes >= blocksize) ? 0 : blocksize - tailbytes;
        if (len <= avail)
        {
            buffer->tail->bytes += len;
            buffer->total_bytes += len;
            assert(buffer->tail->bytes <= blocksize);
            return (char *) buffer->tail->data + tailbytes;
        } // if
    } // if

    // need to allocate a new block (even if a previous block wasn't filled,
    //  so this buffer is contiguous).
    const size_t bytecount = len > blocksize ? len : blocksize;
    const size_t malloc_len = sizeof (BufferBlock) + bytecount;
    BufferBlock *item = (BufferBlock *) buffer->m(malloc_len, buffer->d);
    if (item == NULL)
        return NULL;

    item->data = ((uint8 *) item) + sizeof (BufferBlock);
    item->bytes = len;
    item->next = NULL;
    if (buffer->tail != NULL)
        buffer->tail->next = item;
    else
        buffer->head = item;
    buffer->tail = item;

    buffer->total_bytes += len;

    return (char *) item->data;
} // buffer_reserve

int buffer_append(Buffer *buffer, const void *_data, size_t len)
{
    const uint8 *data = (const uint8 *) _data;

    // note that we make the blocks bigger than blocksize when we have enough
    //  data to overfill a fresh block, to reduce allocations.
    const size_t blocksize = buffer->block_size;

    if (len == 0)
        return 1;

    if (buffer->tail != NULL)
    {
        const size_t tailbytes = buffer->tail->bytes;
        const size_t avail = (tailbytes >= blocksize) ? 0 : blocksize - tailbytes;
        const size_t cpy = (avail > len) ? len : avail;
        if (cpy > 0)
        {
            memcpy(buffer->tail->data + tailbytes, data, cpy);
            len -= cpy;
            data += cpy;
            buffer->tail->bytes += cpy;
            buffer->total_bytes += cpy;
            assert(buffer->tail->bytes <= blocksize);
        } // if
    } // if

    if (len > 0)
    {
        assert((!buffer->tail) || (buffer->tail->bytes >= blocksize));
        const size_t bytecount = len > blocksize ? len : blocksize;
        const size_t malloc_len = sizeof (BufferBlock) + bytecount;
        BufferBlock *item = (BufferBlock *) buffer->m(malloc_len, buffer->d);
        if (item == NULL)
            return 0;

        item->data = ((uint8 *) item) + sizeof (BufferBlock);
        item->bytes = len;
        item->next = NULL;
        if (buffer->tail != NULL)
            buffer->tail->next = item;
        else
            buffer->head = item;
        buffer->tail = item;

        memcpy(item->data, data, len);
        buffer->total_bytes += len;
    } // if

    return 1;
} // buffer_append

int buffer_append_fmt(Buffer *buffer, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = buffer_append_va(buffer, fmt, ap);
    va_end(ap);
    return retval;
} // buffer_append_fmt

int buffer_append_va(Buffer *buffer, const char *fmt, va_list va)
{
    char scratch[256];

    va_list ap;
    va_copy(ap, va);
    const uint len = vsnprintf(scratch, sizeof (scratch), fmt, ap);
    va_end(ap);

    // If we overflowed our scratch buffer, heap allocate and try again.

    if (len == 0)
        return 1;  // nothing to do.
    else if (len < sizeof (scratch))
        return buffer_append(buffer, scratch, len);

    char *buf = (char *) buffer->m(len + 1, buffer->d);
    va_copy(ap, va);
    vsnprintf(buf, len + 1, fmt, ap);  // rebuild it.
    va_end(ap);
    const int retval = buffer_append(buffer, buf, len);
    buffer->f(buf, buffer->d);
    return retval;
} // buffer_append_va

size_t buffer_size(Buffer *buffer)
{
    return buffer->total_bytes;
} // buffer_size

void buffer_empty(Buffer *buffer)
{
    BufferBlock *item = buffer->head;
    while (item != NULL)
    {
        BufferBlock *next = item->next;
        buffer->f(item, buffer->d);
        item = next;
    } // while
    buffer->head = buffer->tail = NULL;
    buffer->total_bytes = 0;
} // buffer_empty

char *buffer_flatten(Buffer *buffer)
{
    char *retval = (char *) buffer->m(buffer->total_bytes + 1, buffer->d);
    if (retval == NULL)
        return NULL;
    BufferBlock *item = buffer->head;
    char *ptr = retval;
    while (item != NULL)
    {
        BufferBlock *next = item->next;
        memcpy(ptr, item->data, item->bytes);
        ptr += item->bytes;
        buffer->f(item, buffer->d);
        item = next;
    } // while
    *ptr = '\0';

    assert(ptr == (retval + buffer->total_bytes));

    buffer->head = buffer->tail = NULL;
    buffer->total_bytes = 0;

    return retval;
} // buffer_flatten

char *buffer_merge(Buffer **buffers, const size_t n, size_t *_len)
{
    Buffer *first = NULL;
    size_t len = 0;
    size_t i;
    for (i = 0; i < n; i++)
    {
        Buffer *buffer = buffers[i];
        if (buffer == NULL)
            continue;
        if (first == NULL)
            first = buffer;
        len += buffer->total_bytes;
    } // for

    char *retval = (char *) (first ? first->m(len + 1, first->d) : NULL);
    if (retval == NULL)
    {
        *_len = 0;
        return NULL;
    } // if

    *_len = len;
    char *ptr = retval;
    for (i = 0; i < n; i++)
    {
        Buffer *buffer = buffers[i];
        if (buffer == NULL)
            continue;
        BufferBlock *item = buffer->head;
        while (item != NULL)
        {
            BufferBlock *next = item->next;
            memcpy(ptr, item->data, item->bytes);
            ptr += item->bytes;
            buffer->f(item, buffer->d);
            item = next;
        } // while

        buffer->head = buffer->tail = NULL;
        buffer->total_bytes = 0;
    } // for
    *ptr = '\0';

    assert(ptr == (retval + len));

    return retval;
} // buffer_merge

void buffer_destroy(Buffer *buffer)
{
    if (buffer != NULL)
    {
        MOJOSHADER_free f = buffer->f;
        void *d = buffer->d;
        buffer_empty(buffer);
        f(buffer, d);
    } // if
} // buffer_destroy

static int blockscmp(BufferBlock *item, const uint8 *data, size_t len)
{
    if (len == 0)
        return 1;  // "match"

    while (item != NULL)
    {
        const size_t itemremain = item->bytes;
        const size_t avail = len < itemremain ? len : itemremain;
        if (memcmp(item->data, data, avail) != 0)
            return 0;  // not a match.

        if (len == avail)
            return 1;   // complete match!

        len -= avail;
        data += avail;
        item = item->next;
    } // while

    return 0;  // not a complete match.
} // blockscmp

ssize_t buffer_find(Buffer *buffer, const size_t start,
                    const void *_data, const size_t len)
{
    if (len == 0)
        return 0;  // I guess that's right.

    if (start >= buffer->total_bytes)
        return -1;  // definitely can't match.

    if (len > (buffer->total_bytes - start))
        return -1;  // definitely can't match.

    // Find the start point somewhere in the center of a buffer.
    BufferBlock *item = buffer->head;
    const uint8 *ptr = item->data;
    size_t pos = 0;
    if (start > 0)
    {
        while (1)
        {
            assert(item != NULL);
            if ((pos + item->bytes) > start)  // start is in this block.
            {
                ptr = item->data + (start - pos);
                break;
            } // if

            pos += item->bytes;
            item = item->next;
        } // while
    } // if

    // okay, we're at the origin of the search.
    assert(item != NULL);
    assert(ptr != NULL);

    const uint8 *data = (const uint8 *) _data;
    const uint8 first = *data;
    while (item != NULL)
    {
        const size_t itemremain = item->bytes - ((size_t)(ptr-item->data));
        ptr = (uint8 *) memchr(ptr, first, itemremain);
        while (ptr != NULL)
        {
            const size_t retval = pos + ((size_t) (ptr - item->data));
            if (len == 1)
                return retval;  // we're done, here it is!

            const size_t itemremain = item->bytes - ((size_t)(ptr-item->data));
            const size_t avail = len < itemremain ? len : itemremain;
            if ((avail == 0) || (memcmp(ptr, data, avail) == 0))
            {
                // okay, we've got a (sub)string match! Move to the next block.
                // check all blocks until we get a complete match or a failure.
                if (blockscmp(item->next, data+avail, len-avail))
                    return (ssize_t) retval;
            } // if

            // try again, further in this block.
            ptr = (uint8 *) memchr(ptr + 1, first, itemremain - 1);
        } // while

        pos += item->bytes;
        item = item->next;
        if (item != NULL)
            ptr = item->data;
    } // while

    return -1;  // no match found.
} // buffer_find

// end of mojoshader_common.c ...

