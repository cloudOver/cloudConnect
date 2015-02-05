#include <log.h>

void co_log_init() {
    openlog("/var/log/cloudover", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

void lock_and_log(char *lock_name, pthread_mutex_t *mutex) {
    syslog(LOG_INFO, "lock_and_log: locking mutex: %s", lock_name);
    pthread_mutex_lock(mutex);
    syslog(LOG_DEBUG, "lock_and_log: mutex acquired");
}

void unlock_and_log(char *lock_name, pthread_mutex_t *mutex) {
    syslog(LOG_INFO, "unlock_and_log: unlocking mutex: %s", lock_name);
    pthread_mutex_unlock(mutex);
}
