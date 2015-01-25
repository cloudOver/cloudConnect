#ifndef ROUTER_H
#define ROUTER_H

#include <log.h>
#include <zmq.h>
#include <unistd.h>
#include <signal.h>


struct router_context {
    void *syscall_context;
    void *syscall_socket;

    void *file_context;
    void *file_socket;
};

struct router_context* router_init(int syscall_port, int file_port);
void start_router();
void router_cleanup(router_context *ctx);

#endif // ROUTER_H
