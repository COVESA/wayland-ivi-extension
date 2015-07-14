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

static void callbackFunction(ilmObjectType object, t_ilm_uint id, t_ilm_bool created, void *user_data)
{
    (void)user_data;
    struct ilmSurfaceProperties sp;

    if (object == ILM_SURFACE) {
        if (created) {
            printf("surface: %d created\n",id);
            ilm_getPropertiesOfSurface(id, &sp);
            ilm_surfaceSetDestinationRectangle(id, 0, 0, sp.sourceWidth, sp.sourceHeight);
            ilm_surfaceSetSourceRectangle(id, 0, 0, sp.sourceWidth, sp.sourceHeight);
            ilm_surfaceSetVisibility(id, ILM_TRUE);
            ilm_layerAddSurface(layer,id);
            ilm_commitChanges();
            pthread_cond_signal( &waiterVariable );
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

    int number_of_surfaces = strtol(argv[2], NULL, 0);

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
    ilm_layerSetVisibility(layer,ILM_TRUE);
    ilm_displaySetRenderOrder(0,renderOrder,1);
    ilm_registerNotification(callbackFunction, NULL);
    ilm_commitChanges();

    while (number_of_surfaces > 0) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait( &waiterVariable, &mutex);
        number_of_surfaces--;
    }

    ilm_unregisterNotification();
    ilm_destroy();

    return 0;
}
