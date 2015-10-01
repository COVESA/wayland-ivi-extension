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
#ifndef MULTI_TOUCH_TEST_H
#define MULTI_TOUCH_TEST_H

#include "window.h"
#include "util.h"

struct touch_point_params
{
    struct wl_list link;
    int            display;
    int            id;
    float          x, y;
    float          r;
    int32_t        n_vtx;
    GLfloat       *p_vertices;
    GLfloat        color[3];
};

struct touch_event_test_params
{
    struct WaylandDisplay   *p_display;
    struct WaylandEglWindow *p_window;
    struct wl_seat          *p_seat;
    struct wl_touch         *p_touch;
    struct wl_list           touch_point_list;
    struct event_log_array   log_array;
    char                    *p_logfile;
    int                      n_fail;

    struct {
        GLuint pos;
        GLuint loc_col;
        GLuint loc_x;
        GLuint loc_y;
        GLuint loc_w;
        GLuint loc_h;
    } gl;
};

#endif /* MULTI_TOUCH_TEST_H */
