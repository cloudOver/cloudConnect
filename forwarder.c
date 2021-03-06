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


struct co_forward_context *co_forward_init(char *router_addr, char *dev_path) {
    int ret;
    struct co_forward_context *ctx;
    ctx = malloc(sizeof(struct co_forward_context));
    memset(ctx, 0x00, sizeof(struct co_forward_context));

    ctx->zmq_ctx = zmq_ctx_new();
    if (ctx->zmq_ctx == NULL) {
        co_forward_cleanup(ctx);
        return NULL;
    }

    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, ZMQ_PAIR);
    if (ctx->zmq_sock == NULL) {
        syslog(LOG_CRIT, "forward_init: cannot create socket");
        co_forward_cleanup(ctx);
        return NULL;
    }

    //TODO: Check return value
    ret = zmq_connect(ctx->zmq_sock, router_addr);
    if (ret != 0) {
        perror("co_forward_init: connect to router");
        co_forward_cleanup(ctx);
    }

    if (dev_path != NULL) {
        ctx->dev_fd = open(dev_path, O_RDWR);
        if (ctx->dev_fd < 0) {
            syslog(LOG_CRIT, "forward_init: cannot open cloud dev");
            co_forward_cleanup(ctx);
            return NULL;
        }
    }

    return ctx;
}


void co_forward(struct co_forward_context *ctx) {
    char msg[1024*1024];
    long msg_size;

    // Send message to router, if available
    msg_size = read(ctx->dev_fd, msg, 1024*1024);

    // TODO: Handle -EAGAIN to increase the buffer size
    if (msg_size > 0) {
        syslog(LOG_INFO, "forward: got new messages from kernel (%d bytes)", msg_size);
        int ret = zmq_send(ctx->zmq_sock, msg, msg_size, 0);
        struct router_route *r_info = msg;
        struct co_syscall_data *sc_info = msg + sizeof(struct router_route);
        syslog(LOG_DEBUG, "forward: message forwarded: %d(%d,%d,%d,%d,%d,%d)", sc_info->syscall_num, sc_info->param_mode[0], sc_info->param_mode[1], sc_info->param_mode[2], sc_info->param_mode[3], sc_info->param_mode[4], sc_info->param_mode[5]);
    }

    // Receive message from router, if available
    msg_size = zmq_recv(ctx->zmq_sock, msg, 1024*1024, ZMQ_DONTWAIT);
    if (msg_size > 0) {
        syslog(LOG_INFO, "forward: got new mesages from router");
        write(ctx->dev_fd, msg, msg_size);
        struct router_route *r_info = msg;
        struct co_syscall_data *sc_info = msg + sizeof(struct router_route);
        syslog(LOG_DEBUG, "forward: message forwarded: %d(%d,%d,%d,%d,%d,%d)", sc_info->syscall_num, sc_info->param_mode[0], sc_info->param_mode[1], sc_info->param_mode[2], sc_info->param_mode[3], sc_info->param_mode[4], sc_info->param_mode[5]);
    }
}

void co_forward_cleanup(struct co_forward_context *ctx) {
    if (ctx->dev_fd > 0) {
        close(ctx->dev_fd);
    }
    if (ctx->zmq_sock != NULL) {
        zmq_close(ctx->zmq_sock);
    }
    if (ctx->zmq_ctx != NULL) {
        zmq_close(ctx->zmq_ctx);
    }
    free(ctx);
    return;
}
