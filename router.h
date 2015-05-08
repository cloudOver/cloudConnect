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

#ifndef ROUTER_H
#define ROUTER_H

#include <log.h>
#include <zmq.h>
#include <glib.h>

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <proto/file.h>
#include <proto/syscall.h>

#define PIPES_DIR  "/tmp/pipes"
#define PIPES_PATH "ipc://" PIPES_DIR "/"

/**
 * @brief The router_mgmt_actions enum defines possible management actions for process
 */
enum router_mgmt_actions {
    MGMT_CREATE,
    MGMT_SIGNAL,
    MGMT_KILL,
};


struct router_mgmt {
    enum router_mgmt_actions action;
    unsigned int pid;
    long signal;
};

/**
 * @brief The router_route struct - prepended to each message received from process and forwarded
 * to any other router. Contains pid of source process. Primary use of this struct is to identify
 * each message and forward to proper process' pipe
 */
struct router_route {
    unsigned int pid;
};

/**
 * @brief The router_process struct - represents each process connected with router. List of
 * objects is handled in router_context structure.
 */
struct router_process {
    unsigned int pid;

    void *zmq_ctx;
    void *zmq_sock;
};

/**
 * @brief router_process_init Initializes new router_process structure (sockets, zmq context) for
 * new process
 * @param pid pid of new process
 * @param name the name of router's socket
 * @return New router_process structure
 */
struct router_process *router_process_init(long pid, char *name);


/**
 * @brief router_process_cleanup Cleanup all data related to given router_process structure
 * @param process Pointer to router_process structure
 */
void router_process_cleanup(struct router_process *process);


/**
 * @brief The router_context struct - Router's data
 */
struct router_context {
    void *zmq_ctx;

    /// Socket connected with another router, to forward system calls
    void *zmq_sock;

    /// List of connected processes
    GList *process_list;

    /// Lock for process_list to avoid concurent modifications of this list
    pthread_mutex_t process_list_lock;
};

/**
 * @brief router_initialize Initialize router
 * @param port Tcp port for incomming connections, for system calls
 * @param host The host with forwarder
 * @return Context structure with necessary data for router
 */
struct router_context* router_init(int port, const char *host);

/**
 * @brief router_route
 * @param ctx
 */
int router_start(struct router_context *ctx);

/**
 * @brief router_cleanup Cleanup context structure and close router
 * @param ctx
 */
void router_cleanup(struct router_context *ctx);

#endif // ROUTER_H
