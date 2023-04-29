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

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/

#include "app.h"

#define GRAVITY 0.5
#define TIMING_EVENT (START_TIME | END_TIME)


static GLIB_Context_t glibContext;
static OS_MUTEX GameConfig_Mutex;
static OS_MUTEX Platform_Mutex;
static OS_MUTEX Railgun_Mutex;
static OS_MUTEX Shield_Mutex;
static OS_TMR PhysicsTimer;
static OS_TMR LCDTimer;
static OS_TMR SliderTimer;
static OS_TMR EvacTimerOn;
static OS_TMR EvacTimerOff;

//static OS_TMR RailgunTimer;
static OS_SEM PhysicsSemaphore;
static OS_SEM SliderSemaphore;
static OS_SEM LCDSem;
static OS_FLAG_GRP LCD_EventFlagGroup;
static OS_FLAG_GRP LED_EventFlagGroup;


volatile bool slider_position0;
volatile bool slider_position1;
volatile bool slider_position2;
volatile bool slider_position3;
volatile int slider_direction = 0;
volatile bool satchel_in_air = true;
volatile bool shot_in_air = false;
volatile bool shot_still_in_air;
bool blink = true;
int game_over = -1;
int evacuation = 0;

int foundation_hit_count = 0;
int lower = 5;
int upper = 20;


double plat_x = 64;
double plat_x_vel = 0;
double plat_x_acc = 0;
double sat_x_vel = 0;

double sat_x = 20;
double sat_y = 0;
double sat_y_vel = 0;
double sat_x_init_vel = 0;
double sat_y_init_vel = 0;
double sat_y_acc = GRAVITY;

double rail_x = 0;
double rail_y = 0;
double rail_vel = 0;
double rail_x_vel = 0;
double rail_y_init_vel = 0;
double rail_y_vel = 0;
double rail_y_acc = GRAVITY*4;
double rail_energy;
double impulse;

double tau_physics;

OS_TCB OSIdleTCB, LedOutputTCB, LcdDisplayTCB, PhysicsEngineTCB, SliderTCB;

CPU_STK OSIdleTaskStk[TASK_STACK_SIZE], LedOutputStack[TASK_STACK_SIZE], LcdDisplayStack[TASK_STACK_SIZE], SliderStack[TASK_STACK_SIZE],
PhysicsEngineStack[TASK_STACK_SIZE];

game_config_t game;

platform_t platform;

railgun_t railgun;

shield_t shield;

void LcdDisplayTask(void *p_arg)
{
    RTOS_ERR err;
    (void)&p_arg;

    OSTmrStart(&LCDTimer, &err);

    while(1) {
        // Wait for LCD semaphore
        OSSemPend(&LCDSem,
                  10,
                  OS_OPT_PEND_BLOCKING,
                  DEF_NULL,
                  &err);

        int has_shield = shield.on;

        // Acquire mutex
        OSMutexPend(&GameConfig_Mutex,
                    0,
                    OS_OPT_PEND_BLOCKING,
                    DEF_NULL,
                    &err);

        // Clear LCD display and draw game objects
        GLIB_clear(&glibContext);

        // Convert the number to a string
        char str[16];
        sprintf(str, "%d", game.generator.energy_storage_kj);

        // Draw the number at the calculated coordinates
        GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNarrow6x8);
        GLIB_drawStringOnLine(&glibContext, str, 0, GLIB_ALIGN_RIGHT, -1, 5, true);

        draw_platform(&glibContext, plat_x);
        draw_satchel(&glibContext, sat_x, sat_y, game.satchel_charges.display_diameter_pixels);
        draw_castle(&glibContext, foundation_hit_count);
        draw_stars(&glibContext);

        // Draw shield if it is on
        if(has_shield == 1){
            draw_shield(&glibContext, plat_x);

            shield.on = -1;

        }

        if(shot_in_air){
            draw_railgun_shot(&glibContext, rail_x, rail_y, game.railgun.shot_display_diameter_pixels);
        }

        if(evacuation > 8){

            GLIB_drawStringOnLine(&glibContext, "You Win!", 6, GLIB_ALIGN_CENTER, 2, 0, true);

            OSTaskDel(&LedOutputTCB, &err);
            OSTaskDel(&PhysicsEngineTCB, &err);
            OSTaskDel(&SliderTCB, &err);


        }

        if (game_over == 0){
            GLIB_drawStringOnLine(&glibContext, "Game Over!", 6, GLIB_ALIGN_CENTER, 2, 0, true);
            GPIO_PinOutClear(LED1_port, LED1_pin);
            GPIO_PinOutSet(LED1_port, LED1_pin);

            OSTaskDel(&LedOutputTCB, &err);
            OSTaskDel(&PhysicsEngineTCB, &err);
            OSTaskDel(&SliderTCB, &err);


        }

        // Release mutex
        OSMutexPost(&GameConfig_Mutex, OS_OPT_POST_1, &err);

        // Update LCD display
        DMD_updateDisplay();
    }
}



void LedOutputTask(void *p_arg)
{
    RTOS_ERR err;
    OS_TICK start_time = 0;
    OS_TICK end_time = 0;
    OS_FLAGS event_flags;
    uint32_t duty_cycle_led0 = 0;

    OSTmrStart(&EvacTimerOn, &err);

    (void)&p_arg;

    while(1){

        event_flags = OSFlagPend(&LED_EventFlagGroup,
                       TIMING_EVENT,
                       100,
                       OS_OPT_PEND_FLAG_SET_ANY |
                       OS_OPT_PEND_BLOCKING     |
                       OS_OPT_PEND_FLAG_CONSUME,
                       DEF_NULL,
                       &err);

        OSMutexPend(&Railgun_Mutex,
                     0,
                     OS_OPT_PEND_BLOCKING,
                     DEF_NULL,
                     &err);

        if ((event_flags & START_TIME) == START_TIME) {
            start_time = OSTimeGet(&err);
        }
        if ((event_flags & END_TIME) == END_TIME) {
            end_time = OSTimeGet(&err);

        }

        if (end_time > start_time) {
            // Calculate the time difference in milliseconds
            uint32_t time_diff_ms = (end_time - start_time) * 100 / OSCfg_TickRate_Hz;

            // Increase the duty cycle based on the button press duration for LED0
            duty_cycle_led0 += time_diff_ms / 100;  // Increase duty cycle by 1% every 100ms

            railgun.time += time_diff_ms / 100;

            if (railgun.time > 50){
                railgun.time = 50;
            }



            // Limit the duty cycle of LED0 to a maximum of 50%
            if (duty_cycle_led0 > 50) {
                duty_cycle_led0 = 50;
            }

            // Set COMP1 to control duty cycle of LED0
            LETIMER_CompareSet(LETIMER0, 1,
            CMU_ClockFreqGet(cmuClock_LETIMER0) * duty_cycle_led0 / (OUT_FREQ * 100));
        }
        else {
            // Reset the duty cycle of LED0 when the button is released
            duty_cycle_led0 = 0;
            railgun.time = 0;
            LETIMER_CompareSet(LETIMER0, 1,
            CMU_ClockFreqGet(cmuClock_LETIMER0) * 0 / (OUT_FREQ * 100));
        }

        OSMutexPost(&Platform_Mutex, OS_OPT_POST_1, &err);


    }

}




void SliderTask(void *p_arg)
{
  RTOS_ERR err;
  (void)&p_arg;

  OSTmrStart(&SliderTimer, &err);

  while(1){
      OSSemPend(&SliderSemaphore,
                100,
                OS_OPT_PEND_BLOCKING,
                DEF_NULL,
                &err);

      CAPSENSE_Sense();

      slider_position0 = CAPSENSE_getPressed(0);
      slider_position1 = CAPSENSE_getPressed(1);
      slider_position2 = CAPSENSE_getPressed(2);
      slider_position3 = CAPSENSE_getPressed(3);

      OSMutexPend(&Platform_Mutex,
                  0,
                  OS_OPT_PEND_BLOCKING,
                  DEF_NULL,
                  &err);

      if((slider_position0) && (!slider_position1 && !slider_position2 && !slider_position3)){
            if(slider_direction != 1){
                slider_direction = 1;
                platform.force = -80;

            }
      }

      else if((slider_position1) && (!slider_position2 && !slider_position3 && !slider_position0)){
            if(slider_direction != 2){
                slider_direction = 2;
                platform.force = -40;

          }
      }

      else if((slider_position2) && (!slider_position1 && !slider_position3 && !slider_position0)){
            if(slider_direction != 3){
                slider_direction = 3;
                platform.force = 40;

            }
      }

      else if((slider_position3) && (!slider_position1 && !slider_position2 && !slider_position0)){
            if(slider_direction != 4){
                slider_direction = 4;
                platform.force = 80;

            }
      }

      else{
          slider_direction = 0; //no finger on slider
          platform.force = 0;

      }

      OSMutexPost(&Platform_Mutex, OS_OPT_POST_1, &err);
  }

  if(err.Code != RTOS_ERR_NONE){

  }


}



void PhysicsEngineTask(void* p_arg) {
    RTOS_ERR err;
    (void) p_arg; // unused parameter

    OSTmrStart(&PhysicsTimer, &err);

    while (1) {

        OSSemPend(&PhysicsSemaphore,
                  10,
                  OS_OPT_PEND_BLOCKING,
                  DEF_NULL,
                  &err);

        OSMutexPend(&GameConfig_Mutex,
                    0,
                    OS_OPT_PEND_BLOCKING,
                    DEF_NULL,
                    &err);

        OSMutexPend(&Platform_Mutex,
                    0,
                    OS_OPT_PEND_BLOCKING,
                    DEF_NULL,
                    &err);

        OSTmrStart(&EvacTimerOn, &err);

        int has_shield = shield.on;

        if(shield.on == -1){
            if(game.generator.energy_storage_kj < game.shield.activation_energy_kj){
                shield.on = 0;
                has_shield = shield.on;
            }
            else{
                game.generator.energy_storage_kj -= game.shield.activation_energy_kj;
                shield.on = 1;
                has_shield = shield.on;
            }
        }


        // Calculate x acceleration for platform
        plat_x_acc = (double) platform.force / (double) game.platform.mass_kg;

        tau_physics = (double) game.tau_physics_ms / 1000;

        double max_force = (2*plat_x_vel*game.platform.mass_kg)/tau_physics;

        // edge collision detection
        if (plat_x - 8 < 0) {
            plat_x = 9;
            if (max_force > game.platform.max_force_N){
                plat_x_vel = game.platform.max_platform_bounce_speed_cm_s/1000;
            }
            impulse = 0;
            plat_x_vel *= -1;

        }
        else if (plat_x + 8 > 128) {
            plat_x = 119;
            impulse = 0;
            plat_x_vel *= -1;
        }


        if (satchel_in_air) {
            // Satchel is in the air, update position and velocity
            if (sat_y > 128) { // satchel hits ground
                // Reset position and velocity, generate new random initial velocities
                satchel_in_air = false;
                sat_x = 20;
                sat_y = 0;
                sat_x_vel = (double)((rand() % (upper - lower + 1)) + lower);
                sat_y_vel = 0;
            } else {
                // Satchel is still in the air, update position and velocity
                if (sat_x < 0) {
                    sat_x = 1;
                    sat_x_vel *= -1;
                }
                if (sat_x > 128) {
                    sat_x = 127;
                    sat_x_vel *= -1;
                }
                if ((sat_x > plat_x - 8 && sat_x < plat_x + 8) &&
                    (sat_y > 126 && sat_y < 128)) { // satchel collides with platform
                    satchel_in_air = false;
                    game_over = 0;
                }

                if ((sat_x > plat_x - 8 && sat_x < plat_x + 8) &&
                    (sat_y > (128 - (game.shield.effective_range_cm/1000)) && sat_y < 128) && has_shield) { // satchel collides with shield
                    satchel_in_air = false;
                    sat_x = 20;
                    sat_y = 0;
                }

                sat_x += sat_x_vel * tau_physics;
                sat_y_vel += sat_y_acc * tau_physics;
                sat_y += sat_y_vel * tau_physics;
            }
        } else {
            // Satchel is on the ground, reset position and velocity, generate new random initial velocities
            sat_x = 20;
            sat_y = 0;
            sat_x_vel = (double)((rand() % (upper - lower + 1)) + lower);
            sat_y_vel = 0;
            satchel_in_air = true;
        }

        double railgun_time = railgun.time;

        if(((rail_x > 2 && rail_x < 8) &&
            (rail_y > 3 && rail_y < 18) && shot_still_in_air)) // shot hits the foundation
        {
            shot_still_in_air = false;
            rail_x = 130;
            rail_y = 130;
            rail_x_vel = 0;
            rail_y_vel = 0;

            foundation_hit_count++;


        }
        if(((rail_x > 0 && rail_x < 20) &&
            (rail_y > 0 && rail_y < 2) && shot_still_in_air)) // shot hits the foundation
        {
            shot_still_in_air = false;
            rail_x = 130;
            rail_y = 130;
            rail_x_vel = 0;
            rail_y_vel = 0;

            foundation_hit_count++;


        }

        else if (rail_x < 0 || rail_x > 128 || rail_y > 128){ // shot is out of bounds
            shot_still_in_air = false;
        }
        else if((rail_x > plat_x - 8 && rail_x < plat_x + 8) &&
            (rail_y > 126 && rail_y < 128)){
            satchel_in_air = false;
            game_over = 0;
        }

        else{
            shot_still_in_air = true;
        }

        if ((railgun_time > 0) && !shot_in_air) {
            rail_energy = game.generator.power_kw * (railgun_time/50);
            if (game.generator.energy_storage_kj - rail_energy > 0){
                rail_vel = sqrt((2*rail_energy)/(game.railgun.shot_mass_kg));
                rail_x = plat_x - 8;
                rail_y = 118;
                rail_x_vel = (cos(game.railgun.elevation_angle_mrad * 0.0572957795) * rail_vel) + plat_x_vel;
                rail_y_init_vel = -sin(game.railgun.elevation_angle_mrad * 0.0572957795) * rail_vel;
                rail_y_vel = rail_y_init_vel;  // initialize rail_y_vel with rail_y_init_vel

                shot_in_air = false;
            }
            else{
                rail_energy = game.generator.energy_storage_kj;
                rail_vel = sqrt((2*rail_energy)/(game.railgun.shot_mass_kg));
                rail_x = plat_x - 8;
                rail_y = 118;
                rail_x_vel = (cos(game.railgun.elevation_angle_mrad * 0.0572957795) * rail_vel) + plat_x_vel;
                rail_y_init_vel = -sin(game.railgun.elevation_angle_mrad * 0.0572957795) * rail_vel;
                rail_y_vel = rail_y_init_vel;  // initialize rail_y_vel with rail_y_init_vel

                shot_in_air = false;
            }

        }
        else {
            if(game.generator.energy_storage_kj <= 0){
                game.generator.energy_storage_kj = 0;
            }
            else{
                game.generator.energy_storage_kj -= rail_energy;
                rail_energy = 0;
                if(shot_still_in_air){
                    impulse = ((game.railgun.shot_mass_kg * (rail_x_vel * -1))/35);
                    shot_in_air = true;
                }
                else{
                    shot_in_air = false;
                    impulse = 0;
                }
            }
        }

        rail_x += (rail_x_vel * tau_physics);
        rail_y_vel += (rail_y_acc * tau_physics);  // update rail_y_vel with gravity
        rail_y += rail_y_vel * tau_physics;  // use updated rail_y_vel in position update
        // Calculate x velocity and position for platform
        plat_x_vel += (plat_x_acc * tau_physics) + (impulse / game.platform.mass_kg);
        plat_x += plat_x_vel * tau_physics;


        if(game.generator.energy_storage_kj < 50000){
            game.generator.energy_storage_kj += 10;
        }

        OSMutexPost(&GameConfig_Mutex, OS_OPT_POST_1, &err);

        OSMutexPost(&Platform_Mutex, OS_OPT_POST_1, &err);

        if (err.Code != RTOS_ERR_NONE) {

        }
    }
}


void Physics_TimerCallback(void *p_tmr, void *p_arg){
  RTOS_ERR err;

  (void)&p_tmr;
  (void)&p_arg;

  OSSemPost(&PhysicsSemaphore, OS_OPT_POST_1, &err);

  if(err.Code != RTOS_ERR_NONE){

  }

}

void LCD_TimerCallback(void *p_tmr, void *p_arg){
  RTOS_ERR err;

  (void)&p_tmr;
  (void)&p_arg;

  OSSemPost(&LCDSem, OS_OPT_POST_1, &err);

  if(foundation_hit_count >= game.foundation_hits_required){
      if(evacuation > 8){
          GPIO_PinOutSet(LED1_port, LED1_pin);
      }
      else{
          GPIO_PinOutToggle(LED1_port, LED1_pin);
          evacuation++;
      }
  }

  if(err.Code != RTOS_ERR_NONE){

  }

}

void Slider_TimerCallback(void *p_tmr, void *p_arg){
  RTOS_ERR err;

  (void)&p_tmr;
  (void)&p_arg;

  OSSemPost(&SliderSemaphore, OS_OPT_POST_1, &err);

  if(err.Code != RTOS_ERR_NONE){

  }

}

void Evac_On_TimerCallback(void *p_tmr, void *p_arg){
  RTOS_ERR err;

  (void)&p_tmr;
  (void)&p_arg;

  GPIO_PinOutSet(LED1_port, LED1_pin);

  OSTmrCreate(&EvacTimerOff,
                "Evacuation Timer Off",
                0,
                5,
                OS_OPT_TMR_ONE_SHOT,
                &Evac_Off_TimerCallback,
                DEF_NULL,
                &err);

  OSTmrStart(&EvacTimerOff, &err);


}

void Evac_Off_TimerCallback(void *p_tmr, void *p_arg){

  (void)&p_tmr;
  (void)&p_arg;

  GPIO_PinOutClear(LED1_port, LED1_pin);

}


void initResources(void){
  RTOS_ERR err;

  InitGameConfig(&game);

  srand((unsigned int) OSTimeGet(&err)); // seed random number generator


  OSSemCreate(&PhysicsSemaphore,
              "Physics Semaphore",
              0,
              &err);

  OSSemCreate(&SliderSemaphore,
              "Slider Semaphore",
              0,
              &err);

  OSSemCreate(&LCDSem,
                "LCD Semaphore",
                0,
                &err);


  OSMutexCreate(&GameConfig_Mutex,
                "Game Configuration Mutex",
                &err);

  OSMutexCreate(&Platform_Mutex,
                "Platform Mutex",
                &err);

  OSMutexCreate(&Railgun_Mutex,
                "Railgun Mutex",
                &err);

  OSMutexCreate(&Shield_Mutex,
                "Shield Mutex",
                &err);

  OSFlagCreate(&LCD_EventFlagGroup,
               "LCD Event Flag Group",
               0,
               &err);


  OSFlagCreate(&LED_EventFlagGroup,
               "LED Event Flag Group",
               0,
               &err);


}

void init_timers(void){
  RTOS_ERR err;

  OSTmrCreate(&PhysicsTimer,
              "Direction Timer",
              0,
              game.tau_physics_ms/10,
              OS_OPT_TMR_PERIODIC,
              &Physics_TimerCallback,
              DEF_NULL,
              &err);


  if(err.Code != RTOS_ERR_NONE){

  }

  OSTmrCreate(&LCDTimer,
              "LCD Timer",
              0,
              game.tau_display_ms/10,
              OS_OPT_TMR_PERIODIC,
              &LCD_TimerCallback,
              DEF_NULL,
              &err);

  if(err.Code != RTOS_ERR_NONE){

  }

  OSTmrCreate(&EvacTimerOn,
              "Evacuation Timer On",
              0,
              game.tau_physics_ms/10,
              OS_OPT_TMR_PERIODIC,
              &Physics_TimerCallback,
              DEF_NULL,
              &err);

  OSTmrCreate(&SliderTimer,
              "Slider Timer",
              0,
              game.tau_slider_ms/10,
              OS_OPT_TMR_PERIODIC,
              &Slider_TimerCallback,
              DEF_NULL,
              &err);

  if(err.Code != RTOS_ERR_NONE){

  }


  OSTmrStart(&PhysicsTimer, &err);

  OSTmrStart(&LCDTimer, &err);

  OSTmrStart(&SliderTimer, &err);

  if(err.Code != RTOS_ERR_NONE){

  }
}

void idle_task (void *p_arg)
{
    (void)&p_arg;

    while (DEF_TRUE)
    {
        EMU_EnterEM1();
    }
}

void PhysicsEngineTaskCreate(void){
  RTOS_ERR err;

  /* Create the LedOutput task */
  OSTaskCreate(&PhysicsEngineTCB,
                "PhysicsEngine",
                PhysicsEngineTask,
                0u,
                5u,
                &PhysicsEngineStack[0u],
                TASK_STACK_SIZE / 10u,
                TASK_STACK_SIZE,
                0u,
                0u,
                0u,
                OS_OPT_TASK_STK_CLR,
                &err);

  if (err.Code != RTOS_ERR_NONE) {
      /* Handle error on task creation. */
  }

}

void SliderTaskCreate(void){
  RTOS_ERR err;

  /* Create the LedOutput task */
  OSTaskCreate(&SliderTCB,
                "Slider",
                SliderTask,
                0u,
                10u,
                &SliderStack[0u],
                TASK_STACK_SIZE / 10u,
                TASK_STACK_SIZE,
                0u,
                0u,
                0u,
                OS_OPT_TASK_STK_CLR,
                &err);

  if (err.Code != RTOS_ERR_NONE) {
      /* Handle error on task creation. */
  }

}


void IdleTaskCreate(void){
  RTOS_ERR err;

  OSTaskCreate(&OSIdleTCB,
                   "Idle Task",
                   idle_task,
                   DEF_NULL,
                   OS_IDLE_PRIO,
                   &OSIdleTaskStk[0u],
                   TASK_STACK_SIZE / 10u,
                   TASK_STACK_SIZE,
                   DEF_NULL,
                   0u,
                   DEF_NULL,
                   (OS_OPT_TASK_STK_CLR),
                   &err);

  if (err.Code != RTOS_ERR_NONE) {
     /* Handle error on task creation. */
  }
}

void LcdDisplayTaskCreate(void){
  RTOS_ERR err;

  /* Create the LedOutput task */
  OSTaskCreate(&LcdDisplayTCB,
                "LcdDisplay",
                LcdDisplayTask,
                0u,
                15u,
                &LcdDisplayStack[0u],
                TASK_STACK_SIZE / 10u,
                TASK_STACK_SIZE,
                0u,
                0u,
                0u,
                OS_OPT_TASK_STK_CLR,
                &err);

  if (err.Code != RTOS_ERR_NONE) {
      /* Handle error on task creation. */
  }

}

void LedOutputTaskCreate(void){
  RTOS_ERR err;

  /* Create the LedOutput task */
  OSTaskCreate(&LedOutputTCB,
                "LedOutput",
                LedOutputTask,
                0u,
                30u,
                &LedOutputStack[0u],
                TASK_STACK_SIZE / 10u,
                TASK_STACK_SIZE,
                0u,
                0u,
                0u,
                OS_OPT_TASK_STK_CLR,
                &err);

  if (err.Code != RTOS_ERR_NONE) {
      /* Handle error on task creation. */
  }

}

void GPIO_EVEN_IRQHandler(void) {
    RTOS_ERR err;

    // Check if the interrupt was caused by the button press
    if (GPIO_IntGetEnabled() & (1 << BUTTON0_pin)) {
        // Clear the interrupt flag
        GPIO_IntClear(1 << BUTTON0_pin);

        if (GPIO_PinInGet(BUTTON0_port, BUTTON0_pin)) {
            // Button 0 is pressed (rising edge)
            OSFlagPost(&LED_EventFlagGroup, START_TIME, OS_OPT_POST_FLAG_SET, &err);
        } else {
            // Button 0 is released (falling edge)
            OSFlagPost(&LED_EventFlagGroup, END_TIME, OS_OPT_POST_FLAG_SET, &err);
        }

        if (err.Code != RTOS_ERR_NONE) {
            /* Handle error on flag post. */
        }
    }
}


void GPIO_ODD_IRQHandler(void)
{
  RTOS_ERR  err;

  // Check if the interrupt was caused by the button press
   if (GPIO_IntGetEnabled() & (1 << BUTTON1_pin))
   {
       //Clear the interrupt flag
       GPIO_IntClear(1 << BUTTON1_pin);

       shield.on = -1;

       if (err.Code != RTOS_ERR_NONE) {
       /* Handle error on flag post. */
       }

   }
}


static void LCD_init()
{
  uint32_t status;
  /* Enable the memory lcd */
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  /* Initialize the DMD support for memory lcd display */
  status = DMD_init(0);
  EFM_ASSERT(status == DMD_OK);

  /* Initialize the glib context */
  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == GLIB_OK);

  glibContext.backgroundColor = Black;
  glibContext.foregroundColor = White;

  /* Fill lcd with background color */
  GLIB_clear(&glibContext);

  /* Use Normal font */
  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNormal8x8);

  /* Draw text on the memory lcd display*/
  GLIB_drawStringOnLine(&glibContext,
                        "Ryan is\n100% UnNatty!",
                        0,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);

  /* Draw text on the memory lcd display*/
  GLIB_drawStringOnLine(&glibContext,
                        "Definitely on \nroids!",
                        2,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);
  /* Post updates to display */
  DMD_updateDisplay();
}



void app_init(void)
{
  // Initialize GPIO
  gpio_open();

  // Initialize our capactive touch sensor driver!
  CAPSENSE_Init();

  // Initialize our LCD system
  LCD_init();

  initResources();
  init_timers();
  PhysicsEngineTaskCreate();
  LcdDisplayTaskCreate();
  LedOutputTaskCreate();
  SliderTaskCreate();
  IdleTaskCreate();


}
