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

#define PIPES_PATH "ipc:///tmp/pipes/"

#define ROUTER_CLIENT 1
#define ROUTER_CLOUD 2

enum router_mgmt_actions {
    MGMT_CREATE,
    MGMT_SIGNAL,
    MGMT_KILL,
};

struct router_mgmt {
    enum router_mgmt_actions action;
    long pid;
    long signal;
};

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
 * @brief router_process_init Initializes new router_process structure (sockets, zmq context) for
 * new process
 * @param pid pid of new process
 * @return New router_process structure
 */
struct router_process *router_process_init(long pid);


/**
 * @brief router_process_cleanup Cleanup all data related to given router_process structure
 * @param process Pointer to router_process structure
 */
void router_process_cleanup(struct router_process *process);


/**
 * @brief The router_context struct - Router's data
 */
struct router_context {
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
struct router_context* router_init(int syscall_port, int file_port, int mode, const char *host);

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
