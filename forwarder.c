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

#include "forwarder.h"


struct co_forward_context *co_forward_init(char *file_addr, char *syscall_addr, char *clouddev_file, char *clouddev_syscall) {
    co_forward_context *ctx = malloc(sizeof(struct co_forward_context));

    ctx->zmq_file_ctx = zmq_ctx_new();
    ctx->zmq_syscall_ctx = zmq_ctx_new();

    ctx->zmq_file_sock = zmq_socket(ctx->zmq_file_ctx, ZMQ_PAIR);
    ctx->zmq_syscall_sock = zmq_socket(ctx->zmq_syscall_ctx, ZMQ_PAIR);

    ctx->dev_fd_file = open(clouddev_file, O_RDWR);
    ctx->dev_fd_syscall = open(clouddev_syscall, O_RDWR);
}
