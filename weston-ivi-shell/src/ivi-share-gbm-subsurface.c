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
#ifdef ENABLE_SHARE_SUBSURFACE

#include <assert.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <wayland-egl.h>
#include <fcntl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "ivi-share.h"
#include "ivi-share-gbm.h"

struct gbm_subcompositor
{
    struct gbm_surface *surface;
    struct gbm_bo *locked_bo;
    EGLSurface egl_surface;
    EGLContext egl_context;
    EGLDisplay egl_display;
    int32_t width;
    int32_t height;
    struct gl_output_state *output_state;
    int pos, tex_pos, tex_id;
    struct wl_list subsurface_state_list;
    GLint program;
    int is_initialized;
};

struct subsurface_state
{
    struct weston_surface *surface;
    uint32_t name;
    int32_t posx, posy;
    int32_t render_order;
    struct wl_list link;
};


static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC pfGLEglImageTargetTexture2DOES;

static const char *vert_shader_text =
    "attribute vec4 position;\n"
    "attribute vec2 texcoord;\n"
    "varying vec2 texcoord_out;\n"
    "void main() {\n"
    "    gl_Position = position;\n"
    "    texcoord_out = texcoord;\n"
    "}\n";

static const char *frag_shader_text =
    "precision mediump float;\n"
    "varying vec2 texcoord_out;\n"
    "uniform mediump sampler2D tex_unit;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex_unit, texcoord_out);\n"
    "}\n";

static const GLfloat vertices[][2] = {
    { 1.0, -1.0},
    { 1.0,  1.0},
    {-1.0,  1.0},
    {-1.0, -1.0}
};

static const GLfloat tex_coords[][2] = {
    {1.0, 1.0},
    {1.0, 0.0},
    {0.0, 0.0},
    {0.0, 1.0}
};

static void
check_GL_status(const char *msg)
{
    const char *emsg;
    int err = glGetError();

    switch (err) {
    case GL_NO_ERROR:
        return;
    #define ERR(tok) case tok: emsg = #tok; break;
    ERR(GL_INVALID_ENUM);
    ERR(GL_INVALID_VALUE);
    ERR(GL_INVALID_OPERATION);
    ERR(GL_OUT_OF_MEMORY);
    #undef ERR
    default:
        weston_log("%s: unknown GL error 0x%04x\n", msg, err);
        exit(1);
    }

    weston_log("%s: GL error 0x%04x\n", msg, err);
}

static struct gl_renderer *
get_renderer(struct weston_compositor *ec)
{
    return (struct gl_renderer *)ec->renderer;
}

static GLuint
create_shader(const char *source, GLenum type)
{
    GLuint s;
    char msg[512];
    GLint status;

    s = glCreateShader(type);

    glShaderSource(s, 1, (const char**)&source, NULL);

    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderInfoLog(s, sizeof msg, NULL, msg);
        weston_log("shader info: %s\n", msg);
        return GL_NONE;
    }

    return s;
}

static int
init_gl(struct ivi_share_nativesurface *native_surface)
{
    GLint vert, frag;
    GLint status;
    struct gbm_subcompositor *subcomp = native_surface->sub_compositor;

    pfGLEglImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
    eglGetProcAddress("glEGLImageTargetTexture2DOES");

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    vert = create_shader(vert_shader_text, GL_VERTEX_SHADER);
    frag = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);

    if (vert == 0 || frag == 0) {
        return -1;
    }

    subcomp->program = glCreateProgram();
    glAttachShader(subcomp->program, frag);
    glAttachShader(subcomp->program, vert);
    glLinkProgram(subcomp->program);

    glGetProgramiv(subcomp->program, GL_LINK_STATUS, &status);
    if (0 == status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(subcomp->program, sizeof(log), &len, log);
        weston_log("[ERR] linking:\n%s\n", log);
        return -1;
    }

    glUseProgram(subcomp->program);

    subcomp->pos = glGetAttribLocation(subcomp->program, "position");
    subcomp->tex_pos = glGetAttribLocation(subcomp->program, "texcoord");
    glLinkProgram(subcomp->program);

    glEnableVertexAttribArray(subcomp->pos);
    glEnableVertexAttribArray(subcomp->tex_pos);
    check_GL_status("init_gl");

    return 0;
}

static int
render_surface(struct weston_subsurface *sub,
               struct ivi_share_nativesurface *native_surface)
{
    struct weston_surface *surface = sub->surface;
    struct gl_surface_state *gs = (struct gl_surface_state*)surface->renderer_state;
    struct gbm_subcompositor *sub_compositor = native_surface->sub_compositor;
    struct gl_renderer *gr = get_renderer(surface->compositor);

    if (gs == NULL) {
        weston_log("gs is NULL\n");
        return 0;
    }

    glGenTextures(1, &sub_compositor->tex_id);
    glBindTexture(GL_TEXTURE_2D, sub_compositor->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glViewport(sub->position.x,
               native_surface->surface->height - surface->height - sub->position.y,
               surface->width,
               surface->height);

    glEnableVertexAttribArray(sub_compositor->pos);
    glEnableVertexAttribArray(sub_compositor->tex_pos);

    /*position*/
    glVertexAttribPointer(sub_compositor->pos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    /*texcoord*/
    glVertexAttribPointer(sub_compositor->tex_pos, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);

    glBindTexture(GL_TEXTURE_2D, sub_compositor->tex_id);

    pfGLEglImageTargetTexture2DOES(GL_TEXTURE_2D, gs->images[0]->image);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(sub_compositor->pos);
    glDisableVertexAttribArray(sub_compositor->tex_pos);
    glDeleteTextures(1, &sub_compositor->tex_id);
    check_GL_status("render_surface");

    return 0;
}

static int
fallback_format_for(uint32_t format)
{
    switch (format) {
    case GBM_FORMAT_XRGB8888:
        return GBM_FORMAT_ARGB8888;
    case GBM_FORMAT_XRGB2101010:
        return GBM_FORMAT_ARGB2101010;
    default:
        return 0;
    }
}

static int
match_config_to_visual(EGLDisplay egl_display,
                       EGLint visual_id,
                       EGLConfig *configs,
                       int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        EGLint id;

        if (!eglGetConfigAttrib(egl_display,
                                configs[i], EGL_NATIVE_VISUAL_ID,
                                &id))
            continue;

        if (id == visual_id) {
            return i;
        }
    }

    return -1;
}

static int
egl_choose_config(struct gl_renderer *gr, const EGLint *attribs,
                  const EGLint *visual_id, const int n_ids,
                  EGLConfig *config_out)
{
    EGLint count = 0;
    EGLint matched = 0;
    EGLConfig *configs;
    int i, config_index = -1;

    if (!eglGetConfigs(gr->egl_display, NULL, 0, &count) || count < 1) {
        weston_log("No EGL configs to choose from.\n");
        return -1;
    }
    configs = calloc(count, sizeof *configs);
    if (!configs)
        return -1;

    if (!eglChooseConfig(gr->egl_display, attribs, configs,
                         count, &matched) || !matched) {
        weston_log("No EGL configs with appropriate attributes.\n");
        goto out;
    }

    if (!visual_id)
        config_index = 0;

    for (i = 0; config_index == -1 && i < n_ids; i++)
        config_index = match_config_to_visual(gr->egl_display,
                                              visual_id[i],
                                              configs,
                                              matched);

    if (config_index != -1)
        *config_out = configs[config_index];

out:
    free(configs);
    if (config_index == -1)
        return -1;

    if (i > 1)
        weston_log("Unable to use first choice EGL config with id"
                   " 0x%x, succeeded with alternate id 0x%x.\n",
                    visual_id[0], visual_id[i - 1]);
    return 0;
}

int
setup_subcompositor(struct ivi_share_nativesurface *native_surface)
{
    if (native_surface->surface->width == 0 || native_surface->surface->height == 0) {
        weston_log("parent surface isn't correct size.\n");
        return -1;
    }

    if (native_surface->sub_compositor != NULL) {
        weston_log("already setup_subcompositor\n");
        return 0;
    }

    struct gbm_subcompositor *subcomp = calloc(1, sizeof *subcomp);
    if (subcomp == NULL) {
        weston_log("fails to allocate memory\n");
        return -1;
    }
    struct gl_renderer *gr = get_renderer(native_surface->surface->compositor);

    struct drm_backend *backend = (struct drm_backend *)native_surface->surface->compositor->backend;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };


    struct weston_output *output = NULL;
    wl_list_for_each(output, &native_surface->surface->compositor->output_list, link) {
        break; /* pick first one */
    }

    struct gl_output_state *go = output->renderer_state;
    subcomp->output_state = go;

    /* create new gbm_surface */
    subcomp->surface = gbm_surface_create(backend->gbm,
                                          native_surface->surface->width,
                                          native_surface->surface->height,
                                          GBM_FORMAT_ARGB8888,
                                          GBM_BO_USE_RENDERING);
    if (!subcomp->surface) {
        weston_log("failed to create gbm surface to composite subsurface\n");
        free(subcomp);
        return -1;
    }

    /* reuse existing egl_display */
    subcomp->egl_display = gr->egl_display;

    /* create new egl_config */
    EGLint rgba_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig egl_config;
    EGLint format[3] = {
        backend->gbm_format,
        fallback_format_for(backend->gbm_format),
        0,
    };
    int n_formats = 2;
    if (format[1])
        n_formats = 3;

    if (egl_choose_config(gr, rgba_attribs, format,
                          n_formats, &egl_config) == -1) {
        weston_log("failed to choose EGL config for output\n");
        return -1;
    }

    /* create new egl_surface */
    subcomp->egl_surface = eglCreateWindowSurface(subcomp->egl_display,
                                                  egl_config,
                                                  (EGLNativeWindowType)subcomp->surface,
                                                  NULL);
    if (subcomp->egl_surface == EGL_NO_SURFACE) {
        weston_log("failded to create egl surface\n");
        return -1;
    }

    /* create new egl_context */
    subcomp->egl_context = eglCreateContext(subcomp->egl_display, egl_config,
                                            EGL_NO_CONTEXT, context_attribs);
    if (subcomp->egl_context == EGL_NO_CONTEXT /*NULL*/) {
        weston_log("failded to create egl context\n");
        return -1;
    }

    subcomp->is_initialized = 0;

    struct weston_subsurface *sub;
    wl_list_init(&subcomp->subsurface_state_list);
    wl_list_for_each(sub, &native_surface->surface->subsurface_list, parent_link) {
        struct subsurface_state *state = calloc(1, sizeof *state);
        if (state == NULL) {
            weston_log("failed to allocate memory\n");
            return -1;
        }

        state->surface = sub->surface;
        state->name = 0;
        state->posx = sub->position.x;
        state->posy = sub->position.y;
        wl_list_insert(subcomp->subsurface_state_list.prev, &state->link);
    }

    native_surface->sub_compositor = subcomp;

    return 0;
}

void
destroy_subcompositor(struct ivi_share_nativesurface *native_surface)
{
    struct gbm_subcompositor *subcomp = native_surface->sub_compositor;

    if (subcomp) {
        eglDestroyContext(subcomp->egl_display, subcomp->egl_context);
        eglDestroySurface(subcomp->egl_display, subcomp->egl_surface);
        gbm_surface_destroy(subcomp->surface);
        struct subsurface_state *state, *state_next;
        wl_list_for_each_safe(state, state_next, &subcomp->subsurface_state_list, link) {
            wl_list_remove(&state->link);
            free(state);
        }
        subcomp->is_initialized = 0;
        free(subcomp);
        native_surface->sub_compositor = NULL;
   }
}

static uint32_t
update_subsurface_state(struct ivi_share_nativesurface *p_nativesurface)
{
    struct drm_backend *backend = (struct drm_backend *)p_nativesurface->surface->compositor->backend;

    struct gbm_subcompositor *sub_compositor = p_nativesurface->sub_compositor;
    struct weston_surface *surface = p_nativesurface->surface;
    struct weston_subsurface *sub;
    struct weston_buffer *buf;
    uint32_t status = IVI_SHAREBUFFER_NOT_AVAILABLE;
    int render_order = 0;
    struct wl_list pending_list;

    wl_list_init(&pending_list);

    wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
        uint32_t name;
        struct gbm_bo *bo;
        struct drm_gem_flink flink = {};

        if (sub->surface->buffer_ref.buffer == NULL) {
            return IVI_SHAREBUFFER_INVALID;
        }
        buf = sub->surface->buffer_ref.buffer;
        bo = gbm_bo_import(backend->gbm, GBM_BO_IMPORT_WL_BUFFER,
                           buf->legacy_buffer, GBM_BO_USE_SCANOUT);
        if (NULL == bo) {
            weston_log("Texture Sharing Failed to import gbm_bo\n");
            return IVI_SHAREBUFFER_INVALID;
        }
        uint32_t flink_ret;
        flink_ret = get_flink_from_bo(bo, backend->gbm, &flink);
        if (flink_ret == IVI_SHAREBUFFER_INVALID) {
            return IVI_SHAREBUFFER_INVALID;
        }

        gbm_bo_destroy(bo);

        struct subsurface_state *state;
        int found = 0;
        wl_list_for_each(state, &sub_compositor->subsurface_state_list, link) {
            if (sub->surface == state->surface) {
                wl_list_remove(&state->link);
                found = 1;
                break;
            }
        }

        if (found == 0) {
            if (sub->position.set == 0) {
                /* add new subsurface state */
                struct subsurface_state *new_state;
                new_state = calloc(1, sizeof *new_state);
                if (new_state == NULL) {
                    weston_log("failed to allocate memory");
                    return IVI_SHAREBUFFER_INVALID;
                }

                new_state->surface = sub->surface;
                new_state->name = flink.name;
                new_state->posx = sub->position.x;
                new_state->posy = sub->position.y;
                new_state->render_order = render_order;
                wl_list_insert(pending_list.prev, &new_state->link);
                status = IVI_SHAREBUFFER_DAMAGE;
            }
        } else {
            if (sub->position.set == 0) {
                name = flink.name;
                if (state->name != name) {
                    state->name = name;
                    status = IVI_SHAREBUFFER_DAMAGE;
                }
                if (state->posx != sub->position.x ||
                    state->posy != sub->position.y) {
                    state->posx = sub->position.x;
                    state->posy = sub->position.y;
                    status = IVI_SHAREBUFFER_DAMAGE;
                }
            }
            if (state->render_order != render_order) {
                state->render_order = render_order;
                status = IVI_SHAREBUFFER_DAMAGE;
            }
            wl_list_insert(pending_list.prev, &state->link);
        }
        ++render_order;
    }

    /* check if state is still remained */
    if (!wl_list_empty(&sub_compositor->subsurface_state_list)) {
        struct subsurface_state *state, *next;
        wl_list_for_each_safe(state, next,
                              &sub_compositor->subsurface_state_list, link) {
            wl_list_remove(&state->link);
            free(state);
        }
        status = IVI_SHAREBUFFER_DAMAGE;
    }

    wl_list_init(&sub_compositor->subsurface_state_list);
    wl_list_insert_list(&sub_compositor->subsurface_state_list, &pending_list);

    return status;
}

void
composite_subsurface(struct ivi_share_nativesurface *native_surface)
{
    struct gl_renderer *gr = get_renderer(native_surface->surface->compositor);
    struct weston_surface *surface = native_surface->surface;
    struct gbm_subcompositor *sub_compositor = native_surface->sub_compositor;
    struct weston_subsurface *sub;
    EGLBoolean ret;

    if (!sub_compositor->is_initialized) {
        if (init_gl(native_surface) < 0) {
            weston_log("[ERR] init_gl");
        }
        sub_compositor->is_initialized = 1;
    }
    glUseProgram(sub_compositor->program);

    wl_list_for_each_reverse(sub, &surface->subsurface_list, parent_link) {
        render_surface(sub, native_surface);
    }

    ret = eglSwapBuffers(sub_compositor->egl_display, sub_compositor->egl_surface);
    if (ret == EGL_FALSE) {
        weston_log("Failed in eglSwapBuffers\n");
        return;
    }
}

uint32_t
update_subsurfaces(struct ivi_share_nativesurface *p_nativesurface)
{
    uint32_t status = update_subsurface_state(p_nativesurface);

    if (status != IVI_SHAREBUFFER_DAMAGE) {
        return status;
    }

    struct drm_backend *backend = (struct drm_backend *)p_nativesurface->surface->compositor->backend;
    struct gbm_subcompositor *sub_compositor = p_nativesurface->sub_compositor;
    struct gl_renderer *gr = get_renderer(p_nativesurface->surface->compositor);
    struct gl_output_state *go = sub_compositor->output_state;
    EGLBoolean ret;

    struct gbm_bo *bo_sub;
    struct drm_gem_flink flink_sub = {};
    uint32_t flink_ret;

    ret = eglMakeCurrent(sub_compositor->egl_display, sub_compositor->egl_surface,
                         sub_compositor->egl_surface, sub_compositor->egl_context);
    assert(ret == EGL_TRUE);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    composite_subsurface(p_nativesurface);

    bo_sub = gbm_surface_lock_front_buffer(sub_compositor->surface);
    if (!bo_sub) {
        weston_log("failed to get drm_fb for bo\n");
        gbm_surface_release_buffer(sub_compositor->surface, bo_sub);
        return IVI_SHAREBUFFER_INVALID;
    }

    flink_ret = get_flink_from_bo(bo_sub, backend->gbm, &flink_sub);
    if (flink_ret == IVI_SHAREBUFFER_INVALID) {
        return IVI_SHAREBUFFER_INVALID;
    }

    p_nativesurface->name = flink_sub.name;
    p_nativesurface->width = gbm_bo_get_width(bo_sub);
    p_nativesurface->height = gbm_bo_get_height(bo_sub);
    p_nativesurface->stride = gbm_bo_get_stride(bo_sub);

    if (sub_compositor->locked_bo &&
        sub_compositor->locked_bo != bo_sub) {
            gbm_surface_release_buffer(sub_compositor->surface, sub_compositor->locked_bo);
    }
    sub_compositor->locked_bo = bo_sub;

    ret = eglMakeCurrent(gr->egl_display, go->egl_surface,
                         go->egl_surface, gr->egl_context);
    assert(ret == EGL_TRUE);

    return status;
}
#endif
