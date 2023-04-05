/****** Robert Li, 2023  


1.The Fist External button (named extBtn1)  is connected to PC1 (cannot use PB1, for 429i-DISCO ,pb1 is used by LCD), ExtBtn1_Config()  //
					cannot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
					can not use pin PA1, used by gyro. if use this pin, never interrupt
					pd1----WILL ACT AS PC13, To trigger the RTC timestamp event
					....ONLY PC1 CAN BE USED TO FIRE EXTI1 !!!!
2. the Second external button (extBtn2) is conected to  PD2.  ExtBtn2_Config() --The pin PB2 on the board have trouble.
    when connect external button to the pin PB2, the voltage at this pin is not 3V as it is supposed to be, it is 0.3V, why?
		so changed to use pin PD2.
		PA2: NOT OK. (USED BY LCD??)
		PB2: OK.
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in the manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		

**********************************************/

#include "main.h"


#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)

__IO uint16_t ADC3ConvertedValue = 10000;
ADC_HandleTypeDef    AdcHandle;
double initTemp;
double measuredTemp;
double temp_diff;

//timer declerations
TIM_HandleTypeDef    Tim3_Handle;
TIM_OC_InitTypeDef Tim3_OCInitStructure;
uint16_t TIM3Prescaler=1799;      //with the prescaler as 180, every 500 ticks of TIM3 is 1ms. if 1800, then every 50 ticks is 1ms
uint16_t TIM3Period=1000;         //1,000 ticks of TIM3, with 1800 prescaller, is 20ms
__IO uint16_t TIM3_CCR2_Val=200;  // dynamically set the pulse width for channel 1. 
																  // can vary it from 1 to 1,000 to vary the pulse width.
																  // the CCR value must be less than period. otherwise, the interrupt will never happen.


 volatile double  setPoint=27.5;  //NOTE: if declare as float, when +0.5, the compiler will give warning:
															//"single_precision perand implicitly converted to double-precision"
															//ALTHOUGH IT IS A WARNING, THIS WILL MAKE THE PROGRAM stop WORKing!!!!!!
															//if declare as float, when setPoint+=0.5, need to cast : as setPioint+=(float)0.5, otherwise,
															//the whole program will not work! even this line has  not been used/excuted yet
															//BUT, if declare as double, there is no such a problem.
															
	    														
	//You must then have code to enable the FPU hardware prior to using any FPU instructions. This is typically in the ResetHandler or SystemInit()

	//            system_stm32f4xx.c
  ////            void SystemInit(void)
	//							{
	//								/* FPU settings ------------------------------------------------------------*/
	//								#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	//									SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
	//								#endif
	//							...											
	//		-----MODIFY the system_stm32f4xx.c in the above way, will also fix the "float" type problem mentioned above. 												
												
	/*
	according to the reference manual of STM32f4Discovery, the Vref+ should =VDD=VDDA=3.0V, while Vref-=VSSA=0
	so the voltage of 3V is mapped to 12 bits ADC result, which ranges from 0 to 4095.  
	(althoug ADC_DR register is 16 bits, ADC converted result is just of 12bits)   
	so the voltage resolution is 3/4095  (v/per_reading_number)   
	since the temperature is amplified 3 times before it is fed in MCU, the actual voltage from the temperature sensor is: 
	ADC_convertedvalue* (3/4095) /3. 
	
	since the temperature sensor sensitity is 10mV/C ,that is:  (0.01V/C)
	so the temperature is: ADC_convertedvalue* (3/4095) /3 /0.01 
	
	NOTE: you'd better not, or cannot, let the MCU to the calculation becuase its power is limited. Otherwise your program may not work as expected. 
	*/

void  LEDs_Config(void);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);
static void SystemClock_Config(void);
static void Error_Handler(void);
void ExtBtn1_Config(void);
void ExtBtn2_Config(void);
void TIM3_PWM_Config(void);
void ADC_Config(void);

int main(void){
		/*
		ADC is PF6 for ADC3_channel4
		PWM is PA7 for CCR2
		Extern 1 is PC1
		Extern 2 is PD2
	For fan:
		white is positive, top of diode
		purple is negative, bottom of diode
	  */
	
		HAL_Init();
	
		SystemClock_Config();
		
		HAL_InitTick(0x0000); // set systick's priority to the highest.
	
		//Configure LED3 and LED4 ======================================
		LEDs_Config();
	
		//configure the USER button as exti mode. 
		BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c
																			
		//Interrupt,button,timer,ADC initialize
		TIM3_PWM_Config();
		ExtBtn1_Config();
		ExtBtn2_Config();
		ADC_Config();
	
		//LCD initialize
		BSP_LCD_Init();
		BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER); 
		BSP_LCD_SelectLayer(0);
		BSP_LCD_SetLayerVisible(0, ENABLE); 
		BSP_LCD_Clear(LCD_COLOR_WHITE);
		BSP_LCD_DisplayOn();
		BSP_LCD_SetFont(&Font20); 
	
	  LCD_DisplayString(3, 0, (uint8_t *) "Lab 4: Group 20");
		LCD_DisplayString(4, 0, (uint8_t *) "Luai Bashar");
		LCD_DisplayString(5, 0, (uint8_t *) "Mohammed Fuzail");
		
		LCD_DisplayString(9, 0, (uint8_t *) "Current ");
		LCD_DisplayString(10, 0, (uint8_t *)"setPoint");
		LCD_DisplayFloat(10, 10, setPoint, 2);
		
		LCD_DisplayString(12, 0, (uint8_t *)"Fan Mode");
	
	while(1) {		
		
		//convert sensor reading to correct value
		HAL_ADC_PollForConversion(&AdcHandle,100);
		initTemp = HAL_ADC_GetValue(&AdcHandle);
		measuredTemp = (double) initTemp*(ADC3ConvertedValue* ((3.0/4095.0) / (3.0/0.01)));
		
		//print temperature value
		LCD_DisplayString(9, 15, (uint8_t *)" ");
		LCD_DisplayFloat(9, 10, measuredTemp, 2);

		//calculate the temperature difference
		temp_diff = measuredTemp - setPoint;
		if (temp_diff > 0.00) {																									//if current temp is lower than set temp, start the fan
			
			HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_2);												
			if (temp_diff > 3.00) {                                               //if temp change greater than 3 deg, fan to high power
				TIM3_CCR2_Val= 1000;																									//CCR at 900, fan supplied with power at almost all times
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_2,TIM3_CCR2_Val);   //write new CCR value to timer
			  LCD_DisplayString(12, 10, (uint8_t *)"High   ");                    //display new mode
			}
			else if (temp_diff > 2.00) {                                          //if temp change greater than 2 deg, fan to medium high power
				TIM3_CCR2_Val= 800;
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_2,TIM3_CCR2_Val);
			  LCD_DisplayString(12, 10, (uint8_t *)"Med-Hi ");

			}
			else if (temp_diff > 1.00) {                                         //if temp change greater than 1 deg, fan to medium power
				TIM3_CCR2_Val= 600;
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_2,TIM3_CCR2_Val);
			  LCD_DisplayString(12, 10, (uint8_t *)"Med    ");
			}
			else {                                                               //if temp change less than 1 deg, fan to low power
				TIM3_CCR2_Val= 500;
				__HAL_TIM_SET_COMPARE(&Tim3_Handle, TIM_CHANNEL_2,TIM3_CCR2_Val);
			  LCD_DisplayString(12, 10, (uint8_t *)"Low    ");
			}
		}
		else {                                                                //if temp change less than 0 deg, fan turned off
			HAL_TIM_PWM_Stop(&Tim3_Handle, TIM_CHANNEL_2);
			LCD_DisplayString(12, 10, (uint8_t *)"Off!   ");
		}
		
		HAL_Delay(500);                                                       //add a delay of 0.5 sec so that LCD display does not mess up
	} // end of while loop
	
}  //end of main


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 

	* 					Oscillator											=HSE
	*    				HSE frequencey 										=8,000,000   (8MHz)
	*      ----However, if the project is created by uVision, the default HSE_VALUE is 25MHz. thereore, need to define HSE_VALUE
	*						PLL Source											=HSE
  *            PLL_M                          = 4
  *            PLL_N                          = 72
  *            PLL_P                          = 2
  *            PLL_Q                          = 3
  *        --->therefore, PLLCLK =8MHz X N/M/P=72MHz   
	*            System Clock source            = PLL (HSE)
  *        --> SYSCLK(Hz)                     = 72,000,000
  *            AHB Prescaler                  = 1
	*        --> HCLK(Hz)                       = 72 MHz
  *            APB1 Prescaler                 = 2
	*        --> PCLK1=36MHz,  -->since TIM2, TIM3, TIM4 TIM5...are on APB1, thiese TIMs CLK is 36X2=72MHz
							 	
  *            APB2 Prescaler                 = 1
	*        --> PCLK1=72MHz 

  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
	
	
//Clock configuration
//##################################################
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
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}
//##################################################


void LEDs_Config(void)
{
 /* Initialize Leds mounted on STM32F429-Discovery board */
	BSP_LED_Init(LED3);   //BSP_LED_....() are in stm32f4291_discovery.c
  BSP_LED_Init(LED4);
}


//Timer 3 config
//##################################################
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
	
	
	Tim3_OCInitStructure.Pulse=TIM3_CCR2_Val;   //200
	
	if(HAL_TIM_PWM_ConfigChannel(&Tim3_Handle, &Tim3_OCInitStructure, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }
	
	if(HAL_TIM_PWM_Start(&Tim3_Handle, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }  
	
}
//##################################################


//Configuration of ADC
//##################################################
void ADC_Config(void)//Configuration of ADC copied from ADC example.
{
	
	ADC_ChannelConfTypeDef sConfig = {0};
  
  AdcHandle.Instance          = ADCx;
  AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
  AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
  AdcHandle.Init.ScanConvMode = DISABLE;
  AdcHandle.Init.ContinuousConvMode = ENABLE;
  AdcHandle.Init.DiscontinuousConvMode = DISABLE;
  AdcHandle.Init.NbrOfDiscConversion = 0;
  AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
  AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  AdcHandle.Init.NbrOfConversion = 1;
  AdcHandle.Init.DMAContinuousRequests = DISABLE;
  AdcHandle.Init.EOCSelection = DISABLE;
      
  HAL_ADC_Init(&AdcHandle);
  sConfig.Channel = ADCx_CHANNEL;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfig.Offset = 0;

  HAL_ADC_ConfigChannel(&AdcHandle, &sConfig);
  HAL_ADC_Start(&AdcHandle);
}
//##################################################


//LCD Configuration
//##################################################
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				BSP_LCD_DisplayChar(COLUMN(ColumnNumber),LINE(LineNumber), *ptr); //new version of this function need Xpos first. so COLUMN() is the first para.
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if ((ColumnNumber+1)*(((sFONT *)BSP_LCD_GetFont())->Width)>=BSP_LCD_GetXSize() ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}
//##################################################


//Interrupt and Extern button configuration
//##################################################
void ExtBtn1_Config(void)     // for GPIO C pin 1
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* set priority of the GPIO pin interrupt */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********
	  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull =GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																				
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	
}

//factor is the value the temp changes by
void temp_change(float factor) {
	setPoint += factor;                                  //adds factor to current setPoint
	LCD_DisplayString(10, 10, (uint8_t *) "      ");		 //displays new value
	LCD_DisplayFloat(10, 10, setPoint, 2);
	HAL_Delay(25);                                       //delays change so that every 0.5 seconds 1 deg changes
}

int time;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
	time = HAL_GetTick();																               //time variable keeps track of time button was first pressed
	if(GPIO_Pin == GPIO_PIN_1) {	
		while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1) == GPIO_PIN_RESET) {   //if button is still held, enter the while loop
			if (HAL_GetTick()-time > 500) {                                //checks if its been 500 ms since button pressed (current_time - time_press)
				temp_change(0.05);																					 //increase by 0.05 for every cycle
			}
		}
	}  //end of PIN_1

	if(GPIO_Pin == GPIO_PIN_2) {
		while(HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2) == GPIO_PIN_RESET) {   //same process
			if (HAL_GetTick()-time > 500) {
				temp_change(-0.05);                                          //decrease by 0.05 for every cycle
			}
		}
	} //end of PIN_2	
	
}
//##################################################


void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef * htim) //see  stm32fxx_hal_tim.c for different callback function names. 
{																																//for timer4 
	//	if ((*htim).Instance==TIM4) {
			
	//	}	
		//clear the timer counter!  in stm32f4xx_hal_tim.c, the counter is not cleared after  OC interrupt
		__HAL_TIM_SET_COUNTER(htim, 0x0000);   //this maro is defined in stm32f4xx_hal_tim.h
	
}
 
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef * htim){  //this is for TIM3_pwm
	
	//__HAL_TIM_SET_COUNTER(htim, 0x0000);  not necessary
}

static void Error_Handler(void)
{
  /* Turn LED4 on */
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



