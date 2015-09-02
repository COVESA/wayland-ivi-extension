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
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

/* Log Macro */
#ifndef LOG_ERROR
#define LOG_ERROR(...) {                     \
    fprintf(stderr, "ERROR : " __VA_ARGS__); \
}
#endif

#ifndef LOG_INFO
#define LOG_INFO(...) {                      \
    fprintf(stderr, "INFO  : " __VA_ARGS__); \
}
#endif

#ifndef LOG_WARNING
#define LOG_WARNING(...) {                   \
    fprintf(stderr, "WARN  : " __VA_ARGS__); \
}
#endif

enum
{
    TOUCH_DOWN   = (1 << 0),
    TOUCH_MOTION = (1 << 1),
    TOUCH_UP     = (1 << 2),
    TOUCH_FRAME  = (1 << 3),
    TOUCH_CANCEL = (1 << 4)
};

struct event_log
{
    uint32_t event;
    int32_t  id;
    float    x;
    float    y;
    uint32_t time;
    char     dmy[4];
};

struct event_log_array
{
    size_t            n_log;
    size_t            n_alloc;
    struct event_log *p_logs;
};

void* allocate(uint32_t alloc_size, int clear);
void log_array_init(struct event_log_array *p_array, int n_alloc);
void log_array_release(struct event_log_array *p_array);
void log_array_add(struct event_log_array *p_array, struct event_log *p_log);

#endif /* UTIL_H */
