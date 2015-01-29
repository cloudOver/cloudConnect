#ifndef ROUTER_H
#define ROUTER_H

#include <log.h>
#include <zmq.h>
#include <glib.h>

#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <proto/file.h>
#include <proto/syscall.h>


struct router_route {
    long pid;
};

struct router_process {
    long pid;

    void *syscall_context;
    void *syscall_socket;

    void *file_context;
    void *file_socket;
};

struct router_context {
    void *syscall_context;
    void *syscall_socket;
    void *file_context;
    void *file_socket;

    GList *process_list;
    pthread_mutex_t process_list_lock;
};

struct router_context* router_initialize(int syscall_port, int file_port);
void router_start(struct router_context *ctx);
void router_cleanup(struct router_context *ctx);

#endif // ROUTER_H
