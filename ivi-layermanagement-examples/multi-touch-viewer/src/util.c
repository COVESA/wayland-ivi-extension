/***************************************************************************
 *
 * Copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void*
allocate(uint32_t alloc_size, int clear)
{
    if (0 == alloc_size)
        return NULL;

    return (0 == clear) ? malloc(alloc_size)
                        : calloc(1, alloc_size);
}

void
log_array_init(struct event_log_array *p_array, int n_alloc)
{
    if (NULL != p_array)
    {
        memset(p_array, 0x00, sizeof *p_array);
        if (0 < n_alloc)
        {
            p_array->p_logs =
                (struct event_log*)malloc(sizeof(struct event_log) * n_alloc);
            if (NULL == p_array->p_logs)
            {
                LOG_ERROR("Memory allocation for logs failed\n");
                exit(-1);
            }
            p_array->n_alloc = n_alloc;
        }
    }
}

void
log_array_release(struct event_log_array *p_array)
{
    if (NULL != p_array)
    { 
        if ((0 < p_array->n_alloc) && (NULL != p_array->p_logs))
        {
            free(p_array->p_logs);
        }
        memset(p_array, 0x00, sizeof *p_array);
    }
}

void
log_array_add(struct event_log_array *p_array, struct event_log *p_log)
{
    if ((NULL != p_array) && (NULL != p_log))
    {
        if ((p_array->n_log + 1) > p_array->n_alloc)
        {
            p_array->n_alloc += 500;
            p_array->p_logs = (struct event_log*)
                realloc(p_array->p_logs,
                        sizeof(struct event_log) * p_array->n_alloc);
            if (NULL == p_array->p_logs)
            {
                LOG_ERROR("Memory allocation for logs failed\n");
                exit(-1);
            }
        }

        p_array->p_logs[p_array->n_log] = *p_log;
        ++(p_array->n_log);
    }
}
