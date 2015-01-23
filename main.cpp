#include <zmq.h>
#include <log.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    syslog(LOG_INFO, "main: initializing server");
    void *zmq_ctx_syscall = zmq_ctx_new();
    void *zmq_ctx_file = zmq_ctx_new();
    void *zmq_sock_syscall = zmq_socket(zmq_ctx_syscall, ZMQ_PAIR);
    void *zmq_sock_syscall = zmq_socket(zmq_ctx_file, ZMQ_PAIR);

    syslog(LOG_INFO, "main: binding connections");
    assert(zmq_bind(zmq_sock_syscall, "tcp://*:3314") == 0);
    assert(zmq_bind(zmq_sock_syscall, "tcp://*:3315") == 0);

}
