/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef APP_H
#define APP_H

#include <gpio.h>
#include <capsense.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cpu.h>
#include <math.h>

#include "sl_board_control.h"
#include "em_assert.h"
#include "glib.h"
#include "em_types.h"
#include "dmd.h"
#include "os.h"
#include "em_emu.h"
#include "fifo.h"
#include "gpio.h"
#include "game.h"

#define TASK_STACK_SIZE 1024u
#define OS_IDLE_PRIO   50u

typedef struct{
  int force;
}platform_t;

typedef struct{
  int time;
}railgun_t;

typedef struct{
  int on;
}shield_t;

typedef enum {
  START_TIME = 1u << 1,
  END_TIME = 1u << 2,
} event_flag_t;

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void);

void LcdDisplayTask(void *p_arg);

void LedOutputTask(void *p_arg);

void Physics_TimerCallback(void *p_tmr, void *p_arg);

void LCD_TimerCallback(void *p_tmr, void *p_arg);

void Slider_TimerCallback(void *p_tmr, void *p_arg);

void Evac_On_TimerCallback(void *p_tmr, void *p_arg);

void Evac_Off_TimerCallback(void *p_tmr, void *p_arg);

void initResources(void);

void idle_task (void *p_arg);

void IdleTaskCreate(void);

void LcdDisplayTaskCreate(void);

void LedOutputTaskCreate(void);

void GPIO_EVEN_IRQHandler(void);

void GPIO_ODD_IRQHandler(void);

#endif  // APP_H
