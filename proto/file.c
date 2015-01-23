#include "file.h"

struct co_file_context *co_file_initialize(void *socket) {
    struct co_file_context *ctx = (struct co_file_context*) malloc(sizeof(struct co_file_context));
    if (ctx == NULL) {
        syslog(LOG_ERR, "co_file_initialize: cannot allocate memory");
        return 1;
    }

    ctx->socket = sock;
    ctx->file = (struct co_file_data*) malloc(sizeof(struct co_file_data));

    if (ctx->socket == NULL) {
        syslog(LOG_ERR, "co_file_initialize: cannot create socket");
        return 1;
    }

    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        syslog(LOG_ERR, "co_file_initialize: cannot initialize mutex");
        return 1;
    }

    return 0;
}


void co_file_cleanup(struct co_file_context *ctx) {
    syslog(LOG_INFO, "co_file_cleanup: freeing ctx structure");
    pthread_mutex_destroy(&ctx->lock);
    free(ctx->file);
    free(ctx);
}


void co_file_retreive(struct co_file_context *ctx) {
    lock_and_log("file_retreive", &ctx->lock);
    zmq_recv(ctx->socket, (void*)ctx->file, sizeof(struct co_file_data), 0);

    long pos = lseek(ctx->file->fd, 0, SEEK_CUR);
    lseek(ctx->file->fd, ctx->file->offset, SEEK_SET);

    void *data = malloc(ctx->file->length);
    long len = read(ctx->file->fd, data, ctx->file->length);
    lseek(ctx->file->fd, pos, SEEK_SET);

    zmq_send(ctx->socket, data, ctx->file->length, 0);

    unlock_and_log("file_retreive", &ctx->lock);
}

void co_file_save(struct co_file_context *ctx) {
    lock_and_log("file_retreive", &ctx->lock);
    zmq_recv(ctx->socket, (void*)ctx->file, sizeof(struct co_file_data), 0);

    void *data = malloc(ctx->file->length);
    zmq_recv(ctx->socket, data, ctx->file->length, 0);

    long pos = lseek(ctx->file->fd, 0, SEEK_CUR);
    lseek(ctx->file->fd, ctx->file->offset, SEEK_SET);
    long len = write(ctx->file->fd, data, ctx->file->length);
    lseek(ctx->file->fd, pos, SEEK_SET);

    unlock_and_log("file_retreive", &ctx->lock);
}
