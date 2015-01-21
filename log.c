#include <log.h>

void co_log_init() {
    openlog("cloudover", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}
