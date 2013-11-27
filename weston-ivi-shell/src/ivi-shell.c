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

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <cairo.h>

#include "compositor.h"
#include "ivi-application-server-protocol.h"
#include "ivi-controller-server-protocol.h"

enum {
    PROP_EVENT_OPACITY     = 0x00000001,
    PROP_EVENT_SRC_RECT    = 0x00000002,
    PROP_EVENT_DST_RECT    = 0x00000004,
    PROP_EVENT_ORIENTATION = 0x00000008,
    PROP_EVENT_VISIBILITY  = 0x00000010,
    PROP_EVENT_PIXELFORMAT = 0x00000020,
    PROP_EVENT_ADD         = 0x00000040,
    PROP_EVENT_ALL         = 0x7FFFFFFF
};

struct ivishell;

struct ivi_properties {
    wl_fixed_t opacity;
    int32_t src_x;
    int32_t src_y;
    int32_t src_width;
    int32_t src_height;
    int32_t dest_x;
    int32_t dest_y;
    int32_t dest_width;
    int32_t dest_height;
    int32_t orientation;
    uint32_t visibility;
};

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
    struct wl_list list_layer;
    struct wl_client *client;
    uint32_t id_surface;
    struct ivishell *shell;
    uint32_t update_count;

    struct weston_surface *surface;
    struct wl_listener surface_destroy_listener;
    struct weston_transform surface_rotation;
    struct weston_transform layer_rotation;
    struct weston_transform surface_pos;
    struct weston_transform layer_pos;
    struct weston_transform scaling;
    struct ivi_properties prop;
    int32_t pixelformat;
    uint32_t event_mask;

    struct {
        struct ivi_properties prop;
        struct wl_list link;
    } pending;

    struct {
        struct wl_list link;
        struct wl_list list_layer;
    } order;
};

struct ivilayer {
    struct wl_list link;
    struct wl_list list_screen;
    struct ivishell *shell;
    struct weston_layer el;
    uint32_t id_layer;

    struct ivi_properties prop;
    uint32_t event_mask;

    struct {
        struct wl_list list_surface;
        struct wl_list link;
        struct ivi_properties prop;
    } pending;

    struct {
        struct wl_list list_surface;
        struct wl_list link;
    } order;
};

struct iviscreen {
    struct wl_list link;
    struct ivishell *shell;
    struct wl_list list_resource;

    uint32_t id_screen;
    struct weston_output *output;
    struct wl_list list_layer;
    uint32_t event_mask;

    struct {
        struct wl_list list_layer;
        struct wl_list link;
    } pending;

    struct {
        struct wl_list list_layer;
        struct wl_list link;
    } order;
};

struct ivicontroller_surface {
    struct wl_resource *resource;
    uint32_t id;
    uint32_t id_surface;
    struct wl_client *client;
    struct wl_list link;
    struct ivishell *shell;
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

struct ping_timer {
    struct wl_event_source *source;
    uint32_t serial;
};

struct shell_surface {
    struct wl_resource *resource;

    struct weston_surface *surface;
    struct wl_listener surface_destroy_listener;

    struct ivishell *shell;
    struct ping_timer *ping_timer;

    char *title, *class;
    int32_t width;
    int32_t height;
    pid_t pid;
    struct wl_list link;

    const struct weston_shell_client *client;
    struct weston_output *output;
};

struct ivishell {
    struct wl_resource *resource;

    struct wl_listener destroy_listener;

    struct weston_compositor *compositor;

    struct weston_surface *surface;

    struct weston_process process;

    struct weston_seat *seat;

    struct wl_list list_surface;
    struct wl_list list_layer;
    struct wl_list list_screen;

    struct wl_list list_weston_surface;

    struct wl_list list_controller;
    struct wl_list list_controller_surface;
    struct wl_list list_controller_layer;
    struct wl_list list_controller_screen;

    struct {
        struct weston_process process;
        struct wl_client *client;
        struct wl_resource *desktop_shell;

        unsigned deathcount;
        uint32_t deathstamp;
    } child;

    int state;
    int previous_state;
    int event_restriction;
};

static const
struct ivi_surface_interface surface_implementation;

static void
add_ordersurface_to_layer(struct ivisurface *ivisurf,
                          struct ivilayer *ivilayer)
{
    struct link_layer *link_layer = NULL;

    link_layer = malloc(sizeof *link_layer);
    if (link_layer == NULL) {
        weston_log("memory insufficient for link_layer\n");
        return;
    }

    link_layer->layer = ivilayer;
    wl_list_init(&link_layer->link);
    wl_list_insert(&ivisurf->list_layer, &link_layer->link);
}

static void
remove_ordersurface_from_layer(struct ivisurface *ivisurf)
{
    struct link_layer *link_layer = NULL;
    struct link_layer *next = NULL;

    wl_list_for_each_safe(link_layer, next, &ivisurf->list_layer, link) {
        if (!wl_list_empty(&link_layer->link)) {
            wl_list_remove(&link_layer->link);
        }
        free(link_layer);
    }
    wl_list_init(&ivisurf->list_layer);
}

static void
add_orderlayer_to_screen(struct ivilayer *ivilayer,
                         struct iviscreen *iviscreen)
{
    struct link_screen *link_scrn = NULL;

    link_scrn = malloc(sizeof *link_scrn);
    if (link_scrn == NULL) {
        weston_log("memory insufficient for link_screen\n");
        return;
    }

    link_scrn->screen = iviscreen;
    wl_list_init(&link_scrn->link);
    wl_list_insert(&ivilayer->list_screen, &link_scrn->link);
}

static void
remove_orderlayer_from_screen(struct ivilayer *ivilayer)
{
    struct link_screen *link_scrn = NULL;
    struct link_screen *next = NULL;

    wl_list_for_each_safe(link_scrn, next, &ivilayer->list_screen, link) {
        if (!wl_list_empty(&link_scrn->link)) {
            wl_list_remove(&link_scrn->link);
        }
        free(link_scrn);
    }
    wl_list_init(&ivilayer->list_screen);
}

static void
destroy_ivicontroller_surface(struct wl_resource *resource)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivisurf->shell;

    struct ivicontroller_surface *ctrlsurf = NULL;
    struct ivicontroller_surface *next = NULL;

    wl_list_for_each_safe(ctrlsurf, next,
                          &shell->list_controller_surface, link) {
        if (ivisurf->id_surface != ctrlsurf->id_surface) {
            continue;
        }
        wl_list_remove(&ctrlsurf->link);
        free(ctrlsurf);
        break;
    }
}

static void
destroy_ivicontroller_layer(struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivilayer->shell;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivicontroller_layer *next = NULL;

    wl_list_for_each_safe(ctrllayer, next,
                          &shell->list_controller_layer, link) {
        if (ivilayer->id_layer != ctrllayer->id_layer) {
            continue;
        }
        wl_list_remove(&ctrllayer->link);
        free(ctrllayer);
        break;
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
        if (iviscrn->id_screen != ctrlscrn->id_screen) {
            continue;
        }
        wl_list_remove(&ctrlscrn->link);
        free(ctrlscrn);
        break;
    }
}

static void
unbind_resource_controller(struct wl_resource *resource)
{
    struct ivicontroller *controller = wl_resource_get_user_data(resource);

    wl_list_remove(&controller->link);

    free(controller);
}

static struct ivisurface*
get_surface(struct wl_list *list_surf, uint32_t id_surface)
{
    struct ivisurface *ivisurf;
    wl_list_for_each(ivisurf, list_surf, link) {
        if (ivisurf->id_surface == id_surface) {
            return ivisurf;
        }
    }

    return NULL;
}

static struct ivilayer*
get_layer(struct wl_list *list_layer, uint32_t id_layer)
{
    struct ivilayer *ivilayer;
    wl_list_for_each(ivilayer, list_layer, link) {
        if (ivilayer->id_layer == id_layer) {
            return ivilayer;
        }
    }

    return NULL;
}

static void
init_properties(struct ivi_properties *prop)
{
    memset(prop, 0, sizeof *prop);
    prop->opacity = wl_fixed_from_double(1.0);
}

static const
struct ivi_controller_screen_interface controller_screen_implementation;

static struct ivicontroller_screen*
controller_screen_create(struct ivishell *shell,
                        struct wl_client *client,
                        struct iviscreen *iviscrn)
{
    struct ivicontroller_screen *ctrlscrn;

    ctrlscrn = calloc(1, sizeof *ctrlscrn);
    if (ctrlscrn == NULL) {
        weston_log("no memory to allocate controller screen\n");
        return NULL;
    }

    ctrlscrn->client = client;
    ctrlscrn->shell = shell;
    ctrlscrn->id_screen = iviscrn->id_screen;

    ctrlscrn->resource =
        wl_resource_create(client, &ivi_controller_screen_interface, 1, 0);
    if (ctrlscrn->resource == NULL) {
        weston_log("couldn't new screen controller object");
        free(ctrlscrn);
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
update_opacity(struct ivilayer *ivilayer,
               struct ivisurface *ivisurf)
{
    double layer_alpha = wl_fixed_to_double(ivilayer->prop.opacity);
    double surf_alpha = wl_fixed_to_double(ivisurf->prop.opacity);

    if ((ivilayer->event_mask & PROP_EVENT_OPACITY) ||
        (ivisurf->event_mask & PROP_EVENT_OPACITY)) {
        if (ivisurf->surface == NULL) {
            return;
        }
        ivisurf->surface->alpha = layer_alpha * surf_alpha;
    }
}

static void
shell_surface_configure(struct weston_surface *,
                        int32_t, int32_t, int32_t, int32_t);

static struct shell_surface *
get_shell_surface(struct weston_surface *surface)
{
    if (surface->configure == shell_surface_configure) {
        return surface->configure_private;
    } else {
        return NULL;
    }
}

static void
update_surface_orientation(struct ivilayer *ivilayer,
                           struct ivisurface *ivisurf)
{
    float cx = 0.0f;
    float cy = 0.0f;
    struct weston_surface *es = ivisurf->surface;
    float v_sin = 0.0f;
    float v_cos = 0.0f;
    float sx = 1.0f;
    float sy = 1.0f;
    float width = 0.0f;
    float height = 0.0f;
    struct weston_matrix *matrix = &ivisurf->surface_rotation.matrix;
    struct shell_surface *shsurf = NULL;

    if (es == NULL) {
        return;
    }

    shsurf = get_shell_surface(es);
    if (shsurf == NULL) {
        return;
    }

    if ((shsurf->width == 0) || (shsurf->height == 0)) {
        return;
    }

    if ((ivilayer->prop.dest_width == 0) ||
        (ivilayer->prop.dest_height == 0)) {
        return;
    }
    width = (float)ivilayer->prop.dest_width;
    height = (float)ivilayer->prop.dest_height;

    switch (ivisurf->prop.orientation) {
    case IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES:
        v_sin = 0.0f;
        v_cos = 1.0f;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES:
        v_sin = 1.0f;
        v_cos = 0.0f;
        sx = width / height;
        sy = height / width;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES:
        v_sin = 0.0f;
        v_cos = -1.0f;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES:
    default:
        v_sin = -1.0f;
        v_cos = 0.0f;
        sx = width / height;
        sy = height / width;
        break;
    }
    wl_list_remove(&ivisurf->surface_rotation.link);
    weston_surface_geometry_dirty(es);

    weston_matrix_init(matrix);
    cx = 0.5f * width;
    cy = 0.5f * height;
    weston_matrix_translate(matrix, -cx, -cy, 0.0f);
    weston_matrix_rotate_xy(matrix, v_cos, v_sin);
    weston_matrix_scale(matrix, sx, sy, 1.0);
    weston_matrix_translate(matrix, cx, cy, 0.0f);
    wl_list_insert(&es->geometry.transformation_list,
                   &ivisurf->surface_rotation.link);

    weston_surface_set_transform_parent(es, NULL);
    weston_surface_update_transform(es);
}

static void
update_layer_orientation(struct ivilayer *ivilayer,
                         struct ivisurface *ivisurf)
{
    float cx = 0.0f;
    float cy = 0.0f;
    struct weston_surface *es = ivisurf->surface;
    struct weston_output *output = NULL;
    float v_sin = 0.0f;
    float v_cos = 0.0f;
    float sx = 1.0f;
    float sy = 1.0f;
    float width = 0.0f;
    float height = 0.0f;
    struct weston_matrix *matrix = &ivisurf->layer_rotation.matrix;
    struct shell_surface *shsurf = NULL;

    if (es == NULL) {
        return;
    }

    shsurf = get_shell_surface(es);
    if (shsurf == NULL) {
        return;
    }

    if ((shsurf->width == 0) || (shsurf->height == 0)) {
        return;
    }

    output = es->output;
    if (output == NULL) {
        return;
    }
    if ((output->width == 0) || (output->height == 0)) {
        return;
    }
    width = (float)output->width;
    height = (float)output->height;

    switch (ivilayer->prop.orientation) {
    case IVI_CONTROLLER_SURFACE_ORIENTATION_0_DEGREES:
        v_sin = 0.0f;
        v_cos = 1.0f;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_90_DEGREES:
        v_sin = 1.0f;
        v_cos = 0.0f;
        sx = width / height;
        sy = height / width;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_180_DEGREES:
        v_sin = 0.0f;
        v_cos = -1.0f;
        break;
    case IVI_CONTROLLER_SURFACE_ORIENTATION_270_DEGREES:
    default:
        v_sin = -1.0f;
        v_cos = 0.0f;
        sx = width / height;
        sy = height / width;
        break;
    }
    wl_list_remove(&ivisurf->layer_rotation.link);
    weston_surface_geometry_dirty(es);

    weston_matrix_init(matrix);
    cx = 0.5f * width;
    cy = 0.5f * height;
    weston_matrix_translate(matrix, -cx, -cy, 0.0f);
    weston_matrix_rotate_xy(matrix, v_cos, v_sin);
    weston_matrix_scale(matrix, sx, sy, 1.0);
    weston_matrix_translate(matrix, cx, cy, 0.0f);
    wl_list_insert(&es->geometry.transformation_list,
                   &ivisurf->layer_rotation.link);

    weston_surface_set_transform_parent(es, NULL);
    weston_surface_update_transform(es);
}

static void
update_surface_position(struct ivisurface *ivisurf)
{
    struct weston_surface *es = ivisurf->surface;
    float tx  = (float)ivisurf->prop.dest_x;
    float ty  = (float)ivisurf->prop.dest_y;
    struct weston_matrix *matrix = &ivisurf->surface_pos.matrix;
    struct shell_surface *shsurf = NULL;

    if (es == NULL) {
        return;
    }

    shsurf = get_shell_surface(es);
    if (shsurf == NULL) {
        return;
    }

    if ((shsurf->width == 0) || (shsurf->height == 0)) {
        return;
    }

    wl_list_remove(&ivisurf->surface_pos.link);

    weston_matrix_init(matrix);
    weston_matrix_translate(matrix, tx, ty, 0.0f);
    wl_list_insert(&es->geometry.transformation_list,
                   &ivisurf->surface_pos.link);

    weston_surface_set_transform_parent(es, NULL);
    weston_surface_update_transform(es);

    //weston_zoom_run(es, 0.0, 1.0, NULL, NULL);
}

static void
update_layer_position(struct ivilayer *ivilayer,
               struct ivisurface *ivisurf)
{
    struct weston_surface *es = ivisurf->surface;
    float tx  = (float)ivilayer->prop.dest_x;
    float ty  = (float)ivilayer->prop.dest_y;
    struct weston_matrix *matrix = &ivisurf->layer_pos.matrix;
    struct shell_surface *shsurf = NULL;

    if (es == NULL) {
        return;
    }

    shsurf = get_shell_surface(es);
    if (shsurf == NULL) {
        return;
    }

    if ((shsurf->width == 0) || (shsurf->height == 0)) {
        return;
    }

    wl_list_remove(&ivisurf->layer_pos.link);

    weston_matrix_init(matrix);
    weston_matrix_translate(matrix, tx, ty, 0.0f);
    wl_list_insert(
        &es->geometry.transformation_list,
        &ivisurf->layer_pos.link);

    weston_surface_set_transform_parent(es, NULL);
    weston_surface_update_transform(es);
}

static void
update_scale(struct ivilayer *ivilayer,
               struct ivisurface *ivisurf)
{
    struct weston_surface *es = ivisurf->surface;
    float sx = 0.0f;
    float sy = 0.0f;
    float lw = 0.0f;
    float sw = 0.0f;
    float lh = 0.0f;
    float sh = 0.0f;
    struct weston_matrix *matrix = &ivisurf->scaling.matrix;
    struct shell_surface *shsurf = NULL;

    if (es == NULL) {
        return;
    }

    shsurf = get_shell_surface(es);
    if (shsurf == NULL) {
        return;
    }

    if ((shsurf->width == 0) || (shsurf->height == 0)) {
        return;
    }

    lw = ((float)ivilayer->prop.dest_width / shsurf->width);
    sw = ((float)ivisurf->prop.dest_width / shsurf->width);
    lh = ((float)ivilayer->prop.dest_height / shsurf->height);
    sh = ((float)ivisurf->prop.dest_height / shsurf->height);
    sx = sw * lw;
    sy = sh * lh;

    wl_list_remove(&ivisurf->scaling.link);
    weston_matrix_init(matrix);
    weston_matrix_scale(matrix, sx, sy, 1.0f);

    wl_list_insert(&es->geometry.transformation_list,
                   &ivisurf->scaling.link);

    weston_surface_set_transform_parent(es, NULL);
    weston_surface_update_transform(es);
}

static void
update_prop(struct ivilayer *ivilayer,
            struct ivisurface *ivisurf)
{
    if (ivilayer->event_mask | ivisurf->event_mask) {
        update_opacity(ivilayer, ivisurf);
        update_layer_orientation(ivilayer, ivisurf);
        update_layer_position(ivilayer, ivisurf);
        update_surface_position(ivisurf);
        update_surface_orientation(ivilayer, ivisurf);
        update_scale(ivilayer, ivisurf);

        ivisurf->update_count++;

        if (ivisurf->surface == NULL) {
            return;
        }
        weston_surface_geometry_dirty(ivisurf->surface);
        weston_surface_damage(ivisurf->surface);
    }
}

static void
send_surface_add_event(struct ivisurface *ivisurf,
                       struct wl_resource *res_surf)
{
    struct ivishell *shell = ivisurf->shell;
    struct link_layer* link_layer = NULL;
    struct ivicontroller_layer *ctrllayer = NULL;
    struct wl_client *client_surf = NULL;

    client_surf = wl_resource_get_client(res_surf);
    wl_list_for_each(link_layer, &ivisurf->list_layer, link) {
        wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {

            if (ctrllayer->client != client_surf) {
                continue;
            }

            if (ctrllayer->id_layer != link_layer->layer->id_layer) {
                continue;
            }

            ivi_controller_surface_send_layer(res_surf, NULL);
            ivi_controller_surface_send_layer(res_surf, ctrllayer->resource);
        }
    }
}

static void
send_surface_event(struct ivisurface *ivisurf,
                   struct wl_resource *resource, uint32_t mask)
{
    if (mask & PROP_EVENT_OPACITY) {
        ivi_controller_surface_send_opacity(resource,
                                            ivisurf->prop.opacity);
    }
    if (mask & PROP_EVENT_SRC_RECT) {
        ivi_controller_surface_send_source_rectangle(resource,
            ivisurf->prop.src_x, ivisurf->prop.src_y,
            ivisurf->prop.src_width, ivisurf->prop.src_height);
    }
    if (mask & PROP_EVENT_DST_RECT) {
        ivi_controller_surface_send_destination_rectangle(resource,
            ivisurf->prop.dest_x, ivisurf->prop.dest_y,
            ivisurf->prop.dest_width, ivisurf->prop.dest_height);
    }
    if (mask & PROP_EVENT_ORIENTATION) {
        ivi_controller_surface_send_orientation(resource,
                                                ivisurf->prop.orientation);
    }
    if (mask & PROP_EVENT_VISIBILITY) {
        ivi_controller_surface_send_visibility(resource,
                                               ivisurf->prop.visibility);
    }
    if (mask & PROP_EVENT_PIXELFORMAT) {
        ivi_controller_surface_send_pixelformat(resource,
                                                ivisurf->pixelformat);
    }
    if (mask & PROP_EVENT_ADD) {
        send_surface_add_event(ivisurf, resource);
    }
}

static void
send_layer_add_event(struct ivilayer *ivilayer,
                       struct wl_resource *res_layer)
{
    struct link_screen *link_scrn = NULL;
    struct wl_client *client_layer = NULL;
    struct wl_resource *resource_output = NULL;

    client_layer = wl_resource_get_client(res_layer);
    wl_list_for_each(link_scrn, &ivilayer->list_screen, link) {

        resource_output = wl_resource_find_for_client(
                              &link_scrn->screen->output->resource_list,
                              client_layer);
        if (resource_output == NULL) {
            continue;
        }

        ivi_controller_layer_send_screen(res_layer, resource_output);
    }
}

static void
send_layer_event(struct ivilayer *ivilayer,
                 struct wl_resource *resource, uint32_t mask)
{
    if (mask & PROP_EVENT_OPACITY) {
        ivi_controller_layer_send_opacity(resource,
                                          ivilayer->prop.opacity);
    }
    if (mask & PROP_EVENT_SRC_RECT) {
        ivi_controller_layer_send_source_rectangle(resource,
                                          ivilayer->prop.src_x,
                                          ivilayer->prop.src_y,
                                          ivilayer->prop.src_width,
                                          ivilayer->prop.src_height);
    }
    if (mask & PROP_EVENT_DST_RECT) {
        ivi_controller_layer_send_destination_rectangle(resource,
                                          ivilayer->prop.dest_x,
                                          ivilayer->prop.dest_y,
                                          ivilayer->prop.dest_width,
                                          ivilayer->prop.dest_height);
    }
    if (mask & PROP_EVENT_ORIENTATION) {
        ivi_controller_layer_send_orientation(resource,
                                          ivilayer->prop.orientation);
    }
    if (mask & PROP_EVENT_VISIBILITY) {
        ivi_controller_layer_send_visibility(resource,
                                          ivilayer->prop.visibility);
    }
    if (mask & PROP_EVENT_ADD) {
        send_layer_add_event(ivilayer, resource);
    }
}

static void
send_surface_prop(struct ivisurface *ivisurf)
{
    struct ivicontroller_surface *ctrlsurf = 0;
    struct ivishell *shell = ivisurf->shell;
    wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
        if (ivisurf->id_surface != ctrlsurf->id_surface) {
            continue;
        }
        send_surface_event(ivisurf, ctrlsurf->resource, ivisurf->event_mask);
    }
    ivisurf->event_mask = 0;
}

static void
send_layer_prop(struct ivilayer *ivilayer)
{
    struct ivicontroller_layer *ctrllayer = 0;
    struct ivishell *shell = ivilayer->shell;
    wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
        if (ivilayer->id_layer != ctrllayer->id_layer) {
            continue;
        }
        send_layer_event(ivilayer, ctrllayer->resource, ivilayer->event_mask);
    }
    ivilayer->event_mask = 0;
}

static void
commit_changes(struct ivishell *shell)
{
    struct iviscreen *iviscrn;
    struct ivilayer *ivilayer;
    struct ivisurface *ivisurf;
    wl_list_for_each(iviscrn, &shell->list_screen, link) {
        wl_list_for_each(ivilayer,
                        &iviscrn->order.list_layer,
                        order.link) {
            wl_list_for_each(ivisurf,
                             &ivilayer->order.list_surface,
                             order.link) {
                update_prop(ivilayer, ivisurf);
            }
        }
    }
}

static void
send_prop(struct ivishell *shell)
{
    struct ivilayer *ivilayer;
    struct ivisurface *ivisurf;
    wl_list_for_each(ivilayer, &shell->list_layer, link) {
        send_layer_prop(ivilayer);
    }

    wl_list_for_each(ivisurf, &shell->list_surface, link) {
        send_surface_prop(ivisurf);
    }
}

static void
surface_destroy(struct wl_client *client,
                struct wl_resource *resource)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    (void)client;

    if (!wl_list_empty(&ivisurf->pending.link)) {
        wl_list_remove(&ivisurf->pending.link);
    }
    if (!wl_list_empty(&ivisurf->order.link)) {
        wl_list_remove(&ivisurf->order.link);
    }
    if (!wl_list_empty(&ivisurf->link)) {
        wl_list_remove(&ivisurf->link);
    }
    remove_ordersurface_from_layer(ivisurf);
    wl_list_remove(&ivisurf->surface_destroy_listener.link);

    free(ivisurf);
}

static const struct ivi_surface_interface surface_implementation = {
    surface_destroy
};

static void
controller_surface_set_opacity(struct wl_client *client,
                   struct wl_resource *resource,
                   wl_fixed_t opacity)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct ivi_properties *prop = &ivisurf->pending.prop;
    (void)client;

    prop->opacity = opacity;
    ivisurf->event_mask |= PROP_EVENT_OPACITY;
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
    struct ivi_properties *prop = &ivisurf->pending.prop;
    (void)client;

    prop->src_x = x;
    prop->src_y = y;
    prop->src_width = width;
    prop->src_height = height;
    ivisurf->event_mask |= PROP_EVENT_SRC_RECT;
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
    struct ivi_properties *prop = &ivisurf->pending.prop;
    (void)client;

    prop->dest_x = x;
    prop->dest_y = y;
    prop->dest_width = width;
    prop->dest_height = height;

    ivisurf->event_mask |= PROP_EVENT_DST_RECT;
}

static void
controller_surface_set_visibility(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t visibility)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct ivi_properties *prop = &ivisurf->pending.prop;
    (void)client;

    prop->visibility = visibility;
    ivisurf->event_mask |= PROP_EVENT_VISIBILITY;
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
    struct ivi_properties *prop = &ivisurf->pending.prop;
    (void)client;

    prop->orientation = orientation;
    ivisurf->event_mask |= PROP_EVENT_ORIENTATION;
}

static void
controller_surface_screenshot(struct wl_client *client,
                  struct wl_resource *resource,
                  const char *filename)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct weston_compositor *compositor = ivisurf->shell->compositor;
    cairo_surface_t *surface;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint8_t *pixels;
    (void)client;

    width = ivisurf->prop.dest_width;
    height = ivisurf->prop.dest_height;
    stride = width *
             (PIXMAN_FORMAT_BPP(compositor->read_format) / 8);
    pixels = malloc(stride * height);
    if (pixels == NULL) {
        return;
    }

    compositor->renderer->read_surface_pixels(ivisurf->surface,
                             compositor->read_format, pixels,
                             0, 0, width, height);

    surface = cairo_image_surface_create_for_data(pixels,
                                                  CAIRO_FORMAT_ARGB32,
                                                  width, height, stride);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);
    free(pixels);
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
                                      ivisurf->update_count, pid);
}

static void
controller_surface_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    struct ivisurface *ivisurf = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivisurf->shell;
    struct ivicontroller_surface *ctrlsurf = NULL;
    (void)client;
    (void)destroy_scene_object;

    wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
        if (ctrlsurf->id_surface != ivisurf->id_surface) {
            continue;
        }

        if (!wl_list_empty(&ctrlsurf->link)) {
            wl_list_remove(&ctrlsurf->link);
        }
        wl_resource_destroy(resource);
        break;
    }
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
    controller_surface_destroy
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
    struct ivi_properties *prop = &ivilayer->pending.prop;
    (void)client;

    prop->src_x = x;
    prop->src_y = y;
    prop->src_width = width;
    prop->src_height = height;
    ivilayer->event_mask |= PROP_EVENT_SRC_RECT;
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
    struct ivi_properties *prop = &ivilayer->pending.prop;
    (void)client;

    prop->dest_x = x;
    prop->dest_y = y;
    prop->dest_width = width;
    prop->dest_height = height;
    ivilayer->event_mask |= PROP_EVENT_DST_RECT;
}

static void
controller_layer_set_visibility(struct wl_client *client,
                    struct wl_resource *resource,
                    uint32_t visibility)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivi_properties *prop = &ivilayer->pending.prop;
    (void)client;

    prop->visibility = visibility;
    ivilayer->event_mask |= PROP_EVENT_VISIBILITY;
}

static void
controller_layer_set_opacity(struct wl_client *client,
                 struct wl_resource *resource,
                 wl_fixed_t opacity)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivi_properties *prop = &ivilayer->pending.prop;
    (void)client;

    prop->opacity = opacity;
    ivilayer->event_mask |= PROP_EVENT_OPACITY;
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
    struct ivi_properties *prop = &ivilayer->pending.prop;
    (void)client;

    prop->orientation = orientation;
    ivilayer->event_mask |= PROP_EVENT_ORIENTATION;
}

static void
controller_layer_clear_surfaces(struct wl_client *client,
                    struct wl_resource *resource)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    (void)client;

    wl_list_init(&ivilayer->pending.list_surface);

    ivilayer->event_mask |= PROP_EVENT_ADD;
}

static int
is_surface_in_layer(struct ivisurface *ivisurf,
                    struct ivilayer *ivilayer)
{
    struct ivisurface *surf = NULL;

    wl_list_for_each(surf, &ivilayer->pending.list_surface, pending.link) {
        if (surf->id_surface == ivisurf->id_surface) {
            return 1;
        }
    }

    return 0;
}

static void
controller_layer_add_surface(struct wl_client *client,
                 struct wl_resource *resource,
                 struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *addsurf = wl_resource_get_user_data(surface);
    struct ivisurface *ivisurf;
    int is_surf_in_layer = 0;
    (void)client;

    if (addsurf == NULL) {
        weston_log("invalid surface in layer_add_surface\n");
        return;
    }
    is_surf_in_layer = is_surface_in_layer(addsurf, ivilayer);
    if (is_surf_in_layer == 1) {
        return;
    }

    wl_list_for_each(ivisurf, &ivilayer->shell->list_surface, link) {
        if (ivisurf->id_surface == addsurf->id_surface) {
            if (!wl_list_empty(&ivisurf->pending.link)) {
                wl_list_remove(&ivisurf->pending.link);
            }
            wl_list_init(&ivisurf->pending.link);
            wl_list_insert(&ivilayer->pending.list_surface,
                           &ivisurf->pending.link);
            break;
        }
    }

    ivilayer->event_mask |= PROP_EVENT_ADD;
}

static void
controller_layer_remove_surface(struct wl_client *client,
                    struct wl_resource *resource,
                    struct wl_resource *surface)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivisurface *remsurf = wl_resource_get_user_data(surface);
    struct ivisurface *ivisurf = NULL;
    struct ivisurface *next = NULL;
    (void)client;
    (void)resource;
    (void)surface;

    wl_list_for_each_safe(ivisurf, next,
                          &ivilayer->pending.list_surface, pending.link) {
        if (ivisurf->id_surface == remsurf->id_surface) {
            if (!wl_list_empty(&ivisurf->pending.link)) {
                wl_list_remove(&ivisurf->pending.link);
            }
            wl_list_init(&ivisurf->pending.link);
            break;
        }
    }
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
    struct ivisurface *ivisurf = NULL;
    uint32_t *id_surface = NULL;
    (void)client;

    wl_list_init(&ivilayer->pending.list_surface);
    wl_array_for_each(id_surface, id_surfaces) {
        wl_list_for_each(ivisurf, &ivilayer->shell->list_surface, link) {
            if (*id_surface != ivisurf->id_surface) {
                continue;
            }

            if (!wl_list_empty(&ivisurf->pending.link)) {
                wl_list_remove(&ivisurf->pending.link);
            }
            wl_list_init(&ivisurf->pending.link);
            wl_list_insert(&ivilayer->pending.list_surface,
                           &ivisurf->pending.link);
            break;
        }
    }

    ivilayer->event_mask |= PROP_EVENT_ADD;
}

static void
layer_destroy(struct ivilayer *ivilayer)
{
    if (!wl_list_empty(&ivilayer->pending.link)) {
        wl_list_remove(&ivilayer->pending.link);
    }
    if (!wl_list_empty(&ivilayer->order.link)) {
        wl_list_remove(&ivilayer->order.link);
    }
    if (!wl_list_empty(&ivilayer->link)) {
        wl_list_remove(&ivilayer->link);
    }
    remove_orderlayer_from_screen(ivilayer);

    free(ivilayer);
}

static void
controller_layer_destroy(struct wl_client *client,
              struct wl_resource *resource,
              int32_t destroy_scene_object)
{
    struct ivilayer *ivilayer = wl_resource_get_user_data(resource);
    struct ivishell *shell = ivilayer->shell;
    struct ivicontroller_layer *ctrllayer = NULL;
    (void)client;
    (void)destroy_scene_object;

    wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
        if (ctrllayer->id_layer != ivilayer->id_layer) {
            continue;
        }

        if (!wl_list_empty(&ctrllayer->link)) {
            wl_list_remove(&ctrllayer->link);
        }
        wl_resource_destroy(resource);
        break;
    }

    layer_destroy(ivilayer);
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
        if (iviscrn->id_screen != ctrlscrn->id_screen) {
            continue;
        }
        destroy_ivicontroller_screen(ctrlscrn->resource);
        wl_resource_destroy(ctrlscrn->resource);
        break;
    }
}

static void
controller_screen_clear(struct wl_client *client,
                struct wl_resource *resource)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    (void)client;

    wl_list_init(&iviscrn->pending.list_layer);

    iviscrn->event_mask |= PROP_EVENT_ADD;
}

static int
is_layer_in_screen(struct ivilayer *ivilayer,
                    struct iviscreen *iviscrn)
{
    struct ivilayer *layer = NULL;

    wl_list_for_each(layer, &iviscrn->pending.list_layer, pending.link) {
        if (layer->id_layer == ivilayer->id_layer) {
            return 1;
        }
    }

    return 0;
}

static void
controller_screen_add_layer(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_resource *layer)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct ivilayer *addlayer = wl_resource_get_user_data(layer);
    struct ivilayer *ivilayer;
    int is_layer_in_scrn = 0;
    (void)client;

    is_layer_in_scrn = is_layer_in_screen(addlayer, iviscrn);
    if (is_layer_in_scrn == 1) {
        return;
    }

    wl_list_for_each(ivilayer, &iviscrn->shell->list_layer, link) {
        if (ivilayer->id_layer == addlayer->id_layer) {
            if (!wl_list_empty(&ivilayer->pending.link)) {
                wl_list_remove(&ivilayer->pending.link);
            }
            wl_list_init(&ivilayer->pending.link);
            wl_list_insert(&iviscrn->pending.list_layer,
                           &ivilayer->pending.link);
            break;
        }
    }

    iviscrn->event_mask |= PROP_EVENT_ADD;
}

static void
controller_screen_screenshot(struct wl_client *client,
                struct wl_resource *resource,
                const char *filename)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    struct weston_output *output = iviscrn->output;
    cairo_surface_t *surface = NULL;
    int i = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t stride = 0;
    uint8_t *readpixs = NULL;
    uint8_t *writepixs = NULL;
    uint8_t *d = NULL;
    uint8_t *s = NULL;
    (void)client;

    output->disable_planes--;

    width = output->current_mode->width;
    height = output->current_mode->height;
    stride = width *
             (PIXMAN_FORMAT_BPP(output->compositor->read_format) / 8);
    readpixs = malloc(stride * height);
    if (readpixs == NULL) {
        return;
    }
    writepixs = malloc(stride * height);
    if (writepixs == NULL) {
        return;
    }

    output->compositor->renderer->read_pixels(output,
                             output->compositor->read_format, readpixs,
                             0, 0, width, height);

    s = readpixs;
    d = writepixs + stride * (height - 1);

    for (i = 0; i < height; i++) {
        memcpy(d, s, stride);
        d -= stride;
        s += stride;
    }

    surface = cairo_image_surface_create_for_data(writepixs,
                                                  CAIRO_FORMAT_ARGB32,
                                                  width, height, stride);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);
    free(writepixs);
    free(readpixs);
}

static void
controller_screen_set_render_order(struct wl_client *client,
                struct wl_resource *resource,
                struct wl_array *id_layers)
{
    struct iviscreen *iviscrn = wl_resource_get_user_data(resource);
    uint32_t *id_layer = NULL;
    struct ivilayer *ivilayer;
    (void)client;

    wl_list_init(&iviscrn->pending.list_layer);
    wl_array_for_each(id_layer, id_layers) {
        wl_list_for_each(ivilayer, &iviscrn->shell->list_layer, link) {
            if (*id_layer == ivilayer->id_layer) {
                continue;
            }

            if (!wl_list_empty(&ivilayer->pending.link)) {
                wl_list_remove(&ivilayer->pending.link);
            }
            wl_list_init(&ivilayer->pending.link);
            wl_list_insert(&iviscrn->pending.list_layer,
                           &ivilayer->pending.link);
            break;
        }
    }

    iviscrn->event_mask |= PROP_EVENT_ADD;
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
commit_list_surface(struct ivishell *shell)
{
    struct ivisurface *ivisurf;
    wl_list_for_each(ivisurf, &shell->list_surface, link) {
        ivisurf->prop = ivisurf->pending.prop;
    }
}

static void
commit_list_layer(struct ivishell *shell)
{
    struct ivilayer *ivilayer;
    struct ivisurface *ivisurf;

    wl_list_for_each(ivilayer, &shell->list_layer, link) {
        ivilayer->prop = ivilayer->pending.prop;

        if (!(ivilayer->event_mask & PROP_EVENT_ADD)) {
            continue;
        }
        wl_list_init(&ivilayer->order.list_surface);
        wl_list_for_each(ivisurf, &ivilayer->pending.list_surface,
                              pending.link) {
            remove_ordersurface_from_layer(ivisurf);

            if (!wl_list_empty(&ivisurf->order.link)) {
                wl_list_remove(&ivisurf->order.link);
            }
            wl_list_init(&ivisurf->order.link);
            wl_list_insert(&ivilayer->order.list_surface,
                           &ivisurf->order.link);
            add_ordersurface_to_layer(ivisurf, ivilayer);
        }
    }
}

static void
commit_list_screen(struct ivishell *shell)
{
    struct weston_compositor *ec = shell->compositor;
    struct iviscreen *iviscrn;
    struct ivilayer *ivilayer;
    struct ivisurface *ivisurf;
    wl_list_for_each(iviscrn, &shell->list_screen, link) {
        if (iviscrn->event_mask & PROP_EVENT_ADD) {
            wl_list_init(&iviscrn->order.list_layer);

            wl_list_for_each(ivilayer, &iviscrn->pending.list_layer,
                             pending.link) {
                remove_orderlayer_from_screen(ivilayer);

                wl_list_insert(&iviscrn->order.list_layer,
                               &ivilayer->order.link);
                add_orderlayer_to_screen(ivilayer, iviscrn);
            }
            iviscrn->event_mask = 0;
        }

        /* For rendering */
        wl_list_init(&ec->layer_list);
        wl_list_for_each(ivilayer, &iviscrn->order.list_layer, order.link) {
            if (ivilayer->prop.visibility == 0) {
                continue;
            }

            wl_list_insert(&ec->layer_list, &ivilayer->el.link);
            wl_list_init(&ivilayer->el.surface_list);

            wl_list_for_each(ivisurf, &ivilayer->order.list_surface,
                             order.link) {
                if (ivisurf->prop.visibility == 0) {
                    continue;
                }

                if (ivisurf->surface == NULL) {
                    continue;
                }

                wl_list_insert(&ivilayer->el.surface_list,
                               &ivisurf->surface->layer_link);
                ivisurf->surface->output = iviscrn->output;
            }
        }
        break;
    }
}

static void
controller_commit_changes(struct wl_client *client,
                          struct wl_resource *resource)
{
    struct ivicontroller *controller = wl_resource_get_user_data(resource);
    (void)client;

    commit_list_surface(controller->shell);
    commit_list_layer(controller->shell);
    commit_list_screen(controller->shell);

    commit_changes(controller->shell);
    send_prop(controller->shell);
    weston_compositor_schedule_repaint(controller->shell->compositor);
}

static struct ivilayer*
layer_create(struct wl_client *client,
                        struct ivicontroller *ctrl,
                        uint32_t id_layer,
                        int32_t width,
                        int32_t height,
                        uint32_t id)
{
    struct ivishell *shell = ctrl->shell;
    struct ivilayer *ivilayer = NULL;
    struct ivicontroller *controller = NULL;
    struct ivicontroller_layer *ctrllayer = NULL;
    (void)client;
    (void)width;
    (void)height;
    (void)id;

    ivilayer = get_layer(&shell->list_layer, id_layer);
    if (ivilayer != NULL)
    {
        return ivilayer;
    }

    ivilayer = calloc(1, sizeof *ivilayer);
    if (!ivilayer) {
        weston_log("no memory to allocate client layer\n");
        return NULL;
    }

    init_properties(&ivilayer->prop);
    ivilayer->pending.prop = ivilayer->prop;
    ivilayer->id_layer = id_layer;
    ivilayer->shell = shell;
    ivilayer->event_mask = 0;
    wl_list_init(&ivilayer->link);
    wl_list_init(&ivilayer->pending.link);
    wl_list_init(&ivilayer->order.link);
    wl_list_init(&ivilayer->list_screen);

    wl_list_init(&ivilayer->pending.list_surface);
    wl_list_init(&ivilayer->order.list_surface);
    wl_list_insert(&shell->list_layer, &ivilayer->link);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_layer(controller->resource,
                                  ivilayer->id_layer);

        wl_list_for_each(ctrllayer, &shell->list_controller_layer, link) {
            if (ivilayer->id_layer != ctrllayer->id_layer) {
                continue;
            }
            send_layer_event(ivilayer, ctrllayer->resource, PROP_EVENT_ALL);
        }
    }

    return ivilayer;
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
    struct ivicontroller_layer *ctrllayer = NULL;
    struct ivilayer *ivilayer = NULL;

weston_log("controller_layer_create is called\n");
    ctrllayer = calloc(1, sizeof *ctrllayer);
    if (!ctrllayer) {
        weston_log("no memory to allocate client layer\n");
        return;
    }

    ctrllayer->shell = shell;
    ctrllayer->client = client;
    ctrllayer->id = id;
    ctrllayer->id_layer = id_layer;
    ctrllayer->resource = wl_resource_create(client,
                               &ivi_controller_layer_interface, 1, id);
    if (ctrllayer->resource == NULL) {
        weston_log("couldn't layer object\n");
        return;
    }

weston_log("controller_layer_create is called\n");
    wl_list_init(&ctrllayer->link);
    wl_list_insert(&shell->list_controller_layer, &ctrllayer->link);

    ivilayer = layer_create(client, ctrl, id_layer, width, height, id);

    if (ivilayer != NULL) {
        wl_resource_set_implementation(ctrllayer->resource,
                                   &controller_layer_implementation,
                                   ivilayer, destroy_ivicontroller_layer);
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
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller_surface *ctrlsurf = NULL;

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
    ctrlsurf->resource = wl_resource_create(client,
                               &ivi_controller_surface_interface, 1, id);
    if (ctrlsurf->resource == NULL) {
        weston_log("couldn't surface object");
        return;
    }

    wl_list_for_each(ivisurf, &shell->list_surface, link) {
        if (ivisurf->id_surface == ctrlsurf->id_surface) {
            wl_resource_set_implementation(ctrlsurf->resource,
                                   &controller_surface_implementation,
                                   ivisurf, destroy_ivicontroller_surface);
            break;
        }
    }
}

static const struct ivi_controller_interface controller_implementation = {
    controller_commit_changes,
    controller_layer_create,
    controller_surface_create
};

static void
westonsurface_destroy_from_ivisurface(struct wl_listener *listener, void *data)
{
    struct ivisurface *ivisurf = container_of(listener,
                                               struct ivisurface,
                                               surface_destroy_listener);
    (void)data;
    ivisurf->surface = NULL;
}

static void
application_surface_create(struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id_surface,
                      struct wl_resource *surface_resource,
                      uint32_t id)
{
weston_log("application_surface_create\n");
    struct ivishell *shell = wl_resource_get_user_data(resource);
    struct weston_surface *es = NULL;
    struct ivisurface *ivisurf = NULL;
    struct ivicontroller *controller = NULL;
    struct ivicontroller_surface *ctrlsurf = NULL;

    es = wl_resource_get_user_data(surface_resource);
    if (es == NULL) {
        weston_log("application_surface_create: invalid surface\n");
    }

    ivisurf = get_surface(&shell->list_surface, id_surface);
    if (ivisurf != NULL)
    {
        weston_log("id_surface is already created\n");

        resource = wl_resource_create(client, &ivi_surface_interface, 1, id);
        ivisurf->surface = es;
        if (es != NULL) {
            ivisurf->surface_destroy_listener.notify =
                westonsurface_destroy_from_ivisurface;
            wl_resource_add_destroy_listener(es->resource,
                          &ivisurf->surface_destroy_listener);
        }
        return;
    }

    ivisurf = calloc(1, sizeof *ivisurf);
    if (ivisurf == NULL) {
        weston_log("no memory to allocate client surface\n");
        return;
    }

    init_properties(&ivisurf->prop);
    ivisurf->pixelformat = IVI_CONTROLLER_SURFACE_PIXELFORMAT_RGBA_8888;
    ivisurf->pending.prop = ivisurf->prop;
    ivisurf->surface = es;
    if (es != NULL) {
        ivisurf->surface_destroy_listener.notify =
            westonsurface_destroy_from_ivisurface;
        wl_resource_add_destroy_listener(es->resource,
                      &ivisurf->surface_destroy_listener);
    }
    ivisurf->id_surface = id_surface;
    ivisurf->shell = shell;
    ivisurf->event_mask = 0;

    wl_list_init(&ivisurf->surface_rotation.link);
    wl_list_init(&ivisurf->layer_rotation.link);
    wl_list_init(&ivisurf->surface_pos.link);
    wl_list_init(&ivisurf->layer_pos.link);
    wl_list_init(&ivisurf->scaling.link);

    wl_list_init(&ivisurf->link);
    wl_list_init(&ivisurf->pending.link);
    wl_list_init(&ivisurf->order.link);
    wl_list_init(&ivisurf->list_layer);
    wl_list_insert(&shell->list_surface, &ivisurf->link);

    wl_list_for_each(controller, &shell->list_controller, link) {
        ivi_controller_send_surface(controller->resource,
                                    ivisurf->id_surface);

        wl_list_for_each(ctrlsurf, &shell->list_controller_surface, link) {
            if (ivisurf->id_surface != ctrlsurf->id_surface) {
                continue;
            }
            send_surface_event(ivisurf, ctrlsurf->resource, PROP_EVENT_ALL);
        }
    }

    resource = wl_resource_create(client, &ivi_surface_interface, 1, id);
    if (resource == NULL) {
        weston_log("couldn't surface object");
        return;
    }
    wl_resource_set_implementation(resource,
                                   &surface_implementation,
                                   ivisurf, NULL);
}

static const struct ivi_application_interface application_implementation = {
    application_surface_create
};

static void
bind_ivi_application(struct wl_client *client, void *data,
                     uint32_t version, uint32_t id)
{
    struct ivishell *shell = data;
    struct wl_resource *resource = NULL;
    (void)version;

    resource = wl_resource_create(client, &ivi_application_interface, 1, id);
    wl_resource_set_implementation(resource,
                                   &application_implementation,
                                   shell, NULL);
}

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

    wl_list_for_each(ivisurf, &shell->list_surface, link) {
        ivi_controller_send_surface(controller->resource,
                                    ivisurf->id_surface);
    }

    wl_list_for_each(ivilayer, &shell->list_layer, link) {
        ivi_controller_send_layer(controller->resource,
                                  ivilayer->id_layer);
    }

    wl_list_for_each(iviscrn, &shell->list_screen, link) {
        resource_output =
            wl_resource_find_for_client(&iviscrn->output->resource_list,
                                 client);
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
    iviscrn->event_mask = 0;

    wl_list_init(&iviscrn->link);
    wl_list_init(&iviscrn->list_layer);
    wl_list_init(&iviscrn->pending.list_layer);
    wl_list_init(&iviscrn->order.list_layer);
    wl_list_init(&iviscrn->list_resource);

    return iviscrn;
}

static void
init_ivi_shell(struct weston_compositor *ec, struct ivishell *shell)
{
    struct weston_output *output = NULL;
    struct iviscreen *iviscrn = NULL;
    shell->compositor = ec;

    wl_list_init(&ec->layer_list);
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
        wl_list_init(&iviscrn->order.list_layer);
    }
}

static void
shell_destroy(struct wl_listener *listener, void *data)
{
    struct ivishell *shell =
        container_of(listener, struct ivishell, destroy_listener);
    (void)data;

    free(shell);
}

static void
ping_timer_destroy(struct shell_surface *shsurf)
{
    if (!shsurf || !shsurf->ping_timer) {
        return;
    }

    if (shsurf->ping_timer->source) {
        wl_event_source_remove(shsurf->ping_timer->source);
    }

    free(shsurf->ping_timer);
    shsurf->ping_timer = NULL;
}

static void
destroy_shell_surface(struct shell_surface *shsurf)
{
    wl_list_remove(&shsurf->surface_destroy_listener.link);
    shsurf->surface->configure = NULL;
    ping_timer_destroy(shsurf);
    free(shsurf->title);

    wl_list_remove(&shsurf->link);
    free(shsurf);
}

static void
shell_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    struct shell_surface *shsurf = container_of(listener,
                                               struct shell_surface,
                                               surface_destroy_listener);
    (void)data;

    if (wl_resource_get_client(shsurf->resource)) {
        wl_resource_destroy(shsurf->resource);
    } else {
        wl_resource_destroy(shsurf->resource);
        destroy_shell_surface(shsurf);
    }
}

static void
configure(struct weston_surface *surface,
          float x, float y, int32_t width, int32_t height)
{
    weston_surface_configure(surface, x, y, width, height);
    if (surface->output) {
        weston_surface_update_transform(surface);
    }
}

static void
shell_surface_configure(struct weston_surface *es,
                        int32_t sx, int32_t sy,
                        int32_t width, int32_t height)
{
    float from_x = 0.0f;
    float from_y = 0.0f;
    float to_x = 0.0f;
    float to_y = 0.0f;
    struct shell_surface *shsurf = get_shell_surface(es);

    if ((width == 0) || (height == 0) || (shsurf == NULL)) {
        return;
    }

    if (shsurf->width != width || shsurf->height != height) {

        shsurf->width = width;
        shsurf->height = height;

        weston_surface_to_global_float(es, 0, 0, &from_x, &from_y);
        weston_surface_to_global_float(es, sx, sy, &to_x, &to_y);
        configure(es,
                  es->geometry.x + to_x - from_x,
                  es->geometry.y + to_y - from_y,
                  width, height);

        commit_changes(shsurf->shell);
    }
}

static struct shell_surface *
create_shell_surface(void *shell, struct weston_surface *surface,
                     const struct weston_shell_client *client)
{
    struct shell_surface *shsurf;

    if (surface->configure) {
        weston_log("surface->configure already set\n");
        return NULL;
    }

    shsurf = calloc(1, sizeof *shsurf);
    if (shsurf == NULL) {
        weston_log("no memory to allocate shell surface\n");
        return NULL;
    }

    surface->configure = shell_surface_configure;
    surface->configure_private = shsurf;

    shsurf->shell = (struct ivishell *)shell;
    shsurf->surface = surface;
    shsurf->ping_timer = NULL;
    shsurf->title = strdup("");

    /* init link so its safe to always remove it in destroy_shell_surface */
    wl_list_init(&shsurf->link);

    shsurf->client = client;

    return shsurf;
}

static void
send_configure(struct weston_surface *surface,
               uint32_t edges, int32_t width, int32_t height)
{
    struct shell_surface *shsurf = get_shell_surface(surface);

    wl_shell_surface_send_configure(shsurf->resource,
                                    edges, width, height);
}

static const struct weston_shell_client shell_client = {
    send_configure
};

static void
shell_destroy_shell_surface(struct wl_resource *resource)
{
    struct shell_surface *shsurf = wl_resource_get_user_data(resource);

    destroy_shell_surface(shsurf);
}

static void
shell_surface_pong(struct wl_client *client,
                   struct wl_resource *resource, uint32_t serial)
{
    struct shell_surface *shsurf = wl_resource_get_user_data(resource);
    (void)client;

    if (shsurf->ping_timer == NULL) {
        return;
    }

    if (shsurf->ping_timer->serial == serial) {
        ping_timer_destroy(shsurf);
    }
}

static void
shell_surface_move(struct wl_client *client, struct wl_resource *resource,
                   struct wl_resource *seat_resource, uint32_t serial)
{
    (void)client;
    (void)resource;
    (void)seat_resource;
    (void)serial;
}

static void
shell_surface_resize(struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *seat_resource, uint32_t serial,
                     uint32_t edges)
{
    (void)client;
    (void)resource;
    (void)seat_resource;
    (void)serial;
    (void)edges;
}

static void
shell_surface_set_toplevel(struct wl_client *client,
                           struct wl_resource *resource)
{
    (void)client;
    (void)resource;
}

static void
shell_surface_set_transient(struct wl_client *client,
                            struct wl_resource *resource,
                            struct wl_resource *parent_resource,
                            int x, int y, uint32_t flags)
{
    (void)client;
    (void)resource;
    (void)parent_resource;
    (void)x;
    (void)y;
    (void)flags;
}

static void
shell_surface_set_fullscreen(struct wl_client *client,
                             struct wl_resource *resource,
                             uint32_t method,
                             uint32_t framerate,
                             struct wl_resource *output_resource)
{
    (void)client;
    (void)resource;
    (void)method;
    (void)framerate;
    (void)output_resource;
}

static void
shell_surface_set_popup(struct wl_client *client,
                        struct wl_resource *resource,
                        struct wl_resource *seat_resource,
                        uint32_t serial,
                        struct wl_resource *parent_resource,
                        int32_t x, int32_t y, uint32_t flags)
{
    (void)client;
    (void)resource;
    (void)seat_resource;
    (void)serial;
    (void)parent_resource;
    (void)x;
    (void)y;
    (void)flags;
}

static void
shell_surface_set_maximized(struct wl_client *client,
                            struct wl_resource *resource,
                            struct wl_resource *output_resource)
{
    (void)client;
    (void)resource;
    (void)output_resource;
}

static void
shell_surface_set_title(struct wl_client *client,
                        struct wl_resource *resource, const char *title)
{
    (void)client;
    (void)resource;
    (void)title;
}

static void
shell_surface_set_class(struct wl_client *client,
                        struct wl_resource *resource, const char *class)
{
    struct shell_surface *shsurf = wl_resource_get_user_data(resource);
    (void)client;

    free(shsurf->class);
    shsurf->class = strdup(class);
}

static const struct wl_shell_surface_interface
shell_surface_implementation = {
    shell_surface_pong,
    shell_surface_move,
    shell_surface_resize,
    shell_surface_set_toplevel,
    shell_surface_set_transient,
    shell_surface_set_fullscreen,
    shell_surface_set_popup,
    shell_surface_set_maximized,
    shell_surface_set_title,
    shell_surface_set_class
};

static void
shell_weston_surface_destroy(struct wl_listener *listener, void *data)
{
    struct link_shell_weston_surface *lsurf =
        container_of(listener,
                     struct link_shell_weston_surface,
                     destroy_listener);
    (void)data;

    wl_list_remove(&lsurf->link);
    free(lsurf);
}


static void
shell_get_shell_surface(struct wl_client *client,
                        struct wl_resource *resource,
                        uint32_t id,
                        struct wl_resource *surface_resource)
{
    struct weston_surface *surface =
        wl_resource_get_user_data(surface_resource);
    struct ivishell *shell = wl_resource_get_user_data(resource);
    struct shell_surface *shsurf;
    struct link_shell_weston_surface *lsurf;
    pid_t pid;
    uid_t uid;
    gid_t gid;

    if (get_shell_surface(surface)) {
        wl_resource_post_error(surface_resource,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "ivishell::get_shell_surface already requested");
        return;
    }

    shsurf = create_shell_surface(shell, surface, &shell_client);
    if (!shsurf) {
        wl_resource_post_error(surface_resource,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "surface->configure already set");
        return;
    }

    shsurf->resource = wl_resource_create(client,
                                          &wl_shell_surface_interface,
                                          1, id);
    wl_resource_set_implementation(shsurf->resource,
                                   &shell_surface_implementation,
                                   shsurf, shell_destroy_shell_surface);

    shsurf->surface_destroy_listener.notify = shell_handle_surface_destroy;
    wl_resource_add_destroy_listener(surface->resource,
                  &shsurf->surface_destroy_listener);

    lsurf = (struct link_shell_weston_surface*)malloc(sizeof *lsurf);
    lsurf->surface = surface;
    wl_list_init(&lsurf->link);
    wl_list_insert(&shell->list_weston_surface, &lsurf->link);

    lsurf->destroy_listener.notify = shell_weston_surface_destroy;
    wl_resource_add_destroy_listener(surface->resource,
                                     &lsurf->destroy_listener);

    wl_client_get_credentials(client, &pid, &uid, &gid);
    shsurf->pid = pid;
}

static const struct wl_shell_interface shell_implementation = {
    shell_get_shell_surface
};

static void
bind_shell(struct wl_client *client, void *data,
           uint32_t version, uint32_t id)
{
    struct ivishell *shell = data;
    struct wl_resource *resource = NULL;
    (void)version;

    resource = wl_resource_create(client, &wl_shell_interface, 1, id);
    wl_resource_set_implementation(resource,
                                   &shell_implementation,
                                   shell, NULL);
}

WL_EXPORT int
module_init(struct weston_compositor *ec,
	    int *argc, char *argv[])
{
    struct weston_seat *seat;
    struct ivishell *shell;
    (void)argc;
    (void)argv;

    shell = malloc(sizeof *shell);
    if (shell == NULL)
        return -1;

    memset(shell, 0, sizeof *shell);
    init_ivi_shell(ec, shell);

    shell->destroy_listener.notify = shell_destroy;
    wl_signal_add(&ec->destroy_signal, &shell->destroy_listener);

    if (wl_global_create(ec->wl_display, &wl_shell_interface, 1,
                              shell, bind_shell) == NULL) {
        return -1;
    }

    if (wl_global_create(ec->wl_display, &ivi_application_interface, 1,
                  shell, bind_ivi_application) == NULL)
        return -1;

    if (wl_global_create(ec->wl_display, &ivi_controller_interface, 1,
                  shell, bind_ivi_controller) == NULL)
        return -1;

    wl_list_for_each(seat, &ec->seat_list, link) {
        if (seat) {
            shell->seat = seat;
        }
    }

    return 0;
}
