/***************************************************************************
*
* Copyright 2010,2011 BMW Car IT GmbH
* Copyright (C) 2012 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
*
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
#ifndef _ILM_TYPES_H_
#define _ILM_TYPES_H_

#include "ilm_platform.h"

/**
 * convenience macro to access single bits of a bitmask
 */
#define ILM_BIT(x) (1 << (x))

/**
 * invalid ID does not refer to an acutaly object id.
 */
#define INVALID_ID 0xFFFFFFFF

/**
 * \brief Represent the logical true value
 * \ingroup ilmClient
 **/
#define ILM_TRUE     1u

/**
 * \brief Represent the logical false value
 * \ingroup ilmClient
 **/
#define ILM_FALSE     0u

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

/* Deprecated attribute */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_DEPRECATED __attribute__ ((deprecated))
#else
#define ILM_DEPRECATED
#endif

/**
 * \brief Enumeration on possible error codes
 * \ingroup ilmClient
 **/
typedef enum e_ilmErrorTypes
{
    ILM_SUCCESS = 0,                       /*!< ErrorCode if the method call was successful */
    ILM_FAILED = 1,                        /*!< ErrorCode if the method call has failed */
    ILM_ERROR_INVALID_ARGUMENTS = 2,       /*!< ErrorCode if the method was called with invalid arguments */
    ILM_ERROR_ON_CONNECTION = 3,           /*!< ErrorCode if connection error has occured */
    ILM_ERROR_RESOURCE_ALREADY_INUSE = 4,  /*!< ErrorCode if resource is already in use */
    ILM_ERROR_RESOURCE_NOT_FOUND = 5,      /*!< ErrorCode if resource was not found */
    ILM_ERROR_NOT_IMPLEMENTED = 6,         /*!< ErrorCode if feature is not implemented */
    ILM_ERROR_UNEXPECTED_MESSAGE = 7       /*!< ErrorCode if received message has unexpected type */
} ilmErrorTypes;

/**
 * \brief Macro to translate error codes into error description strings
 * \ingroup ilmClient
 **/
#define ILM_ERROR_STRING(x)                                                   \
    ( (x) == ILM_SUCCESS                      ? "success"                     \
    : (x) == ILM_FAILED                       ? "failed"                      \
    : (x) == ILM_ERROR_INVALID_ARGUMENTS      ? "invalid arguments provided"  \
    : (x) == ILM_ERROR_ON_CONNECTION          ? "connection error"            \
    : (x) == ILM_ERROR_RESOURCE_ALREADY_INUSE ? "resource is already in use"  \
    : (x) == ILM_ERROR_RESOURCE_NOT_FOUND     ? "resource was not found"      \
    : (x) == ILM_ERROR_NOT_IMPLEMENTED        ? "feature is not implemented"  \
    : (x) == ILM_ERROR_UNEXPECTED_MESSAGE     ? "unexpected message received" \
    : "unknown error code"                                                    )


/**
 * \brief Enumeration for supported pixelformats
 * \ingroup ilmClient
 **/
typedef enum e_ilmPixelFormat
{
    ILM_PIXELFORMAT_R_8 = 0,           /*!< Pixelformat value, to describe a 8 bit luminance surface */
    ILM_PIXELFORMAT_RGB_888 = 1,       /*!< Pixelformat value, to describe a 24 bit rgb surface */
    ILM_PIXELFORMAT_RGBA_8888 = 2,      /*!< Pixelformat value, to describe a 24 bit rgb surface with 8 bit alpha */
    ILM_PIXELFORMAT_RGB_565 = 3,       /*!< Pixelformat value, to describe a 16 bit rgb surface */
    ILM_PIXELFORMAT_RGBA_5551 = 4,     /*!< Pixelformat value, to describe a 16 bit rgb surface, with binary mask */
    ILM_PIXELFORMAT_RGBA_6661 = 5,     /*!< Pixelformat value, to describe a 18 bit rgb surface, with binars mask */
    ILM_PIXELFORMAT_RGBA_4444 = 6,     /*!< Pixelformat value, to describe a 12 bit rgb surface, with 4 bit alpha */
    ILM_PIXEL_FORMAT_UNKNOWN = 7       /*!< Pixelformat not known */
} ilmPixelFormat;

/**
 * \brief Enumeration for supported graphical objects
 * \ingroup ilmControl
 **/
typedef enum e_ilmObjectType
{
    ILM_SURFACE = 0,                   /*!< Surface Object Type */
    ILM_LAYER = 1                      /*!< Layer Object Type */
} ilmObjectType;

/**
 * \brief Enumeration for supported surface types
 * \ingroup ilmControl
 **/
typedef enum e_ilmSurfaceType
{
    ILM_SURFACETYPE_RESTRICTED = 0,                       /*!< SurfaceType value, to describe an IVI type surface*/
    ILM_SURFACETYPE_DESKTOP = 1,                     /*!< SurfaceType value, to describe a desktop compatible surface*/
} ilmSurfaceType;

/**
 * \brief Identifier of different input device types. Can be used as a bitmask.
 * \ingroup ilmClient
 */
typedef unsigned int ilmInputDevice;
#define ILM_INPUT_DEVICE_KEYBOARD   ((ilmInputDevice) 1 << 0)
#define ILM_INPUT_DEVICE_POINTER    ((ilmInputDevice) 1 << 1)
#define ILM_INPUT_DEVICE_TOUCH      ((ilmInputDevice) 1 << 2)
#define ILM_INPUT_DEVICE_ALL        ((ilmInputDevice) ~0)

/**
 * \brief Typedef for representing a layer
 * \ingroup ilmClient
 **/
typedef t_ilm_uint     t_ilm_layer;

/**
 * \brief Typedef for representing a surface
 * \ingroup ilmClient
 **/
typedef t_ilm_uint     t_ilm_surface;

/**
 * \brief Typedef for representing a display number
 * \ingroup ilmClient
 **/
typedef t_ilm_uint     t_ilm_display;

/**
 * \brief Typedef for representing a native display
 * \ingroup ilmCommon
 **/
typedef t_ilm_ulong    t_ilm_nativedisplay;

/**
 * \brief Typedef for representing a native window handle
 * \ingroup ilmClient
 **/
typedef t_ilm_ulong    t_ilm_nativehandle;

/**
 * \brief Typedef for representing a ascii string
 * \ingroup ilmClient
 **/
typedef t_ilm_char* t_ilm_string;

/**
 * \brief Typedef for representing a const ascii string
 * \ingroup ilmClient
 **/
typedef t_ilm_const_char* t_ilm_const_string;

/**
 * \brief Typedef for representing a the surface properties structure
 * \ingroup ilmClient
 **/
struct ilmSurfaceProperties
{
    t_ilm_float opacity;                    /*!< opacity value of the surface */
    t_ilm_uint sourceX;                     /*!< x source position value of the surface */
    t_ilm_uint sourceY;                     /*!< y source position value of the surface */
    t_ilm_uint sourceWidth;                 /*!< source width value of the surface */
    t_ilm_uint sourceHeight;                /*!< source height value of the surface */
    t_ilm_uint origSourceWidth;             /*!< original source width value of the surface */
    t_ilm_uint origSourceHeight;            /*!< original source height value of the surface */
    t_ilm_uint destX;                       /*!< x destination position value of the surface */
    t_ilm_uint destY;                       /*!< y desitination position value of the surface */
    t_ilm_uint destWidth;                   /*!< destination width value of the surface */
    t_ilm_uint destHeight;                  /*!< destination height value of the surface */
    t_ilm_bool visibility;                  /*!< visibility value of the surface */
    t_ilm_uint frameCounter;                /*!< already rendered frames of surface */
    t_ilm_uint drawCounter;                 /*!< content updates of surface */
    t_ilm_uint updateCounter;               /*!< content updates of surface */
    t_ilm_int  creatorPid;                  /*!< process id of application that created this surface */
    ilmInputDevice focus;                   /*!< bitmask of every type of device that this surface has focus in */
};

/**
 * \brief Typedef for representing a the layer properties structure
 * \ingroup ilmControl
 **/
struct ilmLayerProperties
{
    t_ilm_float opacity;         /*!< opacity value of the layer */
    t_ilm_uint sourceX;          /*!< x source position value of the layer */
    t_ilm_uint sourceY;          /*!< y source position value of the layer */
    t_ilm_uint sourceWidth;      /*!< source width value of the layer */
    t_ilm_uint sourceHeight;     /*!< source height value of the layer */
    t_ilm_uint destX;            /*!< x destination position value of the layer */
    t_ilm_uint destY;            /*!< y desitination position value of the layer */
    t_ilm_uint destWidth;        /*!< destination width value of the layer */
    t_ilm_uint destHeight;       /*!< destination height value of the layer */
    t_ilm_bool visibility;       /*!< visibility value of the layer */
};

/**
 * \brief Typedef for representing a the screen properties structure
 * \ingroup ilmControl
 **/
struct ilmScreenProperties
{
    t_ilm_uint layerCount;          /*!< number of layers displayed on the screen */
    t_ilm_layer* layerIds;          /*!< array of layer ids */
    t_ilm_uint screenWidth;         /*!< width value of screen in pixels */
    t_ilm_uint screenHeight;        /*!< height value of screen in pixels */
};

/**
 * enum representing the possible flags for changed properties in notification callbacks.
 */
typedef enum
{
    ILM_NOTIFICATION_VISIBILITY = ILM_BIT(1),
    ILM_NOTIFICATION_OPACITY = ILM_BIT(2),
    ILM_NOTIFICATION_ORIENTATION = ILM_BIT(3),
    ILM_NOTIFICATION_SOURCE_RECT = ILM_BIT(4),
    ILM_NOTIFICATION_DEST_RECT = ILM_BIT(5),
    ILM_NOTIFICATION_CONTENT_AVAILABLE = ILM_BIT(6),
    ILM_NOTIFICATION_CONTENT_REMOVED = ILM_BIT(7),
    ILM_NOTIFICATION_CONFIGURED = ILM_BIT(8),
    ILM_NOTIFICATION_ALL = 0xffff
} t_ilm_notification_mask;

/**
 * Typedef for notification callback on property changes of a layer
 */
typedef void(*layerNotificationFunc)(t_ilm_layer layer,
                                        struct ilmLayerProperties*,
                                        t_ilm_notification_mask mask);

/**
 * Typedef for notification callback on property changes of a surface
 */
typedef void(*surfaceNotificationFunc)(t_ilm_surface surface,
                                        struct ilmSurfaceProperties*,
                                        t_ilm_notification_mask mask);

/**
 * Typedef for notification callback on object creation/deletion
 */
typedef void(*notificationFunc)(ilmObjectType object,
                                        t_ilm_uint id,
                                        t_ilm_bool created,
                                        void* user_data);

#endif /* _ILM_TYPES_H_*/
