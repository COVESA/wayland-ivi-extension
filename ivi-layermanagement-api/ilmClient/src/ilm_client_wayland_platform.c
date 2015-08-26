/**************************************************************************
 *
 * Copyright (C) 2013 DENSO CORPORATION
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
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include "ilm_client.h"
#include "ilm_client_platform.h"
#include "wayland-util.h"
#include "ivi-application-client-protocol.h"

static ilmErrorTypes wayland_surfaceCreate(t_ilm_nativehandle nativehandle,
                         t_ilm_int width, t_ilm_int height,
                         ilmPixelFormat pixelFormat,
                         t_ilm_surface* pSurfaceId);
static ilmErrorTypes wayland_surfaceRemove(const t_ilm_surface surfaceId);
static ilmErrorTypes wayland_init(t_ilm_nativedisplay nativedisplay);
static ilmErrorTypes wayland_destroy(void);

void init_ilmClientPlatformTable(void)
{
    gIlmClientPlatformFunc.surfaceCreate =
        wayland_surfaceCreate;
    gIlmClientPlatformFunc.surfaceRemove =
        wayland_surfaceRemove;
    gIlmClientPlatformFunc.init =
        wayland_init;
    gIlmClientPlatformFunc.destroy =
        wayland_destroy;
}

struct surface_context {
    struct ivi_surface *surface;
    t_ilm_uint id_surface;
    struct ilmSurfaceProperties prop;

    struct wl_list link;
};

struct ilm_client_context {
    int32_t valid;
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct ivi_application *ivi_application;
    struct wl_event_queue *queue;

    struct wl_list list_surface;

    uint32_t internal_id_surface;
    uint32_t name_controller;
};

static void
wayland_client_init(struct ilm_client_context *ctx)
{
    ctx->internal_id_surface = 0;
}

static uint32_t
wayland_client_gen_surface_id(struct ilm_client_context *ctx)
{
    struct surface_context *ctx_surf = NULL;
    do {
        int found = 0;
        if (wl_list_length(&ctx->list_surface) == 0) {
            ctx->internal_id_surface++;
            return ctx->internal_id_surface;
        }
        wl_list_for_each(ctx_surf, &ctx->list_surface, link) {
            if (ctx_surf->id_surface == ctx->internal_id_surface) {
                found = 1;
                break;
            }

            if (found == 0) {
                return ctx->internal_id_surface;
            }
        }
        ctx->internal_id_surface++;
    } while(1);
}

static void
registry_handle_client(void *data, struct wl_registry *registry,
                       uint32_t name, const char *interface,
                       uint32_t version)
{
    struct ilm_client_context *ctx = data;
    (void)version;

    if (strcmp(interface, "ivi_application") == 0) {
        ctx->ivi_application = wl_registry_bind(registry, name,
                                           &ivi_application_interface, 1);
        if (ctx->ivi_application == NULL) {
            fprintf(stderr, "Failed to registry bind ivi_application\n");
            return;
        }
    }
}

static void
registry_handle_client_remove(void *data, struct wl_registry *registry,
	      uint32_t name)
{
}

static const struct wl_registry_listener registry_client_listener = {
    registry_handle_client,
    registry_handle_client_remove
};

static struct ilm_client_context ilm_context = {0};

static void
destroy_client_resouses(void);

static ilmErrorTypes
wayland_destroy(void)
{
    struct ilm_client_context *ctx = &ilm_context;
    if (ctx->valid)
    {
        destroy_client_resouses();
        ctx->valid = 0;

    }

    return ILM_SUCCESS;
}

static void
destroy_client_resouses(void)
{
    struct ilm_client_context *ctx = &ilm_context;

    {
        struct surface_context *c = NULL;
        struct surface_context *n = NULL;
        wl_list_for_each_safe(c, n, &ctx->list_surface, link) {
            wl_list_remove(&c->link);
            ivi_surface_destroy(c->surface);
            free(c);
        }
    }

    if (ctx->ivi_application != NULL) {
        ivi_application_destroy(ctx->ivi_application);
        ctx->ivi_application = NULL;
    }

    if (ctx->queue)
    {
        wl_event_queue_destroy(ctx->queue);
        ctx->queue = NULL;
    }

    if (ctx->registry)
    {
        wl_registry_destroy(ctx->registry);
        ctx->registry = NULL;
    }
}

static ilmErrorTypes
wayland_init(t_ilm_nativedisplay nativedisplay)
{
    struct ilm_client_context *ctx = &ilm_context;
    memset(ctx, 0, sizeof *ctx);

    wayland_client_init(ctx);
    ctx->display = (struct wl_display*)nativedisplay;

    return ILM_SUCCESS;
}

static void
init_client(void)
{
    struct ilm_client_context *ctx = &ilm_context;

    if (ctx->display == NULL) {
	ctx->display = wl_display_connect(NULL);
    }

    wl_list_init(&ctx->list_surface);

    ctx->queue = wl_display_create_queue(ctx->display);
    ctx->registry = wl_display_get_registry(ctx->display);
    if (ctx->registry == NULL) {
        wl_event_queue_destroy(ctx->queue);
        fprintf(stderr, "Failed to get registry\n");
        return;
    }

    wl_proxy_set_queue((void*)ctx->registry, ctx->queue);
    if (wl_registry_add_listener(ctx->registry,
                             &registry_client_listener, ctx)) {
        fprintf(stderr, "Failed to add registry listener\n");
        return;
    }
    wl_display_roundtrip_queue(ctx->display, ctx->queue);

    if ((ctx->display == NULL) || (ctx->ivi_application == NULL)) {
        fprintf(stderr, "Failed to connect display at ilm_client\n");
        return;
    }

    ctx->valid = 1;
}

static struct ilm_client_context*
get_client_instance(void)
{
    struct ilm_client_context *ctx = &ilm_context;
    if (ctx->valid == 0) {
        init_client();
    }

    if (ctx->valid < 0) {
        exit(0);
    }

    wl_display_roundtrip_queue(ctx->display, ctx->queue);

    return ctx;
}

static void
create_client_surface(struct ilm_client_context *ctx,
                      uint32_t id_surface,
                      struct ivi_surface *surface)
{
    struct surface_context *ctx_surf = NULL;

    ctx_surf = calloc(1, sizeof *ctx_surf);
    if (ctx_surf == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface_context\n");
        return;
    }

    ctx_surf->surface = surface;
    ctx_surf->id_surface = id_surface;
    wl_list_insert(&ctx->list_surface, &ctx_surf->link);
}

static ilmErrorTypes
wayland_surfaceCreate(t_ilm_nativehandle nativehandle,
                      t_ilm_int width,
                      t_ilm_int height,
                      ilmPixelFormat pixelFormat,
                      t_ilm_surface* pSurfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_client_context *ctx = get_client_instance();
    uint32_t surfaceid = 0;
    struct ivi_surface *surf = NULL;
    (void)pixelFormat;
    (void)width;
    (void)height;

    if (nativehandle == 0) {
        return returnValue;
    }

    if (pSurfaceId != NULL) {
        if (*pSurfaceId == INVALID_ID) {
            surfaceid =
                wayland_client_gen_surface_id(ctx);
        }
        else {
            surfaceid = *pSurfaceId;
        }

        surf = ivi_application_surface_create(ctx->ivi_application, surfaceid,
                                         (struct wl_surface*)nativehandle);

        if (surf != NULL) {
            create_client_surface(ctx, surfaceid, surf);
            *pSurfaceId = surfaceid;
            returnValue = ILM_SUCCESS;
        }
        else {
            fprintf(stderr, "Failed to create ivi_surface\n");
        }
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceRemove(t_ilm_surface surfaceId)
{
    struct ilm_client_context *ctx = get_client_instance();
    struct surface_context *ctx_surf = NULL;
    struct surface_context *ctx_next = NULL;

    wl_list_for_each_safe(ctx_surf, ctx_next,
                          &ctx->list_surface,
                          link) {
        if (ctx_surf->id_surface == surfaceId) {
            ivi_surface_destroy(ctx_surf->surface);
            wl_list_remove(&ctx_surf->link);
            free(ctx_surf);
            break;
        }
    }

    return ILM_SUCCESS;
}
