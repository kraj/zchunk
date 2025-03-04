#ifndef ZCK_H
#define ZCK_H

#define ZCK_VERSION "1.2.4"

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>

typedef enum zck_hash {
    ZCK_HASH_SHA1,
    ZCK_HASH_SHA256,
    ZCK_HASH_SHA512,
    ZCK_HASH_SHA512_128,
    ZCK_HASH_UNKNOWN
} zck_hash;

typedef enum zck_comp {
    ZCK_COMP_NONE,
    ZCK_COMP_GZIP, /* Not implemented yet */
    ZCK_COMP_ZSTD
} zck_comp;

typedef enum zck_ioption {
    ZCK_HASH_FULL_TYPE = 0,     /* Set full file hash type, using zck_hash */
    ZCK_HASH_CHUNK_TYPE,        /* Set chunk hash type using zck_hash */
    ZCK_VAL_HEADER_HASH_TYPE,   /* Set what the header hash type *should* be */
    ZCK_VAL_HEADER_LENGTH,      /* Set what the header length *should* be */
    ZCK_UNCOMP_HEADER,          /* Header should contain uncompressed size, too */
    ZCK_COMP_TYPE = 100,        /* Set compression type using zck_comp */
    ZCK_MANUAL_CHUNK,           /* Disable auto-chunking */
    ZCK_CHUNK_MIN,              /* Minimum chunk size when manual chunking */
    ZCK_CHUNK_MAX,              /* Maximum chunk size when manual chunking */
    ZCK_ZSTD_COMP_LEVEL = 1000  /* Set zstd compression level */
} zck_ioption;

typedef enum zck_soption {
    ZCK_VAL_HEADER_DIGEST = 0,  /* Set what the header hash *should* be */
    ZCK_COMP_DICT = 100         /* Set compression dictionary */
} zck_soption;

typedef enum zck_log_type {
    ZCK_LOG_DDEBUG = -1,
    ZCK_LOG_DEBUG,
    ZCK_LOG_INFO,
    ZCK_LOG_WARNING,
    ZCK_LOG_ERROR,
    ZCK_LOG_NONE
} zck_log_type;

typedef struct zckCtx zckCtx;
typedef struct zckHash zckHash;
typedef struct zckChunk zckChunk;
typedef struct zckIndex zckIndex;
typedef struct zckRange zckRange;
typedef struct zckDL zckDL;

typedef size_t (*zck_wcb)(void *ptr, size_t l, size_t c, void *dl_v);

#ifdef _WIN32
    #define ZCK_WARN_UNUSED
    typedef ptrdiff_t ssize_t;
    #ifdef ZCHUNK_STATIC_LIB
        #define ZCK_PUBLIC_API
    #else
        #ifdef ZCHUNK_EXPORTS
            #define ZCK_PUBLIC_API __declspec(dllexport)
        #else
            #define ZCK_PUBLIC_API __declspec(dllimport)
        #endif
    #endif
#else
    #define ZCK_WARN_UNUSED __attribute__ ((warn_unused_result))
    #define ZCK_PUBLIC_API __attribute__((visibility("default")))
#endif

/*******************************************************************
 * Reading a zchunk file
 *******************************************************************/
/* Initialize zchunk context */
zckCtx ZCK_PUBLIC_API *zck_create(void)
    ZCK_WARN_UNUSED;
/* Initialize zchunk for reading */
bool ZCK_PUBLIC_API zck_init_read (zckCtx *zck, int src_fd)
    ZCK_WARN_UNUSED;
/* Decompress dst_size bytes from zchunk file to dst, while verifying hashes */
ssize_t ZCK_PUBLIC_API zck_read(zckCtx *zck, char *dst, size_t dst_size)
    ZCK_WARN_UNUSED;
/* Get zchunk flags */
ssize_t ZCK_PUBLIC_API zck_get_flags (zckCtx *zck)
    ZCK_WARN_UNUSED;


/*******************************************************************
 * Writing a zchunk file
 *******************************************************************/
/* Initialize zchunk for writing */
bool ZCK_PUBLIC_API zck_init_write (zckCtx *zck, int dst_fd)
    ZCK_WARN_UNUSED;
/* Compress data src of size src_size, and write to zchunk file
 * Due to the nature of zchunk files and how they are built, no data will
 * actually appear in the zchunk file until zck_close() is called */
ssize_t ZCK_PUBLIC_API zck_write(zckCtx *zck, const char *src, const size_t src_size)
    ZCK_WARN_UNUSED;
/* Create a chunk boundary */
ssize_t ZCK_PUBLIC_API zck_end_chunk(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Create the database for uthash if not present (done automatically by read */
bool ZCK_PUBLIC_API zck_generate_hashdb(zckCtx *zck);

/*******************************************************************
 * Common functions for finishing a zchunk file
 *******************************************************************/
/* Close a zchunk file so it may no longer be read from or written to. The
 * context still contains information about the file */
bool ZCK_PUBLIC_API zck_close(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Free a zchunk context.  You must pass the address of the context, and the
 * context will automatically be set to NULL after it is freed */
void ZCK_PUBLIC_API zck_free(zckCtx **zck);


/*******************************************************************
 * Options
 *******************************************************************/
/* Set string option */
bool ZCK_PUBLIC_API zck_set_soption(zckCtx *zck, zck_soption option, const char *value,
                     size_t length)
    ZCK_WARN_UNUSED;
/* Set integer option */
bool ZCK_PUBLIC_API zck_set_ioption(zckCtx *zck, zck_ioption option, ssize_t value)
    ZCK_WARN_UNUSED;


/*******************************************************************
 * Error handling
 *******************************************************************/
/* Set logging level */
void ZCK_PUBLIC_API zck_set_log_level(zck_log_type ll);
/* Set logging fd */
void ZCK_PUBLIC_API zck_set_log_fd(int fd);
/* Check whether zck is in error state
 * Returns 0 if not, 1 if recoverable error, 2 if fatal error */
int ZCK_PUBLIC_API zck_is_error(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get error message
 * Returns char* containing error message.  char* will contain empty string if
 * there is no error message */
const ZCK_PUBLIC_API char *zck_get_error(zckCtx *zck);
/* Clear error message
 * Returns 1 if message was cleared, 0 if error is fatal and can't be cleared */
bool ZCK_PUBLIC_API zck_clear_error(zckCtx *zck);
/* Set a callback for logs instead to write into a fd */
typedef void (*logcallback)(const char *function, zck_log_type lt, const char *format, va_list args);
void ZCK_PUBLIC_API zck_set_log_callback(logcallback function);

/*******************************************************************
 * Miscellaneous utilities
 *******************************************************************/
/* Validate the chunk and data checksums for the current file.
 * Returns 0 for error, -1 for invalid checksum and 1 for valid checksum */
int ZCK_PUBLIC_API zck_validate_checksums(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Validate just the data checksum for the current file
 * Returns 0 for error, -1 for invalid checksum and 1 for valid checksum */
int ZCK_PUBLIC_API zck_validate_data_checksum(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Go through file and mark valid chunks as valid
 * Returns 0 for error, -1 for invalid checksum and 1 for valid checksum */
int ZCK_PUBLIC_API zck_find_valid_chunks(zckCtx *zck)
    ZCK_WARN_UNUSED;

/* Get a zckRange of ranges that need to still be downloaded.
 * max_ranges is the maximum number of ranges supported in a single request
 *     by the server.  If the server supports unlimited ranges, set this to -1
 * Returns NULL if there's an error */
zckRange ZCK_PUBLIC_API *zck_get_missing_range(zckCtx *zck, int max_ranges)
    ZCK_WARN_UNUSED;
/* Get a string representation of a zckRange */
char ZCK_PUBLIC_API *zck_get_range_char(zckCtx *zck, zckRange *range)
    ZCK_WARN_UNUSED;
/* Get file descriptor attached to zchunk context */
int ZCK_PUBLIC_API zck_get_fd(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Set file descriptor attached to zchunk context */
bool ZCK_PUBLIC_API zck_set_fd(zckCtx *zck, int fd)
    ZCK_WARN_UNUSED;

/* Return number of missing chunks (-1 if error) */
int ZCK_PUBLIC_API zck_missing_chunks(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Return number of failed chunks (-1 if error) */
int ZCK_PUBLIC_API zck_failed_chunks(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Reset failed chunks to become missing */
void ZCK_PUBLIC_API zck_reset_failed_chunks(zckCtx *zck);
/* Find chunks from a source */
bool ZCK_PUBLIC_API zck_find_matching_chunks(zckCtx *src, zckCtx *tgt);

/*******************************************************************
 * The functions should be all you need to read and write a zchunk
 * file.  After this point are advanced functions with an unstable
 * API, so use them with care.
 *******************************************************************/


/*******************************************************************
 * Advanced miscellaneous zchunk functions
 *******************************************************************/
/* Get lead length */
ssize_t ZCK_PUBLIC_API zck_get_lead_length(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get header length (lead + preface + index + sigs) */
ssize_t ZCK_PUBLIC_API zck_get_header_length(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get data length */
ssize_t ZCK_PUBLIC_API zck_get_data_length(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get file length */
ssize_t ZCK_PUBLIC_API zck_get_length(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get index digest */
char ZCK_PUBLIC_API *zck_get_header_digest(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get data digest */
char ZCK_PUBLIC_API *zck_get_data_digest(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get whether this context is pointing to a detached header */
bool ZCK_PUBLIC_API zck_is_detached_header(zckCtx *zck)
    ZCK_WARN_UNUSED;


/*******************************************************************
 * Advanced compression functions
 *******************************************************************/
/* Get name of compression type */
const ZCK_PUBLIC_API char *zck_comp_name_from_type(int comp_type)
    ZCK_WARN_UNUSED;
/* Initialize compression.  Compression type and parameters *must* be done
 * before this is called */


/*******************************************************************
 * Advanced zchunk reading functions
 *******************************************************************/
/* Initialize zchunk for reading using advanced options */
bool ZCK_PUBLIC_API zck_init_adv_read (zckCtx *zck, int src_fd)
    ZCK_WARN_UNUSED;
/* Read zchunk lead */
bool ZCK_PUBLIC_API zck_read_lead(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Read zchunk header */
bool ZCK_PUBLIC_API zck_read_header(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Validate lead */
bool ZCK_PUBLIC_API zck_validate_lead(zckCtx *zck)
    ZCK_WARN_UNUSED;

/*******************************************************************
 * Indexes
 *******************************************************************/
/* Get chunk count */
ssize_t ZCK_PUBLIC_API zck_get_chunk_count(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get chunk by number */
zckChunk ZCK_PUBLIC_API *zck_get_chunk(zckCtx *zck, size_t number)
    ZCK_WARN_UNUSED;
/* Get first chunk */
zckChunk ZCK_PUBLIC_API *zck_get_first_chunk(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get next chunk */
zckChunk ZCK_PUBLIC_API *zck_get_next_chunk(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get chunk starting location */
ssize_t ZCK_PUBLIC_API zck_get_chunk_start(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get uncompressed chunk size */
ssize_t ZCK_PUBLIC_API zck_get_chunk_size(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get compressed chunk size */
ssize_t ZCK_PUBLIC_API zck_get_chunk_comp_size(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get chunk number */
ssize_t ZCK_PUBLIC_API zck_get_chunk_number(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get validity of current chunk - 1 = valid, 0 = missing, -1 = invalid */
int ZCK_PUBLIC_API zck_get_chunk_valid(zckChunk *idx)
    ZCK_WARN_UNUSED;
/* Get chunk digest */
char ZCK_PUBLIC_API *zck_get_chunk_digest(zckChunk *item)
    ZCK_WARN_UNUSED;
/* Get digest size of chunk hash type */
ssize_t ZCK_PUBLIC_API zck_get_chunk_digest_size(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get uncompressed chunk digest */
char ZCK_PUBLIC_API *zck_get_chunk_digest_uncompressed(zckChunk *item)
    ZCK_WARN_UNUSED;
/* Get chunk data */
ssize_t ZCK_PUBLIC_API zck_get_chunk_data(zckChunk *idx, char *dst, size_t dst_size)
    ZCK_WARN_UNUSED;
/* Get compressed chunk data */
ssize_t ZCK_PUBLIC_API zck_get_chunk_comp_data(zckChunk *idx, char *dst, size_t dst_size)
    ZCK_WARN_UNUSED;
/* Find out if two chunk digests are the same */
bool ZCK_PUBLIC_API zck_compare_chunk_digest(zckChunk *a, zckChunk *b)
    ZCK_WARN_UNUSED;
/* Get associate src chunk if any */
zckChunk ZCK_PUBLIC_API *zck_get_src_chunk(zckChunk *idx)
    ZCK_WARN_UNUSED;

/*******************************************************************
 * Advanced hash functions
 *******************************************************************/
/* Get overall hash type */
int ZCK_PUBLIC_API zck_get_full_hash_type(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get digest size of overall hash type */
ssize_t ZCK_PUBLIC_API zck_get_full_digest_size(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get chunk hash type */
int ZCK_PUBLIC_API zck_get_chunk_hash_type(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Get name of hash type */
const ZCK_PUBLIC_API char *zck_hash_name_from_type(int hash_type)
    ZCK_WARN_UNUSED;



/*******************************************************************
 * Downloading (should this go in a separate header and library?)
 *******************************************************************/

/*******************************************************************
 * Ranges
 *******************************************************************/
/* Get any matching chunks from src and put them in the right place in tgt */
bool ZCK_PUBLIC_API zck_copy_chunks(zckCtx *src, zckCtx *tgt)
    ZCK_WARN_UNUSED;
/* Free zckRange */
void ZCK_PUBLIC_API zck_range_free(zckRange **info);
/* Get range string from start and end location */
char ZCK_PUBLIC_API *zck_get_range(size_t start, size_t end)
    ZCK_WARN_UNUSED;
/* Get the minimum size needed to download in order to know how large the header
 * is */
int ZCK_PUBLIC_API zck_get_min_download_size(void)
    ZCK_WARN_UNUSED;
/* Get the number of separate range items in the range */
int ZCK_PUBLIC_API zck_get_range_count(zckRange *range)
    ZCK_WARN_UNUSED;

/*******************************************************************
 * Downloading
 *******************************************************************/
/* Initialize zchunk download context */
zckDL ZCK_PUBLIC_API *zck_dl_init(zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Reset zchunk download context for reuse */
void ZCK_PUBLIC_API zck_dl_reset(zckDL *dl);
/* Free zchunk download context */
void ZCK_PUBLIC_API zck_dl_free(zckDL **dl);
/* Get zchunk context from download context */
zckCtx ZCK_PUBLIC_API *zck_dl_get_zck(zckDL *dl)
    ZCK_WARN_UNUSED;
/* Set zchunk context in download context */
bool ZCK_PUBLIC_API zck_dl_set_zck(zckDL *dl, zckCtx *zck)
    ZCK_WARN_UNUSED;
/* Clear regex used for extracting download ranges from multipart download */
void ZCK_PUBLIC_API zck_dl_clear_regex(zckDL *dl);
/* Download and process the header from url */
bool ZCK_PUBLIC_API zck_dl_get_header(zckCtx *zck, zckDL *dl, char *url)
    ZCK_WARN_UNUSED;
/* Get number of bytes downloaded using download context */
ssize_t ZCK_PUBLIC_API zck_dl_get_bytes_downloaded(zckDL *dl)
    ZCK_WARN_UNUSED;
/* Get number of bytes uploaded using download context */
ssize_t ZCK_PUBLIC_API zck_dl_get_bytes_uploaded(zckDL *dl)
    ZCK_WARN_UNUSED;
/* Set download ranges for zchunk download context */
bool ZCK_PUBLIC_API zck_dl_set_range(zckDL *dl, zckRange *range)
    ZCK_WARN_UNUSED;
/* Get download ranges from zchunk download context */
zckRange ZCK_PUBLIC_API *zck_dl_get_range(zckDL *dl)
    ZCK_WARN_UNUSED;

/* Set header callback function */
bool ZCK_PUBLIC_API zck_dl_set_header_cb(zckDL *dl, zck_wcb func)
    ZCK_WARN_UNUSED;
/* Set header userdata */
bool ZCK_PUBLIC_API zck_dl_set_header_data(zckDL *dl, void *data)
    ZCK_WARN_UNUSED;
/* Set write callback function */
bool ZCK_PUBLIC_API zck_dl_set_write_cb(zckDL *dl, zck_wcb func)
    ZCK_WARN_UNUSED;
/* Set write userdata */
bool ZCK_PUBLIC_API zck_dl_set_write_data(zckDL *dl, void *data)
    ZCK_WARN_UNUSED;

/* Write callback.  You *must* pass this and your initialized zchunk download
 * context to the downloader when downloading a zchunk file.  If you have your
 * own callback, set dl->write_cb to your callback and dl->wdata to your
 * callback data. */
size_t ZCK_PUBLIC_API zck_write_chunk_cb(void *ptr, size_t l, size_t c, void *dl_v);
size_t ZCK_PUBLIC_API zck_write_zck_header_cb(void *ptr, size_t l, size_t c, void *dl_v);
size_t ZCK_PUBLIC_API zck_header_cb(char *b, size_t l, size_t c, void *dl_v);

#endif
