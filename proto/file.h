#ifndef FILE_H
#define FILE_H

#include <unistd.h>
#include <fcntl.h>
#include <proto/syscall.h>
#include <zmq.h>
#include <log.h>

struct co_file_data {
    int fd;
    long offset;
    long length;
    enum co_file_direction direction;
};

struct co_file_context {
    void *socket;
    pthread_mutex_t lock;
    struct co_file_data *file;
};

/**
 * @brief co_file_initialize Initialize file passing channel
 * @param socket Socket used for file transfers
 * @return New co_file_context object
 */
extern struct co_file_context *co_file_initialize(void *socket);

/**
 * @brief co_file_cleanup Cleanup co_file_context structure
 * @param ctx Pointer to context structure, which should be cleaned up
 */
extern void co_file_cleanup(struct co_file_context *ctx);

/**
 * @brief co_file_retreive Retreive part of file identified by co_file_data structure. Function could not be
 * executed in parallel with system calls due to lseeks in files!
 * @param ctx Context object
 */
extern void co_file_retreive(struct co_file_context *ctx);

/**
 * @brief co_file_save Save part of file identified by co_file_data
 * @param ctx
 */
extern void co_file_save(struct co_file_context *ctx);

#endif // FILE_H
