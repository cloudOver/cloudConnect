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

#ifndef FORWARDER_H
#define FORWARDER_H

#include <glib.h>
#include <zmq.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

struct co_forward_context {
    void *zmq_file_ctx;
    void *zmq_file_sock;
    void *zmq_syscall_ctx;
    void *zmq_syscall_sock;
    int dev_fd_file;
    int dev_fd_syscall;
};

/**
 * @brief co_forward_init initializez forwarding to cloud kernel mechanisms
 */
struct co_forward_context *co_forward_init(char *file_addr, char *syscall_addr, char *clouddev_file, char *clouddev_syscall);

void co_forward();

void co_forward_cleanup(struct co_forward_context *ctx);

#endif // FORWARDER_H
