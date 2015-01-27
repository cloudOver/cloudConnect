#ifndef ROUTER_H
#define ROUTER_H

#include <log.h>
#include <zmq.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


struct router_route {
    long pid;
};

struct router_context {
    void *syscall_context;
    void *syscall_socket;

    void *file_context;
    void *file_socket;
};

struct router_context* router_init(int syscall_port, int file_port);
void router_start(struct router_context *ctx);
void router_cleanup(struct router_context *ctx);

#endif // ROUTER_H
