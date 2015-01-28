#include "router.h"

struct router_context* router_initialize(int syscall_port, int file_port) {
    struct router_context *ctx = (struct router_context*)malloc(sizeof(struct router_context));
    if (ctx == NULL) {
        syslog(LOG_CRIT, "router_init: cannot allocate memory");
        return NULL;
    }

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

    kv_init(ctx->syscall_ctx_list);
    kv_init(ctx->syscall_sock_list);
    kv_init(ctx->file_ctx_list);
    kv_init(ctx->file_sock_list);

    if (pthread_mutex_init(&ctx->socket_lock, NULL) != 0) {
        syslog(LOG_ERR, "router_initialize: cannot initialize socket list mutex");
        return 1;
    }
    if (pthread_mutex_init(&ctx->file_lock, NULL) != 0) {
        syslog(LOG_ERR, "router_initialize: cannot initialize file list mutex");
        return 1;
    }
}

void router_cleanup(struct router_context *ctx) {
    kv_destroy(ctx->syscall_ctx_list);
    kv_destroy(ctx->syscall_sock_list);
    kv_destroy(ctx->file_ctx_list);
    kv_destroy(ctx->file_sock_list);

    zmq_close(ctx->syscall_socket);
    zmq_ctx_destroy(ctx->syscall_context);
    free(ctx);

    zmq_close(ctx->file_socket);
    zmq_ctx_destroy(ctx->file_context);
    free(ctx);
}

void router_file_receiver(struct router_context *ctx) {
    struct router_route route;
    char pipe_path[256];

    zmq_recv(ctx->file_socket, &route, sizeof(struct router_route), 0);

    zmq_ctx_new();
}

void router_syscall_start(struct router_context *ctx) {

}
