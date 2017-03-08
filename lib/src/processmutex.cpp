/*
 * Copyright (C) 2017 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Matt Vogt <matthew.vogt@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "sailfishkeyprovider_processmutex.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>

namespace {

// Defined as required for ::semun
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo  *__buf;
};

void semaphoreError(const char *msg, const char *id, int error)
{
    fprintf(stderr, "semaphore error: %s %s: %s %d\n", msg, id, ::strerror(error), error);
}

int semaphoreInit(const char *id, size_t count, const int *initialValues)
{
    int rv = -1;

    // It doesn't matter what proj_id we use, there are no other ftok uses on this ID
    key_t key = ::ftok(id, 2);

    rv = ::semget(key, count, 0);
    if (rv == -1) {
        if (errno != ENOENT) {
            semaphoreError("Unable to get semaphore", id, errno);
        } else {
            // The semaphore does not currently exist
            rv = ::semget(key, count, IPC_CREAT | IPC_EXCL | S_IRWXO | S_IRWXG | S_IRWXU);
            if (rv == -1) {
                if (errno == EEXIST) {
                    // Someone else won the race to create the semaphore - retry get
                    rv = ::semget(key, count, 0);
                }

                if (rv == -1) {
                    semaphoreError("Unable to create semaphore", id, errno);
                }
            } else {
                // Set the initial value
                for (size_t i = 0; i < count; ++i) {
                    union semun arg = { 0 };
                    arg.val = *initialValues++;

                    int status = ::semctl(rv, static_cast<int>(i), SETVAL, arg);
                    if (status == -1) {
                        rv = -1;
                        semaphoreError("Unable to initialize semaphore", id, errno);
                    }
                }
            }
        }
    }

    return rv;
}

bool semaphoreIncrement(int id, size_t index, bool wait, size_t ms, int value)
{
    if (id == -1) {
        errno = 0;
        return false;
    }

    struct sembuf op;
    op.sem_num = index;
    op.sem_op = value;
    op.sem_flg = SEM_UNDO;
    if (!wait) {
        op.sem_flg |= IPC_NOWAIT;
    }

    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = ms * 1000;

    do {
        int rv = ::semtimedop(id, &op, 1, (wait && ms > 0 ? &timeout : 0));
        if (rv == 0)
            return true;
    } while (errno == EINTR);

    return false;
}

static const int initialSemaphoreValues[] = { 1, 0, 1 };
static size_t fileOwnershipIndex = 0;
static size_t fileReadersIndex = 1;
static size_t writeAccessIndex = 2;

}

Sailfish::KeyProvider::Semaphore::Semaphore(const char *id, int initial)
    : m_identifier(0)
    , m_id(-1)
{
    m_identifier = strdup(id);
    m_id = semaphoreInit(m_identifier, 1, &initial);
}

Sailfish::KeyProvider::Semaphore::Semaphore(const char *id, size_t count, const int *initialValues)
    : m_identifier(0)
    , m_id(-1)
{
    m_identifier = strdup(id);
    m_id = semaphoreInit(m_identifier, count, initialValues);
}

Sailfish::KeyProvider::Semaphore::~Semaphore()
{
    free(m_identifier);
}

bool Sailfish::KeyProvider::Semaphore::isValid() const
{
    return (m_id != -1);
}

bool Sailfish::KeyProvider::Semaphore::decrement(size_t index, bool wait, size_t timeoutMs)
{
    if (!semaphoreIncrement(m_id, index, wait, timeoutMs, -1)) {
        if (errno != EAGAIN || wait) {
            error("Unable to decrement semaphore", errno);
        }
        return false;
    }
    return true;
}

bool Sailfish::KeyProvider::Semaphore::increment(size_t index, bool wait, size_t timeoutMs)
{
    if (!semaphoreIncrement(m_id, index, wait, timeoutMs, 1)) {
        if (errno != EAGAIN || wait) {
            error("Unable to increment semaphore", errno);
        }
        return false;
    }
    return true;
}

int Sailfish::KeyProvider::Semaphore::value(size_t index) const
{
    if (m_id == -1)
        return -1;

    return ::semctl(m_id, index, GETVAL, 0);
}

void Sailfish::KeyProvider::Semaphore::error(const char *msg, int error)
{
    semaphoreError(msg, m_identifier, error);
}

// Adapted from the inter-process mutex in QMF
// The first user creates the semaphore that all subsequent instances
// attach to.  We rely on undo semantics to release locked semaphores
// on process failure.
Sailfish::KeyProvider::ProcessMutex::ProcessMutex(const char *path)
    : m_semaphore(path, 3, initialSemaphoreValues)
    , m_initialProcess(false)
{
    if (!m_semaphore.isValid()) {
        semaphoreError("ProcessMutex: Unable to create semaphore array!", path, EAGAIN);
    } else {
        if (!m_semaphore.decrement(fileOwnershipIndex)) {
            semaphoreError("ProcessMutex: Unable to determine file semaphore ownership!", path, EAGAIN);
        } else {
            // Only the first process to connect to the semaphore is the owner
            m_initialProcess = (m_semaphore.value(fileReadersIndex) == 0);
            if (!m_semaphore.increment(fileReadersIndex)) {
                semaphoreError("ProcessMutex: Unable to increment file readers semaphore!", path, EAGAIN);
            }
            m_semaphore.increment(fileOwnershipIndex);
        }
    }
}

bool Sailfish::KeyProvider::ProcessMutex::lock()
{
    return m_semaphore.decrement(writeAccessIndex);
}

bool Sailfish::KeyProvider::ProcessMutex::unlock()
{
    return m_semaphore.increment(writeAccessIndex);
}

bool Sailfish::KeyProvider::ProcessMutex::isLocked() const
{
    return (m_semaphore.value(writeAccessIndex) == 0);
}

bool Sailfish::KeyProvider::ProcessMutex::isInitialProcess() const
{
    return m_initialProcess;
}
