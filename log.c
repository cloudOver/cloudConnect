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

#include <log.h>

void co_log_init() {
    openlog("/var/log/cloudover", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_LOCAL1);
}

void lock_and_log(char *lock_name, pthread_mutex_t *mutex) {
    //syslog(LOG_INFO, "lock_and_log: locking mutex: %s", lock_name);
    pthread_mutex_lock(mutex);
    //syslog(LOG_DEBUG, "lock_and_log: \tmutex acquired");
}

void unlock_and_log(char *lock_name, pthread_mutex_t *mutex) {
    //syslog(LOG_INFO, "unlock_and_log: unlocking mutex: %s", lock_name);
    pthread_mutex_unlock(mutex);
}
