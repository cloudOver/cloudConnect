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

#include "router.h"

struct router_process *router_process_init(long pid, char *name) {
    int ret;

    struct router_process* process = malloc(sizeof(struct router_process));
    process->pid = pid;

    process->zmq_ctx = zmq_ctx_new();

    process->zmq_sock = zmq_socket(process->zmq_ctx, ZMQ_PAIR);

    char zmq_process_path[255];
    sprintf(zmq_process_path, PIPES_PATH "/%ld_%s", pid, name);

    syslog(LOG_DEBUG, "router_process_init: connecting with process' pipe: %s", zmq_process_path);

    ret = zmq_bind(process->zmq_sock, zmq_process_path);
    if (ret != 0) {
        syslog(LOG_ERR, "router_process_init: binding %s socket for pid %ld failed: %d", name, pid, ret);
        router_process_cleanup(process);
        return NULL;
    }

    return process;
}

void router_process_cleanup(struct router_process *process) {
    if (process->zmq_sock != NULL)
        zmq_close(process->zmq_sock);

    if (process->zmq_ctx != NULL)
        zmq_ctx_destroy(process->zmq_ctx);

    free((void *)process);
}

struct router_context* router_init(int port, const char *host) {
    struct router_context *ctx = (struct router_context*)malloc(sizeof(struct router_context));
    if (ctx == NULL) {
        syslog(LOG_CRIT, "router_init: cannot allocate memory");
        return NULL;
    }

    ctx->zmq_ctx = zmq_ctx_new();
    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, ZMQ_PAIR);

    if (ctx->zmq_sock == NULL) {
        syslog(LOG_CRIT, "router_init: failed to create syscall socket");
        return NULL;
    }

    syslog(LOG_DEBUG, "router_init: created syscall socket");

    char zmq_port_str[256];
    sprintf(zmq_port_str, "tcp://%s:%d", host, port);
    syslog(LOG_INFO, "Starting router at %s", zmq_port_str);
    int ret = zmq_bind(ctx->zmq_sock, zmq_port_str);

    if (ret != 0) {
        syslog(LOG_CRIT, "router_init: failed to bind router socket. Zmq_bind returned %d", ret);
    }
    //TODO: check ctx_new, socket and bind return codes

    syslog(LOG_DEBUG, "router_init: initializing process list lock");
    if (pthread_mutex_init(&ctx->process_list_lock, NULL) != 0) {
        syslog(LOG_ERR, "router_init: cannot initialize socket list mutex");
        return NULL;
    }
    ctx->process_list = NULL;


    return ctx;
}

static void router_process_destroy(struct router_process *process) {
    // TODO
}

void router_cleanup(struct router_context *ctx) {
    router_process_destroy(NULL);

    zmq_close(ctx->zmq_sock);
    zmq_ctx_destroy(ctx->zmq_ctx);
    free(ctx);
}


static void router_forward_to_proc(void *dest_sock, void *msg, int size) {
    int ret;
    syslog(LOG_DEBUG, "router_fwd_to_proc: got message at %p with size %d", msg, size);
    ret = zmq_send(dest_sock, msg, size, 0);
    syslog(LOG_DEBUG, "router_fwd_to_proc: zmq_send returned %d", ret);
}


static void router_forward_from_proc(void *dest_sock, zmq_msg_t *msg, int pid) {
    zmq_msg_t back_route;
    struct router_route *route_data;

    zmq_msg_init_size(&back_route, sizeof(struct router_route) + zmq_msg_size(msg));
    route_data = zmq_msg_data(&back_route);
    route_data->pid = pid;
    // TODO: universal router_route size, arch. independent
    memcpy(zmq_msg_data(&back_route) + sizeof(struct router_route), zmq_msg_data(msg), zmq_msg_size(msg));

    zmq_sendmsg(dest_sock, &back_route, 0);
    // Following send is no longer used
    //zmq_sendmsg(dest_sock, msg, 0);
    zmq_msg_close(&back_route);
}


int router_start(struct router_context *ctx) {
    zmq_msg_t msg;
    struct router_route *route_info;
    char pipe_path[256];
    int ret, forwarded = 0;
    GList *l;

    lock_and_log("process_list_lock", &ctx->process_list_lock);

    // Forward messages from peer router
    zmq_msg_init(&msg);
    ret = zmq_recvmsg(ctx->zmq_sock, &msg, ZMQ_NOBLOCK);

    if (ret > 0) {
        route_info = zmq_msg_data(&msg);
        syslog(LOG_DEBUG, "router_route: message base: %p", route_info);

        syslog(LOG_DEBUG, "router_route: received message from forwarder to pid %d", route_info->pid);
        for (l = ctx->process_list; l != NULL; l = l->next) {
            syslog(LOG_DEBUG, "router_route: trying to %p", l->data);
            syslog(LOG_DEBUG, "router_route: trying to %d", ((struct router_process*)l->data)->pid);
            if (((struct router_process*)l->data)->pid == route_info->pid) {
                syslog(LOG_DEBUG, "router_forward_msg: \tforwarding message to pid %u", route_info->pid);
                router_forward_to_proc(((struct router_process*)l->data)->zmq_sock,
                                       zmq_msg_data(&msg) + sizeof(struct router_route),
                                       zmq_msg_size(&msg) - sizeof(struct router_route));
                forwarded += 1;
                break;
            }
        }

        if (forwarded == 0)
            syslog(LOG_WARNING, "router_route: pid %d not found", route_info->pid);
    }

    // Forward messages from running processes
    for (l = ctx->process_list; l != NULL; l = l->next) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        ret = zmq_recvmsg(((struct router_process*)l->data)->zmq_sock, &msg, ZMQ_NOBLOCK);

        if (ret > 0) {
            syslog(LOG_DEBUG, "router_route: received message from pid %u", ((struct router_process*)l->data)->pid);
            router_forward_from_proc(ctx->zmq_sock, &msg, ((struct router_process*)l->data)->pid);
            forwarded += 1;
        }
    }

    unlock_and_log("process_list_lock", &ctx->process_list_lock);

    return forwarded;
}
