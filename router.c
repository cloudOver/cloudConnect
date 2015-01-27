#include "router.h"

struct router_context* router_init(int syscall_port, int file_port) {
    if (access("/var/lib/cloudOver/pipes/", F_OK) != 0) {
        syslog(LOG_CRIT, "router_init: no access to /var/lib/cloudOver/pipes/");
        return NULL;
    }

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
}

void router_cleanup(struct router_context *ctx) {
    zmq_close(ctx->syscall_socket);
    zmq_ctx_destroy(ctx->syscall_context);
    free(ctx);

    zmq_close(ctx->file_socket);
    zmq_ctx_destroy(ctx->file_context);
    free(ctx);
}

void router_file_start(struct router_context *ctx) {
    struct router_route route;
    char pipe_path[256];
    int pipe_fd;

    zmq_recv(ctx->file_socket, &route, sizeof(struct router_route), 0);
    sprintf(pipe_path, "/var/lib/cloudOver/pipes/%d", route.pid);
    if (access(pipe_path, F_OK) != 0) {
        syslog(LOG_CRIT, "router_file_start: cannot find pipe %d", route.pid);
        return;
    }

    pipe_fd = open(pipe_path, O_RDWR);

    while (1) {
        //select() //pipe
    }
}

void router_syscall_start(struct router_context *ctx) {

}
