#include "router.h"

struct router_process *router_process_init(long pid) {
    int ret;

    struct router_process* process = malloc(sizeof(struct router_process));
    process->pid = pid;

    process->file_context = zmq_ctx_new();
    process->syscall_context = zmq_ctx_new();

    process->file_socket = zmq_socket(process->file_context, ZMQ_PAIR);
    process->syscall_socket = zmq_socket(process->syscall_context, ZMQ_PAIR);


    char syscall_path[255], file_path[255];
    sprintf(syscall_path, PIPES_PATH "/%ld_syscall", pid);
    sprintf(file_path, PIPES_PATH "/%ld_file", pid);

    ret = zmq_bind(process->syscall_socket, syscall_path);
    if (ret != 0) {
        syslog(LOG_ERR, "router_process_init: binding syscall socket for pid %ld failed: %d", pid, ret);
        router_process_cleanup(process);
        return NULL;
    }

    ret = zmq_bind(process->file_socket, file_path);
    if (ret != 0) {
        syslog(LOG_ERR, "router_process_init: binding file socket for pid %ld failed: %d", pid, ret);
        router_process_cleanup(process);
        return NULL;
    }

    return process;
}

void router_process_cleanup(struct router_process *process) {
    if (process->file_socket != NULL)
        zmq_close(process->file_socket);

    if (process->syscall_socket != NULL)
        zmq_close(process->syscall_socket);

    if (process->file_context != NULL)
        zmq_ctx_destroy(process->file_context);

    if (process->syscall_context != NULL)
        zmq_ctx_destroy(process->syscall_context);

    free((void *)process);
}

struct router_context* router_init(int syscall_port, int file_port, const char *host) {
    struct router_context *ctx = (struct router_context*)malloc(sizeof(struct router_context));
    if (ctx == NULL) {
        syslog(LOG_CRIT, "router_init: cannot allocate memory");
        return NULL;
    }

    ctx->syscall_context = zmq_ctx_new();
    ctx->syscall_socket = zmq_socket(ctx->syscall_context, ZMQ_PAIR);

    if (ctx->syscall_socket == NULL) {
        syslog(LOG_CRIT, "router_init: failed to create syscall socket");
        return NULL;
    }

    syslog(LOG_DEBUG, "router_init: created syscall socket");

    char syscall_port_str[256];
    sprintf(syscall_port_str, "tcp://%s:%d", host, syscall_port);
    zmq_connect(ctx->syscall_socket, syscall_port_str);

    //TODO: check ctx_new, socket and bind return codes

    ctx->file_context = zmq_ctx_new();
    ctx->file_socket = zmq_socket(ctx->file_context, ZMQ_PAIR);

    if (ctx->syscall_socket == NULL) {
        syslog(LOG_CRIT, "router_init: failed to create file socket");
        return NULL;
    }

    syslog(LOG_DEBUG, "router_init: created file socket");

    char file_port_str[256];
    sprintf(file_port_str, "tcp://%s:%d", host, file_port);
    zmq_connect(ctx->file_socket, file_port_str);
    //TODO: check ctx_new, socket and bind return codes

    syslog(LOG_DEBUG, "router_init: initializing process list lock");
    if (pthread_mutex_init(&ctx->process_list_lock, NULL) != 0) {
        syslog(LOG_ERR, "router_init: cannot initialize socket list mutex");
        return ctx;
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


static void router_forward_to_proc(void *src_sock, void *dest_sock) {
    // Forward one message from peer router to process
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    zmq_recvmsg(src_sock, &msg, 0);
    zmq_sendmsg(dest_sock, &msg, 0);
}


static void router_forward_from_proc(void *dest_sock, zmq_msg_t *msg, int pid) {
    zmq_msg_t back_route;
    struct router_route *route_data;

    zmq_msg_init_size(&back_route, sizeof(struct router_route));
    route_data = zmq_msg_data(&back_route);
    route_data->pid = pid;
    zmq_sendmsg(dest_sock, &back_route, 0);
    zmq_sendmsg(dest_sock, msg, 0);
}


static int router_file(struct router_context *ctx) {
    zmq_msg_t route;
    struct router_route *route_data;
    char pipe_path[256];
    int ret, forwarded = 0;
    GList *l;

    lock_and_log("process_list_lock", &ctx->process_list_lock);

    // Forward messages from peer router
    zmq_msg_init(&route);
    ret = zmq_recvmsg(ctx->file_socket, &route, ZMQ_NOBLOCK);

    if (ret > 0) {
        syslog(LOG_DEBUG, "router_file: received message from peer router");
        route_data = zmq_msg_data(&route);
        for (l = ctx->process_list; l != NULL; l = l->next) {
            if (((struct router_process*)l->data)->pid == route_data->pid) {
                syslog(LOG_DEBUG, "router_forward_msg: \tforwarding message to pid %u", route_data->pid);
                router_forward_to_proc(ctx->file_socket, ((struct router_process*)l->data)->file_socket);
                forwarded += 1;
                break;
            }
        }

        if (forwarded == 0)
            syslog(LOG_WARNING, "router_syscall: pid not found");
    }

    // Forward messages from running processes
    for (l = ctx->process_list; l != NULL; l = l->next) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        ret = zmq_recvmsg(((struct router_process*)l->data)->file_socket, &msg, ZMQ_NOBLOCK);

        if (ret > 0) {
            syslog(LOG_DEBUG, "router_file: received message from pid %u", ((struct router_process*)l->data)->pid);
            router_forward_from_proc(ctx->file_socket, &msg, ((struct router_process*)l->data)->pid);
            forwarded += 1;
        }
    }

    unlock_and_log("process_list_lock", &ctx->process_list_lock);
    return forwarded;
}

static int router_syscall(struct router_context *ctx) {
    zmq_msg_t route;
    struct router_route *route_data;
    char pipe_path[256];
    int ret, forwarded = 0;
    GList *l;

    lock_and_log("process_list_lock", &ctx->process_list_lock);

    // Forward messages from peer router
    zmq_msg_init(&route);
    ret = zmq_recvmsg(ctx->syscall_socket, &route, ZMQ_NOBLOCK);

    if (ret > 0) {
        syslog(LOG_DEBUG, "router_syscall: received message from peer router");
        route_data = zmq_msg_data(&route);
        for (l = ctx->process_list; l != NULL; l = l->next) {
            if (((struct router_process*)l->data)->pid == route_data->pid) {
                syslog(LOG_DEBUG, "router_forward_msg: \tforwarding message to pid %u", route_data->pid);
                router_forward_to_proc(ctx->syscall_socket, ((struct router_process*)l->data)->syscall_socket);
                forwarded += 1;
                break;
            }
        }

        if (forwarded == 0)
            syslog(LOG_WARNING, "router_syscall: pid not found");
    }

    // Forward messages from running processes
    for (l = ctx->process_list; l != NULL; l = l->next) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        ret = zmq_recvmsg(((struct router_process*)l->data)->syscall_socket, &msg, ZMQ_NOBLOCK);

        if (ret > 0) {
            syslog(LOG_DEBUG, "router_syscall: received message from pid %u", ((struct router_process*)l->data)->pid);
            router_forward_from_proc(ctx->syscall_socket, &msg, ((struct router_process*)l->data)->pid);
            forwarded += 1;
        }
    }

    unlock_and_log("process_list_lock", &ctx->process_list_lock);
    return forwarded;
}


void router_start(struct router_context *ctx) {
    int messages = 0;
    messages += router_syscall(ctx);
    messages += router_file(ctx);
    if (messages == 0) {
        syslog(LOG_DEBUG, "router_start: no messages. sleeping");
        sleep(10);
    }
}
