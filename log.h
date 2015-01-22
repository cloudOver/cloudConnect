#ifndef LOG_H
#define LOG_H

#include <syslog.h>
#include <pthread.h>

/**
 * @brief co_log_init - Initialize logging subsystem (via Syslog)
 */
void co_log_init();

/**
 * @brief lock_and_log - Try to lock mutex and log this fact (before qcquiring and after)
 * @param log_msg - Message which will be shown in logs, when mutex is locked
 * @param mutex - pointer to phread mutex object
 */
void lock_and_log(char *log_msg, pthread_mutex_t *mutex);

/**
 * @brief unlock_and_log - Unlock mutex and log this fact
 * @param log_msg - Message which will be shown in logs, when mutex is unlocked
 * @param mutex - pointer to phread mutex object
 */
void unlock_and_log(char *log_msg, pthread_mutex_t *mutex);
#endif // LOG_H
