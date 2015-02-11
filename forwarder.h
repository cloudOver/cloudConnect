#ifndef FORWARDER_H
#define FORWARDER_H

#include <glib.h>

struct co_forward_context {
    void *zmq_router_ctx;
    void *zmq_router_sock;
    int dev_fd;
};

/**
 * @brief co_forward_init initializez forwarding to cloud kernel mechanisms
 */
struct co_forward_context *co_forward_init();

void co_forward();

void co_forward_cleanup(struct co_forward_context *ctx);

#endif // FORWARDER_H
