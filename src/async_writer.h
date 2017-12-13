/**
 * Copyright (C) 2017 Cisco Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// @author Changxue Deng <chadeng@cisco.com>

#ifndef __ASYNC_WRITER_H__
#define __ASYNC_WRITER_H__

#include <pthread.h>

#include "db.h"
#include "mb_rc.h"
#include "dict.h"

namespace mabain {

#define MABAIN_ASYNC_TYPE_NONE       0
#define MABAIN_ASYNC_TYPE_ADD        1
#define MABAIN_ASYNC_TYPE_REMOVE     2
#define MABAIN_ASYNC_TYPE_REMOVE_ALL 3
#define MABAIN_ASYNC_TYPE_RC         4

typedef struct _AsyncNode
{
    std::atomic<bool> in_use;
    pthread_mutex_t   mutex;
    pthread_cond_t    cond;

    char *key;
    char *data;
    int key_len;
    int data_len;
    bool overwrite;
    char type;
    int min_index_rc_size;
    int min_data_rc_size;
} AsyncNode;

class AsyncWriter
{
public:

    AsyncWriter(DB *db_ptr);
    ~AsyncWriter();

    // async add/remove by enqueue
    void UpdateNumUsers(int delta);
    int  Add(const char *key, int key_len, const char *data, int data_len, bool overwrite);
    int  Remove(const char *key, int len);
    int  RemoveAll();
    int  CollectResource(int m_index_rc_size, int m_data_rc_size);
    int  StopAsyncThread();

private:
    static void *async_thread_wrapper(void *context);
    AsyncNode* AcquireSlot();
    int PrepareSlot(AsyncNode *node_ptr) const;
    void* async_writer_thread();

    static const int max_num_queue_node;

    // db pointer
    DB *db;
    Dict *dict;
    ResourceCollection *rc_async;

    int num_users;
    AsyncNode *queue;

    // thread id
    pthread_t tid;

    bool stop_processing;
    std::atomic<uint32_t> queue_index;
    uint32_t writer_index;
};

}

#endif
