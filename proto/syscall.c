#include <proto/syscall.h>

struct co_syscall_context* co_syscall_initialize(void *sock) {
    struct co_syscall_context *ctx = (struct co_syscall_context*) malloc(sizeof(struct co_syscall_context));
    if (ctx == NULL) {
        syslog(LOG_ERR, "co_syscall_initialize: cannot allocate memory");
        return 1;
    }

    ctx->syscall_id = 0;
    ctx->socket = sock;
    ctx->syscall = (struct co_syscall_data*) malloc(sizeof(struct co_syscall_data));

    if (ctx->socket == NULL) {
        syslog(LOG_ERR, "co_syscall_initialize: cannot create socket");
        return 1;
    }

    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        syslog(LOG_ERR, "co_syscall_initialize: cannot initialize mutex");
        return 1;
    }

    return 0;
}


void co_syscall_execute(struct co_syscall_context *ctx) {
    syslog(LOG_INFO, "co_syscall_execute: executing syscall %d", ctx->syscall->syscall_num);
    ctx->syscall->ret_code = syscall(ctx->syscall->syscall_num,
                                     ctx->syscall->param[0],
                                     ctx->syscall->param[1],
                                     ctx->syscall->param[2],
                                     ctx->syscall->param[3],
                                     ctx->syscall->param[4],
                                     ctx->syscall->param[5]);
    syslog(LOG_DEBUG, "co_syscall_execute: \treturned %d", ctx->syscall->ret_code);
}


void co_syscall_serialize(struct co_syscall_context *ctx) {
    syslog(LOG_INFO, "co_syscall_serialize: serializing syscall %d", ctx->syscall_id);

    zmq_send(ctx->socket, (void *)ctx->syscall, sizeof(struct co_syscall_data), 0);
    for (int i = 0; i < CO_PARAM_COUNT; i++) {
        // Serialize required params (READ and BOTH directions)
        if (ctx->syscall->param_mode[i] == CO_PARAM_READ || ctx->syscall->param_mode[i] == CO_PARAM_BOTH) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \tsending parameter %d (%d bytes)", i, ctx->syscall->param_size[i]);
            zmq_send(ctx->socket, (void*)ctx->syscall->param[i], ctx->syscall->param_size[i], 0);
        }

        // Free unused memory
        if (ctx->syscall->param_mode[i] != CO_PARAM_VALUE) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \trelease memory for parameter %d", i);
            free(ctx->syscall->param[i]);
            ctx->syscall->param[i] = NULL;
        }
    }
}

void co_syscall_deserialize(struct co_syscall_context *ctx) {
    ctx->syscall_id += 1;

    zmq_recv(ctx->socket, (void *)ctx->syscall, sizeof(struct co_syscall_data), 0);
    for (int i = 0; i < CO_PARAM_COUNT; i++) {
        if (ctx->syscall->param_mode[i] != CO_PARAM_VALUE) {
            ctx->syscall->param[i] = (unsigned long) malloc(ctx->syscall->param_size[i]);
        }

        if (ctx->syscall->param_mode[i] == CO_PARAM_WRITE || ctx->syscall->param_mode[i] == CO_PARAM_BOTH) {
            zmq_recv(ctx->socket, (void *)ctx->syscall->param[i], ctx->syscall->param_size[i], 0);
        }
    }
}
