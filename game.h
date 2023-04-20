/*
 * game.h
 *
 *  Created on: Apr 6, 2023
 *      Author: daniellee
 */

#ifndef GAME_H_
#define GAME_H_

#include "sl_board_control.h"
#include "glib.h"
#include "dmd.h"
#include <stdlib.h>
#include <time.h>

// Declare the data structure to hold game configuration parameters
typedef struct {
    int version;
    int tau_physics_ms;
    int tau_display_ms;
    int tau_slider_ms;
    int canyon_size_cm;
    int castle_height_cm;
    int foundation_hits_required;
    int foundation_depth_cm;
    struct {
        int limiting_method;
        int display_diameter_pixels;
        union {
            int tau_throw_ms;
            int max_num_in_flight;
        };
    } satchel_charges;
    struct {
        int max_force_N;
        int mass_kg;
        int length_cm;
        int max_platform_bounce_speed_cm_s;
    } platform;
    struct {
        int effective_range_cm;
        int activation_energy_kj;
    } shield;
    struct {
        int elevation_angle_mrad;
        int shot_mass_kg;
        int shot_display_diameter_pixels;
    } railgun;
    struct {
        int energy_storage_kj;
        int power_kw;
    } generator;
} game_config_t;

void InitGameConfig(game_config_t *p_config);

void draw_platform(GLIB_Context_t *pContext, int32_t x);

void draw_satchel(GLIB_Context_t *pContext, int32_t x, int32_t y, int display_diameter);

void draw_railgun_shot(GLIB_Context_t *pContext, int32_t x, int32_t y, int display_diameter);

void draw_castle(GLIB_Context_t * pContext, int foundation_hit_count);

void draw_stars(GLIB_Context_t * pContext);

void draw_shield(GLIB_Context_t *pContext, int32_t x);


#endif /* GAME_H_ */
