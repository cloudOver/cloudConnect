#include <zmq.h>
#include <log.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <proto/syscall.h>
#include <proto/file.h>

#include <router.h>

int i_am_running = 1;

void print_help() {
    fprintf(stderr, "Usage: %s [router|plug]");
}

void handle_router_sigterm(int signum) {
    i_am_running = 0;
    fprintf(stderr, "Sig %d!", signum);
}

void handle_plug_sigterm(int signum) {
    i_am_running = 0;
    fprintf(stderr, "Sig %d!", signum);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_help();
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "router") == 0) {
        struct router_context *ctx = router_init(3313, 3323);


    } else if (strcmp(argv[1], "plug") == 0) {
        pid_t pid = getpid();
        char path[] = "/var/lib/cloudOver/pipes/";
        char syscall_pipe_path[256];
        char file_pipe_path[256];
        sprintf(syscall_pipe_path, "%s/%d_syscall", path, (int)pid);
        sprintf(file_pipe_path, "%s/%d_file", path, (int)pid);

        int fd;
        if (access(syscall_pipe_path, F_OK) == 0) {
            syslog(LOG_ERR, "main: pipe %s exists", syscall_pipe_path);
            close(fd);
            exit(1);
        }
        if (access(file_pipe_path, F_OK) == 0) {
            syslog(LOG_ERR, "main: pipe %s exists", file_pipe_path);
            exit(1);
        }

        if (mkfifo(syscall_pipe_path, 0770) != 0) {
            syslog(LOG_ERR, "main: cannot create pipe %s", syscall_pipe_path);
            exit(1);
        }

        if (mkfifo(file_pipe_path, 0770) != 0) {
            syslog(LOG_ERR, "main: cannot create pipe %s", file_pipe_path);
            exit(1);
        }

        struct co_syscall_context *syscall_ctx = co_syscall_initialize(syscall_pipe_path);
        struct co_file_context *file_ctx = co_file_initialize(file_pipe_path);

        while (1) {
        }
    } else {
        print_help();
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "main: initializing server");
    void *zmq_ctx_syscall = zmq_ctx_new();
    void *zmq_ctx_file = zmq_ctx_new();
    void *zmq_sock_syscall = zmq_socket(zmq_ctx_syscall, ZMQ_PAIR);
    void *zmq_file_syscall = zmq_socket(zmq_ctx_file, ZMQ_PAIR);

    syslog(LOG_INFO, "main: binding connections");
    assert(zmq_bind(zmq_sock_syscall, "tcp://*:3314") == 0);
    assert(zmq_bind(zmq_sock_syscall, "tcp://*:3315") == 0);

}
