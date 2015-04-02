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
void lock_and_log(char *lock_name, pthread_mutex_t *mutex);

/**
 * @brief unlock_and_log - Unlock mutex and log this fact
 * @param log_msg - Message which will be shown in logs, when mutex is unlocked
 * @param mutex - pointer to phread mutex object
 */
void unlock_and_log(char *lock_name, pthread_mutex_t *mutex);
#endif // LOG_H
