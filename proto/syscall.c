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

#include <proto/syscall.h>

struct co_syscall_context* co_syscall_initialize(char *path) {
    struct co_syscall_context *ctx = (struct co_syscall_context*) malloc(sizeof(struct co_syscall_context));
    if (ctx == NULL) {
        syslog(LOG_ERR, "co_syscall_initialize: cannot allocate memory");
        return NULL;
    }

    ctx->syscall_id = 0;

    ctx->zmq_ctx = zmq_ctx_new();
    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, ZMQ_PAIR);
    zmq_bind(ctx->zmq_sock, path);
    //TODO: check return codes

    ctx->syscall = (struct co_syscall_data*) malloc(sizeof(struct co_syscall_data));

    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        syslog(LOG_ERR, "co_syscall_initialize: cannot initialize mutex");
        zmq_close(ctx->zmq_sock);
        zmq_ctx_destroy(ctx->zmq_ctx);
        free(ctx);
        return NULL;
    }

    return ctx;
}

void co_syscall_cleanup(struct co_syscall_context *ctx) {
    syslog(LOG_INFO, "co_syscall_cleanup: freeing ctx structure");
    pthread_mutex_destroy(&ctx->lock);
    free(ctx->syscall);

    zmq_close(ctx->zmq_sock);
    zmq_ctx_destroy(ctx->zmq_ctx);

    free(ctx);
}


void co_syscall_execute(struct co_syscall_context *ctx) {
    lock_and_log("syscall_execute", &ctx->lock);
    syslog(LOG_INFO, "co_syscall_execute: executing syscall %ld", ctx->syscall->syscall_num);
    ctx->syscall->ret_code = syscall(ctx->syscall->syscall_num,
                                     ctx->syscall->param[0],
                                     ctx->syscall->param[1],
                                     ctx->syscall->param[2],
                                     ctx->syscall->param[3],
                                     ctx->syscall->param[4],
                                     ctx->syscall->param[5]);
    syslog(LOG_DEBUG, "co_syscall_execute: \treturned %d", ctx->syscall->ret_code);
    unlock_and_log("syscall_execute", &ctx->lock);
}


void co_syscall_serialize(struct co_syscall_context *ctx) {
    lock_and_log("syscall_serialize", &ctx->lock);
    syslog(LOG_INFO, "co_syscall_serialize: serializing syscall %ld", ctx->syscall_id);

    zmq_send(ctx->zmq_sock, (void *)ctx->syscall, sizeof(struct co_syscall_data), 0);
    int i;
    for (i = 0; i < CO_PARAM_COUNT; i++) {
        // Serialize required params (READ and BOTH directions)
        if (ctx->syscall->param_mode[i] == CO_PARAM_READ || ctx->syscall->param_mode[i] == CO_PARAM_BOTH) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \tsending parameter %d (%ld bytes)", i, ctx->syscall->param_size[i]);
            zmq_send(ctx->zmq_sock, (void*)ctx->syscall->param[i], ctx->syscall->param_size[i], 0);
        }

        // Free unused memory
        if (ctx->syscall->param_mode[i] != CO_PARAM_VALUE) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \trelease memory for parameter %d", i);
            free((void*)ctx->syscall->param[i]);
            ctx->syscall->param[i] = 0x00;
        }
    }
    unlock_and_log("syscall_serialize", &ctx->lock);
}


int co_syscall_deserialize(struct co_syscall_context *ctx) {
    lock_and_log("syscall_deserialize", &ctx->lock);

    if (zmq_recv(ctx->zmq_sock, (void *)ctx->syscall, sizeof(struct co_syscall_data), ZMQ_NOBLOCK) != 0) {
        syslog(LOG_DEBUG, "co_syscall_deserialize: no new messages");
        unlock_and_log("syscall_deserialize", &ctx->lock);
        return -1;
    }

    ctx->syscall_id += 1;
    syslog(LOG_DEBUG, "co_syscall_deserialize: received syscall: %ld", ctx->syscall->syscall_num);

    int i;
    for (i = 0; i < CO_PARAM_COUNT; i++) {
        if (ctx->syscall->param_mode[i] != CO_PARAM_VALUE) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \tallocating memory for param %d (%ld bytes)", i, ctx->syscall->param_size[i]);
            ctx->syscall->param[i] = (unsigned long) malloc(ctx->syscall->param_size[i]);
        }

        if (ctx->syscall->param_mode[i] == CO_PARAM_WRITE || ctx->syscall->param_mode[i] == CO_PARAM_BOTH) {
            syslog(LOG_DEBUG, "co_syscall_serialize: \treceiving parameter %d", i);
            zmq_recv(ctx->zmq_sock, (void *)ctx->syscall->param[i], ctx->syscall->param_size[i], 0);
        }
    }
    unlock_and_log("syscall_deserialize", &ctx->lock);
    return ctx->syscall_id;
}
