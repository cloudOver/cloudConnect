#ifndef CO_SYSCALL_H
#define CO_SYSCALL_H

#include <zmq.hpp>
#include <stdlib.h>
#include <unistd.h>

namespace CloudOver {
    class Syscall {
        zmq::socket_t *socket;
        struct SyscallData data;

    public:
        Syscall(zmq::socket_t *sock);
        void execute();
        void serialize();
        void deserialize();
    };
}

#endif // CO_SYSCALL_H
