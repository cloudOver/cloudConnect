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

#define MGMT_PATH "/co_mgmt"

int i_am_running = 1;

void print_help(char *prog_name) {
    fprintf(stderr, "Usage: %s [clientrouter|cloudrouter|plug]\n", prog_name);
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
    if (argc < 2) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    co_log_init();

    if (strcmp(argv[1], "clientrouter") == 0 || strcmp(argv[1], "cloudrouter") == 0) {
        void *mgmt_ctx = zmq_ctx_new();
        void *mgmt_sock = zmq_socket(mgmt_ctx, ZMQ_PULL);
        zmq_bind(mgmt_sock, PIPES_PATH MGMT_PATH);


        struct router_context *ctx = NULL;
        if (strcmp(argv[1], "cloudrouter") == 0)
            router_init(3313, 3323, "*", ROUTER_CLOUD);
        if (strcmp(argv[1], "clientrouter") == 0)
            router_init(3313, 3323, argv[2], ROUTER_CLIENT);

        if (ctx == NULL) {
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }

        while (i_am_running) {
            zmq_msg_t msg;
            int ret = 0;
            struct router_mgmt *msg_data;

            // Forward messages
            router_start(ctx);

            // Check for new processes
            zmq_msg_init(&msg);
            ret = zmq_recvmsg(mgmt_sock, &msg, ZMQ_NOBLOCK);
            if (ret == 0) {
                msg_data = zmq_msg_data(&msg);
                if (msg_data->action == MGMT_CREATE) {
                    lock_and_log("router_mgmt", &ctx->process_list_lock);
                    struct router_process *process = router_process_init(msg_data->pid);
                    g_list_append(ctx->process_list, &process);
                }
            }
        }
        zmq_close(mgmt_sock);
        zmq_ctx_destroy(mgmt_ctx);
        router_cleanup(ctx);
    } else if (strcmp(argv[1], "plug") == 0) {
        pid_t pid = getpid();
        char syscall_path[256];
        char file_path[256];
        sprintf(syscall_path, PIPES_PATH "/%d_syscall", (int)pid);
        sprintf(file_path, PIPES_PATH "/%d_file", (int)pid);

        // Create syscall and file communication channels
        struct co_syscall_context *syscall_ctx = co_syscall_initialize(syscall_path);
        struct co_file_context *file_ctx = co_file_initialize(file_path);

        if (syscall_ctx == NULL || file_ctx == NULL) {
            syslog(LOG_CRIT, "main: context is null. Exiting");
            exit(EXIT_FAILURE);
        }

        // Notify new process
        void *mgmt_ctx = zmq_ctx_new();
        void *mgmt_sock = zmq_socket(mgmt_ctx, ZMQ_PUSH);
        zmq_connect(mgmt_sock, PIPES_PATH MGMT_PATH);

        struct router_mgmt mgmt_notify;
        mgmt_notify.action = MGMT_CREATE;
        mgmt_notify.pid = pid;
        mgmt_notify.signal = 0x00;
        zmq_send(mgmt_sock, (void *)&mgmt_notify, sizeof(struct router_mgmt), 0);

        while (i_am_running) {
            if (co_syscall_deserialize(syscall_ctx) > 0) {
                co_syscall_execute(syscall_ctx);
                co_syscall_serialize(syscall_ctx);
            }
            sleep(10);
            // TODO: files
        }
    } else {
        print_help(argv[0]);
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
