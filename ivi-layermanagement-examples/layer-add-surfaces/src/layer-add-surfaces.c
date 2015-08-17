/*
 * Copyright (C) 2015 Advanced Driver Information Technology Joint Venture GmbH
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>

#include "ilm_control.h"

t_ilm_uint screenWidth;
t_ilm_uint screenHeight;
t_ilm_uint layer;
pthread_mutex_t mutex;
static pthread_cond_t  waiterVariable = PTHREAD_COND_INITIALIZER;
static int number_of_surfaces;

static void configure_ilm_surface(t_ilm_uint id, t_ilm_uint width, t_ilm_uint height)
{
    ilm_surfaceSetDestinationRectangle(id, 0, 0, width, height);
    printf("SetDestinationRectangle: surface ID (%d), Width (%u), Height (%u)\n", id, width, height);
    ilm_surfaceSetSourceRectangle(id, 0, 0, width, height);
    printf("SetSourceRectangle     : surface ID (%d), Width (%u), Height (%u)\n", id, width, height);
    ilm_surfaceSetVisibility(id, ILM_TRUE);
    printf("SetVisibility          : surface ID (%d), ILM_TRUE\n", id);
    ilm_layerAddSurface(layer,id);
    printf("layerAddSurface        : surface ID (%d) is added to layer ID (%d)\n", id, layer);
    ilm_commitChanges();
    pthread_cond_signal( &waiterVariable );
}

static void surfaceCallbackFunction(t_ilm_uint id, struct ilmSurfaceProperties* sp, t_ilm_notification_mask m)
{
    if ((unsigned)m & ILM_NOTIFICATION_CONFIGURED)
    {
        configure_ilm_surface(id, sp->origSourceWidth, sp->origSourceHeight);
    }
}

static void callbackFunction(ilmObjectType object, t_ilm_uint id, t_ilm_bool created, void *user_data)
{
    (void)user_data;
    struct ilmSurfaceProperties sp;

    if (object == ILM_SURFACE) {
        if (created) {
            if (number_of_surfaces > 0) {
                number_of_surfaces--;
                printf("surface                : %d created\n",id);
                ilm_getPropertiesOfSurface(id, &sp);

                if ((sp.origSourceWidth != 0) && (sp.origSourceHeight !=0))
                {   // surface is already configured
                    configure_ilm_surface(id, sp.origSourceWidth, sp.origSourceHeight);
                } else {
                    // wait for configured event
                    ilm_surfaceAddNotification(id,&surfaceCallbackFunction);
                    ilm_commitChanges();
                }
            }
        }
        else if(!created)
            printf("surface: %d destroyed\n",id);
    } else if (object == ILM_LAYER) {
        if (created)
            printf("layer: %d created\n",id);
        else if(!created)
            printf("layer: %d destroyed\n",id);
    }
}

int main (int argc, const char * argv[])
{
    // Get command-line options
    if ( argc != 3) {
        printf("Call layer-add-surface <layerID> <number_of_surfaces>\n");
        return -1;
    }

    layer = strtol(argv[1], NULL, 0);

    number_of_surfaces = strtol(argv[2], NULL, 0);

    pthread_mutexattr_t a;
    if (pthread_mutexattr_init(&a) != 0)
    {
       return -1;
    }

    if (pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE) != 0)
    {
       pthread_mutexattr_destroy(&a);
       return -1;
    }

    if (pthread_mutex_init(&mutex, &a) != 0)
    {
        pthread_mutexattr_destroy(&a);
        fprintf(stderr, "failed to initialize pthread_mutex\n");
        return -1;
    }

    pthread_mutexattr_destroy(&a);

    struct ilmScreenProperties screenProperties;
    t_ilm_layer renderOrder[1];
    renderOrder[0] = layer;
    ilm_init();
    ilm_getPropertiesOfScreen(0, &screenProperties);
    screenWidth = screenProperties.screenWidth;
    screenHeight = screenProperties.screenHeight;
    ilm_layerCreateWithDimension(&layer, screenWidth, screenHeight);
    printf("CreateWithDimension: layer ID (%d), Width (%u), Height (%u)\n", layer, screenWidth, screenHeight);
    ilm_layerSetVisibility(layer,ILM_TRUE);
    printf("SetVisibility      : layer ID (%d), ILM_TRUE\n", layer);
    ilm_displaySetRenderOrder(0,renderOrder,1);
    ilm_commitChanges();
    ilm_registerNotification(callbackFunction, NULL);

    while (number_of_surfaces > 0) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait( &waiterVariable, &mutex);
    }

    ilm_unregisterNotification();
    ilm_destroy();

    return 0;
}
