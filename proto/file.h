#ifndef FILE_H
#define FILE_H

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
    struct co_file_data *file_data;
};

#endif // FILE_H
