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

/**
 * The ivi-layout library supports API set of controlling properties of
 * surface and layer which groups surfaces. An unique ID whose type is integer
 * is required to create surface and layer. With the unique ID, surface and
 * layer are identified to control them. The API set consists of APIs to control
 * properties of surface and layers about followings,
 * - visibility.
 * - opacity.
 * - clipping (x,y,width,height).
 * - position and size of it to be displayed.
 * - orientation per 90 degree.
 * - add or remove surfaces to a layer.
 * - order of surfaces/layers in layer/screen to be displayed.
 * - commit to apply property changes.
 * - notifications of property change.
 *
 * Management of surfaces and layers grouping these surfaces are common way in
 * In-Vehicle Infotainment system, which integrate several domains in one system.
 * A layer is allocated to a domain in order to control application surfaces
 * grouped to the layer all together.
 */

#ifndef _IVI_LAYOUT_EXPORT_H_
#define _IVI_LAYOUT_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "compositor.h"
#include "ivi-layout.h"

struct ivi_layout_layer;
struct ivi_layout_screen;

enum ivi_layout_notification_mask {
    IVI_NOTIFICATION_NONE        = 0,
    IVI_NOTIFICATION_OPACITY     = (1 << 1),
    IVI_NOTIFICATION_SOURCE_RECT = (1 << 2),
    IVI_NOTIFICATION_DEST_RECT   = (1 << 3),
    IVI_NOTIFICATION_DIMENSION   = (1 << 4),
    IVI_NOTIFICATION_POSITION    = (1 << 5),
    IVI_NOTIFICATION_ORIENTATION = (1 << 6),
    IVI_NOTIFICATION_VISIBILITY  = (1 << 7),
    IVI_NOTIFICATION_PIXELFORMAT = (1 << 8),
    IVI_NOTIFICATION_ADD         = (1 << 9),
    IVI_NOTIFICATION_REMOVE      = (1 << 10),
    IVI_NOTIFICATION_CONFIGURE   = (1 << 11),
    IVI_NOTIFICATION_ALL         = 0xFFFF
};

enum ivi_layout_transition_type{
    IVI_LAYOUT_TRANSITION_NONE,
    IVI_LAYOUT_TRANSITION_VIEW_DEFAULT,
    IVI_LAYOUT_TRANSITION_VIEW_DEST_RECT_ONLY,
    IVI_LAYOUT_TRANSITION_VIEW_FADE_ONLY,
    IVI_LAYOUT_TRANSITION_LAYER_FADE,
    IVI_LAYOUT_TRANSITION_LAYER_MOVE,
    IVI_LAYOUT_TRANSITION_LAYER_VIEW_ORDER,
    IVI_LAYOUT_TRANSITION_MAX,
};

typedef void(*shellWarningNotificationFunc)(uint32_t id_surface,
                                            enum ivi_layout_warning_flag warn,
                                            void *userdata);

typedef void(*layerPropertyNotificationFunc)(struct ivi_layout_layer *ivilayer,
                                            struct ivi_layout_LayerProperties*,
                                            enum ivi_layout_notification_mask mask,
                                            void *userdata);

typedef void(*surfacePropertyNotificationFunc)(struct ivi_layout_surface *ivisurf,
                                            struct ivi_layout_SurfaceProperties*,
                                            enum ivi_layout_notification_mask mask,
                                            void *userdata);

typedef void(*layerCreateNotificationFunc)(struct ivi_layout_layer *ivilayer,
                                            void *userdata);

typedef void(*layerRemoveNotificationFunc)(struct ivi_layout_layer *ivilayer,
                                            void *userdata);

typedef void(*surfaceCreateNotificationFunc)(struct ivi_layout_surface *ivisurf,
                                            void *userdata);

typedef void(*surfaceRemoveNotificationFunc)(struct ivi_layout_surface *ivisurf,
                                            void *userdata);

typedef void(*surfaceConfigureNotificationFunc)(struct ivi_layout_surface *ivisurf,
                                            void *userdata);

typedef void(*ivi_controller_surface_content_callback)(struct ivi_layout_surface *ivisurf,
                                            int32_t content,
                                            void *userdata);

int32_t
ivi_layout_addNotificationShellWarning(shellWarningNotificationFunc callback,
                                       void *userdata);

void
ivi_layout_removeNotificationShellWarning(shellWarningNotificationFunc callback,
                                          void *userdata);

/**
 * \brief to be called by ivi-shell in order to set initail view of
 * weston_surface.
 */
/*
struct weston_view *
ivi_layout_get_weston_view(struct ivi_layout_surface *surface);
*/

/**
 * \brief initialize ivi-layout
 */
/*
void
ivi_layout_initWithCompositor(struct weston_compositor *ec);
*/

/**
 * \brief register for notification when layer is created
 */
int32_t
ivi_layout_addNotificationCreateLayer(layerCreateNotificationFunc callback,
                                      void *userdata);

void
ivi_layout_removeNotificationCreateLayer(layerCreateNotificationFunc callback,
                                         void *userdata);

/**
 * \brief register for notification when layer is removed
 */
int32_t
ivi_layout_addNotificationRemoveLayer(layerRemoveNotificationFunc callback,
                                      void *userdata);

void
ivi_layout_removeNotificationRemoveLayer(layerRemoveNotificationFunc callback,
                                         void *userdata);

/**
 * \brief register for notification when surface is created
 */
int32_t
ivi_layout_addNotificationCreateSurface(surfaceCreateNotificationFunc callback,
                                        void *userdata);

void
ivi_layout_removeNotificationCreateSurface(surfaceCreateNotificationFunc callback,
                                           void *userdata);

/**
 * \brief register for notification when surface is removed
 */
int32_t
ivi_layout_addNotificationRemoveSurface(surfaceRemoveNotificationFunc callback,
                                        void *userdata);

void
ivi_layout_removeNotificationRemoveSurface(surfaceRemoveNotificationFunc callback,
                                           void *userdata);

/**
 * \brief register for notification when surface is configured
 */
int32_t
ivi_layout_addNotificationConfigureSurface(surfaceConfigureNotificationFunc callback,
                                           void *userdata);

void
ivi_layout_removeNotificationConfigureSurface(surfaceConfigureNotificationFunc callback,
                                              void *userdata);

/**
 * \brief get id of surface from ivi_layout_surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
uint32_t
ivi_layout_getIdOfSurface(struct ivi_layout_surface *ivisurf);

/**
 * \brief get id of layer from ivi_layout_layer
 *
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
uint32_t
ivi_layout_getIdOfLayer(struct ivi_layout_layer *ivilayer);

/**
 * \brief get ivi_layout_layer from id of layer
 *
 * \return (struct ivi_layout_layer *)
 *              if the method call was successful
 * \return NULL if the method call was failed
 */
struct ivi_layout_layer *
ivi_layout_getLayerFromId(uint32_t id_layer);

/**
 * \brief get ivi_layout_surface from id of surface
 *
 * \return (struct ivi_layout_surface *)
 *              if the method call was successful
 * \return NULL if the method call was failed
 */
struct ivi_layout_surface *
ivi_layout_getSurfaceFromId(uint32_t id_surface);

/**
 * \brief get ivi_layout_screen from id of screen
 *
 * \return (struct ivi_layout_screen *)
 *              if the method call was successful
 * \return NULL if the method call was failed
 */
struct ivi_layout_screen *
ivi_layout_getScreenFromId(uint32_t id_screen);

/**
 * \brief Get the screen resolution of a specific screen
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getScreenResolution(struct ivi_layout_screen *iviscrn,
                                  int32_t *pWidth,
                                  int32_t *pHeight);

/**
 * \brief register for notification on property changes of surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceAddNotification(struct ivi_layout_surface *ivisurf,
                                     surfacePropertyNotificationFunc callback,
                                     void *userdata);

/**
 * \brief remove notification on property changes of surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceRemoveNotification(struct ivi_layout_surface *ivisurf);

/**
 * \brief Create a surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
/*
struct ivi_layout_surface *
ivi_layout_surfaceCreate(struct weston_surface *wl_surface,
                            uint32_t id_surface);
*/

/**
 * \brief Set the native content of an application to be used as surface content.
 *        If wl_surface is NULL, remove the native content of a surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
/*
int32_t
ivi_layout_surfaceSetNativeContent(struct weston_surface *wl_surface,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t id_surface);
*/

/**
 * \brief Set an observer callback for surface content status change.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetContentObserver(struct ivi_layout_surface *ivisurf,
                                     ivi_controller_surface_content_callback callback,
                                     void* userdata);

/**
 * \brief initialize ivi_layout_surface dest/source width and height
 */
/*
void
ivi_layout_surfaceConfigure(struct ivi_layout_surface *ivisurf,
                               uint32_t width, uint32_t height);
*/

/**
 * \brief Remove a surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceRemove(struct ivi_layout_surface *ivisurf);

/**
 * \brief Set from which kind of devices the surface can accept input events.
 * By default, a surface accept input events from all kind of devices (keyboards, pointer, ...)
 * By calling this function, you can adjust surface preferences. Note that this function only
 * adjust the acceptance for the specified devices. Non specified are keept untouched.
 *
 * Typicall use case for this function is when dealing with pointer or touch events.
 * Those are normally dispatched to the first visible surface below the coordinate.
 * If you want a different behavior (i.e. forward events to an other surface below the coordinate,
 * you can set all above surfaces to refuse input events)
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_UpdateInputEventAcceptanceOn(struct ivi_layout_surface *ivisurf,
                                           int32_t devices,
                                           int32_t acceptance);

/**
 * \brief  Get the layer properties
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getPropertiesOfLayer(struct ivi_layout_layer *ivilayer,
                struct ivi_layout_LayerProperties *pLayerProperties);

/**
 * \brief  Get the number of hardware layers of a screen
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getNumberOfHardwareLayers(uint32_t id_screen,
                                        int32_t *pNumberOfHardwareLayers);

/**
 * \brief Get the screens
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getScreens(int32_t *pLength, struct ivi_layout_screen ***ppArray);

/**
 * \brief Get the screens under the given layer
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getScreensUnderLayer(struct ivi_layout_layer *ivilayer,
                                   int32_t *pLength,
                                   struct ivi_layout_screen ***ppArray);

/**
 * \brief Get all Layers which are currently registered and managed by the services
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getLayers(int32_t *pLength, struct ivi_layout_layer ***ppArray);

/**
 * \brief Get all Layers of the given screen
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getLayersOnScreen(struct ivi_layout_screen *iviscrn,
                                int32_t *pLength,
                                struct ivi_layout_layer ***ppArray);

/**
 * \brief Get all Layers under the given surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getLayersUnderSurface(struct ivi_layout_surface *ivisurf,
                                    int32_t *pLength,
                                    struct ivi_layout_layer ***ppArray);

/**
 * \brief Get all Surfaces which are currently registered and managed by the services
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getSurfaces(int32_t *pLength, struct ivi_layout_surface ***ppArray);

/**
 * \brief Get all Surfaces which are currently registered to a given layer and are managed by the services
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getSurfacesOnLayer(struct ivi_layout_layer *ivilayer,
                                 int32_t *pLength,
                                 struct ivi_layout_surface ***ppArray);

/**
 * \brief Create a layer which should be managed by the service
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
struct ivi_layout_layer *
ivi_layout_layerCreateWithDimension(uint32_t id_layer,
                                       int32_t width, int32_t height);

/**
 * \brief Removes a layer which is currently managed by the service
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerRemove(struct ivi_layout_layer *ivilayer);

/**
 * \brief Get the current type of the layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetType(struct ivi_layout_layer *ivilayer,
                           int32_t *pLayerType);

/**
 * \brief Set the visibility of a layer. If a layer is not visible, the layer and its
 * surfaces will not be rendered.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetVisibility(struct ivi_layout_layer *ivilayer,
                                 int32_t newVisibility);

/**
 * \brief Get the visibility of a layer. If a layer is not visible, the layer and its
 * surfaces will not be rendered.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetVisibility(struct ivi_layout_layer *ivilayer,
                                 int32_t *pVisibility);

/**
 * \brief Set the opacity of a layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetOpacity(struct ivi_layout_layer *ivilayer, float opacity);

/**
 * \brief Get the opacity of a layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetOpacity(struct ivi_layout_layer *ivilayer, float *pOpacity);

/**
 * \brief Set the area of a layer which should be used for the rendering.
 *        Only this part will be visible.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetSourceRectangle(struct ivi_layout_layer *ivilayer,
                                      int32_t x, int32_t y,
                                      int32_t width, int32_t height);

/**
 * \brief Set the destination area on the display for a layer.
 *        The layer will be scaled and positioned to this rectangle for rendering
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetDestinationRectangle(struct ivi_layout_layer *ivilayer,
                                           int32_t x, int32_t y,
                                           int32_t width, int32_t height);

/**
 * \brief Get the horizontal and vertical dimension of the layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetDimension(struct ivi_layout_layer *ivilayer,
                                int32_t *pDimension);

/**
 * \brief Set the horizontal and vertical dimension of the layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetDimension(struct ivi_layout_layer *ivilayer,
                                int32_t *pDimension);

/**
 * \brief Get the horizontal and vertical position of the layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetPosition(struct ivi_layout_layer *ivilayer,
                               int32_t *pPosition);

/**
 * \brief Sets the horizontal and vertical position of the layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetPosition(struct ivi_layout_layer *ivilayer,
                               int32_t *pPosition);

/**
 * \brief Sets the orientation of a layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetOrientation(struct ivi_layout_layer *ivilayer,
                                  int32_t orientation);

/**
 * \brief Gets the orientation of a layer.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetOrientation(struct ivi_layout_layer *ivilayer,
                                  int32_t *pOrientation);

/**
 * \brief Sets the color value which defines the transparency value.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetChromaKey(struct ivi_layout_layer *ivilayer,
                                int32_t* pColor);

/**
 * \brief Sets render order of surfaces within one layer
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerSetRenderOrder(struct ivi_layout_layer *ivilayer,
                                  struct ivi_layout_surface **pSurface,
                                  int32_t number);

/**
 * \brief Get the capabilities of a layer
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerGetCapabilities(struct ivi_layout_layer *ivilayer,
                                   int32_t *pCapabilities);

/**
 * \brief Get the possible capabilities of a layertype
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerTypeGetCapabilities(int32_t layerType,
                                       int32_t *pCapabilities);

/**
 * \brief Create the logical surface, which has no native buffer associated
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceInitialize(struct ivi_layout_surface **pSurface);

/**
 * \brief Set the visibility of a surface.
 *        If a surface is not visible it will not be rendered.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetVisibility(struct ivi_layout_surface *ivisurf,
                                   int32_t newVisibility);

/**
 * \brief Get the visibility of a surface.
 *        If a surface is not visible it will not be rendered.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetVisibility(struct ivi_layout_surface *ivisurf,
                                   int32_t *pVisibility);

/**
 * \brief Set the opacity of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetOpacity(struct ivi_layout_surface *ivisurf,
                                float opacity);

/**
 * \brief Get the opacity of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetOpacity(struct ivi_layout_surface *ivisurf,
                                float *pOpacity);

/**
 * \brief Set the keyboard focus on a certain surface
 * To receive keyboard events, 2 conditions must be fulfilled:
 *  1- The surface must accept events from keyboard. See ilm_UpdateInputEventAcceptanceOn
 *  2- The keyboard focus must be set on that surface
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_SetKeyboardFocusOn(struct ivi_layout_surface *ivisurf);

/**
 * \brief Get the indentifier of the surface which hold the keyboard focus
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_GetKeyboardFocusSurfaceId(struct ivi_layout_surface **pSurfaceId);

/**
 * \brief Set the destination area of a surface within a layer for rendering.
 *        The surface will be scaled to this rectangle for rendering.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetDestinationRectangle(struct ivi_layout_surface *ivisurf,
                                             int32_t x, int32_t y,
                                             int32_t width, int32_t height);

/**
 * \brief Set the horizontal and vertical dimension of the surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetDimension(struct ivi_layout_surface *ivisurf,
                                  int32_t *pDimension);

/**
 * \brief Get the horizontal and vertical dimension of the surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetDimension(struct ivi_layout_surface *ivisurf,
                                  int32_t *pDimension);

/**
 * \brief Sets the horizontal and vertical position of the surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetPosition(struct ivi_layout_surface *ivisurf,
                                 int32_t *pPosition);

/**
 * \brief Get the horizontal and vertical position of the surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetPosition(struct ivi_layout_surface *ivisurf,
                                 int32_t *pPosition);

/**
 * \brief Sets the orientation of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetOrientation(struct ivi_layout_surface *ivisurf,
                                    int32_t orientation);

/**
 * \brief Gets the orientation of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetOrientation(struct ivi_layout_surface *ivisurf,
                                    int32_t *pOrientation);

/**
 * \brief Gets the pixelformat of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceGetPixelformat(struct ivi_layout_layer *ivisurf,
                                    int32_t *pPixelformat);

/**
 * \brief Sets the color value which defines the transparency value of a surface.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetChromaKey(struct ivi_layout_surface *ivisurf,
                                  int32_t* pColor);

/**
 * \brief Add a layer to a screen which is currently managed by the service
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_screenAddLayer(struct ivi_layout_screen *iviscrn,
                             struct ivi_layout_layer *addlayer);

/**
 * \brief Sets render order of layers on a display
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_screenSetRenderOrder(struct ivi_layout_screen *iviscrn,
                                   struct ivi_layout_layer **pLayer,
                                   const int32_t number);

/**
 * \brief Enable or disable a rendering optimization
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_SetOptimizationMode(uint32_t id, int32_t mode);

/**
 * \brief Get the current enablement for an optimization
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_GetOptimizationMode(uint32_t id, int32_t *pMode);

/**
 * \brief register for notification on property changes of layer
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerAddNotification(struct ivi_layout_layer *ivilayer,
                                   layerPropertyNotificationFunc callback,
                                   void *userdata);

/**
 * \brief remove notification on property changes of layer
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerRemoveNotification(struct ivi_layout_layer *ivilayer);

/**
 * \brief Get the surface properties
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_getPropertiesOfSurface(struct ivi_layout_surface *ivisurf,
                struct ivi_layout_SurfaceProperties *pSurfaceProperties);

/**
 * \brief Add a surface to a layer which is currently managed by the service
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerAddSurface(struct ivi_layout_layer *ivilayer,
                              struct ivi_layout_surface *addsurf);

/**
 * \brief Removes a surface from a layer which is currently managed by the service
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_layerRemoveSurface(struct ivi_layout_layer *ivilayer,
                                 struct ivi_layout_surface *remsurf);

/**
 * \brief Set the area of a surface which should be used for the rendering.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_surfaceSetSourceRectangle(struct ivi_layout_surface *ivisurf,
                                        int32_t x, int32_t y,
                                        int32_t width, int32_t height);

/**
 * \brief get weston_output from ivi_layout_screen.
 *
 * \return (struct weston_screen *)
 *              if the method call was successful
 * \return NULL if the method call was failed
 */
struct weston_output *
ivi_layout_screenGetOutput(struct ivi_layout_screen *);

struct weston_surface *
ivi_layout_surfaceGetWestonSurface(struct ivi_layout_surface *ivisurf);

int32_t
ivi_layout_surfaceGetSize(struct ivi_layout_surface *ivisurf, int32_t *width, int32_t *height, int32_t *stride);

int32_t
ivi_layout_layerSetTransition(struct ivi_layout_layer *ivilayer,
                              enum ivi_layout_transition_type type,
                              uint32_t duration);

int32_t
ivi_layout_layerSetFadeInfo(struct ivi_layout_layer* layer,
                                    uint32_t is_fade_in,
                                    double start_alpha, double end_alpha);

int32_t
ivi_layout_surfaceSetTransition(struct ivi_layout_surface *ivisurf,
                                enum ivi_layout_transition_type type,
                                uint32_t duration);

int32_t
ivi_layout_surfaceSetTransitionDuration(struct ivi_layout_surface *ivisurf,uint32_t duration);

/**
 * \brief Commit all changes and execute all enqueued commands since last commit.
 *
 * \return  0 if the method call was successful
 * \return -1 if the method call was failed
 */
int32_t
ivi_layout_commitChanges(void);

#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _IVI_LAYOUT_EXPORT_H_ */
