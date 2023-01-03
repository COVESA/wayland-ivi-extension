/*
 * Copyright (C) 2017 Advanced Driver Information Technology Joint Venture GmbH
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <weston.h>
#include <libweston-6/libweston-desktop.h>
#include "config-parser.h"
#include <ivi-layout-export.h>

#ifndef INVALID_ID
#define INVALID_ID 0xFFFFFFFF
#endif

struct db_elem
{
    struct wl_list link;
    uint32_t surface_id;
    char *cfg_app_id;
    char *cfg_title;
    struct ivi_layout_surface *layout_surface;
};

struct ivi_id_agent
{
    uint32_t default_behavior_set;
    uint32_t default_surface_id;
    uint32_t default_surface_id_max;
    struct wl_list app_list;
    struct weston_compositor *compositor;
    const struct ivi_layout_interface *interface;

    struct wl_listener desktop_surface_configured;
    struct wl_listener destroy_listener;
    struct wl_listener surface_removed;
};

static int32_t
check_config_parameter(char *cfg_val, char *val)
{
    if (cfg_val == NULL)
        return IVI_SUCCEEDED;
    else if (val == NULL || strcmp(cfg_val, val) != 0)
        return IVI_FAILED;

    return IVI_SUCCEEDED;
}

static int32_t
get_id_from_config(struct ivi_id_agent *ida, struct ivi_layout_surface
        *layout_surface) {
    struct db_elem *db_elem;
    char *temp_app_id = NULL;
    char *temp_title = NULL;
    int ret = IVI_FAILED;

    struct weston_surface *weston_surface =
            ida->interface->surface_get_weston_surface(layout_surface);

    /* Get app id and title */
    struct weston_desktop_surface *wds = weston_surface_get_desktop_surface(
            weston_surface);

    if (weston_desktop_surface_get_app_id(wds) != NULL)
        temp_app_id = strdup(weston_desktop_surface_get_app_id(wds));

    if (weston_desktop_surface_get_title(wds) != NULL)
        temp_title = strdup(weston_desktop_surface_get_title(wds));

    /*
     * Check for every config parameter to be fulfilled. This part must be
     * extended, if additional attributes are desired to be checked.
     */
    wl_list_for_each(db_elem, &ida->app_list, link)
    {
        if (check_config_parameter(db_elem->cfg_app_id, temp_app_id) == 0) {
            if (check_config_parameter(db_elem->cfg_title, temp_title) == 0) {
                /* Found configuration for application. */
                int res = ida->interface->surface_set_id(layout_surface,
                        db_elem->surface_id);
                if (res)
                    continue;

                db_elem->layout_surface = layout_surface;
                ret = IVI_SUCCEEDED;

                break;
            }
        }
    }

    free(temp_app_id);
    free(temp_title);

    return ret;
}

/*
 * This function generates the id of a surface in regard to the desired
 * parameters. For implementation of different behavior in id generation please
 * adjust this function.
 * In this implementation the app_id and/or title of the application is used for
 * identification. It is also possible to use the pid, uid or gid for example.
 */
static int32_t
get_id(struct ivi_id_agent *ida, struct ivi_layout_surface *layout_surface)
{
    if (get_id_from_config(ida, layout_surface) == IVI_SUCCEEDED)
        return IVI_SUCCEEDED;

    /* No default layer available */
    if (ida->default_behavior_set == 0) {
        weston_log("ivi-id-agent: Could not find configuration for application\n");
        goto ivi_failed;

    /* Default behavior for unknown applications */
    } else if (ida->default_surface_id < ida->default_surface_id_max) {
        weston_log("ivi-id-agent: No configuration for application adding to "
                "default layer\n");

        /*
         * Check if ivi-shell application already created an application with
         * desired surface_id
         */
        struct ivi_layout_surface *temp_layout_surf =
                ida->interface->get_surface_from_id(
                        ida->default_surface_id);
        if ((temp_layout_surf != NULL) && (temp_layout_surf != layout_surface)) {
            weston_log("ivi-id-agent: surface_id already used by an ivi-shell "
                                "application\n");
            goto ivi_failed;
        }

        ida->interface->surface_set_id(layout_surface,
                                              ida->default_surface_id);
        ida->default_surface_id++;

    } else {
        weston_log("ivi-id-agent: Interval for default surface_id generation "
                "exceeded\n");
        goto ivi_failed;
    }

    return IVI_SUCCEEDED;

ivi_failed:
    return IVI_FAILED;
}

static void
desktop_surface_event_configure(struct wl_listener *listener,
        void *data)
{
    struct ivi_id_agent *ida = wl_container_of(listener, ida,
            desktop_surface_configured);

    struct ivi_layout_surface *layout_surface =
            (struct ivi_layout_surface *) data;

    if (get_id(ida, layout_surface) == IVI_FAILED)
        weston_log("ivi-id-agent: Could not create surface_id for application\n");
}

static void
surface_event_remove(struct wl_listener *listener, void *data) {
    struct ivi_id_agent *ida = wl_container_of(listener, ida,
                surface_removed);
    struct ivi_layout_surface *layout_surface =
                (struct ivi_layout_surface *) data;
    struct db_elem *db_elem = NULL;

    wl_list_for_each(db_elem, &ida->app_list, link)
    {
        if(db_elem->layout_surface == layout_surface) {
            db_elem->layout_surface = NULL;
            break;
        }
    }
}

static int32_t deinit(struct ivi_id_agent *ida);

static void
id_agent_module_deinit(struct wl_listener *listener, void *data) {
    (void)data;
    struct ivi_id_agent *ida = wl_container_of(listener, ida, destroy_listener);

    deinit(ida);
}

static int32_t
check_config(struct db_elem *curr_db_elem, struct ivi_id_agent *ida)
{
    struct db_elem *db_elem;

    if (ida->default_surface_id <= curr_db_elem->surface_id
            && curr_db_elem->surface_id <= ida->default_surface_id_max) {
        weston_log("ivi-id-agent: surface_id: %d in default id interval "
                "[%d, %d] (CONFIG ERROR)\n", curr_db_elem->surface_id,
                ida->default_surface_id, ida->default_surface_id_max);
        goto ivi_failed;
    }

    wl_list_for_each(db_elem, &ida->app_list, link)
    {
        if(curr_db_elem == db_elem)
            continue;

        if (db_elem->surface_id == curr_db_elem->surface_id) {
            weston_log("ivi-id-agent: Duplicate surface_id: %d (CONFIG ERROR)\n",
                    curr_db_elem->surface_id);
            goto ivi_failed;
        }
    }

    return IVI_SUCCEEDED;

ivi_failed:
    return IVI_FAILED;
}

static int32_t
read_config(struct ivi_id_agent *ida)
{
    struct weston_config *config = NULL;
    struct weston_config_section *section = NULL;
    const char *name = NULL;

    config = wet_get_config(ida->compositor);
    if (!config)
        goto ivi_failed;

    section = weston_config_get_section(config, "desktop-app-default", NULL,
            NULL);

    if (section) {
        weston_log("ivi-id-agent: Default behavior for unknown applications is "
                "set\n");
        ida->default_behavior_set = 1;

        weston_config_section_get_uint(section, "default-surface-id",
                &ida->default_surface_id, INVALID_ID);
        weston_config_section_get_uint(section, "default-surface-id-max",
                &ida->default_surface_id_max, INVALID_ID);

        if (ida->default_surface_id == INVALID_ID ||
                ida->default_surface_id_max == INVALID_ID) {
            weston_log("ivi-id-agent: Missing configuration for default "
                    "behavior\n");
            ida->default_behavior_set = 0;
        }
    } else {
        ida->default_behavior_set = 0;
    }

    section = NULL;
    while (weston_config_next_section(config, &section, &name)) {
        struct db_elem *db_elem = NULL;

        if (strcmp(name, "desktop-app") != 0)
            continue;

        db_elem = calloc(1, sizeof *db_elem);
        if (db_elem == NULL) {
            weston_log("ivi-id-agent: No memory to allocate\n");
            goto ivi_failed;
        }

        wl_list_insert(&ida->app_list, &db_elem->link);

        weston_config_section_get_uint(section, "surface-id",
                         &db_elem->surface_id, INVALID_ID);

        if (db_elem->surface_id == INVALID_ID) {
            weston_log("ivi-id-agent: surface-id is not set in configuration\n");
            goto ivi_failed;
        }

        weston_config_section_get_string(section, "app-id",
                         &db_elem->cfg_app_id, NULL);
        weston_config_section_get_string(section, "app-title",
                         &db_elem->cfg_title, NULL);

        if (db_elem->cfg_app_id == NULL && db_elem->cfg_title == NULL) {
            weston_log("ivi-id-agent: Every parameter is NULL in app "
                    "configuration\n");
            goto ivi_failed;
        }

        if (check_config(db_elem, ida) == IVI_FAILED) {
            weston_log("ivi-id-agent: No valid config found, deinit...\n");
            goto ivi_failed;
        }
    }

    if(ida->default_behavior_set == 0 && wl_list_empty(&ida->app_list)) {
        weston_log("ivi-id-agent: No valid config found, deinit...\n");
        goto ivi_failed;
    }

    return IVI_SUCCEEDED;

ivi_failed:
    return IVI_FAILED;
}

WL_EXPORT int32_t
id_agent_module_init(struct weston_compositor *compositor,
        const struct ivi_layout_interface *interface)
{
    struct ivi_id_agent *ida = NULL;

    ida = calloc(1, sizeof *ida);
    if (ida == NULL) {
        weston_log("failed to allocate ivi_id_agent\n");
        goto ivi_failed;
    }

    ida->compositor = compositor;
    ida->interface = interface;
    ida->desktop_surface_configured.notify = desktop_surface_event_configure;
    ida->destroy_listener.notify = id_agent_module_deinit;
    ida->surface_removed.notify = surface_event_remove;

    wl_signal_add(&compositor->destroy_signal, &ida->destroy_listener);
    ida->interface->add_listener_configure_desktop_surface(
            &ida->desktop_surface_configured);
    interface->add_listener_remove_surface(&ida->surface_removed);

    wl_list_init(&ida->app_list);
    if(read_config(ida) != 0) {
        weston_log("ivi-id-agent: Read config failed\n");
        deinit(ida);
        goto ivi_failed;
    }

    return IVI_SUCCEEDED;

ivi_failed:
    return IVI_FAILED;
}

static int32_t
deinit(struct ivi_id_agent *ida)
{
    struct db_elem *db_elem;
    wl_list_for_each(db_elem, &ida->app_list, link) {
        free(db_elem->cfg_app_id);
        free(db_elem->cfg_title);
        free(db_elem);
    }

    wl_list_remove(&ida->desktop_surface_configured.link);
    wl_list_remove(&ida->destroy_listener.link);
    wl_list_remove(&ida->surface_removed.link);
    free(ida);

    return IVI_SUCCEEDED;
}
