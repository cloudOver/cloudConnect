/**
This file is part of KernelConnect project.

KernelConnect is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KernelConnect is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with KernelConnect.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FILE_H
#define FILE_H

#include <unistd.h>
#include <fcntl.h>
#include <proto/syscall.h>
#include <zmq.h>
#include <log.h>

enum co_file_direction {
    CO_FILEDIR_RETR,
    CO_FILEDIR_SAVE,
};

struct co_file_data {
    int fd;
    long offset;
    long length;
    enum co_file_direction direction;
};

struct co_file_context {
    void *zmq_ctx;
    void *zmq_sock;

    pthread_mutex_t lock;
    struct co_file_data *file;
};

/**
 * @brief co_file_initialize Initialize file passing channel
 * @param socket Socket used for file transfers
 * @return New co_file_context object
 */
extern struct co_file_context *co_file_initialize(char *path);

/**
 * @brief co_file_cleanup Cleanup co_file_context structure
 * @param ctx Pointer to context structure, which should be cleaned up
 */
extern void co_file_cleanup(struct co_file_context *ctx);

/**
 * @brief co_file_retreive Remote process wants to retreive part of file identified
 * by co_file_data structure. Function could not be executed in parallel with system
 * calls due to lseeks in files!
 * @param ctx Context object
 */
extern void co_file_retreive(struct co_file_context *ctx);

/**
 * @brief co_file_save Remote process wants to save file contents on local disk
 * @param ctx
 */
extern void co_file_save(struct co_file_context *ctx);

#endif // FILE_H
