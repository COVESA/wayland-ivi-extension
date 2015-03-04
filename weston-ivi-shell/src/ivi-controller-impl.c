/*
 * Copyright (C) 2013 DENSO CORPORATION
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * This is implementation of ivi-controller.xml. This implementation uses
 * ivi-extension APIs, which uses ivi_controller_interface pvoided by
 * ivi-layout.c in weston.
 */

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <cairo.h>
#include <GLES2/gl2.h>

#include "weston/compositor.h"
#include "ivi-controller-server-protocol.h"
#include "bitmap.h"

#include "ivi-layout-export.h"
#include "ivi-extension.h"

struct ivilayer;
struct iviscreen;

struct link_layer {
    struct ivilayer *layer;
    struct wl_list link;
};

struct link_screen {
    struct iviscreen *screen;
    struct wl_list link;
};

struct ivisurface {
    struct wl_list link;
    struct wl_client *client;
    struct ivishell *shell;
    uint32_t update_count;
    struct ivi_layout_surface *layout_surface;
    struct wl_listener surface_destroy_listener;
    struct wl_list list_layer;
    uint32_t controller_surface_count;
    int can_be_removed;
};

struct ivilayer {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_layer *layout_layer;
    struct wl_list list_screen;
    uint32_t controller_layer_count;
    int layer_canbe_removed;
};

struct iviscreen {
    struct wl_list link;
    struct ivishell *shell;
    struct ivi_layout_screen *layout_screen;
    struct weston_output *output;
};

struct ivicontroller_surface {
    struct wl_resource *resource;
    uint32_t id;
    uint32_t id_surface;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
    int implementation_set;
};

struct ivicontroller_layer {
    struct wl_resource *resource;
    uint32_t id;
    uint32_t id_layer;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
};

struct ivicontroller_screen {
    struct wl_resource *resource;
    uint32_t id;
    uint32_t id_screen;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
};

struct ivicontroller {
    struct wl_resource *resource;
    uint32_t id;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
};

struct link_shell_weston_surface
{
    struct wl_resource *resource;
    struct wl_listener destroy_listener;
    struct weston_surface *surface;
    struct wl_list link;
};

static void surface_event_remove(struct ivi_layout_surface *, void *);

static void
destroy_ivicontroller_surface(struct wl_resource *resource)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivisurf->shell;
    struct ivicontroller_surface *ctrlsurf = NULL;
    struct ivicontroller_surface *next = NULL;
    int is_removed = 0;

    wl_list_for_each_safe(ctrlsurf, next,
                          &shell->list_controller_surface, link) {

        if (resource != ctrlsurf->resource) {
            continue;
        }

        if (!wl_list_empty(&ctrlsurf->link)) {
            wl_list_remove(&ctrlsurf->link);
        }

        is_removed = 1;
        free(ctrlsurf);
        ctrlsurf = NULL;
        --ivisurf->controller_surface_count;
        break;
    }

    if ((is_removed) && (ivisurf->controller_surface_count == 0)) {
        if (ivisurf->can_be_removed) {
            free(ivisurf);
        }
    }
}

static void
destroy_ivicontroller_layer(struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivilayer->shell;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivicontroller_layer *next = NULL;
    uint32_t id_layer = 0;

    id_layer = ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);

    wl_list_for_each_safe(ctrllayer, next,
                          &shell->list_controller_layer, link) {

        if (resource != ctrllayer->resource) {
            continue;
        }

        wl_list_remove(&ctrllayer->link);
        --ivilayer->controller_layer_count;
        ivi_controller_layer_send_destroyed(ctrllayer->resource);
        free(ctrllayer);
        ctrllayer = NULL;
        break;
    }

    if ((ivilayer->layout_layer != NULL) &&
        (ivilayer->controller_layer_count == 0) &&
        (ivilayer->layer_canbe_removed == 1)) {
        ivi_extension_layer_remove(shell, ivilayer->layout_layer);
    }
}

static void
destroy_ivicontroller_screen(struct wl_resource *resource)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivicontroller_screen *ctrlscrn = NULL;
    struct ivicontroller_screen *next = NULL;

    wl_list_for_each_safe(ctrlscrn, next,
                          &iviscrn->shell->list_controller_screen, link) {
// TODO : Only Single display
#if 0
        if (iviscrn->output->id != ctrlscrn->id_screen) {
            continue;
        }
#endif

        if (resource != ctrlscrn->resource) {
            continue;
        }

        wl_list_remove(&ctrlscrn->link);
        free(ctrlscrn);
        ctrlscrn = NULL;
        break;
    }
}

static void
unbind_resource_controller(struct wl_resource *resource)
{
    struct ivicontroller *controller = wl_resource_get_user_data(resource);

    wl_list_remove(&controller->link);

    free(controller);
    controller = NULL;
}

static struct ivisurface*
get_surface(struct wl_list *list_surf, uint32_t id_surface)
{
    struct ivisurface *ivisurf = NULL;
    uint32_t ivisurf_id = 0;

    wl_list_for_each(ivisurf, list_surf, link) {
        ivisurf_id = ivi_extension_get_id_of_surface(ivisurf->shell, ivisurf->layout_surface);
        if (ivisurf_id == id_surface) {
            return ivisurf;
        }
    }

    return NULL;
}

static struct ivilayer*
get_layer(struct wl_list *list_layer, uint32_t id_layer)
{
    struct ivilayer *ivilayer = NULL;
    uint32_t ivilayer_id = 0;

    wl_list_for_each(ivilayer, list_layer, link) {
        ivilayer_id = ivi_extension_get_id_of_layer(ivilayer->shell, ivilayer->layout_layer);
        if (ivilayer_id == id_layer) {
            return ivilayer;
        }
    }

    return NULL;
}

static const
struct ivi_controller_screen_interface controller_screen_implementation;

static struct ivicontroller_screen*
controller_screen_create(struct ivishell *shell,
                         struct wl_client *client,
                         struct iviscreen *iviscrn)
{
    struct ivicontroller_screen *ctrlscrn = NULL;

    ctrlscrn = calloc(1, sizeof *ctrlscrn);
    if (ctrlscrn == NULL) {
        weston_log("no memory to allocate controller screen\n");
        return NULL;
    }

    ctrlscrn->client = client;
    ctrlscrn->shell  = shell;
// FIXME
// TODO : Only Single display
#if 0
    /* ctrlscrn->id_screen = iviscrn->id_screen; */
#else
    ctrlscrn->id_screen = 0;
#endif

    ctrlscrn->resource =
        wl_resource_create(client, &ivi_controller_screen_interface, 1, 0);
    if (ctrlscrn->resource == NULL) {
        weston_log("couldn't new screen controller object");

        free(ctrlscrn);
        ctrlscrn = NULL;

        return NULL;
    }

    wl_resource_set_implementation(ctrlscrn->resource,
                                   &controller_screen_implementation,
                                   iviscrn, destroy_ivicontroller_screen);

    wl_list_init(&ctrlscrn->link);
    wl_list_insert(&shell->list_controller_screen, &ctrlscrn->link);

    return ctrlscrn;
}

static void
send_surface_add_event(struct ivisurface *ivisurf,
                       struct wl_resource *resource,
                       enum ivi_layout_notification_mask mask)
{
    struct ivi_layout_layer **pArray = NULL;
    uint32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct link_layer *link_layer = NULL;
    struct link_layer *next = NULL;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivilayer *ivilayer = NULL;
    struct ivishell *shell = ivisurf->shell;
    uint32_t id_layout_layer = 0;
    struct wl_client *surface_client = wl_resource_get_client(resource);
    int found = 0;

    ans = ivi_extension_get_layers_under_surface(shell, ivisurf->layout_surface,
                                                  &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get layers at send_surface_add_event\n");
        return;
    }

    /* Send Null to cancel added surface */
    if (mask & IVI_NOTIFICATION_REMOVE) {
        wl_list_for_each_safe(link_layer, next, &ivisurf->list_layer, link) {
            ivi_controller_surface_send_layer(resource, NULL);
        }
    }
    else if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; i++) {
            /* Send new surface event */
            ivilayer = NULL;
            wl_list_for_each(ivilayer, &shell->list_layer, link) {
                if (ivilayer->layout_layer == pArray[i]) {
                    break;
                }
            }

            if (ivilayer == NULL) {
                continue;
            }

            id_layout_layer =
                ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);
            wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
                if (id_layout_layer != ctrllayer->id_layer) {
                    continue;
                }

                struct wl_client *layer_client = wl_resource_get_client(ctrllayer->resource);
                if (surface_client != layer_client) {
                    continue;
                }

                ivi_controller_surface_send_layer(resource, ctrllayer->resource);
            }
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_surface_event(struct wl_resource *resource,
                   struct ivisurface *ivisurf,
                   const struct ivi_layout_surface_properties *prop,
                   uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_controller_surface_send_opacity(resource,
                                            prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_controller_surface_send_source_rectangle(resource,
            prop->source_x, prop->source_y,
            prop->source_width, prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_controller_surface_send_destination_rectangle(resource,
            prop->dest_x, prop->dest_y,
            prop->dest_width, prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_ORIENTATION) {
        ivi_controller_surface_send_orientation(resource,
                                                prop->orientation);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_controller_surface_send_visibility(resource,
                                               prop->visibility);
    }
    if (mask & IVI_NOTIFICATION_REMOVE) {
        send_surface_add_event(ivisurf, resource, IVI_NOTIFICATION_REMOVE);
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        send_surface_add_event(ivisurf, resource, IVI_NOTIFICATION_ADD);
    }
}

static void
update_surface_prop(struct ivisurface *ivisurf,
                    uint32_t mask)
{
    struct ivi_layout_layer **pArray = NULL;
    uint32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct ivishell *shell = ivisurf->shell;

    ans = ivi_extension_get_layers_under_surface(shell, ivisurf->layout_surface,
                                                  &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get layers at send_surface_add_event\n");
        return;
    }

    if (mask & IVI_NOTIFICATION_REMOVE) {
        struct link_layer *link_layer = NULL;
        struct link_layer *next = NULL;

        wl_list_for_each_safe(link_layer, next, &ivisurf->list_layer, link) {
            wl_list_remove(&link_layer->link);
            free(link_layer);
            link_layer = NULL;
        }
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; ++i) {
            /* Create list_layer */
            struct ivilayer *ivilayer = NULL;
            struct link_layer *link_layer = calloc(1, sizeof(*link_layer));
            if (NULL == link_layer) {
                continue;
            }
            wl_list_init(&link_layer->link);
            link_layer->layer = NULL;
            wl_list_for_each(ivilayer, &shell->list_layer, link) {
                if (ivilayer->layout_layer == pArray[i]) {
                    link_layer->layer = ivilayer;
                    break;
                }
            }

            if (link_layer->layer == NULL) {
                free(link_layer);
                link_layer = NULL;
                continue;
            }

            wl_list_insert(&ivisurf->list_layer, &link_layer->link);
        }
    }
}

static void
send_surface_prop(struct ivi_layout_surface *layout_surface,
                  const struct ivi_layout_surface_properties *prop,
                  enum ivi_layout_notification_mask mask,
                  void *userdata)
{
    struct ivisurface *ivisurf = userdata;
    struct ivishell *shell = ivisurf->shell;
    struct ivicontroller_surface *ctrlsurf = NULL;
    uint32_t id_surface = 0;

    id_surface = ivi_extension_get_id_of_surface(shell, layout_surface);

    wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
        if (id_surface != ctrlsurf->id_surface) {
            continue;
        }
        send_surface_event(ctrlsurf->resource, ivisurf, prop, mask);
    }

    update_surface_prop(ivisurf, mask);
}

static void
send_layer_add_event(struct ivilayer *ivilayer,
                     struct wl_resource *resource,
                     enum ivi_layout_notification_mask mask)
{
    struct ivi_layout_screen **pArray = NULL;
    uint32_t length = 0;
    int32_t ans = 0;
    int i = 0;
    struct link_screen *link_scrn = NULL;
    struct link_screen *next = NULL;
    struct iviscreen *iviscrn = NULL;
    struct ivishell *shell = ivilayer->shell;
    struct wl_client *client = wl_resource_get_client(resource);
    struct wl_resource *resource_output = NULL;

    ans = ivi_extension_get_screens_under_layer(shell, ivilayer->layout_layer,
                                                 &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get screens at send_layer_add_event\n");
        return;
    }

    /* Send Null to cancel added layer */
    if (mask & IVI_NOTIFICATION_REMOVE) {
        wl_list_for_each_safe(link_scrn, next, &ivilayer->list_screen, link) {
            ivi_controller_layer_send_screen(resource, NULL);
        }
    }
    else if (mask & IVI_NOTIFICATION_ADD) {
        for (i = 0; i < (int)length; i++) {
            /* Send new layer event */
            iviscrn = NULL;
            wl_list_for_each(iviscrn, &shell->list_screen, link) {
                if (iviscrn->layout_screen == pArray[i]) {
                    break;
                }
            }

            if (iviscrn == NULL) {
                continue;
            }

            resource_output =
                wl_resource_find_for_client(&iviscrn->output->resource_list,
                                     client);
            if (resource_output != NULL) {
                ivi_controller_layer_send_screen(resource, resource_output);
            }
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_layer_event(struct wl_resource *resource,
                 struct ivilayer *ivilayer,
                 const struct ivi_layout_layer_properties *prop,
                 uint32_t mask)
{
    if (mask & IVI_NOTIFICATION_OPACITY) {
        ivi_controller_layer_send_opacity(resource,
                                          prop->opacity);
    }
    if (mask & IVI_NOTIFICATION_SOURCE_RECT) {
        ivi_controller_layer_send_source_rectangle(resource,
                                          prop->source_x,
                                          prop->source_y,
                                          prop->source_width,
                                          prop->source_height);
    }
    if (mask & IVI_NOTIFICATION_DEST_RECT) {
        ivi_controller_layer_send_destination_rectangle(resource,
                                          prop->dest_x,
                                          prop->dest_y,
                                          prop->dest_width,
                                          prop->dest_height);
    }
    if (mask & IVI_NOTIFICATION_ORIENTATION) {
        ivi_controller_layer_send_orientation(resource,
                                          prop->orientation);
    }
    if (mask & IVI_NOTIFICATION_VISIBILITY) {
        ivi_controller_layer_send_visibility(resource,
                                          prop->visibility);
    }
    if (mask & IVI_NOTIFICATION_REMOVE) {
        send_layer_add_event(ivilayer, resource, IVI_NOTIFICATION_REMOVE);
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        send_layer_add_event(ivilayer, resource, IVI_NOTIFICATION_ADD);
    }
}

static void
update_layer_prop(struct ivilayer *ivilayer,
                  enum ivi_layout_notification_mask mask)
{
    struct ivi_layout_screen **pArray = NULL;
    uint32_t length = 0;
    int32_t ans = 0;
    struct link_screen *link_scrn = NULL;
    struct link_screen *next = NULL;
    struct ivishell *shell = ivilayer->shell;

    ans = ivi_extension_get_screens_under_layer(shell, ivilayer->layout_layer,
                                                 &length, &pArray);
    if (0 != ans) {
        weston_log("failed to get screens at send_layer_add_event\n");
        return;
    }

    /* Send Null to cancel added layer */
    if (mask & IVI_NOTIFICATION_REMOVE) {
        wl_list_for_each_safe(link_scrn, next, &ivilayer->list_screen, link) {
            wl_list_remove(&link_scrn->link);
            free(link_scrn);
            link_scrn = NULL;
        }
    }
    if (mask & IVI_NOTIFICATION_ADD) {
        int i = 0;
        for (i = 0; i < (int)length; i++) {
            struct ivishell *shell = ivilayer->shell;
            struct iviscreen *iviscrn = NULL;
            /* Create list_screen */
            link_scrn = calloc(1, sizeof(*link_scrn));
            if (NULL == link_scrn) {
                continue;
            }
            wl_list_init(&link_scrn->link);
            link_scrn->screen = NULL;
            wl_list_for_each(iviscrn, &shell->list_screen, link) {
                if (iviscrn->layout_screen == pArray[i]) {
                    link_scrn->screen = iviscrn;
                    break;
                }
            }

            if (link_scrn->screen == NULL) {
                free(link_scrn);
                link_scrn = NULL;
                continue;
            }
            wl_list_insert(&ivilayer->list_screen, &link_scrn->link);
        }
    }

    free(pArray);
    pArray = NULL;
}

static void
send_layer_prop(struct ivi_layout_layer *layer,
                const struct ivi_layout_layer_properties *prop,
                enum ivi_layout_notification_mask mask,
                void *userdata)
{
    struct ivilayer *ivilayer = userdata;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivishell *shell = ivilayer->shell;
    uint32_t id_layout_layer = 0;

    id_layout_layer = ivi_extension_get_id_of_layer(shell, layer);
    wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
        if (id_layout_layer != ctrllayer->id_layer) {
            continue;
        }
        send_layer_event(ctrllayer->resource, ivilayer, prop, mask);
    }

    update_layer_prop(ivilayer, mask);
}

static void
controller_surface_set_opacity(struct wl_client *client,
                   struct wl_resource *resource,
                   wl_fixed_t opacity)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_opacity(ivisurf->shell, ivisurf->layout_surface, opacity);
}

static void
controller_surface_set_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_source_rectangle(ivisurf->shell, ivisurf->layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_surface_set_destination_rectangle(struct wl_client *client,
                     struct wl_resource *resource,
                     int32_t x,
                     int32_t y,
                     int32_t width,
                     int32_t height)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;

    // TODO: create set transition type protocol
    ivi_extension_surface_set_transition(ivisurf->shell, ivisurf->layout_surface,
                                     IVI_LAYOUT_TRANSITION_NONE,
                                     300); // ms

    ivi_extension_surface_set_destination_rectangle(ivisurf->shell, ivisurf->layout_surface,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_surface_set_visibility(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t visibility)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_visibility(ivisurf->shell, ivisurf->layout_surface, visibility);
}

static void
controller_surface_set_configuration(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t width, int32_t height)
{
    /* This interface has been supported yet. */
    (void)client;
    (void)resource;
    (void)width;
    (void)height;
}

static void
controller_surface_set_orientation(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t orientation)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_surface_set_orientation(ivisurf->shell, ivisurf->layout_surface, (uint32_t)orientation);
}

static int
shm_surface_screenshot(struct weston_surface *surface,
                       int32_t width,
                       int32_t height,
                       int32_t stride,
                       const char *filename)
{
    struct weston_buffer *weston_buffer = NULL;
    struct wl_shm_buffer *shm_buffer = NULL;
    cairo_surface_t *cairo_surf = NULL;
    uint8_t *source_buffer = NULL;
    uint8_t *dest_buffer = NULL;

    weston_buffer = surface->buffer_ref.buffer;
    if (weston_buffer == NULL) {
        fprintf(stderr, "Failed to get weston buffer.\n");
        return -1;
    }

    shm_buffer = wl_shm_buffer_get(weston_buffer->resource);
    if (shm_buffer == NULL) {
        return -1;
    }

    source_buffer = wl_shm_buffer_get_data(shm_buffer);
    if (source_buffer == NULL) {
        fprintf(stderr, "Failed to get data from shm buffer.\n");
        return -1;
    }

    dest_buffer = malloc(stride * height);
    if (dest_buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return -1;
    }

    memcpy(dest_buffer, source_buffer, stride * height);

    cairo_surf = cairo_image_surface_create_for_data(
            dest_buffer,
            CAIRO_FORMAT_RGB24,
            width,
            height,
            stride);

    cairo_surface_write_to_png(cairo_surf, filename);
    cairo_surface_destroy(cairo_surf);
    free(dest_buffer);

    return 0;
}

static void
bind_framebuffer(GLuint *fbo_id, GLuint *tex_id, GLsizei width, GLsizei height)
{
    glGenTextures(1, tex_id);
    glGenFramebuffers(1, fbo_id);
    glBindTexture(GL_TEXTURE_2D, *tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex_id, 0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void
unbind_framebuffer(GLuint fbo_id, GLuint tex_id)
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex_id);
    glDeleteFramebuffers(1, &fbo_id);
}

static void
dump_surface(struct weston_output *output,
             struct weston_compositor *compositor,
             const char *filename,
             struct weston_view *view,
             int32_t x,
             int32_t y,
             int32_t width,
             int32_t height)
{
    struct weston_renderer *renderer = compositor->renderer;
    pixman_region32_t region;
    struct wl_list backup_transformation_list;
    struct weston_transform *link = NULL;
    struct weston_transform *next = NULL;
    uint8_t *readpixs = NULL;
    int32_t stride = width * (PIXMAN_FORMAT_BPP(compositor->read_format) / 8);
    GLuint fbo_id, tex_id;

    readpixs = malloc(stride * height);
    if (readpixs == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return;
    }

    pixman_region32_init_rect(&region, x, y, width, height);
    bind_framebuffer(&fbo_id, &tex_id, output->current_mode->width, output->current_mode->height);

    wl_list_init(&backup_transformation_list);
    wl_list_for_each_safe(link, next, &view->geometry.transformation_list, link) {
        wl_list_remove(&link->link);
        wl_list_insert(&backup_transformation_list, &link->link);
    }

    wl_list_init(&view->geometry.transformation_list);
    weston_view_geometry_dirty(view);
    weston_view_update_transform(view);

    renderer->repaint_output(output, &region);
    glFinish();

    y = output->current_mode->height - (y + height);
    int result = renderer->read_pixels(output,
                                       compositor->read_format,
                                       readpixs,
                                       x,
                                       y,
                                       width,
                                       height);

    wl_list_for_each_safe(link, next, &backup_transformation_list, link) {
        wl_list_remove(&link->link);
        wl_list_insert(&view->geometry.transformation_list, &link->link);
    }

    weston_view_geometry_dirty(view);
    weston_view_update_transform(view);

    unbind_framebuffer(fbo_id, tex_id);

    save_as_bitmap(filename, readpixs, stride * height, width, height, PIXMAN_FORMAT_BPP(compositor->read_format));
    free(readpixs);
}

static struct weston_view *
clear_viewlist_but_specified_surface(struct weston_compositor *compositor,
                                     struct weston_surface *surface)
{
    struct weston_view *view = NULL;
    wl_list_init(&compositor->view_list);

    wl_list_for_each(view, &surface->views, surface_link) {
        if (view == NULL) {
            continue;
        }

        wl_list_insert(compositor->view_list.prev, &view->link);
        break;
    }

    return view;
}

static void
get_gl_surface_rectangle(const struct ivi_layout_surface_properties *prop,
                         int32_t *x,
                         int32_t *y,
                         int32_t *width,
                         int32_t *height)
{
    if (prop == NULL) {
        return;
    }

    *x = prop->source_x;
    *y = prop->source_y;
    *width = prop->source_width;
    *height = prop->source_height;
}

static int
gl_surface_screenshot(struct ivisurface *ivisurf,
                      struct weston_surface *surface,
                      const char *filename)
{
    struct weston_compositor *compositor = surface->compositor;
    struct ivishell *shell = ivisurf->shell;
    struct weston_view *view = NULL;
    struct weston_output *output = NULL;
    struct iviscreen *link_scrn = NULL;
    const struct ivi_layout_surface_properties *prop;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    wl_list_for_each(link_scrn, &shell->list_screen, link) {
        if (link_scrn != NULL) {
            break;
        }
    }

    if (link_scrn == NULL) {
        fprintf(stderr, "Failed to get iviscreen\n");
        return -1;
    }

    prop = ivi_extension_get_properties_of_surface(shell, ivisurf->layout_surface);
    if (prop == NULL) {
        fprintf(stderr, "Failed to get surface properties.\n");
        return -1;
    }

    view = clear_viewlist_but_specified_surface(compositor, surface);
    if (view != NULL) {
        int32_t plane_is_null = 0;
        if (view->plane == NULL) {
            plane_is_null = 1;
            view->plane = &compositor->primary_plane;
        }

        get_gl_surface_rectangle(prop, &x, &y, &width, &height);
        dump_surface(link_scrn->output,
                     compositor,
                     filename,
                     view,
                     x,
                     y,
                     width,
                     height);

        if (plane_is_null) {
            view->plane = NULL;
        }
    }

    return 0;
}

static void
controller_surface_screenshot(struct wl_client *client,
                  struct wl_resource *resource,
                  const char *filename)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct weston_surface *weston_surface = NULL;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    struct ivishell *shell = ivisurf->shell;

    weston_surface = ivi_extension_surface_get_weston_surface(shell, ivisurf->layout_surface);
    if (weston_surface == NULL) {
        fprintf(stderr, "Failed to get weston surface.\n");
        return;
    }

    if (ivi_extension_surface_get_size(shell, ivisurf->layout_surface, &width, &height, &stride) != 0) {
        fprintf(stderr, "Failed to get surface size.\n");
        return;
    }

    if (shm_surface_screenshot(weston_surface, width, height, stride, filename) != 0) {
        if (gl_surface_screenshot(ivisurf, weston_surface, filename) != 0) {
            fprintf(stderr, "Failed to capture surface.\n");
        }
    }
}


static void
controller_surface_send_stats(struct wl_client *client,
                              struct wl_resource *resource)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    pid_t pid;
    uid_t uid;
    gid_t gid;
    wl_client_get_credentials(client, &pid, &uid, &gid);

    ivi_controller_surface_send_stats(resource, 0, 0,
                                      ivisurf->update_count, pid, "");
}

static void
controller_surface_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    (void)client;
    (void)destroy_scene_object;
    wl_resource_destroy(resource);
}

static void
controller_surface_set_input_focus(struct wl_client *client,
              struct wl_resource *resource,
              uint32_t device,
              int32_t enabled)
{
    /* This interface has been supported yet. */
    (void)client;
    (void)resource;
    (void)device;
    (void)enabled;
}

static const
struct ivi_controller_surface_interface controller_surface_implementation = {
    controller_surface_set_visibility,
    controller_surface_set_opacity,
    controller_surface_set_source_rectangle,
    controller_surface_set_destination_rectangle,
    controller_surface_set_configuration,
    controller_surface_set_orientation,
    controller_surface_screenshot,
    controller_surface_send_stats,
    controller_surface_destroy,
    controller_surface_set_input_focus
};

static void
controller_layer_set_source_rectangle(struct wl_client *client,
                   struct wl_resource *resource,
                   int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_source_rectangle(ivilayer->shell, ivilayer->layout_layer,
           (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_layer_set_destination_rectangle(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t x,
                 int32_t y,
                 int32_t width,
                 int32_t height)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_destination_rectangle(ivilayer->shell, ivilayer->layout_layer,
            (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

static void
controller_layer_set_visibility(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t visibility)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_visibility(ivilayer->shell, ivilayer->layout_layer, visibility);
}

static void
controller_layer_set_opacity(struct wl_client *client,
                 struct wl_resource *resource,
                 wl_fixed_t opacity)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_opacity(ivilayer->shell, ivilayer->layout_layer, opacity);
}

static void
controller_layer_set_configuration(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t width,
                 int32_t height)
{
    /* This interface has been supported yet. */
    (void)client;
    (void)resource;
    (void)width;
    (void)height;
}

static void
controller_layer_set_orientation(struct wl_client *client,
                 struct wl_resource *resource,
                 int32_t orientation)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_orientation(ivilayer->shell, ivilayer->layout_layer, (uint32_t)orientation);
}

static void
controller_layer_clear_surfaces(struct wl_client *client,
                    struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_layer_set_render_order(ivilayer->shell, ivilayer->layout_layer, NULL, 0);
}

static void
controller_layer_add_surface(struct wl_client *client,
                 struct wl_resource *resource,
                 struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    (void)client;
    ivi_extension_layer_add_surface(ivilayer->shell, ivilayer->layout_layer, ivisurf->layout_surface);
}

static void
controller_layer_remove_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *ivisurf = wl_resource_get_user_data(surface);
    (void)client;
    ivi_extension_layer_remove_surface(ivilayer->shell, ivilayer->layout_layer, ivisurf->layout_surface);
}

static void
controller_layer_screenshot(struct wl_client *client,
                struct wl_resource *resource,
                const char *filename)
{
    (void)client;
    (void)resource;
    (void)filename;
}

static void
controller_layer_set_render_order(struct wl_client *client,
                                  struct wl_resource *resource,
                                  struct wl_array *id_surfaces)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivi_layout_surface **layoutsurf_array = NULL;
    struct ivisurface *ivisurf = NULL;
    uint32_t *id_surface = NULL;
    uint32_t id_layout_surface = 0;
    int i = 0;
    (void)client;
    struct ivishell *shell = ivilayer->shell;

    layoutsurf_array = (struct ivi_layout_surface**)calloc(
                           id_surfaces->size, sizeof(void*));

    wl_array_for_each(id_surface, id_surfaces) {
        wl_list_for_each(ivisurf, &ivilayer->shell->list_surface, link) {
            id_layout_surface = ivi_extension_get_id_of_surface(shell, ivisurf->layout_surface);
            if (*id_surface == id_layout_surface) {
                layoutsurf_array[i] = ivisurf->layout_surface;
                i++;
                break;
            }
        }
    }

    ivi_extension_layer_set_render_order(shell, ivilayer->layout_layer,
                                   layoutsurf_array, i);
    free(layoutsurf_array);
}

static void
controller_layer_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivilayer->shell;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivicontroller_layer *next = NULL;
    uint32_t id_layer = ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);
    (void)client;
    (void)destroy_scene_object;

    ivilayer->layer_canbe_removed = 1;
    wl_list_for_each_safe(ctrllayer, next, &shell->list_controller_layer, link) {
        if (ctrllayer->id_layer != id_layer) {
            continue;
        }

        wl_resource_destroy(ctrllayer->resource);
    }
}

static const
struct ivi_controller_layer_interface controller_layer_implementation = {
    controller_layer_set_visibility,
    controller_layer_set_opacity,
    controller_layer_set_source_rectangle,
    controller_layer_set_destination_rectangle,
    controller_layer_set_configuration,
    controller_layer_set_orientation,
    controller_layer_screenshot,
    controller_layer_clear_surfaces,
    controller_layer_add_surface,
    controller_layer_remove_surface,
    controller_layer_set_render_order,
    controller_layer_destroy
};

static void
controller_screen_destroy(struct wl_client *client,
                          struct wl_resource *resource)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivicontroller_screen *ctrlscrn = NULL;
    struct ivicontroller_screen *next = NULL;
    (void)client;

    wl_list_for_each_safe(ctrlscrn, next,
                          &iviscrn->shell->list_controller_screen, link) {
        if (resource != ctrlscrn->resource) {
            continue;
        }

        wl_list_remove(&ctrlscrn->link);
        wl_resource_destroy(ctrlscrn->resource);
        free(ctrlscrn);
        ctrlscrn = NULL;
        break;
    }
}

static void
controller_screen_clear(struct wl_client *client,
                struct wl_resource *resource)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    (void)client;
    ivi_extension_screen_set_render_order(iviscrn->shell, iviscrn->layout_screen, NULL, 0);
}

static void
controller_screen_add_layer(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_resource *layer)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivilayer *ivilayer = wl_resource_get_user_data(layer);
    (void)client;
    ivi_extension_screen_add_layer(iviscrn->shell, iviscrn->layout_screen, ivilayer->layout_layer);
}

static void
controller_screen_screenshot(struct wl_client *client,
                struct wl_resource *resource,
                const char *filename)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    (void)client;

    struct weston_output *output = NULL;
    cairo_surface_t *cairo_surf = NULL;
    int32_t i = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    uint8_t *readpixs = NULL;

    output = ivi_extension_screen_get_output(iviscrn->shell, iviscrn->layout_screen);
    --output->disable_planes;

    width = output->current_mode->width;
    height = output->current_mode->height;
    stride = width * (PIXMAN_FORMAT_BPP(output->compositor->read_format) / 8);

    readpixs = malloc(stride * height);
    if (readpixs == NULL) {
        weston_log("fails to allocate memory\n");
        return;
    }

    output->compositor->renderer->read_pixels(
            output,
            output->compositor->read_format,
            readpixs,
            0,
            0,
            width,
            height);

    save_as_bitmap(filename, readpixs, stride * height, width, height, PIXMAN_FORMAT_BPP(output->compositor->read_format));
    free(readpixs);
}

static void
controller_screen_set_render_order(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_array *id_layers)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivi_layout_layer **layoutlayer_array = NULL;
    struct ivilayer *ivilayer = NULL;
    uint32_t *id_layer = NULL;
    uint32_t id_layout_layer = 0;
    int i = 0;
    (void)client;
    struct ivishell *shell = iviscrn->shell;

    layoutlayer_array = (struct ivi_layout_layer**)calloc(
                           id_layers->size, sizeof(void*));

    wl_array_for_each(id_layer, id_layers) {
        wl_list_for_each(ivilayer, &iviscrn->shell->list_layer, link) {
            id_layout_layer = ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);
            if (*id_layer == id_layout_layer) {
                layoutlayer_array[i] = ivilayer->layout_layer;
                i++;
                break;
            }
        }
    }

    ivi_extension_screen_set_render_order(shell, iviscrn->layout_screen,
                                    layoutlayer_array, i);
    free(layoutlayer_array);
}

static const
struct ivi_controller_screen_interface controller_screen_implementation = {
    controller_screen_destroy,
    controller_screen_clear,
    controller_screen_add_layer,
    controller_screen_screenshot,
    controller_screen_set_render_order
};

static void
controller_commit_changes(struct wl_client *client,
                          struct wl_resource *resource)
{
    int32_t ans = 0;
    (void)client;
    struct ivicontroller *controller = wl_resource_get_user_data(resource);

    ans = ivi_extension_commit_changes(controller->shell);
    if (ans < 0) {
        weston_log("Failed to commit changes at controller_commit_changes\n");
    }
}

static void
controller_layer_create(struct wl_client *client,
                        struct wl_resource *resource,
                        uint32_t id_layer,
                        int32_t width,
                        int32_t height,
                        uint32_t id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct ivishell *shell = ctrl->shell;
    struct ivi_layout_layer *layout_layer = NULL;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivilayer *ivilayer = NULL;
    const struct ivi_layout_layer_properties *prop;

    ivilayer = get_layer(&shell->list_layer, id_layer);
    if (ivilayer == NULL) {
        layout_layer = ivi_extension_layer_create_with_dimension(shell, id_layer,
                           (uint32_t)width, (uint32_t)height);
        if (layout_layer == NULL) {
            weston_log("id_layer is already created\n");
            return;
        }

        /* ivilayer will be created by layer_event_create */
        ivilayer = get_layer(&shell->list_layer, id_layer);
        if (ivilayer == NULL) {
            weston_log("couldn't get layer object\n");
            return;
        }
    }

    ctrllayer = calloc(1, sizeof *ctrllayer);
    if (!ctrllayer) {
        weston_log("no memory to allocate client layer\n");
        return;
    }

    ++ivilayer->controller_layer_count;
    ivilayer->layer_canbe_removed = 0;

    ctrllayer->shell = shell;
    ctrllayer->client = client;
    ctrllayer->id = id;
    ctrllayer->id_layer = id_layer;
    ctrllayer->resource = wl_resource_create(client,
                               &ivi_controller_layer_interface, 1, id);
    if (ctrllayer->resource == NULL) {
        weston_log("couldn't get layer object\n");
        return;
    }

    wl_list_init(&ctrllayer->link);
    wl_list_insert(&shell->list_controller_layer, &ctrllayer->link);

    wl_resource_set_implementation(ctrllayer->resource,
                                   &controller_layer_implementation,
                                   ivilayer, destroy_ivicontroller_layer);

    prop = ivi_extension_get_properties_of_layer(shell, ivilayer->layout_layer);
    send_layer_event(ctrllayer->resource, ivilayer,
                     prop, IVI_NOTIFICATION_ALL);
}

static void
surface_event_content(struct ivi_layout_surface *layout_surface, int32_t content, void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivicontroller_surface *ctrlsurf = NULL;
    uint32_t id_surface = 0;

    if (content == 0) {
        surface_event_remove(layout_surface, userdata);
    }
}

static void
controller_surface_create(struct wl_client *client,
                          struct wl_resource *resource,
                          uint32_t id_surface,
                          uint32_t id)
{
    struct ivicontroller *ctrl = wl_resource_get_user_data(resource);
    struct ivishell *shell = ctrl->shell;
    struct ivicontroller_surface *ctrlsurf = NULL;
    const struct ivi_layout_surface_properties *prop;
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller_surface *ctrl_link = NULL;

    ivisurf = get_surface(&shell->list_surface, id_surface);
    if (ivisurf == NULL) {
        return;
    }

    ctrlsurf = calloc(1, sizeof *ctrlsurf);
    if (!ctrlsurf) {
        weston_log("no memory to allocate controller surface\n");
        return;
    }

    ctrlsurf->shell = shell;
    ctrlsurf->client = client;
    ctrlsurf->id = id;
    ctrlsurf->id_surface = id_surface;
    wl_list_init(&ctrlsurf->link);
    wl_list_insert(&shell->list_controller_surface, &ctrlsurf->link);

    ctrlsurf->resource = wl_resource_create(client,
                               &ivi_controller_surface_interface, 1, id);
    if (ctrlsurf->resource == NULL) {
        weston_log("couldn't surface object");
        return;
    }

    wl_list_for_each(ctrl_link, &shell->list_controller_surface, link) {
        if ((ctrl_link->implementation_set == 0) &&
            (ctrl_link->id_surface == id_surface) &&
            (ctrl_link->shell == shell) &&
            (ctrl_link->client != client)) {
            ++ivisurf->controller_surface_count;
            wl_resource_set_implementation(ctrl_link->resource,
                                           &controller_surface_implementation,
                                           ivisurf, destroy_ivicontroller_surface);
            ctrl_link->implementation_set = 1;
        }
    }

    ++ivisurf->controller_surface_count;

    wl_resource_set_implementation(ctrlsurf->resource,
                                   &controller_surface_implementation,
                                   ivisurf, destroy_ivicontroller_surface);

    ctrlsurf->implementation_set = 1;

    prop = ivi_extension_get_properties_of_surface(shell, ivisurf->layout_surface);
    ivi_extension_surface_set_content_observer(shell, ivisurf->layout_surface, surface_event_content, shell);

    send_surface_event(ctrlsurf->resource, ivisurf,
                       prop, IVI_NOTIFICATION_ALL);
}

static const struct ivi_controller_interface controller_implementation = {
    controller_commit_changes,
    controller_layer_create,
    controller_surface_create
};

static void
add_client_to_resources(struct ivishell *shell,
                        struct wl_client *client,
                        struct ivicontroller *controller)
{
    struct ivisurface* ivisurf = NULL;
    struct ivilayer* ivilayer = NULL;
    struct iviscreen* iviscrn = NULL;
    struct ivicontroller_screen *ctrlscrn = NULL;
    struct wl_resource *resource_output = NULL;
    uint32_t id_layout_surface = 0;
    uint32_t id_layout_layer = 0;

    wl_list_for_each(iviscrn, &shell->list_screen, link) {
        resource_output = wl_resource_find_for_client(
                &iviscrn->output->resource_list, client);
        if (resource_output == NULL) {
            continue;
        }

        ctrlscrn = controller_screen_create(iviscrn->shell, client, iviscrn);
        if (ctrlscrn == NULL) {
            continue;
        }

        ivi_controller_send_screen(controller->resource,
                                   wl_resource_get_id(resource_output),
                                   ctrlscrn->resource);
    }
    wl_list_for_each_reverse(ivilayer, &shell->list_layer, link) {
        id_layout_layer =
            ivi_extension_get_id_of_layer(shell, ivilayer->layout_layer);

        ivi_controller_send_layer(controller->resource,
                                  id_layout_layer);
    }
    wl_list_for_each_reverse(ivisurf, &shell->list_surface, link) {
        id_layout_surface =
            ivi_extension_get_id_of_surface(shell, ivisurf->layout_surface);

        ivi_controller_send_surface(controller->resource,
                                    id_layout_surface);
    }
}

static void
bind_ivi_controller(struct wl_client *client, void *data,
                    uint32_t version, uint32_t id)
{
    struct ivishell *shell = data;
    struct ivicontroller *controller;
    (void)version;

    controller = calloc(1, sizeof *controller);
    if (controller == NULL) {
        weston_log("no memory to allocate controller\n");
        return;
    }

    controller->resource =
        wl_resource_create(client, &ivi_controller_interface, 1, id);
    wl_resource_set_implementation(controller->resource,
                                   &controller_implementation,
                                   controller, unbind_resource_controller);

    controller->shell = shell;
    controller->client = client;
    controller->id = id;

    wl_list_init(&controller->link);
    wl_list_insert(&shell->list_controller, &controller->link);

    add_client_to_resources(shell, client, controller);
}

static struct iviscreen*
create_screen(struct ivishell *shell, struct weston_output *output)
{
    struct iviscreen *iviscrn;
    iviscrn = calloc(1, sizeof *iviscrn);
    if (iviscrn == NULL) {
        weston_log("no memory to allocate client screen\n");
        return NULL;
    }

    iviscrn->shell = shell;
    iviscrn->output = output;

// TODO : Only Single display
    iviscrn->layout_screen = ivi_extension_get_screen_from_id(shell, 0);

    wl_list_init(&iviscrn->link);

    return iviscrn;
}

static struct ivilayer*
create_layer(struct ivishell *shell,
             struct ivi_layout_layer *layout_layer,
             uint32_t id_layer)
{
    struct ivilayer *ivilayer = NULL;
    struct ivicontroller *controller = NULL;

    ivilayer = get_layer(&shell->list_layer, id_layer);
    if (ivilayer != NULL) {
        weston_log("id_layer is already created\n");
        return NULL;
    }

    ivilayer = calloc(1, sizeof *ivilayer);
    if (NULL == ivilayer) {
        weston_log("no memory to allocate client layer\n");
        return NULL;
    }

    ivilayer->shell = shell;
    wl_list_init(&ivilayer->list_screen);
    wl_list_init(&ivilayer->link);
    wl_list_insert(&shell->list_layer, &ivilayer->link);
    ivilayer->layout_layer = layout_layer;

    ivi_extension_layer_add_notification(shell, layout_layer, send_layer_prop, ivilayer);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_layer(controller->resource, id_layer);
    }

    return ivilayer;
}

static struct ivisurface*
create_surface(struct ivishell *shell,
               struct ivi_layout_surface *layout_surface,
               uint32_t id_surface)
{
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller *controller = NULL;

    ivisurf = get_surface(&shell->list_surface, id_surface);
    if (ivisurf != NULL) {
        weston_log("id_surface is already created\n");
        return NULL;
    }

    ivisurf = calloc(1, sizeof *ivisurf);
    if (ivisurf == NULL) {
        weston_log("no memory to allocate client surface\n");
        return NULL;
    }

    ivisurf->shell = shell;
    ivisurf->layout_surface = layout_surface;
    wl_list_init(&ivisurf->list_layer);
    wl_list_init(&ivisurf->link);
    wl_list_insert(&shell->list_surface, &ivisurf->link);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_surface(controller->resource,
                                    id_surface);
    }

    ivi_extension_surface_add_notification(shell, layout_surface,
                                    send_surface_prop, ivisurf);

    return ivisurf;
}

static void
layer_event_create(struct ivi_layout_layer *layout_layer,
                     void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivilayer *ivilayer = NULL;
    uint32_t id_layer = 0;

    id_layer = ivi_extension_get_id_of_layer(shell, layout_layer);

    ivilayer = create_layer(shell, layout_layer, id_layer);
    if (ivilayer == NULL) {
        weston_log("failed to create layer");
        return;
    }
}

static void
layer_event_remove(struct ivi_layout_layer *layout_layer,
                     void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivilayer *ivilayer = NULL;
    struct ivilayer *next = NULL;
    uint32_t id_layer = 0;
    int is_removed = 0;

    wl_list_for_each_safe(ivilayer, next, &shell->list_layer, link) {
        if (layout_layer != ivilayer->layout_layer) {
            continue;
        }

        wl_list_remove(&ivilayer->link);

        is_removed = 1;
        free(ivilayer);
        ivilayer = NULL;
        break;
    }

    if (is_removed) {
        id_layer = ivi_extension_get_id_of_layer(shell, layout_layer);

        wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
            if (id_layer != ctrllayer->id_layer) {
                continue;
            }
            ivi_controller_layer_send_destroyed(ctrllayer->resource);
        }
    }
}


static void
surface_event_create(struct ivi_layout_surface *layout_surface,
                     void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivisurface *ivisurf = NULL;
    uint32_t id_surface = 0;
    struct ivicontroller_surface *ctrlsurf = NULL;

    id_surface = ivi_extension_get_id_of_surface(shell, layout_surface);

    ivisurf = create_surface(shell, layout_surface, id_surface);
    if (ivisurf == NULL) {
        weston_log("failed to create surface");
        return;
    }

    wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
        if (id_surface != ctrlsurf->id_surface) {
            continue;
        }
        ivi_controller_surface_send_content(ctrlsurf->resource, IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_AVAILABLE);
    }
}

static void
surface_event_remove(struct ivi_layout_surface *layout_surface,
                     void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivicontroller_surface *ctrlsurf = NULL;
    struct ivisurface *ivisurf = NULL;
    struct ivisurface *next = NULL;
    uint32_t id_surface = 0;
    int is_removed = 0;

    wl_list_for_each_safe(ivisurf, next, &shell->list_surface, link) {
        if (layout_surface != ivisurf->layout_surface) {
            continue;
        }

        wl_list_remove(&ivisurf->link);
        is_removed = 1;

        if (ivisurf->controller_surface_count == 0) {
            free(ivisurf);
        }
        else {
            ivisurf->can_be_removed = 1;
        }

        break;
    }

    if (is_removed) {
        id_surface = ivi_extension_get_id_of_surface(shell, layout_surface);

        wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
            if (id_surface != ctrlsurf->id_surface) {
                continue;
            }
            ivi_controller_surface_send_content(ctrlsurf->resource, IVI_CONTROLLER_SURFACE_CONTENT_STATE_CONTENT_REMOVED);
            ivi_controller_surface_send_destroyed(ctrlsurf->resource);
        }
    }
}

static void
surface_event_configure(struct ivi_layout_surface *layout_surface,
                        void *userdata)
{
    struct ivishell *shell = userdata;
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller_surface *ctrlsurf = NULL;
    const struct ivi_layout_surface_properties *prop;
    uint32_t id_surface = 0;

    id_surface = ivi_extension_get_id_of_surface(shell, layout_surface);

    ivisurf = get_surface(&shell->list_surface, id_surface);
    if (ivisurf == NULL) {
        weston_log("id_surface is not created yet\n");
        return;
    }

    prop = ivi_extension_get_properties_of_surface(shell, layout_surface);

    wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
        if (id_surface != ctrlsurf->id_surface) {
            continue;
        }
        send_surface_event(ctrlsurf->resource, ivisurf,
                           prop, IVI_NOTIFICATION_ALL);
    }
}

static int32_t
check_layout_layers(struct ivishell *shell)
{
    struct ivi_layout_layer **pArray = NULL;
    struct ivilayer *ivilayer = NULL;
    uint32_t id_layer = 0;
    uint32_t length = 0;
    uint32_t i = 0;
    int32_t ret = 0;

    ret = ivi_extension_get_layers(shell, &length, &pArray);
    if(ret != 0) {
        weston_log("failed to get layers at check_layout_layers\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_layer = ivi_extension_get_id_of_layer(shell, pArray[i]);
        ivilayer = create_layer(shell, pArray[i], id_layer);
        if (ivilayer == NULL) {
            weston_log("failed to create layer");
        }
    }

    free(pArray);
    pArray = NULL;

    return 0;
}

static int32_t
check_layout_surfaces(struct ivishell *shell)
{
    struct ivi_layout_surface **pArray = NULL;
    struct ivisurface *ivisurf = NULL;
    uint32_t id_surface = 0;
    uint32_t length = 0;
    uint32_t i = 0;
    int32_t ret = 0;

    ret = ivi_extension_get_surfaces(shell, &length, &pArray);
    if(ret != 0) {
        weston_log("failed to get surfaces at check_layout_surfaces\n");
        return -1;
    }

    if (length == 0) {
        /* if length is 0, pArray doesn't need to free.*/
        return 0;
    }

    for (i = 0; i < length; i++) {
        id_surface = ivi_extension_get_id_of_surface(shell, pArray[i]);
        ivisurf = create_surface(shell, pArray[i], id_surface);
        if (ivisurf == NULL) {
            weston_log("failed to create surface");
        }
    }

    free(pArray);
    pArray = NULL;

    return 0;
}

void
init_ivi_shell(struct weston_compositor *ec, struct ivishell *shell)
{
    struct weston_output *output = NULL;
    struct iviscreen *iviscrn = NULL;
    int32_t ret = 0;

    shell->compositor = ec;

    wl_list_init(&shell->list_surface);
    wl_list_init(&shell->list_layer);
    wl_list_init(&shell->list_screen);
    wl_list_init(&shell->list_weston_surface);
    wl_list_init(&shell->list_controller);
    wl_list_init(&shell->list_controller_screen);
    wl_list_init(&shell->list_controller_layer);
    wl_list_init(&shell->list_controller_surface);
    shell->event_restriction = 0;

    wl_list_for_each(output, &ec->output_list, link) {
        iviscrn = create_screen(shell, output);
        if (iviscrn != NULL) {
            wl_list_insert(&shell->list_screen, &iviscrn->link);
        }
    }

    ret = check_layout_layers(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_layers");
    }

    ret = check_layout_surfaces(shell);
    if (ret != 0) {
        weston_log("failed to check_layout_surfaces");
    }

    ivi_extension_add_notification_create_layer(shell, layer_event_create, shell);
    ivi_extension_add_notification_remove_layer(shell, layer_event_remove, shell);

    ivi_extension_add_notification_create_surface(shell, surface_event_create, shell);
    ivi_extension_add_notification_remove_surface(shell, surface_event_remove, shell);

    ivi_extension_add_notification_configure_surface(shell, surface_event_configure, shell);
}

int
setup_ivi_controller_server(struct weston_compositor *compositor,
                            struct ivishell *shell)
{
    if (wl_global_create(compositor->wl_display, &ivi_controller_interface, 1,
                         shell, bind_ivi_controller) == NULL) {
        return -1;
    }

    return 0;
}
