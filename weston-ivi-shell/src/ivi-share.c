/**************************************************************************
 *
 * Copyright 2015 Codethink Ltd
 * Copyright (C) 2015 Advanced Driver Information Technology Joint Venture GmbH
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

#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "ivi-share.h"
#include "ivi-share-server-protocol.h"

#ifndef container_of
#define container_of(ptr, type, member) ({                              \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

struct ivi_share_nativesurface_client_link
{
    struct wl_resource *resource;
    struct wl_client *client;
    bool firstSendConfigureComp;
    struct wl_list link;                         /* ivi_share_nativesurface link */
    struct ivi_share_nativesurface *parent;
};

struct shell_surface
{
    struct wl_resource *resource;
    struct wl_listener surface_destroy_listener;
    struct weston_surface *surface;
    uint32_t surface_id;
    struct wl_list link;                         /* ivi_shell_share_ext link */
};

struct redirect_target {
    struct wl_client *client;
    struct wl_resource *resource;
    struct wl_resource *target_resource;
    uint32_t id;
    struct wl_list link;
};

struct update_sharing_surface_content {
    struct wl_listener listener;
    struct ivi_shell_share_ext *shell_ext;
};

static void
free_nativesurface(struct ivi_share_nativesurface *nativesurf)
{
    if (NULL == nativesurf) {
        return;
    }

    nativesurf->surface_destroy_listener.notify = NULL;
    wl_list_remove(&nativesurf->surface_destroy_listener.link);
    free(nativesurf);
}

static void
share_surface_destroy(struct wl_client *client,
                      struct wl_resource *resource)
{
    struct ivi_share_nativesurface_client_link *client_link =
        wl_resource_get_user_data(resource);
    if (NULL == client_link) {
        weston_log("share_surface does not have user data\n");
        return;
    }

    wl_resource_destroy(resource);
}

static struct weston_seat *
get_weston_seat(struct weston_compositor *compositor,
                struct ivi_share_nativesurface_client_link *client_link)
{
    struct weston_seat *link = NULL;
    struct wl_client *target_client =
        wl_resource_get_client(client_link->parent->surface->resource);

    wl_list_for_each(link, &compositor->seat_list, link) {
        struct wl_resource *res;
        wl_list_for_each(res, &link->base_resource_list, link) {
            struct wl_client *client = wl_resource_get_client(res);
            if (target_client == client && link->touch_state != NULL) {
                return link;
            }
        }
    }

    return NULL;
}

static uint32_t
get_event_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static void
share_surface_redirect_touch_down(struct wl_client *client,
                                  struct wl_resource *resource,
                                  uint32_t serial,
                                  int32_t id,
                                  wl_fixed_t x,
                                  wl_fixed_t y)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);
    struct weston_seat *seat = NULL;
    struct wl_resource *target_resource = NULL;
    uint32_t time = get_event_time();
    struct redirect_target *redirect_target = NULL;
    struct wl_resource *surface_resource;

    if (client_link->parent == NULL) {
        weston_log("share_surface_redirect_touch_down: No parent surface\n");
        return;
    }
    surface_resource = client_link->parent->surface->resource;

    seat = get_weston_seat(client_link->parent->shell_ext->wc, client_link);
    if (seat == NULL || seat->touch_state == NULL) {
        return;
    }

    wl_list_for_each(target_resource, &seat->touch_state->resource_list, link) {
        if (wl_resource_get_client(target_resource) == wl_resource_get_client(surface_resource)) {
            uint32_t new_serial =
                wl_display_next_serial(client_link->parent->shell_ext->wc->wl_display);
            wl_touch_send_down(target_resource, new_serial, time, surface_resource, id, x, y);
            break;
        }
    }

    redirect_target = malloc(sizeof *redirect_target);
    redirect_target->client = client;
    redirect_target->resource = resource;
    redirect_target->target_resource = target_resource;
    redirect_target->id = id;
    wl_list_insert(&client_link->parent->shell_ext->list_redirect_target, &redirect_target->link);
}

static void
share_surface_redirect_touch_up(struct wl_client *client,
                                struct wl_resource *resource,
                                uint32_t serial,
                                int32_t id)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);
    struct redirect_target *redirect_target = NULL;
    struct redirect_target *next = NULL;
    uint32_t new_serial = wl_display_next_serial(client_link->parent->shell_ext->wc->wl_display);
    uint32_t time = get_event_time();

    wl_list_for_each_safe(redirect_target, next, &client_link->parent->shell_ext->list_redirect_target, link) {
        if (client == redirect_target->client &&
            resource == redirect_target->resource &&
            id == redirect_target->id) {
            wl_touch_send_up(redirect_target->target_resource, new_serial, time, id);
            wl_list_remove(&redirect_target->link);
            free(redirect_target);
            break;
        }
    }
}

static void
share_surface_redirect_touch_motion(struct wl_client *client,
                                    struct wl_resource *resource,
                                    int32_t id,
                                    wl_fixed_t x,
                                    wl_fixed_t y)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);
    struct weston_seat *seat = NULL;
    struct wl_resource *target_resource = NULL;
    uint32_t time = get_event_time();
    struct wl_resource *surface_resource;

    if (client_link->parent == NULL) {
        weston_log("share_surface_redirect_touch_motion: No parent surface\n");
        return;
    }
    surface_resource = client_link->parent->surface->resource;

    seat = get_weston_seat(client_link->parent->shell_ext->wc, client_link);
    if (seat == NULL || seat->touch_state == NULL) {
        return;
    }

    wl_list_for_each(target_resource, &seat->touch_state->resource_list, link) {
        if (wl_resource_get_client(target_resource) == wl_resource_get_client(surface_resource)) {
            wl_touch_send_motion(target_resource, time, id, x, y);
            break;
        }
    }
}

static void
share_surface_redirect_touch_frame(struct wl_client *client,
                                   struct wl_resource *resource)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);
    struct weston_seat *seat = NULL;
    struct wl_resource *target_resource = NULL;
    struct wl_resource *surface_resource = client_link->parent->surface->resource;

    seat = get_weston_seat(client_link->parent->shell_ext->wc, client_link);
    if (seat == NULL || seat->touch_state == NULL) {
        return;
    }

    wl_list_for_each(target_resource, &seat->touch_state->resource_list, link) {
        if (wl_resource_get_client(target_resource) == wl_resource_get_client(surface_resource)) {
            wl_touch_send_frame(target_resource);
            break;
        }
    }
}

static void
share_surface_redirect_touch_cancel(struct wl_client *client,
                                    struct wl_resource *resource)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);
    struct weston_seat *seat = NULL;
    struct wl_resource *target_resource = NULL;
    uint32_t time = get_event_time();
    struct wl_resource *surface_resource = client_link->parent->surface->resource;

    seat = get_weston_seat(client_link->parent->shell_ext->wc, client_link);
    if (seat == NULL || seat->touch_state == NULL) {
        return;
    }

    wl_list_for_each(target_resource, &seat->touch_state->resource_list, link) {
        if (wl_resource_get_client(target_resource) == wl_resource_get_client(surface_resource)) {
            wl_touch_send_cancel(target_resource);
            break;
        }
    }
}

static const
struct ivi_share_surface_interface share_surface_implementation = {
    share_surface_destroy,
    share_surface_redirect_touch_down,
    share_surface_redirect_touch_up,
    share_surface_redirect_touch_motion,
    share_surface_redirect_touch_frame,
    share_surface_redirect_touch_cancel
};

static struct shell_surface *
get_shell_surface_from_weston_surface(struct weston_surface *surface,
                                      struct ivi_shell_share_ext *shell_ext)
{
    struct shell_surface *shsurf;

    wl_list_for_each(shsurf, &shell_ext->list_shell_surface, link) {
        if (shsurf->surface == surface) {
            return shsurf;
        }
    }

    return NULL;
}

static void
send_share_surface_state(struct ivi_share_nativesurface_client_link *client_link,
                         uint32_t share_surface_state);

static void
remove_nativesurface(struct ivi_share_nativesurface *nativesurf)
{
    if (NULL == nativesurf) {
        return;
    }

    struct ivi_share_nativesurface_client_link *p_link = NULL;
    struct ivi_share_nativesurface_client_link *p_next = NULL;
    wl_list_for_each_safe(p_link, p_next, &nativesurf->client_list, link) {
        /* Notify the shared surface has been destroyed */
        send_share_surface_state(p_link,
                                 IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_DESTROYED);

        /* Initialize information about shared surface */
        p_link->parent = NULL;
        p_link->firstSendConfigureComp = false;
        wl_list_remove(&p_link->link);
        wl_list_init(&p_link->link);
    }

    wl_list_remove(&nativesurf->link);
    free_nativesurface(nativesurf);
}

static void
nativesurface_destroy(struct wl_listener *listener, void *data)
{
    struct weston_surface *surface = data;
    struct ivi_share_nativesurface *nativesurf =
        container_of(listener, struct ivi_share_nativesurface,
                     surface_destroy_listener);

    if (nativesurf->surface == surface) {
        remove_nativesurface(nativesurf);
    }
}

struct ivi_share_nativesurface*
alloc_share_nativesurface(struct weston_surface *surface, uint32_t id, uint32_t surface_id,
                          uint32_t bufferType, uint32_t format,
                          struct ivi_shell_share_ext *shell_ext)
{
    struct ivi_share_nativesurface *nativesurface =
        malloc(sizeof(struct ivi_share_nativesurface));
    if (NULL == nativesurface) {
        return NULL;
    }

    nativesurface->id = id;
    nativesurface->surface = surface;
    nativesurface->bufferType = bufferType;
    nativesurface->name = 0;
    nativesurface->width = 0;
    nativesurface->height = 0;
    nativesurface->stride = 0;
    nativesurface->format = format;
    nativesurface->surface_id = surface_id;
    nativesurface->shell_ext = shell_ext;
    wl_list_init(&nativesurface->client_list);

    if (NULL != surface) {
        nativesurface->surface_destroy_listener.notify = nativesurface_destroy;
        wl_signal_add(&surface->destroy_signal, &nativesurface->surface_destroy_listener);
    }

    return nativesurface;
}

static void
destroy_client_link(struct wl_resource *resource)
{
    struct ivi_share_nativesurface_client_link *client_link = wl_resource_get_user_data(resource);

    wl_list_remove(&client_link->link);
    free(client_link);
}

static struct ivi_share_nativesurface_client_link *
add_nativesurface_client(struct ivi_share_nativesurface *nativesurface,
                         uint32_t id, struct wl_client *client, uint32_t version)
{
    struct ivi_share_nativesurface_client_link *link = malloc(sizeof(*link));
    if (NULL == link) {
        return NULL;
    }

    link->resource = wl_resource_create(client, &ivi_share_surface_interface, version, id);
    link->client = client;
    link->firstSendConfigureComp = false;
    link->parent = nativesurface;

    wl_resource_set_implementation(link->resource, &share_surface_implementation,
                                   link, destroy_client_link);
    wl_list_insert(&nativesurface->client_list, &link->link);

    return link;
}

static struct ivi_share_nativesurface_client_link *
create_empty_nativesurface_client(struct wl_client *client,
                                  uint32_t id, uint32_t version,
                                  struct ivi_shell_share_ext *shell_ext)
{
    struct ivi_share_nativesurface *nativesurf;

    nativesurf = alloc_share_nativesurface(NULL, 0, 0,
                                     IVI_SHARE_SURFACE_TYPE_GBM,
                                     IVI_SHARE_SURFACE_FORMAT_ARGB8888,
                                     shell_ext);
    if (NULL == nativesurf) {
        return NULL;
    }

    return add_nativesurface_client(nativesurf, id, client, version);
}

static struct ivi_share_nativesurface*
find_nativesurface(struct shell_surface *shsurf, struct ivi_shell_share_ext *shell_ext)
{
    struct ivi_share_nativesurface *nativesurf = NULL;

    wl_list_for_each(nativesurf, &shell_ext->list_nativesurface, link) {
        if (shsurf->surface_id == nativesurf->surface_id) {
            return nativesurf;
        }
    }
    return NULL;
}

static uint32_t
get_shared_client_input_caps(struct ivi_share_nativesurface_client_link *client_link,
                             struct ivi_shell_share_ext *shell_ext)
{
    uint32_t caps = 0;
    struct weston_seat *seat = get_weston_seat(shell_ext->wc, client_link);
    struct wl_client *creator = wl_resource_get_client(client_link->parent->surface->resource);

    if ((seat != NULL) && (seat->touch_state != NULL)) {
        struct wl_resource *resource = NULL;
        wl_list_for_each(resource, &seat->touch_state->focus_resource_list, link) {
            if (wl_resource_get_client(resource) == creator) {
                caps |= (uint32_t)IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH;
                break;
            }
        }

        wl_list_for_each(resource, &seat->touch_state->resource_list, link) {
            if (wl_resource_get_client(resource) == creator) {
                caps |= (uint32_t)IVI_SHARE_SURFACE_INPUT_CAPS_TOUCH;
                break;
            }
        }
    }

    return caps;
}

static void
send_share_surface_state(struct ivi_share_nativesurface_client_link *client_link,
                         uint32_t share_surface_state)
{
    if (NULL != client_link->resource) {
        ivi_share_surface_send_share_surface_state(client_link->resource,
                                                   share_surface_state);
    }
}

static void
share_get_ivi_share_surface(struct wl_client *client, struct wl_resource *resource,
                            uint32_t id, uint32_t surface_id)
{
    struct ivi_shell_share_ext *shell_ext = wl_resource_get_user_data(resource);
    struct ivi_layout_surface *layout_surface;
    struct weston_surface *surface;
    struct ivi_share_nativesurface *nativesurf;
    struct shell_surface *shsurf;
    struct ivi_share_nativesurface_client_link *client_link = NULL;
    uint32_t caps = 0;
    uint32_t version = wl_resource_get_version(resource);

    layout_surface = shell_ext->controller_interface->get_surface_from_id(surface_id);
    surface = shell_ext->controller_interface->surface_get_weston_surface(layout_surface);

    shsurf = get_shell_surface_from_weston_surface(surface, shell_ext);
    if (shsurf == NULL) {
        client_link = create_empty_nativesurface_client(client, id, version, shell_ext);
        if (NULL == client_link) {
            wl_client_post_no_memory(client);
            return;
        }
        send_share_surface_state(client_link,
                                 IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_NOT_EXIST);
        return;
    }

    if (shsurf->surface_id != surface_id) {
        shsurf->surface_id = surface_id;
    }

    nativesurf = find_nativesurface(shsurf, shell_ext);
    if (nativesurf == NULL) {
        nativesurf = alloc_share_nativesurface(shsurf->surface, id, 0,
                                               (int32_t)IVI_SHARE_SURFACE_TYPE_GBM,
                                               (int32_t)IVI_SHARE_SURFACE_FORMAT_ARGB8888,
                                               shell_ext);

        if (NULL == nativesurf) {
            weston_log("Buffer Sharing Insufficient memory\n");
            wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                                   "Insufficient memory");
            return;
        }

        wl_list_insert(&shell_ext->list_nativesurface, &nativesurf->link);
    }

    client_link = add_nativesurface_client(nativesurf, id, client, version);

    caps = get_shared_client_input_caps(client_link, shell_ext);
    ivi_share_surface_send_input_capabilities(client_link->resource, caps);
}

static struct ivi_share_interface g_share_implementation = {
    share_get_ivi_share_surface
};

static struct ivi_shell_share_ext*
init_ivi_shell_share_ext(struct weston_compositor *wc,
                         const struct ivi_controller_interface *controller_interface,
                         struct ivi_shell_share_ext *shell_ext)
{
    shell_ext = calloc(1, sizeof(*shell_ext));
    if (shell_ext == NULL) {
        weston_log("no memory to allocate ivi_shell_share_ext\n");
        return NULL;
    }

    shell_ext->wc = wc;
    shell_ext->controller_interface = controller_interface;
    wl_list_init(&shell_ext->list_shell_surface);
    wl_list_init(&shell_ext->list_nativesurface);
    wl_list_init(&shell_ext->list_redirect_target);

    return shell_ext;
}

static void
send_configure(struct wl_resource *p_resource, uint32_t id,
               struct ivi_share_nativesurface *p_nativesurface)
{
    if ((NULL == p_resource) || (NULL == p_nativesurface)) {
        return;
    }

    ivi_share_surface_send_configure(p_resource,
                                     p_nativesurface->bufferType,
                                     p_nativesurface->width,
                                     p_nativesurface->height,
                                     p_nativesurface->stride / 4,
                                     p_nativesurface->format);
}

static void
send_damage(struct wl_resource *p_resource, uint32_t id, uint32_t name)
{
    if (!p_resource)
        return;

    ivi_share_surface_send_damage(p_resource, name);
}

static void
bind_share_interface(struct wl_client *p_client, void *p_data,
                     uint32_t version, uint32_t id)
{
    struct ivi_shell_share_ext *shell_ext = p_data;

    weston_log("Buffer Sharing Request bind: version(%d)\n", version);

    if (NULL == p_client) {
        return;
    }

    shell_ext->resource = wl_resource_create(p_client, &ivi_share_interface, 1, id);
    wl_resource_set_implementation(shell_ext->resource,
                                   &g_share_implementation,
                                   shell_ext, NULL);
}

static void
send_to_client(struct ivi_share_nativesurface *p_nativesurface, uint32_t send_flag,
               struct ivi_shell_share_ext *shell_ext)
{
    struct ivi_share_nativesurface_client_link *p_link = NULL;

    if ((IVI_SHAREBUFFER_NOT_AVAILABLE & send_flag) == IVI_SHAREBUFFER_NOT_AVAILABLE) {
        return;
    }

    if (IVI_SHAREBUFFER_INVALID & send_flag) {
        /* Notify the shared surface is invalid */
        wl_list_for_each(p_link, &p_nativesurface->client_list, link) {
            send_share_surface_state(p_link,
                                     IVI_SHARE_SURFACE_SHARE_SURFACE_STATE_INVALID_SURFACE);
        }
        return;
    }

    wl_list_for_each(p_link, &p_nativesurface->client_list, link)
    {
        if ((!p_link->firstSendConfigureComp) ||
            (IVI_SHAREBUFFER_CONFIGURE & send_flag) == IVI_SHAREBUFFER_CONFIGURE) {
            send_configure(p_link->resource, p_nativesurface->id,
                           p_nativesurface);
            p_link->firstSendConfigureComp = true;
        }
        if ((IVI_SHAREBUFFER_DAMAGE & send_flag) == IVI_SHAREBUFFER_DAMAGE) {
            send_damage(p_link->resource, p_nativesurface->id,
                        get_buffer_name(p_nativesurface->surface, shell_ext));
        }
    }
}

static void
send_nativesurface_event(struct wl_listener *listener, void *data)
{
    struct update_sharing_surface_content *surface_content =
        container_of(listener, struct update_sharing_surface_content, listener);
    struct ivi_shell_share_ext *shell_ext = surface_content->shell_ext;
    if (wl_list_empty(&shell_ext->list_nativesurface)) {
        return;
    }

    struct ivi_share_nativesurface *p_nativesurface = NULL;
    struct ivi_share_nativesurface *p_next = NULL;
    wl_list_for_each_safe(p_nativesurface, p_next, &shell_ext->list_nativesurface, link)
    {
        if (NULL == p_nativesurface) {
            continue;
        }

        if (wl_list_empty(&p_nativesurface->client_list) ||
            (0 == p_nativesurface->id)) {
            weston_log("Buffer Sharing warning, Unnecessary nativesurface exists.\n");
            wl_list_remove(&p_nativesurface->link);
            free_nativesurface(p_nativesurface);
            continue;
        }

        p_nativesurface->send_flag = update_buffer_nativesurface(p_nativesurface, shell_ext);
        send_to_client(p_nativesurface, p_nativesurface->send_flag, shell_ext);
    }
}

static int32_t
buffer_sharing_init(struct weston_compositor *wc,
                    struct ivi_shell_share_ext *shell_ext)
{
    if (NULL == wc) {
        weston_log("Can not execute buffer sharing.\n");
        return -1;
    }

    if (NULL == wl_global_create(wc->wl_display, &ivi_share_interface, 1,
                                 shell_ext, bind_share_interface)) {
        weston_log("Buffer Sharing, Failed to global create\n");
        return -1;
    }

    return 0;
}

static void
add_weston_surf_data(struct wl_listener *listener, void *data)
{
    struct weston_surface *ws = data;
    struct ivi_shell_share_ext *shell_ext = NULL;
    struct shell_surface *s = NULL;
    shell_ext = container_of(listener, struct ivi_shell_share_ext, surface_created_listener);

    wl_list_for_each(s, &shell_ext->list_shell_surface, link) {
        if (s->surface == ws) {
            return;
        }
    }

    struct shell_surface *shsurf = NULL;
    shsurf = malloc(sizeof *shsurf);
    if (NULL == shsurf || NULL == ws) {
        return;
    }

    shsurf->resource = ws->resource;
    shsurf->surface = ws;
    wl_list_init(&shsurf->link);
    wl_list_insert(shell_ext->list_shell_surface.prev, &shsurf->link);
}

int32_t
setup_buffer_sharing(struct weston_compositor *wc,
                     const struct ivi_controller_interface *interface)
{
    if (NULL == wc || NULL == interface) {
        weston_log("Can not execute buffer sharing.\n");
        return -1;
    }

    struct ivi_shell_share_ext *shell_ext = NULL;

    shell_ext = init_ivi_shell_share_ext(wc, interface, shell_ext);
    if (shell_ext == NULL) {
        weston_log("ivi_shell_share_ext initialize failed\n");
        return -1;
    }

    int32_t init_ret = buffer_sharing_init(wc, shell_ext);
    if (init_ret < 0) {
        weston_log("Buffer Sharing initialize failed. init_ret = %d\n", init_ret);
        return init_ret;
    }

    struct weston_output *output = NULL;

    wl_list_for_each(output, &wc->output_list, link) {
        struct update_sharing_surface_content *surf_content = malloc(sizeof *surf_content);
        if (surf_content == NULL) {
            weston_log("Failed to allocate surf_content. Buffer Sharing can't use.\n");
            return -1;
        }

        surf_content->shell_ext = shell_ext;
        surf_content->listener.notify = send_nativesurface_event;
        wl_signal_add(&output->frame_signal, &surf_content->listener);
    }

    shell_ext->surface_created_listener.notify = add_weston_surf_data;
    wl_signal_add(&wc->create_surface_signal, &shell_ext->surface_created_listener);
}
