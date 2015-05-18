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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ivi-input-client-protocol.h"
#include "ilm_input.h"
#include "ilm_control_platform.h"


/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define ILM_EXPORT __attribute__ ((visibility("default")))
#else
#define ILM_EXPORT
#endif

extern struct ilm_control_context ilm_context;

ILM_EXPORT ilmErrorTypes
ilm_setInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint num_seats,
                         t_ilm_string *seats)
{
    struct ilm_control_context *ctx;
    int i;
    struct surface_context *surface_ctx = NULL;
    struct accepted_seat *accepted_seat;
    struct seat_context *seat;
    int surface_found = 0;
    int seat_found = 0;

    if ((seats == NULL) && (num_seats != 0)) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();

    wl_list_for_each(surface_ctx, &ctx->wl.list_surface, link) {
        if (surface_ctx->id_surface == surfaceID) {
            surface_found = 1;
            break;
        }
    }

    if (!surface_found) {
        fprintf(stderr, "surface ID %d not found\n", surfaceID);
        release_instance();
        return ILM_FAILED;
    }

    for(i = 0; i < num_seats; i++) {
        wl_list_for_each(seat, &ctx->wl.list_seat, link) {
            if (strcmp(seat->seat_name, seats[i]) == 0)
                    seat_found = 1;
        }

        if (!seat_found) {
            fprintf(stderr, "seat: %s not found\n", seats[i]);
            release_instance();
            return ILM_FAILED;
        }

        seat_found = 0;
    }
    /* Send events to add input acceptance for every seat in 'seats', but
     * not on the surface's list */
    for(i = 0; i < num_seats; i++) {
        int has_seat = 0;

        wl_list_for_each(accepted_seat, &surface_ctx->list_accepted_seats,
                         link) {
            if (strcmp(accepted_seat->seat_name, seats[i]) == 0)
                has_seat = 1;
        }

        if (!has_seat) {
            ivi_input_set_input_acceptance(ctx->wl.input_controller,
                                                      surfaceID, seats[i],
                                                      ILM_TRUE);
        }
    }

    /* Send events to remove input acceptance for every seat on the surface's
     * list but not in 'seats' */
    wl_list_for_each(accepted_seat, &surface_ctx->list_accepted_seats, link) {
        int has_seat = 0;
        for (i = 0; i < num_seats; i++) {
            if (strcmp(accepted_seat->seat_name, seats[i]) == 0)
                has_seat = 1;
        }
        if (!has_seat)
            ivi_input_set_input_acceptance(ctx->wl.input_controller,
                                                      surfaceID,
                                                      accepted_seat->seat_name,
                                                      ILM_FALSE);
    }

    release_instance();
    return ILM_SUCCESS;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputAcceptanceOn(t_ilm_surface surfaceID, t_ilm_uint *num_seats,
                         t_ilm_string **seats)
{
    struct ilm_control_context *ctx;
    struct surface_context *surface_ctx;
    struct accepted_seat *accepted_seat;
    int surface_found = 0;
    int i;

    if ((seats == NULL) || (num_seats == NULL)) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();

    wl_list_for_each(surface_ctx, &ctx->wl.list_surface, link) {
        if (surface_ctx->id_surface == surfaceID) {
            surface_found = 1;
            break;
        }
    }

    if (!surface_found) {
        fprintf(stderr, "Surface ID %d not found\n", surfaceID);
        release_instance();
        return ILM_FAILED;
    }

    *num_seats = wl_list_length(&surface_ctx->list_accepted_seats);
    *seats = calloc(*num_seats, sizeof *seats);
    if (*seats == NULL) {
        fprintf(stderr, "Failed to allocate memory for seat array\n");
        release_instance();
        return ILM_FAILED;
    }

    i = 0;
    wl_list_for_each(accepted_seat, &surface_ctx->list_accepted_seats, link) {
        (*seats)[i] = strdup(accepted_seat->seat_name);
        if ((*seats)[i] == NULL) {
            int j;
            fprintf(stderr, "Failed to copy seat name %s\n",
                    accepted_seat->seat_name);
            release_instance();
            for (j = 0; j < i; j++)
                free((*seats)[j]);
            free(*seats);
            *seats = NULL;
            *num_seats = 0;
            return ILM_FAILED;
        }
        i++;
    }

    release_instance();
    return ILM_SUCCESS;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputDevices(ilmInputDevice bitmask, t_ilm_uint *num_seats,
                    t_ilm_string **seats)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx;
    struct seat_context *seat;
    int max_seats;
    int seats_added = 0;

    if ((seats == NULL) || (num_seats == NULL)) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();
    max_seats = wl_list_length(&ctx->wl.list_seat);
    *seats = calloc(max_seats, sizeof **seats);

    if (*seats == NULL) {
        fprintf(stderr, "Failed to allocate memory for input device list\n");
        release_instance();
        return ILM_FAILED;
    }

    wl_list_for_each(seat, &ctx->wl.list_seat, link) {
        returnValue = ILM_SUCCESS;

        if ((seat->capabilities & bitmask) == 0)
            continue;

        (*seats)[seats_added] = strdup(seat->seat_name);
        if ((*seats)[seats_added] == NULL) {
            int j;
            fprintf(stderr, "Failed to duplicate seat name %s\n",
                    seat->seat_name);
            for (j = 0; j < seats_added; j++)
                free((*seats)[j]);
            free(*seats);
            *seats = NULL;
            returnValue = ILM_FAILED;
            break;
        }

        seats_added++;
    }
    *num_seats = seats_added;
    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputDeviceCapabilities(t_ilm_string seat_name, ilmInputDevice *bitmask)
{
    ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx;
    struct seat_context *seat;

    if ((seat_name == NULL) || (bitmask == NULL)) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();
    wl_list_for_each(seat, &ctx->wl.list_seat, link) {
        if (strcmp(seat_name, seat->seat_name) == 0) {
            *bitmask = seat->capabilities;
            returnValue = ILM_SUCCESS;
        }
    }

    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_setInputFocus(t_ilm_surface *surfaceIDs, t_ilm_uint num_surfaces,
                  ilmInputDevice bitmask, t_ilm_bool is_set)
{
	ilmErrorTypes returnValue = ILM_FAILED;
    struct ilm_control_context *ctx;
    int i;

    if (surfaceIDs == NULL) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    if (bitmask & (ILM_INPUT_DEVICE_POINTER | ILM_INPUT_DEVICE_TOUCH)
        && num_surfaces > 1 && is_set == ILM_TRUE) {
        fprintf(stderr,
                "Cannot set pointer or touch focus for multiple surfaces\n");
        release_instance();
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();
    for (i = 0; i < num_surfaces; i++) {
        struct surface_context *ctx_surf;
        int found_surface = 0;
        wl_list_for_each(ctx_surf, &ctx->wl.list_surface, link) {
            if (ctx_surf->id_surface == surfaceIDs[i]) {
                found_surface = 1;
                break;
            }
        }

        if (!found_surface) {
            fprintf(stderr, "Surface %d was not found\n", surfaceIDs[i]);
            break;
        }

        ivi_input_set_input_focus(ctx->wl.input_controller,
                                             ctx_surf->id_surface,
                                             bitmask, is_set);
        returnValue = ILM_SUCCESS;
    }
    release_instance();
    return returnValue;
}

ILM_EXPORT ilmErrorTypes
ilm_getInputFocus(t_ilm_surface **surfaceIDs, ilmInputDevice **bitmasks,
                  t_ilm_uint *num_ids)
{
    struct ilm_control_context *ctx;
    int i = 0;
    struct surface_context *ctx_surf;

    if ((surfaceIDs == NULL) || (bitmasks == NULL)
         ||(num_ids == NULL)) {
        fprintf(stderr, "Invalid Argument\n");
        return ILM_FAILED;
    }

    ctx = sync_and_acquire_instance();
    *num_ids = wl_list_length(&ctx->wl.list_surface);
    *surfaceIDs = calloc(*num_ids, sizeof **surfaceIDs);

    if (*surfaceIDs == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface ID list\n");
        release_instance();
        return ILM_FAILED;
    }

    *bitmasks = calloc(*num_ids, sizeof **bitmasks);
    if (*bitmasks == NULL) {
        fprintf(stderr, "Failed to allocate memory for bitmask list\n");
        free(*surfaceIDs);
        release_instance();
        return ILM_FAILED;
    }

    wl_list_for_each(ctx_surf, &ctx->wl.list_surface, link) {
        (*surfaceIDs)[i] = ctx_surf->id_surface;
        (*bitmasks)[i] = ctx_surf->prop.focus;
        i++;
    }

    release_instance();
    return ILM_SUCCESS;

}
