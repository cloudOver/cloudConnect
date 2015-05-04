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
    struct co_forward_context *ctx = malloc(sizeof(struct co_forward_context));
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
    zmq_bind(ctx->zmq_sock, router_addr);
    syslog(LOG_DEBUG, "forward_init: forwarder started");

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
    syslog(LOG_DEBUG, "forward: checking messages from kernel");
    msg_size = read(ctx->dev_fd, msg, 1024*1024);
    // TODO: Handle -EAGAIN to increase the buffer size
    if (msg_size > 0) {
        syslog(LOG_DEBUG, "forward: got new messages from kernel (%d bytes)", msg_size);
        int ret = zmq_send(ctx->zmq_sock, msg, msg_size, 0);
        syslog(LOG_DEBUG, "forward: message forwarded. Exit code: %d", ret);
    }

    // Receive message from router, if available
    syslog(LOG_DEBUG, "forward: checking messages from router");
    msg_size = zmq_recv(ctx->zmq_sock, msg, 1024*1024, ZMQ_DONTWAIT);
    if (msg_size > 0) {
        syslog(LOG_DEBUG, "forward: got new mesages from router");
        write(ctx->dev_fd, msg, msg_size);
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
