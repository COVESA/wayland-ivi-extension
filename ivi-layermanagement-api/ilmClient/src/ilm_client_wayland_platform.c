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

static ilmErrorTypes wayland_getScreenResolution(t_ilm_uint screenID,
                         t_ilm_uint* pWidth, t_ilm_uint* pHeight);
static ilmErrorTypes wayland_surfaceCreate(t_ilm_nativehandle nativehandle,
                         t_ilm_int width, t_ilm_int height,
                         ilmPixelFormat pixelFormat,
                         t_ilm_surface* pSurfaceId);
static ilmErrorTypes wayland_surfaceRemove(const t_ilm_surface surfaceId);
static ilmErrorTypes wayland_surfaceRemoveNativeContent(
                         t_ilm_surface surfaceId);
static ilmErrorTypes wayland_surfaceSetNativeContent(
                         t_ilm_nativehandle nativehandle,
                         t_ilm_int width, t_ilm_int height,
                         ilmPixelFormat pixelFormat,
                         t_ilm_surface surfaceId);
static ilmErrorTypes wayland_UpdateInputEventAcceptanceOn(
                         t_ilm_surface surfaceId,
                         ilmInputDevice devices,
                         t_ilm_bool acceptance);
static ilmErrorTypes wayland_init(t_ilm_nativedisplay nativedisplay);
static void wayland_destroy(void);
static ilmErrorTypes wayland_surfaceInitialize(t_ilm_surface *pSurfaceId);

void init_ilmClientPlatformTable(void)
{
    gIlmClientPlatformFunc.getScreenResolution =
        wayland_getScreenResolution;
    gIlmClientPlatformFunc.surfaceCreate =
        wayland_surfaceCreate;
    gIlmClientPlatformFunc.surfaceRemove =
        wayland_surfaceRemove;
    gIlmClientPlatformFunc.surfaceRemoveNativeContent =
        wayland_surfaceRemoveNativeContent;
    gIlmClientPlatformFunc.surfaceSetNativeContent =
        wayland_surfaceSetNativeContent;
    gIlmClientPlatformFunc.UpdateInputEventAcceptanceOn =
        wayland_UpdateInputEventAcceptanceOn;
    gIlmClientPlatformFunc.init =
        wayland_init;
    gIlmClientPlatformFunc.destroy =
        wayland_destroy;
    gIlmClientPlatformFunc.surfaceInitialize =
        wayland_surfaceInitialize;
}

struct surface_context {
    struct ivi_surface *surface;
    t_ilm_uint id_surface;
    struct ilmSurfaceProperties prop;

    struct wl_list link;
};

struct screen_context {
    struct wl_output *output;
    t_ilm_uint id_screen;

    uint32_t width;
    uint32_t height;

    struct wl_list link;
};

struct ilm_client_context {
    int32_t valid;
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct ivi_application *ivi_application;

    int32_t num_screen;
    struct wl_list list_surface;
    struct wl_list list_screen;

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
output_listener_geometry(void *data,
                         struct wl_output *wl_output,
                         int32_t x,
                         int32_t y,
                         int32_t physical_width,
                         int32_t physical_height,
                         int32_t subpixel,
                         const char *make,
                         const char *model,
                         int32_t transform)
{
    struct screen_context *ctx_scrn = data;
    (void)wl_output;
    (void)x;
    (void)y;
    (void)subpixel;
    (void)make;
    (void)model;
    (void)transform;

    ctx_scrn->width = physical_width;
    ctx_scrn->height = physical_height;
}

static void
output_listener_mode(void *data,
                     struct wl_output *wl_output,
                     uint32_t flags,
                     int32_t width,
                     int32_t height,
                     int32_t refresh)
{
    (void)data;
    (void)wl_output;
    (void)flags;
    (void)width;
    (void)height;
    (void)refresh;
}

static void
output_listener_done(void *data,
                     struct wl_output *output)
{
    (void)data;
    (void)output;
}

static void
output_listener_scale(void *data,
                      struct wl_output *output,
                      int32_t factor)
{
    (void)data;
    (void)output;
    (void)factor;
}

static struct
wl_output_listener output_listener = {
    output_listener_geometry,
    output_listener_mode,
    output_listener_done,
    output_listener_scale
};

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
    } else if (strcmp(interface, "wl_output") == 0) {
        struct screen_context *ctx_scrn = calloc(1, sizeof *ctx_scrn);
        if (ctx_scrn == NULL) {
            fprintf(stderr, "Failed to allocate memory for screen_context\n");
            return;
        }
        wl_list_init(&ctx_scrn->link);
        ctx_scrn->output = wl_registry_bind(registry, name,
                                           &wl_output_interface, 1);
        if (ctx_scrn->output == NULL) {
            free(ctx_scrn);
            fprintf(stderr, "Failed to registry bind wl_output\n");
            return;
        }

        if (wl_output_add_listener(ctx_scrn->output,
                                   &output_listener,
                                   ctx_scrn)) {
            free(ctx_scrn);
            fprintf(stderr, "Failed to add wl_output listener\n");
            return;
        }

        ctx_scrn->id_screen = ctx->num_screen;
        ctx->num_screen++;
        wl_list_insert(&ctx->list_screen, &ctx_scrn->link);
    }
}

static const struct wl_registry_listener registry_client_listener = {
    registry_handle_client,
    NULL
};

static struct ilm_client_context ilm_context = {0};

static void
wayland_destroy(void)
{
    struct ilm_client_context *ctx = &ilm_context;
    ctx->valid = 0;
}

static void
destroy_client_resouses(void)
{
    struct ilm_client_context *ctx = &ilm_context;
    struct screen_context *ctx_scrn = NULL;
    struct screen_context *next = NULL;
    wl_list_for_each_safe(ctx_scrn, next, &ctx->list_screen, link) {
        if (ctx_scrn->output != NULL) {
            wl_list_remove(&ctx_scrn->link);
            wl_output_destroy(ctx_scrn->output);
            free(ctx_scrn);
        }
    }
    if (ctx->ivi_application != NULL) {
        ivi_application_destroy(ctx->ivi_application);
        ctx->ivi_application = NULL;
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

    ctx->num_screen = 0;

    wl_list_init(&ctx->list_screen);
    wl_list_init(&ctx->list_surface);

    ctx->registry = wl_display_get_registry(ctx->display);
    if (ctx->registry == NULL) {
        fprintf(stderr, "Failed to get registry\n");
        return;
    }
    if (wl_registry_add_listener(ctx->registry,
                             &registry_client_listener, ctx)) {
        fprintf(stderr, "Failed to add registry listener\n");
        return;
    }
    wl_display_dispatch(ctx->display);
    wl_display_roundtrip(ctx->display);

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

    wl_display_roundtrip(ctx->display);

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

static struct surface_context*
get_surface_context_by_id(struct ilm_client_context *ctx,
                          uint32_t id_surface)
{
    struct surface_context *ctx_surf = NULL;

    wl_list_for_each(ctx_surf, &ctx->list_surface, link) {
        if (ctx_surf->id_surface == id_surface) {
            return ctx_surf;
        }
    }

    return NULL;
}

static ilmErrorTypes
wayland_getScreenResolution(t_ilm_uint screenID,
                            t_ilm_uint* pWidth,
                            t_ilm_uint* pHeight)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_client_context *ctx = get_client_instance();

    if ((pWidth != NULL) && (pHeight != NULL))
    {
        struct screen_context *ctx_scrn;
        wl_list_for_each(ctx_scrn, &ctx->list_screen, link) {
            if (screenID == ctx_scrn->id_screen) {
                *pWidth = ctx_scrn->width;
                *pHeight = ctx_scrn->height;
                returnValue = ILM_SUCCESS;
                break;
            }
        }
    }

    return returnValue;
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

static ilmErrorTypes
wayland_surfaceRemoveNativeContent(t_ilm_surface surfaceId)
{
    (void)surfaceId;

    /* There is no API to set native content
        as such ivi_surface_set_native. */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_surfaceSetNativeContent(t_ilm_nativehandle nativehandle,
                                t_ilm_int width,
                                t_ilm_int height,
                                ilmPixelFormat pixelFormat,
                                t_ilm_surface surfaceId)
{
    (void)nativehandle;
    (void)width;
    (void)height;
    (void)pixelFormat;
    (void)surfaceId;

    /* There is no API to set native content
        as such ivi_surface_set_native. */
    return ILM_FAILED;
}

static ilmErrorTypes
wayland_UpdateInputEventAcceptanceOn(t_ilm_surface surfaceId,
                                     ilmInputDevice devices,
                                     t_ilm_bool acceptance)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_client_context *ctx = get_client_instance();
    struct surface_context *ctx_surf = NULL;

    ctx_surf = get_surface_context_by_id(ctx, (uint32_t)surfaceId);
    if (ctx_surf != NULL) {
        if (acceptance == ILM_TRUE) {
            ctx_surf->prop.inputDevicesAcceptance = devices;
        } else {
            ctx_surf->prop.inputDevicesAcceptance &= ~devices;
        }
        returnValue = ILM_SUCCESS;
    }

    return returnValue;
}

static ilmErrorTypes
wayland_surfaceInitialize(t_ilm_surface *pSurfaceId)
{
    ilmErrorTypes returnValue = ILM_FAILED;

    returnValue = wayland_surfaceCreate((t_ilm_nativehandle)NULL,
                                        100, 100, (ilmPixelFormat)NULL,
                                        (t_ilm_surface*)pSurfaceId);
    return returnValue;
}
