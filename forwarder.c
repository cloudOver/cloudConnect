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
    struct co_forward_context *ctx = malloc(sizeof(struct co_forward_context));
    memset(ctx, 0x00, sizeof(struct co_forward_context));

    // Syscall socket initialization
    ctx->zmq_syscall_ctx = zmq_ctx_new();
    if (ctx->zmq_syscall_ctx == NULL) {
        co_forward_cleanup(ctx);
        return NULL;
    }

    ctx->zmq_syscall_sock = zmq_socket(ctx->zmq_syscall_ctx, ZMQ_PAIR);
    if (ctx->zmq_syscall_sock == NULL) {
        co_forward_cleanup(ctx);
        return NULL;
    }

    // TODO: check return value
    zmq_connect(ctx->zmq_syscall_sock, syscall_addr);

    if (clouddev_syscall != NULL) {
        ctx->dev_fd_syscall = open(clouddev_syscall, O_RDWR);
        if (ctx->dev_fd_syscall < 0) {
            return NULL;
        }
    }

    // File socket initialization
    ctx->zmq_file_ctx = zmq_ctx_new();
    if (ctx->zmq_file_ctx == NULL) {
        co_forward_cleanup(ctx);
        return NULL;
    }

    if (file_addr != NULL) {
        ctx->zmq_file_sock = zmq_socket(ctx->zmq_file_ctx, ZMQ_PAIR);
        if (ctx->zmq_file_sock == NULL) {
            co_forward_cleanup(ctx);
            return NULL;
        }
    }
    //TODO: Check return value
    zmq_connect(ctx->zmq_syscall_sock, syscall_addr);

    if (clouddev_file != NULL) {
        ctx->dev_fd_file = open(clouddev_file, O_RDWR);
        if (ctx->dev_fd_file < 0) {
            co_forward_cleanup(ctx);
            return NULL;
        }
    }

    return ctx;
}


void co_forward_cleanup(struct co_forward_context *ctx) {
    if (ctx->dev_fd_file > 0) {
        close(ctx->dev_fd_file);
    }
    if (ctx->dev_fd_syscall > 0) {
        close(ctx->dev_fd_syscall);
    }
    if (ctx->zmq_file_sock != NULL) {
        zmq_close(ctx->zmq_file_sock);
    }
    if (ctx->zmq_file_ctx != NULL) {
        zmq_close(ctx->zmq_file_ctx);
    }
    if (ctx->zmq_syscall_sock != NULL) {
        zmq_close(ctx->zmq_syscall_sock);
    }
    if (ctx->zmq_syscall_ctx != NULL) {
        zmq_close(ctx->zmq_syscall_ctx);
    }
    free(ctx);
    return;
}
