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

/**
 * @brief The router_route struct - prepended to each message received from process and forwarded
 * to any other router. Contains pid of source process. Primary use of this struct is to identify
 * each message and forward to proper process' pipe
 */
struct router_route {
    long pid;
};

/**
 * @brief The router_process struct - represents each process connected with router. List of
 * objects is handled in router_context structure.
 */
struct router_process {
    long pid;

    void *syscall_context;
    void *syscall_socket;

    void *file_context;
    void *file_socket;
};

/**
 * @brief The router_context struct - Router's data
 */
struct router_context {
    /// Is router running
    int running;

    void *syscall_context;

    /// Socket connected with another router, to forward system calls
    void *syscall_socket;

    void *file_context;
    /// Socket connected with another router, to forward file transfers
    void *file_socket;

    /// List of connected processes
    GList *process_list;
    /// Lock for process_list to avoid concurent modifications of this list
    pthread_mutex_t process_list_lock;
};

/**
 * @brief router_initialize Initialize router
 * @param syscall_port Tcp port for incomming connections, for system calls
 * @param file_port Tcp port for incomming connections, for system calls
 * @return Context structure with necessary data for router
 */
struct router_context* router_initialize(int syscall_port, int file_port);

/**
 * @brief router_start
 * @param ctx
 */
void router_start(struct router_context *ctx);

/**
 * @brief router_cleanup Cleanup context structure and close router
 * @param ctx
 */
void router_cleanup(struct router_context *ctx);

#endif // ROUTER_H
