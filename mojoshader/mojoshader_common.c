#define __MOJOSHADER_INTERNAL__ 1
#include "mojoshader_internal.h"


// We chain errors as a linked list with a head/tail for easy appending.
//  These get flattened before passing to the application.
typedef struct ErrorItem {
    MOJOSHADER_error error;
    struct ErrorItem *next;
} ErrorItem;

struct ErrorList
{
    ErrorItem head;
    ErrorItem *tail;
    int count;
};

ErrorList *errorlist_create(void)
{
    ErrorList *retval = (ErrorList*)malloc(sizeof(ErrorList));
    if(retval != NULL)
    {
        memset(retval, '\0', sizeof(*retval));
        retval->tail = &retval->head;
    }
    return retval;
}

int errorlist_add(ErrorList *list, const char *fname, const int errpos, const char *str)
{
    return errorlist_add_fmt(list, fname, errpos, "%s", str);
}

int errorlist_add_fmt(ErrorList *list, const char *fname, const int errpos, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = errorlist_add_va(list, fname, errpos, fmt, ap);
    va_end(ap);
    return retval;
}

int errorlist_add_va(ErrorList *list, const char *_fname, const int errpos, const char *fmt, va_list va)
{
    ErrorItem *error = (ErrorItem*)malloc(sizeof(ErrorItem));
    if(error == NULL) return 0;

    char *fname = NULL;
    if(_fname != NULL)
    {
        fname = (char*)malloc(strlen(_fname) + 1);
        if (fname == NULL)
        {
            free(error);
            return 0;
        }
        strcpy(fname, _fname);
    }

    char scratch[128];
    va_list ap;
    va_copy(ap, va);
    const uint len = vsnprintf(scratch, sizeof(scratch), fmt, ap);
    va_end(ap);

    // If we overflowed our scratch buffer, that's okay. We were going to
    //  allocate anyhow...the scratch buffer just lets us avoid a second
    //  run of vsnprintf().
    char *failstr = (char*)malloc(len + 1);
    if(len < sizeof (scratch))
        strcpy(failstr, scratch);  // copy it over.
    else
    {
        va_copy(ap, va);
        vsnprintf(failstr, len + 1, fmt, ap);  // rebuild it.
        va_end(ap);
    }

    error->error.error = failstr;
    error->error.filename = fname;
    error->error.error_position = errpos;
    error->next = NULL;

    list->tail->next = error;
    list->tail = error;

    list->count++;
    return 1;
}

int errorlist_count(ErrorList *list)
{
    return list->count;
}

MOJOSHADER_error *errorlist_flatten(ErrorList *list)
{
    if(list->count == 0)
        return NULL;

    int total = 0;
    MOJOSHADER_error *retval = (MOJOSHADER_error*)malloc(sizeof(MOJOSHADER_error) * list->count);
    if(retval == NULL) return NULL;

    ErrorItem *item = list->head.next;
    while(item != NULL)
    {
        ErrorItem *next = item->next;
        // reuse the string allocations
        memcpy(&retval[total], &item->error, sizeof (MOJOSHADER_error));
        free(item);
        item = next;
        total++;
    }

    assert(total == list->count);
    list->count = 0;
    list->head.next = NULL;
    list->tail = &list->head;
    return retval;
}

void errorlist_destroy(ErrorList *list)
{
    if(list == NULL)
        return;

    ErrorItem *item = list->head.next;
    while (item != NULL)
    {
        ErrorItem *next = item->next;
        free((void*)item->error.error);
        free((void*)item->error.filename);
        free(item);
        item = next;
    }
    free(list);
}


typedef struct BufferBlock {
    uint8 *data;
    size_t bytes;
    struct BufferBlock *next;
} BufferBlock;

struct Buffer {
    size_t total_bytes;
    BufferBlock *head;
    BufferBlock *tail;
    size_t block_size;
};

Buffer *buffer_create(size_t blksz)
{
    Buffer *buffer = (Buffer*)malloc(sizeof(Buffer));
    if(buffer != NULL)
    {
        memset(buffer, '\0', sizeof(Buffer));
        buffer->block_size = blksz;
    }
    return buffer;
}

char *buffer_reserve(Buffer *buffer, const size_t len)
{
    if(len == 0)
        return NULL;

    // note that we make the blocks bigger than blocksize when we have enough
    //  data to overfill a fresh block, to reduce allocations.
    const size_t blocksize = buffer->block_size;

    if(buffer->tail != NULL)
    {
        const size_t tailbytes = buffer->tail->bytes;
        const size_t avail = (tailbytes >= blocksize) ? 0 : blocksize - tailbytes;
        if (len <= avail)
        {
            buffer->tail->bytes += len;
            buffer->total_bytes += len;
            assert(buffer->tail->bytes <= blocksize);
            return (char*)buffer->tail->data + tailbytes;
        }
    }

    // need to allocate a new block (even if a previous block wasn't filled,
    //  so this buffer is contiguous).
    const size_t bytecount = len > blocksize ? len : blocksize;
    const size_t malloc_len = sizeof(BufferBlock) + bytecount;
    BufferBlock *item = (BufferBlock*)malloc(malloc_len);
    if(item == NULL) return NULL;

    item->data = ((uint8*)item) + sizeof(BufferBlock);
    item->bytes = len;
    item->next = NULL;
    if(buffer->tail != NULL)
        buffer->tail->next = item;
    else
        buffer->head = item;
    buffer->tail = item;

    buffer->total_bytes += len;

    return (char*)item->data;
}

int buffer_append(Buffer *buffer, const void *_data, size_t len)
{
    if(len == 0)
        return 1;

    // note that we make the blocks bigger than blocksize when we have enough
    //  data to overfill a fresh block, to reduce allocations.
    const size_t blocksize = buffer->block_size;
    const uint8 *data = (const uint8*)_data;

    if(buffer->tail != NULL)
    {
        const size_t tailbytes = buffer->tail->bytes;
        const size_t avail = (tailbytes >= blocksize) ? 0 : blocksize - tailbytes;
        const size_t cpy = (avail > len) ? len : avail;
        if(cpy > 0)
        {
            memcpy(buffer->tail->data + tailbytes, data, cpy);
            len -= cpy;
            data += cpy;
            buffer->tail->bytes += cpy;
            buffer->total_bytes += cpy;
            assert(buffer->tail->bytes <= blocksize);
        }
    }

    if(len > 0)
    {
        assert(!buffer->tail || buffer->tail->bytes >= blocksize);
        const size_t bytecount = len > blocksize ? len : blocksize;
        const size_t malloc_len = sizeof(BufferBlock) + bytecount;
        BufferBlock *item = (BufferBlock*)malloc(malloc_len);
        if(item == NULL) return 0;

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
    }

    return 1;
}

int buffer_append_fmt(Buffer *buffer, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int retval = buffer_append_va(buffer, fmt, ap);
    va_end(ap);
    return retval;
}

int buffer_append_va(Buffer *buffer, const char *fmt, va_list va)
{
    char scratch[512];

    va_list ap;
    va_copy(ap, va);
    const uint len = vsnprintf(scratch, sizeof(scratch), fmt, ap);
    va_end(ap);

    if(len == 0)
        return 1;  // nothing to do.
    else if(len < sizeof(scratch))
        return buffer_append(buffer, scratch, len);

    // If we overflowed our scratch buffer, heap allocate and try again.
    char *buf = (char*)malloc(len + 1);

    va_copy(ap, va);
    vsnprintf(buf, len + 1, fmt, ap);  // rebuild it.
    va_end(ap);
    const int retval = buffer_append(buffer, buf, len);

    free(buf);
    return retval;
}

size_t buffer_size(Buffer *buffer)
{
    return buffer->total_bytes;
}

void buffer_empty(Buffer *buffer)
{
    BufferBlock *item = buffer->head;
    while(item != NULL)
    {
        BufferBlock *next = item->next;
        free(item);
        item = next;
    }
    buffer->head = buffer->tail = NULL;
    buffer->total_bytes = 0;
}

char *buffer_flatten(Buffer *buffer)
{
    char *retval = (char*)malloc(buffer->total_bytes + 1);
    if(retval == NULL) return NULL;

    BufferBlock *item = buffer->head;
    char *ptr = retval;
    while (item != NULL)
    {
        BufferBlock *next = item->next;
        memcpy(ptr, item->data, item->bytes);
        ptr += item->bytes;
        free(item);
        item = next;
    } // while
    *ptr = '\0';

    assert(ptr == (retval + buffer->total_bytes));

    buffer->head = buffer->tail = NULL;
    buffer->total_bytes = 0;

    return retval;
}

char *buffer_merge(Buffer **buffers, const size_t n, size_t *_len)
{
    Buffer *first = NULL;
    size_t len = 0;
    size_t i;
    for(i = 0; i < n; i++)
    {
        Buffer *buffer = buffers[i];
        if(buffer == NULL)
            continue;
        if(first == NULL)
            first = buffer;
        len += buffer->total_bytes;
    }

    char *retval = (char*)malloc(len + 1);
    if(retval == NULL)
    {
        *_len = 0;
        return NULL;
    }

    *_len = len;
    char *ptr = retval;
    for(i = 0; i < n; i++)
    {
        Buffer *buffer = buffers[i];
        if(buffer == NULL)
            continue;

        BufferBlock *item = buffer->head;
        while(item != NULL)
        {
            BufferBlock *next = item->next;
            memcpy(ptr, item->data, item->bytes);
            ptr += item->bytes;
            free(item);
            item = next;
        }

        buffer->head = buffer->tail = NULL;
        buffer->total_bytes = 0;
    }
    *ptr = '\0';

    assert(ptr == (retval + len));

    return retval;
}

void buffer_destroy(Buffer *buffer)
{
    if(buffer != NULL)
    {
        buffer_empty(buffer);
        free(buffer);
    }
}

// end of mojoshader_common.c ...
