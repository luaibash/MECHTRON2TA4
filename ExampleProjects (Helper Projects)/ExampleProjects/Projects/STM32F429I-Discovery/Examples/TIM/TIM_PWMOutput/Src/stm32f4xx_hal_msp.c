/**
  ******************************************************************************
  * @file    TIM/TIM_PWMInput/Src/stm32f4xx_hal_msp.c
  * @author  MCD Application Team
  * @brief   HAL MSP module.    
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @defgroup HAL_MSP
  * @brief HAL MSP module.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Private_Functions
  * @{
  */


/**
  * @brief TIM MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param htim: TIM handle pointer
  * @retval None
  */



void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	//		as for which GPIO pin has to be used for TIM3 alternative functions, refer to the use manualf for Discovery board---UM1472 User Manual , stm32f4discovery
	//		Major pins for TIM3 AF: PA6_TIM3_CH1, PA7--TIM3_CH2(also as TIM14_CH1),  PB0--TIM3_CH3, PB1--TIM3_CH4, PB4--TIM3_CH1,PB5--TIM3_CH2.  PC6--TIM3_CH1,
	//		PC7--TIM3_CH2, PC8--TIM3_CH3, PC9--TIM3_CH4, PD2--TIM3_ETR
		
	GPIO_InitTypeDef   GPIO_InitStruct;
  
  //- Enable peripherals and GPIO Clocks 
  __HAL_RCC_TIM3_CLK_ENABLE();
    
	
	
  // Enable GPIO Port Clocks 
  __HAL_RCC_GPIOA_CLK_ENABLE(); //for PinA7  and PinA6for Channel 2
  
    
  // Common configuration for all channels 
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH; //???? 50MHz, 25Mhz...don't know what is the critics to select pin speed.
  
	
		//for Channel 1
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;//as for which AF, see table "alternate function mapping" in datasheet
																					//for stm32f429i-disco, see table 12 on page 73 (for pin PA7) in document
																					//DocID024030 Rev 7--datasheet for stm32f427xx--stm32f429xx
	
	//all the four channels of TIM3's alternate function are AF2.  
	
	
	
	GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
		 
  // for Channel 2 output   
  GPIO_InitStruct.Pin = GPIO_PIN_7;    //TIM3 AF2 is for TIM3's CH1, CH2 and CH3 and CH4
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	

	//the AF2 of TIM3-CH3 and TIM3-CH4 are mapped to PB0 and PB1
	
	 // Enable GPIO Port Clocks 
/*  __HAL_RCC_GPIOB_CLK_ENABLE(); //for PinB0 and PB1  and PinA6for Channel 2
	
	GPIO_InitStruct.Pin = GPIO_PIN_0;    //TIM3 AF2 is for TIM3's CH1, CH2 and CH3 and CH4
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_1;    //TIM3 AF2 is for TIM3's CH1, CH2 and CH3 and CH4
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
*/


	
	//the AF2 of TIM3-CH3 and TIM3-CH4 are also mapped to PC9 and PC9
	
	 // Enable GPIO Port Clocks 
  __HAL_RCC_GPIOC_CLK_ENABLE(); //for PinB0 and PB1  and PinA6for Channel 2
	
	GPIO_InitStruct.Pin = GPIO_PIN_8;    //TIM3 AF2 is for TIM3's CH1, CH2 and CH3 and CH4
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_9;    //TIM3 AF2 is for TIM3's CH1, CH2 and CH3 and CH4
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	

}






/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
