#include "file.h"

struct co_file_context *co_file_initialize(char *path) {
    struct co_file_context *ctx = (struct co_file_context*) malloc(sizeof(struct co_file_context));
    if (ctx == NULL) {
        syslog(LOG_ERR, "co_file_initialize: cannot allocate memory");
        return NULL;
    }

    ctx->zmq_ctx = zmq_ctx_new();
    ctx->zmq_sock = zmq_socket(ctx->zmq_ctx, ZMQ_PAIR);
    zmq_connect(ctx->zmq_sock, path);
    // TODO: check return codes

    ctx->file = (struct co_file_data*) malloc(sizeof(struct co_file_data));

    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        syslog(LOG_ERR, "co_file_initialize: cannot initialize mutex");
        zmq_close(ctx->zmq_sock);
        zmq_ctx_destroy(ctx->zmq_ctx);

        free(ctx->file);
        free(ctx);
        return NULL;
    }

    return ctx;
}


void co_file_cleanup(struct co_file_context *ctx) {
    syslog(LOG_INFO, "co_file_cleanup: freeing ctx structure");
    pthread_mutex_destroy(&ctx->lock);
    free(ctx->file);

    zmq_close(ctx->zmq_sock);
    zmq_ctx_destroy(ctx->zmq_ctx);

    free(ctx);
}


void co_file_retreive(struct co_file_context *ctx) {
    lock_and_log("file_retreive", &ctx->lock);
    zmq_recv(ctx->zmq_sock, (void*)ctx->file, sizeof(struct co_file_data), 0);

    long pos = lseek(ctx->file->fd, 0, SEEK_CUR);
    lseek(ctx->file->fd, ctx->file->offset, SEEK_SET);

    void *data = malloc(ctx->file->length);
    read(ctx->file->fd, data, ctx->file->length);
    lseek(ctx->file->fd, pos, SEEK_SET);

    zmq_send(ctx->zmq_sock, data, ctx->file->length, 0);

    unlock_and_log("file_retreive", &ctx->lock);
}

void co_file_save(struct co_file_context *ctx) {
    lock_and_log("file_retreive", &ctx->lock);
    zmq_recv(ctx->zmq_sock, (void*)ctx->file, sizeof(struct co_file_data), 0);

    void *data = malloc(ctx->file->length);
    zmq_recv(ctx->zmq_sock, data, ctx->file->length, 0);

    long pos = lseek(ctx->file->fd, 0, SEEK_CUR);
    lseek(ctx->file->fd, ctx->file->offset, SEEK_SET);
    write(ctx->file->fd, data, ctx->file->length);
    lseek(ctx->file->fd, pos, SEEK_SET);

    unlock_and_log("file_retreive", &ctx->lock);
}
