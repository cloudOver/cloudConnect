#include "router.h"

struct router_context* router_initialize(int syscall_port, int file_port) {
    struct router_context *ctx = (struct router_context*)malloc(sizeof(struct router_context));
    if (ctx == NULL) {
        syslog(LOG_CRIT, "router_init: cannot allocate memory");
        return NULL;
    }

    ctx->running = 1;

    ctx->syscall_context = zmq_ctx_new();
    ctx->syscall_socket = zmq_socket(ctx->syscall_context, ZMQ_PAIR);
    char syscall_port_str[256];
    sprintf(syscall_port_str, "tcp://*:%d", syscall_port);
    zmq_bind(ctx->syscall_socket, syscall_port_str);
    //TODO: check ctx_new, socket and bind return codes

    ctx->file_context = zmq_ctx_new();
    ctx->file_socket = zmq_socket(ctx->file_context, ZMQ_PAIR);
    char file_port_str[256];
    sprintf(file_port_str, "tcp://*:%d", file_port);
    zmq_bind(ctx->file_socket, file_port_str);
    //TODO: check ctx_new, socket and bind return codes

    if (pthread_mutex_init(&ctx->process_list_lock, NULL) != 0) {
        syslog(LOG_ERR, "router_initialize: cannot initialize socket list mutex");
        return 1;
    }
    ctx->process_list = NULL;

    return ctx;
}

static void router_process_destroy(struct router_process *process) {
    // TODO
}

void router_cleanup(struct router_context *ctx) {
    router_process_destroy(NULL);

    zmq_close(ctx->syscall_socket);
    zmq_ctx_destroy(ctx->syscall_context);
    free(ctx);

    zmq_close(ctx->file_socket);
    zmq_ctx_destroy(ctx->file_context);
    free(ctx);
}

void router_file(struct router_context *ctx) {
    zmq_msg_t route;
    struct router_route *route_data;
    char pipe_path[256];
    int ret;
    GList *l;

    // Forward messages from peer router
    zmq_msg_init(&route);

    ret = zmq_recvmsg(ctx->file_socket, &route, ZMQ_NOBLOCK);
    lock_and_log("process_list_lock", &ctx->process_list_lock);
    if (ret == 0) {
        syslog(LOG_DEBUG, "router_file: received message from peer router");
        route_data = zmq_msg_data(&route);
        for (l = ctx->process_list; l != NULL; l = l->next) {
            if (((struct router_process*)l->data)->pid == route_data->pid) {
                syslog(LOG_DEBUG, "router_file: \tforwarding message to pid %d", route_data->pid);
                zmq_msg_t msg;

                zmq_msg_init(&msg);
                zmq_recvmsg(ctx->file_socket, &msg, 0);
                zmq_sendmsg(((struct router_process*)l->data)->file_socket, &msg, 0);

                break;
            }
        }
    }

    // Forward messages from running processes
    for (l = ctx->process_list; l != NULL; l = l->next) {
        zmq_msg_t msg, back_route;
        ret = zmq_recvmsg(((struct router_process*)l->data)->file_socket, &msg, ZMQ_NOBLOCK);
        if (ret == 0) {
            syslog(LOG_DEBUG, "router_file: received message from pid %d", ((struct router_process*)l->data)->pid);
            zmq_msg_init(&msg);
            zmq_msg_init_size(&back_route, sizeof(struct router_route));

            route_data = zmq_msg_data(&back_route);
            route_data->pid = ((struct router_process*)l->data)->pid;
            zmq_sendmsg(ctx->file_socket, &back_route, 0);
            zmq_sendmsg(ctx->file_socket, &msg, 0);
        }
    }
    lock_and_log("process_list_lock", &ctx->process_list_lock);

    zmq_ctx_new();
}

void router_syscall(struct router_context *ctx) {

}
