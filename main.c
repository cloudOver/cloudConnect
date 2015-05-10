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

#include <zmq.h>
#include <log.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <proto/syscall.h>
#include <proto/file.h>
#include <router.h>
#include <forwarder.h>

#define MGMT_PATH "/co_mgmt"

int i_am_running = 1;

void print_help(char *prog_name) {
    fprintf(stderr, "Usage: %s [plug|router|forwarder]\n", prog_name);
    fprintf(stderr, "    plug [pid - imitate process exported to cloud. The pid is\n");
    fprintf(stderr, "                cloud process'' pid\n");
    fprintf(stderr, "    router    - collect all messages from plug processes and\n");
    fprintf(stderr, "                pass it to/from cloud\n");
    fprintf(stderr, "    forwarder [router url] [cloud dev] - forward messages from\n");
    fprintf(stderr, "                router to the kernel space (kernelConnect module)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "router url should have ZMQ compatibile format, e.g.:\n");
    fprintf(stderr, "tcp://10.20.30.40:1234    (NO trailing slash!)\n");
}

void handle_router_sigterm(int signum) {
    i_am_running = 0;
    fprintf(stderr, "Sig %d!", signum);
}

void handle_plug_sigterm(int signum) {
    i_am_running = 0;
    fprintf(stderr, "Sig %d!", signum);
}


/**
 * @brief router_prepare Prepare everything to start router
 */
void router_prepare() {
    DIR *dir = opendir(PIPES_DIR);
    if (dir)
        closedir(dir);
    else
        mkdir(PIPES_DIR, 0700);
    perror("router_prepare");
}


/**
 * @brief router start routing process
 */
static void router() {
    int ret;
    void *mgmt_ctx = zmq_ctx_new();
    void *mgmt_sock = zmq_socket(mgmt_ctx, ZMQ_PAIR);
    zmq_bind(mgmt_sock, PIPES_PATH MGMT_PATH);


    struct router_context *router_file = NULL;
    struct router_context *router_syscall = NULL;
    syslog(LOG_INFO, "main: initializing client routers (file and syscalls)");
    router_file = router_init(3313, "*");
    router_syscall = router_init(3323, "*");

    if (router_file == NULL) {
        syslog(LOG_CRIT, "main: failed to initialize file router");
        exit(EXIT_FAILURE);
    }

    if (router_syscall == NULL) {
        syslog(LOG_CRIT, "main: failed to initialize syscall router");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "main: starting router");

    while (i_am_running) {
        zmq_msg_t msg;
        struct router_mgmt *msg_data;

        // Forward messages
        router_start(router_file);
        router_start(router_syscall);

        // Check for new processes
        zmq_msg_init(&msg);
        ret = zmq_recvmsg(mgmt_sock, &msg, ZMQ_NOBLOCK);
        if (ret > 0) {
            msg_data = zmq_msg_data(&msg);
            if (msg_data->action == MGMT_CREATE) {
                syslog(LOG_INFO, "main: connecting new process: %u", msg_data->pid);

                lock_and_log("router_mgmt", &router_syscall->process_list_lock);
                struct router_process *process_syscall = router_process_init(msg_data->pid, "syscall");
                router_syscall->process_list = g_list_append(router_syscall->process_list, process_syscall);
                unlock_and_log("router_mgmt", &router_syscall->process_list_lock);

                lock_and_log("router_mgmt", &router_file->process_list_lock);
                struct router_process *process_file = router_process_init(msg_data->pid, "file");
                router_file->process_list = g_list_append(router_file->process_list, process_file);
                unlock_and_log("router_mgmt", &router_file->process_list_lock);
            } else {
                syslog(LOG_INFO, "main: unknown action from mgmt socket");
            }
        } else {
            syslog(LOG_DEBUG, "main: no mgmt messages, %d", ret);
        }

        zmq_msg_close(&msg);
        sleep(5);
    }
    zmq_close(mgmt_sock);
    zmq_ctx_destroy(mgmt_ctx);
    router_cleanup(router_syscall);
    router_cleanup(router_file);
}


/**
 * @brief plug Start plug process as substitute of real process
 */
void plug(char *pid_str) {
    int ret;
    int pid;
    char syscall_path[256];
    char file_path[256];
    struct co_syscall_context *syscall_ctx;
    struct co_file_context *file_ctx;
    void *mgmt_ctx;
    void *mgmt_sock;
    struct router_mgmt mgmt_notify;

    pid = atoi(pid_str);

    sprintf(syscall_path, PIPES_PATH "/%d_syscall", (int)pid);
    sprintf(file_path, PIPES_PATH "/%d_file", (int)pid);

    // Create syscall and file communication channels
    syscall_ctx = co_syscall_initialize(syscall_path);
    file_ctx = co_file_initialize(file_path);

    if (syscall_ctx == NULL || file_ctx == NULL) {
        syslog(LOG_CRIT, "main: context is null. Exiting");
        exit(EXIT_FAILURE);
    }

    // Notify new process
    mgmt_ctx = zmq_ctx_new();
    mgmt_sock = zmq_socket(mgmt_ctx, ZMQ_PAIR);
    ret = zmq_connect(mgmt_sock, PIPES_PATH MGMT_PATH);
    if (ret != 0) {
        syslog(LOG_CRIT, "main: cannot connect to mgmt socket");
        exit(EXIT_FAILURE);
    }

    mgmt_notify.action = MGMT_CREATE;
    mgmt_notify.pid = pid;
    mgmt_notify.signal = 0x00;
    ret = zmq_send(mgmt_sock, (void *)&mgmt_notify, sizeof(struct router_mgmt), 0);
    syslog(LOG_DEBUG, "main: send (new process notification) returned %d", ret);

    while (i_am_running) {
        if (co_syscall_deserialize(syscall_ctx) > 0) {
            co_syscall_execute(syscall_ctx);
            co_syscall_serialize(syscall_ctx);
        }
        sleep(5);
        // TODO: files
    }
}


void forwarder_prepare(char *dev_path) {
    int fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        syslog(LOG_INFO, "forwarder_prepare: creating special device");
        mknod(dev_path, S_IWUSR | S_IRUSR | S_IFCHR, makedev(109, 0));
    } else {
        close(fd);
    }
}


/**
 * @brief forwarder Start kernel<->router forwarder
 * @param router_url Address of zmq socket in router
 * @param clouddev Cloud device in cloud VM
 */
void forwarder(char *router_url, char *clouddev) {
    struct co_forward_context *ctx;

    if (access(clouddev, R_OK | W_OK) < 0) {
        perror("access to cloud dev");
        exit(1);
    }
    ctx = co_forward_init(router_url, clouddev);

    while (i_am_running) {
        co_forward(ctx);
        sleep(5);
    }

    co_forward_cleanup(ctx);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    co_log_init();

    if (argc == 2 && strcmp(argv[1], "router") == 0) {
        router_prepare();
        router();
    } else if (argc == 3 && strcmp(argv[1], "plug") == 0) {
        plug(argv[2]);
    } else if (argc == 4 && strcmp(argv[1], "forwarder") == 0) {
        forwarder_prepare(argv[3]);
        forwarder(argv[2], argv[3]);
    } else {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
}
