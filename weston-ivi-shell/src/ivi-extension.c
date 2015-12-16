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
#include <string.h>
#include "ivi-controller-interface.h"
#include "ivi-extension.h"
#include "ivi-controller-impl.h"
#ifdef IVI_SHARE_ENABLE
#  include "ivi-share.h"
#endif

struct ivi_controller_shell {
    struct ivishell base;
    const struct ivi_controller_interface *interface;
};

int32_t
ivi_extension_commit_changes(struct ivishell *shell)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->commit_changes();
}

int32_t
ivi_extension_add_notification_create_surface(struct ivishell *shell,
					       surface_create_notification_func callback,
					       void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->add_notification_create_surface(callback, userdata);
}

int32_t
ivi_extension_add_notification_remove_surface(struct ivishell *shell,
					       surface_remove_notification_func callback,
					       void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->add_notification_remove_surface(callback, userdata);
}

int32_t
ivi_extension_add_notification_create_layer(struct ivishell *shell,
					     layer_create_notification_func callback,
					     void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->add_notification_create_layer(callback, userdata);
}

int32_t
ivi_extension_add_notification_remove_layer(struct ivishell *shell,
					     layer_remove_notification_func callback,
					     void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->add_notification_remove_layer(callback, userdata);
}

int32_t
ivi_extension_add_notification_configure_surface(struct ivishell *shell,
						  surface_configure_notification_func callback,
						  void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->add_notification_configure_surface(callback, userdata);
}

int32_t
ivi_extension_get_surfaces(struct ivishell *shell,
			    int32_t *pLength,
			    struct ivi_layout_surface ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_surfaces(pLength, ppArray);
}

uint32_t
ivi_extension_get_id_of_surface(struct ivishell *shell,
				 struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_id_of_surface(ivisurf);
}

struct ivi_layout_surface *
ivi_extension_get_surface_from_id(struct ivishell *shell, uint32_t id_surface)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_surface_from_id(id_surface);
}

const struct ivi_layout_surface_properties *
ivi_extension_get_properties_of_surface(struct ivishell *shell,
					 struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_properties_of_surface(ivisurf);
}

int32_t
ivi_extension_get_surfaces_on_layer(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer,
				     int32_t *pLength,
				     struct ivi_layout_surface ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_surfaces_on_layer(ivilayer, pLength, ppArray);
}

int32_t
ivi_extension_surface_set_visibility(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf,
				      bool newVisibility)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_visibility(ivisurf, newVisibility);
}

bool
ivi_extension_surface_get_visibility(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_visibility(ivisurf);
}

int32_t
ivi_extension_surface_set_opacity(struct ivishell *shell,
				   struct ivi_layout_surface *ivisurf,
				   wl_fixed_t opacity)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_opacity(ivisurf, opacity);
}

wl_fixed_t
ivi_extension_surface_get_opacity(struct ivishell *shell,
				   struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_opacity(ivisurf);
}

int32_t
ivi_extension_surface_set_source_rectangle(struct ivishell *shell,
					    struct ivi_layout_surface *ivisurf,
					    int32_t x, int32_t y,
					    int32_t width, int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_source_rectangle(ivisurf,
                                                                     x, y,
                                                                     width, height);
}

int32_t
ivi_extension_surface_set_destination_rectangle(struct ivishell *shell,
						 struct ivi_layout_surface *ivisurf,
						 int32_t x, int32_t y,
						 int32_t width, int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_destination_rectangle(ivisurf,
                                                                          x, y,
                                                                          width, height);
}

int32_t
ivi_extension_surface_set_position(struct ivishell *shell,
				    struct ivi_layout_surface *ivisurf,
				    int32_t dest_x, int32_t dest_y)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_position(ivisurf, dest_x, dest_y);
}

int32_t
ivi_extension_surface_get_position(struct ivishell *shell,
				    struct ivi_layout_surface *ivisurf,
				    int32_t *dest_x, int32_t *dest_y)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_position(ivisurf, dest_x, dest_y);
}

int32_t
ivi_extension_surface_set_dimension(struct ivishell *shell,
				     struct ivi_layout_surface *ivisurf,
				     int32_t dest_width, int32_t dest_height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_dimension(ivisurf, dest_width, dest_height);
}

int32_t
ivi_extension_surface_get_dimension(struct ivishell *shell,
				     struct ivi_layout_surface *ivisurf,
				     int32_t *dest_width, int32_t *dest_height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_dimension(ivisurf, dest_width, dest_height);
}

int32_t
ivi_extension_surface_set_orientation(struct ivishell *shell,
				       struct ivi_layout_surface *ivisurf,
				       enum wl_output_transform orientation)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_orientation(ivisurf, orientation);
}

enum wl_output_transform
ivi_extension_surface_get_orientation(struct ivishell *shell,
				       struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_orientation(ivisurf);
}

int32_t
ivi_extension_surface_set_content_observer(struct ivishell *shell,
					    struct ivi_layout_surface *ivisurf,
					    ivi_controller_surface_content_callback callback,
					    void* userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_content_observer(ivisurf, callback, userdata);
}

int32_t
ivi_extension_surface_add_notification(struct ivishell *shell,
					struct ivi_layout_surface *ivisurf,
					surface_property_notification_func callback,
					void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_add_notification(ivisurf, callback, userdata);
}

void
ivi_extension_surface_remove_notification(struct ivishell *shell,
					   struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    controller_shell->interface->surface_remove_notification(ivisurf);
}

struct weston_surface *
ivi_extension_surface_get_weston_surface(struct ivishell *shell,
					  struct ivi_layout_surface *ivisurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_weston_surface(ivisurf);
}

int32_t
ivi_extension_surface_set_transition(struct ivishell *shell,
				      struct ivi_layout_surface *ivisurf,
				      enum ivi_layout_transition_type type,
				      uint32_t duration)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_transition(ivisurf, type, duration);
}

int32_t
ivi_extension_surface_set_transition_duration(struct ivishell *shell,
					       struct ivi_layout_surface *ivisurf,
					       uint32_t duration)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_set_transition_duration(ivisurf, duration);
}

int32_t
ivi_extension_surface_dump(struct ivishell *shell,
			   struct weston_surface *surface,
			   void *target,
			   size_t size,
			   int32_t x,
			   int32_t y,
			   int32_t width,
			   int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_dump(surface, target, size, x, y, width, height);
}

int32_t
ivi_extension_surface_get_size(struct ivishell *shell,
				struct ivi_layout_surface *ivisurf,
				int32_t *width,
				int32_t *height,
				int32_t *stride)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->surface_get_size(ivisurf, width, height, stride);
}

struct ivi_layout_layer *
ivi_extension_layer_create_with_dimension(struct ivishell *shell,
					   uint32_t id_layer, int32_t width, int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_create_with_dimension(id_layer, width, height);
}

void
ivi_extension_layer_remove(struct ivishell *shell,
			    struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    controller_shell->interface->layer_remove(ivilayer);
}

int32_t
ivi_extension_get_layers(struct ivishell *shell,
			  int32_t *pLength, struct ivi_layout_layer ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_layers(pLength, ppArray);
}

uint32_t
ivi_extension_get_id_of_layer(struct ivishell *shell,
			       struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_id_of_layer(ivilayer);
}

struct ivi_layout_layer *
ivi_extension_get_layer_from_id(struct ivishell *shell, uint32_t id_layer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_layer_from_id(id_layer);
}

const struct ivi_layout_layer_properties *
ivi_extension_get_properties_of_layer(struct ivishell *shell,
				       struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_properties_of_layer(ivilayer);
}

int32_t
ivi_extension_get_layers_under_surface(struct ivishell *shell,
					struct ivi_layout_surface *ivisurf,
					int32_t *pLength,
					struct ivi_layout_layer ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_layers_under_surface(ivisurf,
                                                                 pLength,
                                                                 ppArray);
}

int32_t
ivi_extension_get_layers_on_screen(struct ivishell *shell,
				    struct ivi_layout_screen *iviscrn,
				    int32_t *pLength,
				    struct ivi_layout_layer ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_layers_on_screen(iviscrn, pLength, ppArray);
}

int32_t
ivi_extension_layer_set_visibility(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    bool newVisibility)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_visibility(ivilayer, newVisibility);
}

bool
ivi_extension_layer_get_visibility(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_get_visibility(ivilayer);
}

int32_t
ivi_extension_layer_set_opacity(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer,
				 wl_fixed_t opacity)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_opacity(ivilayer, opacity);
}

wl_fixed_t
ivi_extension_layer_get_opacity(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_get_opacity(ivilayer);
}

int32_t
ivi_extension_layer_set_source_rectangle(struct ivishell *shell,
					  struct ivi_layout_layer *ivilayer,
					  int32_t x, int32_t y,
					  int32_t width, int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_source_rectangle(ivilayer,
                                                                   x, y,
                                                                   width, height);
}

int32_t
ivi_extension_layer_set_destination_rectangle(struct ivishell *shell,
					       struct ivi_layout_layer *ivilayer,
					       int32_t x, int32_t y,
					       int32_t width, int32_t height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_destination_rectangle(ivilayer,
                                                                        x, y,
                                                                        width, height);
}

int32_t
ivi_extension_layer_set_position(struct ivishell *shell,
				  struct ivi_layout_layer *ivilayer,
				  int32_t dest_x, int32_t dest_y)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_position(ivilayer, dest_x, dest_y);
}

int32_t
ivi_extension_layer_get_position(struct ivishell *shell,
				  struct ivi_layout_layer *ivilayer,
				  int32_t *dest_x, int32_t *dest_y)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_get_position(ivilayer, dest_x, dest_y);
}

int32_t
ivi_extension_layer_set_dimension(struct ivishell *shell,
				   struct ivi_layout_layer *ivilayer,
				   int32_t dest_width, int32_t dest_height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_dimension(ivilayer, dest_width, dest_height);
}

int32_t
ivi_extension_layer_get_dimension(struct ivishell *shell,
				   struct ivi_layout_layer *ivilayer,
				   int32_t *dest_width, int32_t *dest_height)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_get_dimension(ivilayer, dest_width, dest_height);
}

int32_t
ivi_extension_layer_set_orientation(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer,
				     enum wl_output_transform orientation)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_orientation(ivilayer, orientation);
}

enum wl_output_transform
ivi_extension_layer_get_orientation(struct ivishell *shell,
				     struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_get_orientation(ivilayer);
}

int32_t
ivi_extension_layer_add_surface(struct ivishell *shell,
				 struct ivi_layout_layer *ivilayer,
				 struct ivi_layout_surface *addsurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_add_surface(ivilayer, addsurf);
}

void
ivi_extension_layer_remove_surface(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    struct ivi_layout_surface *remsurf)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    controller_shell->interface->layer_remove_surface(ivilayer, remsurf);
}

int32_t
ivi_extension_layer_set_render_order(struct ivishell *shell,
				      struct ivi_layout_layer *ivilayer,
				      struct ivi_layout_surface **pSurface,
				      int32_t number)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_render_order(ivilayer, pSurface, number);
}

int32_t
ivi_extension_layer_add_notification(struct ivishell *shell,
				      struct ivi_layout_layer *ivilayer,
				      layer_property_notification_func callback,
				      void *userdata)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_add_notification(ivilayer, callback, userdata);
}

void
ivi_extension_layer_remove_notification(struct ivishell *shell,
					 struct ivi_layout_layer *ivilayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    controller_shell->interface->layer_remove_notification(ivilayer);
}

int32_t
ivi_extension_layer_set_transition(struct ivishell *shell,
				    struct ivi_layout_layer *ivilayer,
				    enum ivi_layout_transition_type type,
				    uint32_t duration)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_transition(ivilayer, type, duration);
}

struct ivi_layout_screen *
ivi_extension_get_screen_from_id(struct ivishell *shell,
				  uint32_t id_screen)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_screen_from_id(id_screen);
}

int32_t
ivi_extension_get_screen_resolution(struct ivishell *shell,
				     struct ivi_layout_screen *iviscrn,
				     int32_t *pWidth,
				     int32_t *pHeight)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_screen_resolution(iviscrn, pWidth, pHeight);
}

int32_t
ivi_extension_get_screens(struct ivishell *shell,
			   int32_t *pLength, struct ivi_layout_screen ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_screens(pLength, ppArray);
}

int32_t
ivi_extension_get_screens_under_layer(struct ivishell *shell,
				       struct ivi_layout_layer *ivilayer,
				       int32_t *pLength,
				       struct ivi_layout_screen ***ppArray)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->get_screens_under_layer(ivilayer,
                                                                pLength,
                                                                ppArray);
}

int32_t
ivi_extension_screen_add_layer(struct ivishell *shell,
				struct ivi_layout_screen *iviscrn,
				struct ivi_layout_layer *addlayer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->screen_add_layer(iviscrn, addlayer);
}

int32_t
ivi_extension_screen_set_render_order(struct ivishell *shell,
				       struct ivi_layout_screen *iviscrn,
				       struct ivi_layout_layer **pLayer,
				       const int32_t number)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->screen_set_render_order(iviscrn, pLayer, number);
}

struct weston_output *
ivi_extension_screen_get_output(struct ivishell *shell,
				 struct ivi_layout_screen *iviscrn)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->screen_get_output(iviscrn);
}


void
ivi_extension_transition_move_layer_cancel(struct ivishell *shell,
					    struct ivi_layout_layer *layer)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    controller_shell->interface->transition_move_layer_cancel(layer);
}

int32_t
ivi_extension_layer_set_fade_info(struct ivishell *shell,
				   struct ivi_layout_layer* ivilayer,
				   uint32_t is_fade_in,
				   double start_alpha, double end_alpha)
{
    struct ivi_controller_shell *controller_shell = (struct ivi_controller_shell*)shell;

    return controller_shell->interface->layer_set_fade_info(ivilayer,
                                                            is_fade_in,
                                                            start_alpha, end_alpha);
}

static int
load_input_module(struct weston_compositor *ec,
                  const struct ivi_controller_interface *interface,
                  size_t interface_version)
{
    struct weston_config *config = ec->config;
    struct weston_config_section *section;
    char *input_module = NULL;

    int (*input_module_init)(struct weston_compositor *ec,
                             const struct ivi_controller_interface *interface,
                             size_t interface_version);

    section = weston_config_get_section(config, "ivi-shell", NULL, NULL);

    if (weston_config_section_get_string(section, "ivi-input-module",
                                         &input_module, NULL) < 0) {
        /* input events are handled by weston's default grabs */
        weston_log("ivi-controller: No ivi-input-module set\n");
        return 0;
    }

    input_module_init = weston_load_module(input_module, "input_controller_module_init");
    if (!input_module_init)
        return -1;

    if (input_module_init(ec, interface,
                          sizeof(struct ivi_controller_interface)) != 0) {
        weston_log("ivi-controller: Initialization of input module failes");
        return -1;
    }

    free(input_module);

    return 0;
}

WL_EXPORT int
controller_module_init(struct weston_compositor *compositor,
		       int *argc, char *argv[],
		       const struct ivi_controller_interface *interface,
		       size_t interface_version)
{
    struct ivi_controller_shell *controller_shell;
    (void)argc;
    (void)argv;

    controller_shell = malloc(sizeof *controller_shell);
    if (controller_shell == NULL)
        return -1;

    memset(controller_shell, 0, sizeof *controller_shell);

    controller_shell->interface = interface;

    init_ivi_shell(compositor, &controller_shell->base);

#ifdef IVI_SHARE_ENABLE
    if (setup_buffer_sharing(compositor, interface) < 0) {
        free(controller_shell);
        return -1;
    }
#endif

    if (setup_ivi_controller_server(compositor, &controller_shell->base)) {
        free(controller_shell);
        return -1;
    }

    if (load_input_module(compositor, interface, interface_version) < 0) {
        free(controller_shell);
        return -1;
    }

    return 0;
}
