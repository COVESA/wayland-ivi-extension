/***************************************************************************
 *
 * Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
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

#include "common_fake_api.h"
#include <string.h>
#include <stdlib.h>

/* Invalid memory address */
#define WL_ARRAY_POISON_PTR (void *) 4

void custom_wl_list_init(struct wl_list *list)
{
    list->prev = list;
    list->next = list;
}

void custom_wl_list_insert(struct wl_list *list, struct wl_list *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next = elm;
    elm->next->prev = elm;
}

void custom_wl_list_remove(struct wl_list *elm)
{
    elm->prev->next = elm->next;
    elm->next->prev = elm->prev;
    elm->next = NULL;
    elm->prev = NULL;
}

int custom_wl_list_empty(const struct wl_list *list)
{
    return list->next == list;
}

void custom_wl_array_init(struct wl_array *array)
{
    memset(array, 0, sizeof *array);
}

void custom_wl_array_release(struct wl_array *array)
{
    free(array->data);
    array->data = WL_ARRAY_POISON_PTR;
}

void *custom_wl_array_add(struct wl_array *array, size_t size)
{
    size_t alloc;
    void *data, *p;

    if (array->alloc > 0)
        alloc = array->alloc;
    else
        alloc = 16;

    while (alloc < array->size + size)
        alloc *= 2;

    if (array->alloc < alloc) {
        if (array->alloc > 0)
            data = realloc(array->data, alloc);
        else
            data = malloc(alloc);

        if (data == NULL)
            return NULL;
        array->data = data;
        array->alloc = alloc;
    }

    p = (char *)array->data + array->size;
    array->size += size;

    return p;
}
