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
 * limitations under the License.  *
 ****************************************************************************/

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/* copied from libinput-seat.h of weston-1.11.0 */
struct udev_input {
	struct libinput *libinput;
	struct wl_event_source *libinput_source;
	struct weston_compositor *compositor;
	int suspended;
};
/***********************************************/

/* copied from compositor-drm.h of weston-1.11.0 */
struct weston_drm_backend_output_config {
        struct weston_backend_output_config base;

        /** The pixel format to be used by the output. Valid values are:
         * - NULL - The format set at backend creation time will be used;
         * - "xrgb8888";
         * - "rgb565"
         * - "xrgb2101010"
         */
        char *gbm_format;
        /** The seat to be used by the output. Set to NULL to use the
         * default seat. */
        char *seat;
        /** The modeline to be used by the output. Refer to the documentation
         * of WESTON_DRM_BACKEND_OUTPUT_PREFERRED for details. */
        char *modeline;
};
/***********************************************/

/* copied from compositor-drm.c of weston-1.11.0 */
struct drm_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct udev *udev;
	struct wl_event_source *drm_source;

	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_drm_source;

	struct {
		int id;
		int fd;
		char *filename;
	} drm;
	struct gbm_device *gbm;
	uint32_t *crtcs;
	int num_crtcs;
	uint32_t crtc_allocator;
	uint32_t connector_allocator;
	struct wl_listener session_listener;
	uint32_t gbm_format;

	/* we need these parameters in order to not fail drmModeAddFB2()
	* due to out of bounds dimensions, and then mistakenly set
	* sprites_are_broken:
	*/
	uint32_t min_width, max_width;
	uint32_t min_height, max_height;
	int no_addfb2;

	struct wl_list sprite_list;
	int sprites_are_broken;
	int sprites_hidden;

	int cursors_are_broken;

	int use_pixman;

	uint32_t prev_state;

	struct udev_input input;

	int32_t cursor_width;
	int32_t cursor_height;

        /** Callback used to configure the outputs.
	 *
         * This function will be called by the backend when a new DRM
         * output needs to be configured.
         */
        enum weston_drm_backend_output_mode
	(*configure_output)(struct weston_compositor *compositor,
			    bool use_current_mode,
			    const char *name,
			    struct weston_drm_backend_output_config *output_config);
	bool use_current_mode;
};
/***********************************************/

/* copied from gl-renderer.h of weston-1.11.0 */
enum gl_renderer_border_side {
	GL_RENDERER_BORDER_TOP = 0,
	GL_RENDERER_BORDER_LEFT = 1,
	GL_RENDERER_BORDER_RIGHT = 2,
	GL_RENDERER_BORDER_BOTTOM = 3,
};
/***********************************************/

/* copied from gl-renderer.c of weston-1.11.0 */
struct egl_image {
	struct gl_renderer *renderer;
	EGLImageKHR image;
	int refcount;
};

struct gl_shader {
    GLuint program;
    GLuint vertex_shader, fragment_shader;
    GLint proj_uniform;
    GLint tex_uniforms[3];
    GLint alpha_uniform;
    GLint color_uniform;
    const char *vertex_source, *fragment_source;
};

struct gl_renderer {
        struct weston_renderer base;
        int fragment_shader_debug;
        int fan_debug;
        struct weston_binding *fragment_binding;
        struct weston_binding *fan_binding;

        EGLDisplay egl_display;
        EGLContext egl_context;
        EGLConfig egl_config;

        struct wl_array vertices;
        struct wl_array vtxcnt;

        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
        PFNEGLCREATEIMAGEKHRPROC create_image;
        PFNEGLDESTROYIMAGEKHRPROC destroy_image;

#ifdef EGL_EXT_swap_buffers_with_damage
        PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;
#endif

        PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window;

        int has_unpack_subimage;

        PFNEGLBINDWAYLANDDISPLAYWL bind_display;
        PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
        PFNEGLQUERYWAYLANDBUFFERWL query_buffer;
        int has_bind_display;

        int has_egl_image_external;

        int has_egl_buffer_age;

        int has_configless_context;

        int has_dmabuf_import;
        struct wl_list dmabuf_images;

        struct gl_shader texture_shader_rgba;
        struct gl_shader texture_shader_rgbx;
        struct gl_shader texture_shader_egl_external;
        struct gl_shader texture_shader_y_uv;
        struct gl_shader texture_shader_y_u_v;
        struct gl_shader texture_shader_y_xuxv;
        struct gl_shader invert_color_shader;
        struct gl_shader solid_shader;
        struct gl_shader *current_shader;

        struct wl_signal destroy_signal;
};

enum buffer_type {
        BUFFER_TYPE_NULL,
        BUFFER_TYPE_SOLID, /* internal solid color surfaces without a buffer */
        BUFFER_TYPE_SHM,
        BUFFER_TYPE_EGL
};

struct gl_surface_state {
        GLfloat color[4];
        struct gl_shader *shader;

        GLuint textures[3];
        int num_textures;
        int needs_full_upload;
        pixman_region32_t texture_damage;

        /* These are only used by SHM surfaces to detect when we need
         * to do a full upload to specify a new internal texture
         * format */
        GLenum gl_format;
        GLenum gl_pixel_type;

        struct egl_image* images[3];
        GLenum target;
        int num_images;

        struct weston_buffer_reference buffer_ref;
        enum buffer_type buffer_type;
        int pitch; /* in pixels */
        int height; /* in pixels */
        int y_inverted;

        struct weston_surface *surface;

        struct wl_listener surface_destroy_listener;
        struct wl_listener renderer_destroy_listener;
};

#define BUFFER_DAMAGE_COUNT 2

enum gl_border_status {
        BORDER_STATUS_CLEAN = 0,
        BORDER_TOP_DIRTY = 1 << GL_RENDERER_BORDER_TOP,
        BORDER_LEFT_DIRTY = 1 << GL_RENDERER_BORDER_LEFT,
        BORDER_RIGHT_DIRTY = 1 << GL_RENDERER_BORDER_RIGHT,
        BORDER_BOTTOM_DIRTY = 1 << GL_RENDERER_BORDER_BOTTOM,
        BORDER_ALL_DIRTY = 0xf,
        BORDER_SIZE_CHANGED = 0x10
};

struct gl_border_image {
        GLuint tex;
        int32_t width, height;
        int32_t tex_width;
        void *data;
};

struct gl_output_state {
        EGLSurface egl_surface;
        pixman_region32_t buffer_damage[BUFFER_DAMAGE_COUNT];
        int buffer_damage_index;
        enum gl_border_status border_damage[BUFFER_DAMAGE_COUNT];
        struct gl_border_image borders[4];
        enum gl_border_status border_status;

        struct weston_matrix output_matrix;
};
/***********************************************/

uint32_t get_flink_from_bo(struct gbm_bo *bo, struct gbm_device *gbm, struct drm_gem_flink *flink);
