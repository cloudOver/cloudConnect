#ifndef ROUTER_H
#define ROUTER_H

#include <log.h>
#include <zmq.h>
#include <gli.h>

#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


struct router_route {
    long pid;
};

struct router_context {
    void *syscall_context;
    void *syscall_socket;
    kvec_t(void *) syscall_ctx_list;
    kvec_t(void *) syscall_sock_list;
    pthread_mutex_t syscall_lock;

    void *file_context;
    void *file_socket;
    kvec_t(void *) file_ctx_list;
    kvec_t(void *) file_sock_list;
    pthread_mutex_t file_lock;
};

struct router_context* router_initialize(int syscall_port, int file_port);
void router_start(struct router_context *ctx);
void router_cleanup(struct router_context *ctx);

#endif // ROUTER_H
