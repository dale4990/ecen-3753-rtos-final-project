/*
 * game.c
 *
 *  Created on: Apr 6, 2023
 *      Author: daniellee
 */

#include "game.h"

void InitGameConfig(game_config_t *p_config) {
    p_config->version = 1;
    p_config->tau_physics_ms = 50;
    p_config->tau_display_ms = 150;
    p_config->tau_slider_ms = 100;
    p_config->canyon_size_cm = 100000;
    p_config->castle_height_cm = 5000;
    p_config->foundation_hits_required = 2;
    p_config->foundation_depth_cm = 5000;
    p_config->satchel_charges.limiting_method = 0;
    p_config->satchel_charges.display_diameter_pixels = 10;
    p_config->satchel_charges.tau_throw_ms = 0;
    p_config->satchel_charges.max_num_in_flight = 1;
    p_config->platform.max_force_N = 20000000;
    p_config->platform.mass_kg = 100;
    p_config->platform.length_cm = 10000;
    p_config->platform.max_platform_bounce_speed_cm_s = 50000;
    p_config->shield.effective_range_cm = 15000;
    p_config->shield.activation_energy_kj = 30000;
    p_config->railgun.elevation_angle_mrad = 800;
    p_config->railgun.shot_mass_kg = 50;
    p_config->railgun.shot_display_diameter_pixels = 5;
    p_config->generator.energy_storage_kj = 50000;
    p_config->generator.power_kw = 20000;
}

void draw_platform(GLIB_Context_t *pContext, int32_t x) {
  // Calculate the x and y coordinates for the corners of the platform
  int32_t x1 = x - 8;
  int32_t y1 = 126;
  int32_t x2 = x + 8;
  int32_t y2 = 128;

  // Draw the platform using the glib library
  GLIB_Rectangle_t platform = {x1, y1, x2, y2};
  GLIB_drawRectFilled(pContext, &platform);

  int32_t x1_rg1 = x - 2;
  int32_t x2_rg1 = x - 1;
  int32_t y1_rg1 = 124;
  int32_t y2_rg1 = 126;
  GLIB_Rectangle_t railgun_1 = {x1_rg1, y1_rg1, x2_rg1, y2_rg1};
  GLIB_drawRectFilled(pContext, &railgun_1);

  int32_t x1_rg2 = x - 4;
  int32_t x2_rg2 = x - 3;
  int32_t y1_rg2 = 120;
  int32_t y2_rg2 = 123;
  GLIB_Rectangle_t railgun_2 = {x1_rg2, y1_rg2, x2_rg2, y2_rg2};
  GLIB_drawRectFilled(pContext, &railgun_2);

  int32_t x1_rg3 = x - 6;
  int32_t x2_rg3 = x - 5;
  int32_t y1_rg3 = 118;
  int32_t y2_rg3 = 119;
  GLIB_Rectangle_t railgun_3 = {x1_rg3, y1_rg3, x2_rg3, y2_rg3};
  GLIB_drawRectFilled(pContext, &railgun_3);
}

void draw_satchel(GLIB_Context_t *pContext, int32_t x, int32_t y, int display_diameter){

  GLIB_drawCircleFilled(pContext, x, y, display_diameter/5);

  GLIB_Rectangle_t satchel = {x - 1, y - 3, x + 1, y - 1};
  GLIB_drawRectFilled(pContext, &satchel);

}

void draw_railgun_shot(GLIB_Context_t *pContext, int32_t x, int32_t y, int display_diameter){

  GLIB_drawCircleFilled(pContext, x, y, display_diameter/2);

}

void draw_castle(GLIB_Context_t *pContext, int foundation_hit_count){
  if (foundation_hit_count == 0){
      GLIB_drawLineV (pContext, 0, 0, 128);
      GLIB_drawLineV (pContext, 1, 0, 128);
      GLIB_Rectangle_t castle_1 = {2, 14, 4, 18};
      GLIB_drawRectFilled(pContext, &castle_1);
      GLIB_Rectangle_t castle_2 = {2, 3, 8, 13};
      GLIB_drawRectFilled(pContext, &castle_2);
      GLIB_Rectangle_t castle_3 = {0, 0, 20, 2};
      GLIB_drawRectFilled(pContext, &castle_3);
  }

  if (foundation_hit_count == 1){
      GLIB_drawLineV (pContext, 0, 0, 128);
      GLIB_drawLineV (pContext, 1, 0, 128);
      GLIB_Rectangle_t castle_1 = {2, 14, 4, 18};
      GLIB_drawRectFilled(pContext, &castle_1);
      GLIB_Rectangle_t castle_2 = {6, 3, 8, 13};
      GLIB_drawRectFilled(pContext, &castle_2);
      GLIB_Rectangle_t castle_3 = {0, 0, 20, 2};
      GLIB_drawRectFilled(pContext, &castle_3);
  }

  if (foundation_hit_count == 2){
    GLIB_drawLineV (pContext, 0, 0, 128);
    GLIB_drawLineV (pContext, 1, 0, 128);
    GLIB_Rectangle_t castle_3 = {0, 0, 20, 2};
    GLIB_drawRectFilled(pContext, &castle_3);
  }

}

void draw_stars(GLIB_Context_t *pContext){
  srand(time(NULL));
  int lower = 0;
  int upper = 100;
  for(int i = 0; i < 15; i++){
      int32_t x = (rand() % (upper - lower + 1)) + lower;
      int32_t y = (rand() % (upper - lower + 1)) + lower;
      GLIB_drawPixel(pContext, x, y);
  }

}

void draw_shield(GLIB_Context_t *pContext, int32_t x){
  GLIB_Rectangle_t shield_1 = {x - 6, 110, x + 6, 111};
  GLIB_drawRectFilled(pContext, &shield_1);
  GLIB_Rectangle_t shield_2 = {x - 9, 111, x - 7, 111};
  GLIB_drawRectFilled(pContext, &shield_2);
  GLIB_Rectangle_t shield_3 = {x + 7, 111, x + 9, 111};
  GLIB_drawRectFilled(pContext, &shield_3);
}
