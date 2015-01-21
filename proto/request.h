#ifndef CO_CMD_H
#define CO_CMD_H

enum CoReqType {
    COREQ_SYSCALL,
    COREQ_FILE,
};

struct CoRequest {
    CoReqType request;
    long long id;
};

#endif // CO_CMD_H
