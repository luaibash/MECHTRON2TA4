/**
Robert Li, 2021: 
  
 In this example project: 
 
 1: TIM3 is used. 
 2: Each channel of TIM3 will out put a PWM signal.
 3: the cycle period of the  PWM is designed as 20 ms  (1000 TIM3 counter ticks) for the 4 PWM signals
 4: Initially, the duty cycle of the four PWM signals are: 20%, 40%, 60% and 80%. (Pulse widths are: 200, 400, 600 and 800 TIM3 counter ticks respectively)
 5: the output pins for the CH1, CH2, CH3 and CH4 are: PA6, PA7, PC8( or PB0) and PC9( or PB1).  (See Table 12 of the STM32F427xx STM32F429xx datasheet)
  (GPIO pin AF coinfigurartion is in _msp.c file)
 
 
 6. The confinguration for the output pins are in _hal_msp.c 
 
 
 Users can use Oscillospe to check the output signals.
 
 
 7: each time the USER BUTTON is pressed, the pulshe width (duty cycle) is decreased by half. and the LED3 toggled.
 
	
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"


/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup TIM_PWM_Input
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Timer handler declaration */
TIM_HandleTypeDef    Tim3_Handle;
TIM_OC_InitTypeDef Tim3_OCInitStructure;


uint16_t TIM3Prescaler=1799;   //with the prescaler as 180, every 500 ticks of TIM3 is 1ms. if 1800, then every 50 ticks is 1ms
uint16_t TIM3Period=1000;    //1,000 ticks of TIM3, with 1800 prescaller, is 20ms

__IO uint16_t TIM3_CCR1_Val=200, TIM3_CCR2_Val=400,TIM3_CCR3_Val=600,TIM3_CCR4_Val=800;  // for dynamically set the pulse width  for the four channels. 
																																// can vary it from 1 to 1,000 to vary the pulse width.
																  // the CCR value must be less than period. otherwise, the interrupt will never happen.



/* Private function prototypes -----------------------------------------------*/

void TIM3_PWM_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config();
	
	
	HAL_InitTick(0x0000); // set systick's priority to the highest.
  
  
  BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	
	
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c
	
	
	
	//configure PWM output
	TIM3_PWM_Config();

  
  /* Infinite loop */  
  while (1)
  {
  } 
}

/**
  * @brief  Conversion complete callback in non blocking mode 
  * @param  hadc : hadc handle
  * @retval None
  */


void  TIM3_PWM_Config(void)
{

	/*******to make this PWM works with the sevo motor in the future,
 the cycle period will be designed as 20 ms
TIM3 TIM4 are 16 bits, TIM2 and TIM5 are 32 bits	
	TIM2---5 are on APB1 bus, which is 45MHZ
since the APB1 prescaller is not 1, the timer Freq is 2XFreq_of_APB1, that is 90Mhz.
so to make a cyle as 0.02 s, the Period	should be: 0.02/(1/90M)=1800,000, which is larger than allwed MAX period value 65536
so need to use a prescaler for the timer: if set the prescaller as 1800 (assign 1799 to the register), then the period can be 1000
*/
	
	
		/* -----------------------------------------------------------------------
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM3CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
    To get TIM3 counter clock at 10 KHz, the Prescaler is computed as following:
    Prescaler = (TIM3CLK / TIM3 counter clock) - 1
    Prescaler = ((SystemCoreClock /2) /10 KHz) - 1
       
    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to update SystemCoreClock 
     variable value. Otherwise, any configuration based on this variable will be incorrect.
     This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetSysClockFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency  
  ----------------------------------------------------------------------- */  
   
  /* Set TIM3 instance */
  Tim3_Handle.Instance = TIM3; //TIM3 is defined in stm32f429xx.h
   
  Tim3_Handle.Init.Period = TIM3Period;; //pwm frequency? 
  Tim3_Handle.Init.Prescaler = TIM3Prescaler;
  Tim3_Handle.Init.ClockDivision = 0;
  Tim3_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	Tim3_Handle.Init.RepetitionCounter = 0;  //default is 0
	

	Tim3_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
 
 
	if(HAL_TIM_PWM_Init(&Tim3_Handle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
	//configure the PWM channel
	Tim3_OCInitStructure.OCMode=  TIM_OCMODE_PWM1; //TIM_OCMODE_TIMING;
	Tim3_OCInitStructure.OCFastMode=TIM_OCFAST_DISABLE;
	Tim3_OCInitStructure.OCPolarity=TIM_OCPOLARITY_HIGH;
	//Tim3_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH //complementary polarity. 
																	//This parameter is valid only for TIM1 and TIM8.
	//Tim3_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_SET; //This parameter is valid only for TIM1 and TIM8.
  //Tim3_OCInitStructure.OCNIdleState= TIM_OCNIDLESTATE_RESET; //This parameter is valid only for TIM1 and TIM8.
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR1_Val;   //200
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }
	
	
	//***************************
	
if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
	



	
}




void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef * htim){  //this is for TIM3_pwm
	
	//__HAL_TIM_SET_COUNTER(htim, 0x0000);  //not necessary

}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
  if(GPIO_Pin == KEY_BUTTON_PIN)  //GPIO_PIN_0
  {
		BSP_LED_Toggle(LED3);
		
		TIM3_CCR1_Val=TIM3_CCR1_Val/2;
		
		
		__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_1,TIM3_CCR1_Val);
  }

}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void Error_Handler(void)
{
  /* Turn LED3 on */
  BSP_LED_On(LED4);
  while(1)
  {
  }
}





#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
