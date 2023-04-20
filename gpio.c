//***********************************************************************************

// Include files

//***********************************************************************************

#include "gpio.h"
#include "capsense.h"



//***********************************************************************************

// defined files

//***********************************************************************************




//***********************************************************************************

// global variables

//***********************************************************************************
volatile bool button0_state = false;
volatile bool button1_state = false;

//***********************************************************************************

// function prototypes

//***********************************************************************************





//***********************************************************************************

// functions

//***********************************************************************************


/***************************************************************************//**

 * @brief

 *   Setup gpio pins that are being used for the application.

 ******************************************************************************/

void gpio_open(void)
{
  // Set LEDs to be standard output drive with default off (cleared)

  GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, LED0_default);
  GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, LED1_default);


  // Setup Buttons as inputs
  GPIO_DriveStrengthSet(BUTTON0_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(BUTTON0_port, BUTTON0_pin, gpioModeInput, BUTTON0_default);
  GPIO_DriveStrengthSet(BUTTON1_port, gpioDriveStrengthWeakAlternateWeak);
  GPIO_PinModeSet(BUTTON1_port, BUTTON1_pin, gpioModeInput, BUTTON1_default);

  //Initialize GPIO Interrupts
  GPIO_IntConfig(BUTTON0_port, BUTTON0_pin, true, true, true);
  GPIO_IntConfig(BUTTON1_port, BUTTON1_pin, true, false, true);

  // Enable GPIO and clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Enable the GPIO interrupt in the NVIC
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

/**************************************************************************//**
 * @brief TIMER initialization
 *****************************************************************************/
void initLETIMER(void)
{
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;

  // Enable clock to the LE modules interface
  CMU_ClockEnable(cmuClock_HFLE, true);

  // Select LFXO for the LETIMER
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  // Reload COMP0 on underflow, pulse output, and run in repeat mode
  letimerInit.comp0Top = true;
  letimerInit.ufoa0 = letimerUFOAPwm;
  letimerInit.repMode = letimerRepeatFree;
  letimerInit.topValue = LETIMER0_FREQ / OUT_FREQ;

  // Need REP0 != 0 to run PWM
  LETIMER_RepeatSet(LETIMER0, 0, 1);

  // Set COMP1 to control duty cycle
  LETIMER_CompareSet(LETIMER0, 1,
  CMU_ClockFreqGet(cmuClock_LETIMER0) * DUTY_CYCLE / (OUT_FREQ * 100));

  // Enable LETIMER0 output0 on PF4 (Route 28) for LED0
  LETIMER0->ROUTEPEN |=  LETIMER_ROUTEPEN_OUT0PEN;
  LETIMER0->ROUTELOC0 |= LETIMER_ROUTELOC0_OUT0LOC_LOC28;

  // Initialize LETIMER
  LETIMER_Init(LETIMER0, &letimerInit);
}



//Function to sample pushbutton 0
bool sampleButton0(void){
  if(GPIO_PinInGet(BUTTON0_port, BUTTON0_pin) == 0 && GPIO_PinInGet(BUTTON1_port, BUTTON1_pin) == 0){
        button0_state = false;
        button1_state = false;

        return button0_state;
  }
  else if(GPIO_PinInGet(BUTTON0_port, BUTTON0_pin) == 0){
      button0_state = true;
      button1_state = false;

      return button0_state;
  }
  else{
      button0_state = false;

      return button0_state;
  }
}

//Function to sample pushbutton 1
bool sampleButton1(void){
  if(GPIO_PinInGet(BUTTON0_port, BUTTON0_pin) == 0 && GPIO_PinInGet(BUTTON1_port, BUTTON1_pin) == 0){
        button1_state = false;
        button0_state = false;

        return button1_state;
  }
  if(GPIO_PinInGet(BUTTON1_port, BUTTON1_pin) == 0){
      button1_state = true;
      button0_state = false;

      return button1_state;
  }

  else{
      button1_state = false;

      return button1_state;
  }
}


