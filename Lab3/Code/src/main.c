/****** Robert Li,  2023  


This starter kit: 

1. set up I2C and tested I2C and EEPROM

2. Configured RTC and RTC_Alarm and tested RTC


3 For External buttons:  
1):  The First External button (named extBtn1) seems can only be connected to PC1.  
	(cannot use pin PB1, for 429i-DISCO ,pb1 is used by LCD. if use this pin, always interrupt by itself
	cannot use pin PA1, used by gyro. if use this pin, never interrupt
		pd1----WILL ACT AS PC13, To trigger the RTC timestamp event)
					
2) the Second external button (extBtn2) may be conected to  PD2.  

		PA2: NOT OK. (USED BY LCD??)
		PB2: OK.
		PC2: ok, BUT sometimes (every 5 times around), press pc2 will trigger exti1, which is configured to use PC1. (is it because of using internal pull up pin config?)
		      however, press PC1 does not affect exti 2. sometimes press PC2 will also affect time stamp (PC13)
		PD2: OK,     
		PE2:  OK  (PE3, PE4 PE5 , seems has no other AF function, according to the table in the manual for discovery board)
		PF2: NOT OK. (although PF2 is used by SDRAM, it affects LCD. press it, LCD will flick and displayed chars change to garbage)
		PG2: OK
		 
*/


/* Includes ------------------------------------------------------------------*/
#include "main.h"


/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)BSP_LCD_GetFont())->Width))    //see font.h, for defining LINE(X)



/* Private macro -------------------------------------------------------------*/

 

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef  I2c3_Handle;

RTC_HandleTypeDef RTCHandle;
RTC_DateTypeDef RTC_DateStructure, read_RTC_DateStruct;
RTC_TimeTypeDef RTC_TimeStructure, read_RTC_TimeStruct;



HAL_StatusTypeDef Hal_status;  //HAL_ERROR, HAL_TIMEOUT, HAL_OK, of HAL_BUSY 


//memory location to write to in the device
__IO uint16_t memLocation = 0x000A; //pick any location within range

char lcd_buffer[14];

void RTC_Config(void);
void RTC_AlarmAConfig(void);
void display_time(void);

void ExtBtn1_Config(void);  //for the first External button
void ExtBtn2_Config(void);

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Starter variables for states ----------------------------------------------*/
uint8_t readData=0x00;
uint32_t EE_status;
uint32_t ButtonA = 0;
uint32_t ButtonB = 0;
uint32_t write_state = 0;
uint32_t time;
uint32_t state = 0;

int dummy_year;
int dummy_month;
int dummy_day;
int dummy_hour;
int dummy_minute;
int dummy_second;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{

  HAL_Init();
	
	 /* Configure the system clock to 72 MHz */
  SystemClock_Config();
	
	HAL_InitTick(0x0000); // set systick's priority to the highest.

	//configure the USER button as exti mode. 
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);   // BSP_functions in stm32f429i_discovery.c


//Init LCD
	BSP_LCD_Init();
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);   
	BSP_LCD_SelectLayer(0);
	BSP_LCD_SetLayerVisible(0, ENABLE);
	
	
	BSP_LCD_Clear(LCD_COLOR_WHITE);  //need this line, otherwise, the screen is dark	
	BSP_LCD_DisplayOn();
 
	BSP_LCD_SetFont(&Font20);  //the default font,  LCD_DEFAULT_FONT, which is defined in _lcd.h, is Font24
	
	I2C_Init(&I2c3_Handle);


//*********************Initial state set up------------------

	LCD_DisplayString(0, 2, (uint8_t *)"MT2TA4 LAB 3");
	LCD_DisplayString(1,0,(uint8_t *)"HH:MM:SS");
	
	//set up RTC
	RTC_Config();
	RTC_AlarmAConfig(); 
	BSP_LED_Init(LED3);
	
	//Set up external interrupts and I2C
	I2C_Init(&I2c3_Handle);
	ExtBtn1_Config();
	ExtBtn2_Config();

	//display current time
	RTC_DateStructure.Month=3;
	RTC_DateStructure.Date=8;
	RTC_DateStructure.Year=23;
	HAL_RTC_SetDate(&RTCHandle, &RTC_DateStructure, RTC_FORMAT_BIN);
	
	RTC_TimeStructure.Hours=2;
	RTC_TimeStructure.Minutes=30;   
	RTC_TimeStructure.Seconds = 45;
	HAL_RTC_SetTime(&RTCHandle, &RTC_TimeStructure, RTC_FORMAT_BIN);
		
	//Read from RTC 
	HAL_RTC_GetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN);
	
	LCD_DisplayString(2,0,(uint8_t *) "  :  :");
	LCD_DisplayInt(2,0,RTC_TimeStructure.Hours);
	LCD_DisplayInt(2,3,RTC_TimeStructure.Minutes);
	LCD_DisplayInt(2,6,RTC_TimeStructure.Seconds);

	LCD_DisplayString(4,0, (uint8_t *) "Last Pressed:");
	
	/* Infinite loop */
while (1)
  {
		
		//this is needed because without it, the main while loop is re-entered for some reason??
		HAL_Delay(1);
		
		//default display of time if time is not being changed
		if (ButtonA == 0) {
			display_time();
		}
	}
}  

//clean function to constantly display new time everytime it is run, adding a little delay to remove flashing
void display_time(void) {
	HAL_RTC_GetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN);
	
	LCD_DisplayString(2,0,(uint8_t *) "  :  :  ");
	LCD_DisplayInt(2,0,RTC_TimeStructure.Hours);
	LCD_DisplayInt(2,3,RTC_TimeStructure.Minutes);
	LCD_DisplayInt(2,6,RTC_TimeStructure.Seconds);
	HAL_Delay(100);
}

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


/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */



/**
 * Use this function to configure the GPIO to handle input from
 * external pushbuttons and configure them so that you will handle
 * them through external interrupts.
 */
void ExtBtn1_Config(void)     // for GPIO C pin 1
// can only use PA0, PB0... to PA4, PB4 .... because only  only  EXTI0, ...EXTI4,on which the 
	//mentioned pins are mapped to, are connected INDIVIDUALLY to NVIC. the others are grouped! 
		//see stm32f4xx.h, there is EXTI0_IRQn...EXTI4_IRQn, EXTI15_10_IRQn defined
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOB clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_1;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void ExtBtn2_Config(void){  //**********PD2.***********
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//enable GPIOB clock
	__HAL_RCC_GPIOC_CLK_ENABLE();
	
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode =  GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
	//GPIO_InitStructure.Speed=GPIO_SPEED_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);   //is defined the same as the __HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_1); ---check the hal_gpio.h
	__HAL_GPIO_EXTI_CLEAR_FLAG(GPIO_PIN_2);// after moving the chunk of code in the GPIO_EXTI callback from _it.c (before these chunks are in _it.c)
																					//the program "freezed" when start, suspect there is a interupt pending bit there. Clearing it solve the problem.
	
  // Enable and set EXTI Line0 Interrupt to the lowest priority 
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 1);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}


void RTC_Config(void) {
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	/****************
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RTC_AlarmTypeDef RTC_AlarmStructure;
	****************/
	
	//1: Enable the RTC domain access (enable wirte access to the RTC )
			//1.1: Enable the Power Controller (PWR) APB1 interface clock:
        __HAL_RCC_PWR_CLK_ENABLE(); 
			//1.2:  Enable access to RTC domain 
				HAL_PWR_EnableBkUpAccess();
			//1.3: Select the RTC clock source
				__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);  //RCC_RTCCLKSOURCE_LSI is defined in hal_rcc.h
	       // according to P9 of AN3371 Application Note, LSI's accuracy is not suitable for RTC application!!!! 
					//can not use LSE!!!---LSE is not available, at least not available for stm32f407 board.
				//****"Without parts at X3, C16, C27, and removing SB15 and SB16, the LSE is not going to tick or come ready"*****.
			//1.4: Enable RTC Clock
			__HAL_RCC_RTC_ENABLE();   //enable RTC
			
	
			//1.5  Enable LSI
			__HAL_RCC_LSI_ENABLE();   //need to enable the LSI !!!
																//defined in _rcc.c
			while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY)==RESET) {}    //defind in rcc.c
	
			// for the above steps, please see the CubeHal UM1725, p616, section "Backup Domain Access" 	
				
				
				
	//2.  Configure the RTC Prescaler (Asynchronous and Synchronous) and RTC hour 
        
				RTCHandle.Instance = RTC;
				RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
				//RTC time base frequency =LSE/((AsynchPreDiv+1)*(SynchPreDiv+1))=1Hz
				//see the AN3371 Application Note: if LSE=32.768K, PreDiv_A=127, Prediv_S=255
				//    if LSI=32K, PreDiv_A=127, Prediv_S=249
				//also in the note: LSI accuracy is not suitable for calendar application!!!!!! 
				RTCHandle.Init.AsynchPrediv = 127; //if using LSE: Asyn=127, Asyn=255: 
				RTCHandle.Init.SynchPrediv = 249;  //if using LSI(32Khz): Asyn=127, Asyn=249: 
				// but the UM1725 says: to set the Asyn Prescaler a higher value can mimimize power comsumption
				
				RTCHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
				RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
				RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
				
				//HAL_RTC_Init(); 
				if(HAL_RTC_Init(&RTCHandle) != HAL_OK)
				{
						LCD_DisplayString(1, 0, (uint8_t *)"RTC Init Error!");
				}
	
	//3. init the time and date
				RTC_DateStructure.Year = 15;
				RTC_DateStructure.Month = RTC_MONTH_DECEMBER;
				RTC_DateStructure.Date = 18; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x18 for the 18th.
				RTC_DateStructure.WeekDay = RTC_WEEKDAY_FRIDAY; //???  if the real weekday is not correct for the given date, still set as 
																												//what is specified here.
				
				if(HAL_RTC_SetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better 
															//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(2, 0, (uint8_t *)"Date Init Error!");
				} 
  
  
				RTC_TimeStructure.Hours = 9;  
				RTC_TimeStructure.Minutes = 19; //if use RTC_FORMAT_BCD, NEED TO SET IT AS 0x19
				RTC_TimeStructure.Seconds = 29;
				RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
				RTC_TimeStructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
				RTC_TimeStructure.StoreOperation = RTC_STOREOPERATION_RESET;//?????/
				
				if(HAL_RTC_SetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN) != HAL_OK)   //BIN format is better
																																					//before, must set in BCD format and read in BIN format!!
				{
					LCD_DisplayString(3, 0, (uint8_t *)"TIME Init Error!");
				}
  
			//Writes a data in a RTC Backup data Register0   --why need this line?
			//HAL_RTCEx_BKUPWrite(&RTCHandle,RTC_BKP_DR0,0x32F2);   

	/*
			//The RTC Resynchronization mode is write protected, use the
			//__HAL_RTC_WRITEPROTECTION_DISABLE() befor calling this function.
			__HAL_RTC_WRITEPROTECTION_DISABLE(&RTCHandle);
			//wait for RTC APB registers synchronisation
			HAL_RTC_WaitForSynchro(&RTCHandle);
			__HAL_RTC_WRITEPROTECTION_ENABLE(&RTCHandle);				
	 */
				
				
			__HAL_RTC_TAMPER1_DISABLE(&RTCHandle);
			__HAL_RTC_TAMPER2_DISABLE(&RTCHandle);	
				//Optionally, a tamper event can cause a timestamp to be recorded. ---P802 of RM0090
				//Timestamp on tamper event
				//With TAMPTS set to ‘1 , any tamper event causes a timestamp to occur. In this case, either
				//the TSF bit or the TSOVF bit are set in RTC_ISR, in the same manner as if a normal
				//timestamp event occurs. The affected tamper flag register (TAMP1F, TAMP2F) is set at the
				//same time that TSF or TSOVF is set. ---P802, about Tamper detection
				//-------that is why need to disable this two tamper interrupts. Before disable these two, when program start, there is always a timestamp interrupt.
				//----also, these two disable function can not be put in the TSConfig().---put there will make  the program freezed when start. the possible reason is
				//-----one the RTC is configured, changing the control register again need to lock and unlock RTC and disable write protection.---See Alarm disable/Enable 
				//---function.
				
			HAL_RTC_WaitForSynchro(&RTCHandle);	
			//To read the calendar through the shadow registers after Calendar initialization,
			//		calendar update or after wake-up from low power modes the software must first clear
			//the RSF flag. The software must then wait until it is set again before reading the
			//calendar, which means that the calendar registers have been correctly copied into the
			//RTC_TR and RTC_DR shadow registers.The HAL_RTC_WaitForSynchro() function
			//implements the above software sequence (RSF clear and RSF check).	
}


void RTC_AlarmAConfig(void)
{
	
	RTC_AlarmTypeDef RTC_Alarm_Structure;

	RTC_Alarm_Structure.Alarm = RTC_ALARM_A;
  RTC_Alarm_Structure.AlarmMask = RTC_ALARMMASK_ALL;
  
  if(HAL_RTC_SetAlarm_IT(&RTCHandle,&RTC_Alarm_Structure,RTC_FORMAT_BCD) != HAL_OK)
  {
			LCD_DisplayString(4, 0, (uint8_t *)"Alarm setup Error!");
  }
	
	__HAL_RTC_ALARM_CLEAR_FLAG(&RTCHandle, RTC_FLAG_ALRAF); //need it? !!!!, without it, sometimes(SOMETIMES, when first time to use the alarm interrupt)
																			//the interrupt handler will not work!!! 		
	
		//need to set/enable the NVIC for RTC_Alarm_IRQn!!!!
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);   
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x00, 0);  //not important
	
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Disable(RTC_HandleTypeDef *hrtc) 
{ 
 	// Process Locked  
	__HAL_LOCK(hrtc);
  
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_DISABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  
}


HAL_StatusTypeDef  RTC_AlarmA_IT_Enable(RTC_HandleTypeDef *hrtc) 
{	
	// Process Locked  
	__HAL_LOCK(hrtc);	
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  // Disable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);
  
  // __HAL_RTC_ALARMA_ENABLE(hrtc);
    
   // In case of interrupt mode is used, the interrupt source must disabled 
   __HAL_RTC_ALARM_ENABLE_IT(hrtc, RTC_IT_ALRA);


 // Enable the write protection for RTC registers 
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);
  
  hrtc->State = HAL_RTC_STATE_READY; 
  
  // Process Unlocked 
  __HAL_UNLOCK(hrtc);  

}



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



/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */


//all code to increment each value, incremenents the dummy value and displays new value
void incrementYear(void){ // increments the year by one until it reaches 100
	
	dummy_year += 1;
  if (dummy_year >= 99){
		dummy_year = 0;
	}
	
	LCD_DisplayString(11,6,(uint8_t *)"  ");
	LCD_DisplayInt(11,6, dummy_year);

}

/* Increment month */
void incrementMonth(void) // increments the month by one until it reaches 13
{
	
	dummy_month += 1;
  if (dummy_month>12){
		dummy_month = 1;
	}
	
	LCD_DisplayString(11,3,(uint8_t *)"  ");
	LCD_DisplayInt(11,3, dummy_month);
}

/* Increment day */
void incrementDay(void) // increments the days by one until it reaches 32
{
	
	dummy_day += 1;
  if (dummy_day>31){
		dummy_day = 1;
	}
	
	LCD_DisplayString(11,0,(uint8_t *)"  ");
	LCD_DisplayInt(11,0, dummy_day);
}

/* Increment hour */
void incrementHour(void){ // increments the hour by one until it reaches 24
	
	dummy_hour += 1;
  if (dummy_hour > 23){
		dummy_hour = 0;
	}
	
	LCD_DisplayString(2,0,(uint8_t *)"  ");
	LCD_DisplayInt(2,0, dummy_hour);

}

/* Increment minute */
void incrementMinute(void){ // increments the minute by one until it reaches 60
  
	dummy_minute += 1;
  if (dummy_minute>59){
		dummy_minute = 0;
	}
	
	LCD_DisplayString(2,3,(uint8_t *)"  ");
	LCD_DisplayInt(2,3, dummy_minute);
}

/* Increment second */
void incrementSecond(void){ // increments the second by one until it reaches 60
  
	dummy_second += 1;
	if (dummy_second > 59) {
		dummy_second = 0;
	}
	
	LCD_DisplayString(2,6,(uint8_t *)"  ");
	LCD_DisplayInt(2,6, dummy_second);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
	//shows last time checked, and shows full date if held
  if(GPIO_Pin == KEY_BUTTON_PIN && ButtonA == 0)  //GPIO_PIN_0
  {
		
		
    time=HAL_GetTick();																		//starts counting time
    while(HAL_GPIO_ReadPin(GPIOA,KEY_BUTTON_PIN) == 1) {  // while user button is pressed
       
			//if hits over time, display once and constantly stay in state 1 until button is not pressed
			if ((HAL_GetTick()-time)> 600 && state == 0) {
				LCD_DisplayString(10,0, (uint8_t *) "DD:MM:YY");
				LCD_DisplayString(11,0, (uint8_t *) "  :  :  ");
				LCD_DisplayInt(11,0, RTC_DateStructure.Date);
				LCD_DisplayInt(11,3, RTC_DateStructure.Month);
				LCD_DisplayInt(11,6, RTC_DateStructure.Year);
				state = 1;
      }
			else if (state == 1) {
				display_time();
			}
    }
		LCD_DisplayString(10,0, (uint8_t *) "        ");
		LCD_DisplayString(11,0, (uint8_t *) "          ");
		
		//checks if button was not held for long enough, meaning that state would still be 0, and allow last time pressed to show
		if (state == 0) {
			
			//write old most recent to second recent
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+3, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+4, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2);
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+5, readData);
		
			//write current time to most recent time
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation, RTC_TimeStructure.Hours);
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1, RTC_TimeStructure.Minutes);
			EE_status=I2C_ByteWrite(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2, RTC_TimeStructure.Seconds);
		
			//display most recent time
			LCD_DisplayString(5, 0, (uint8_t *)"  :  :  ");
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
			LCD_DisplayInt(5, 0, readData);
			readData=I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
			LCD_DisplayInt(5, 3, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2);
			LCD_DisplayInt(5, 6, readData);
		}
		
		//set state back to 0 so that state will still work for the next times
		state = 0;
  }
	
	//allows you to change time
	if(GPIO_Pin == GPIO_PIN_1) {
		
		//write state will check which current write state it is in to know which next state to go to
		if (write_state == 0) {
			ButtonA = 1;
			ButtonB = 0;
			write_state = 1;
			LCD_DisplayString(7, 0, (uint8_t *)"                 ");
			LCD_DisplayString(8, 0, (uint8_t *)"                 ");
			LCD_DisplayString(13, 0, (uint8_t *)"Change Seconds");
			
			//takes all the current times and saves them to the dummy variables to be changed
		  dummy_year = RTC_DateStructure.Year;
		  dummy_month = RTC_DateStructure.Month;
		  dummy_day = RTC_DateStructure.Date;
		  dummy_hour = RTC_TimeStructure.Hours;
		  dummy_minute = RTC_TimeStructure.Minutes;
		  dummy_second = RTC_TimeStructure.Seconds;
		}
		else if (write_state == 1) {
			write_state = 2;
			LCD_DisplayString(13, 0, (uint8_t *)"Change Minutes");
		}
		else if (write_state == 2) {
			write_state = 3;
			LCD_DisplayString(13, 0, (uint8_t *)"Change Hours  ");
		}
		else if (write_state == 3) {
			write_state = 4;
			LCD_DisplayString(13, 0, (uint8_t *)"Change Days   ");
			
			//shows date month and year as soon as you are prompted to change these values
			LCD_DisplayString(10,0, (uint8_t *) "DD:MM:YY");
			LCD_DisplayString(11,0, (uint8_t *) "  :  :  ");
			LCD_DisplayInt(11,0, RTC_DateStructure.Date);
			LCD_DisplayInt(11,3, RTC_DateStructure.Month);
			LCD_DisplayInt(11,6, RTC_DateStructure.Year);
		}
		else if (write_state == 4) {
			write_state = 5;
			LCD_DisplayString(13, 0, (uint8_t *)"Change Months ");
		}
		else if (write_state == 5) {
			write_state = 6;
			LCD_DisplayString(13, 0, (uint8_t *)"Change Years  ");
		}
		else if (write_state == 6) {
			ButtonA = 0;
			write_state = 0;
			LCD_DisplayString(13, 0, (uint8_t *)"              ");
			LCD_DisplayString(10,0, (uint8_t *) "        ");
			LCD_DisplayString(11,0, (uint8_t *) "        ");
			
			//set all the changed values to RTC to update the values
			RTC_DateStructure.Year = dummy_year;
		  RTC_DateStructure.Month = dummy_month;
		  RTC_DateStructure.Date = dummy_day;
		  RTC_TimeStructure.Hours = dummy_hour;
		  RTC_TimeStructure.Minutes = dummy_minute;
		  RTC_TimeStructure.Seconds = dummy_second;
			HAL_RTC_SetTime(&RTCHandle,&RTC_TimeStructure,RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&RTCHandle,&RTC_DateStructure,RTC_FORMAT_BIN);
		}
		
	}  //end of PIN_1

	//displays two most recent times pressed, and changes time when prompted to
	if(GPIO_Pin == GPIO_PIN_2)
  {
		//turn display on
		if (ButtonB == 0 && ButtonA == 0) {
			LCD_DisplayString(7, 0, (uint8_t *)"Recent:   :  :");
			LCD_DisplayString(8, 0, (uint8_t *)"2nd:      :  :");
			
			//display most recent
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation);
			LCD_DisplayInt(7, 8, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+1);
			LCD_DisplayInt(7, 11, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+2);
			LCD_DisplayInt(7, 14, readData);
			
			//display 2nd most recent
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+3);
			LCD_DisplayInt(8, 8, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+4);
			LCD_DisplayInt(8, 11, readData);
			readData = I2C_ByteRead(&I2c3_Handle,EEPROM_ADDRESS, memLocation+5);
			LCD_DisplayInt(8, 14, readData);
			
			//set buttonB to 1 to know that the display is on
			ButtonB = 1;
		}
		
		//if display is already on and changing times is not prompted, clear the display
		else if (ButtonB == 1 && ButtonA == 0) {
			LCD_DisplayString(7, 0, (uint8_t *)"                ");
			LCD_DisplayString(8, 0, (uint8_t *)"                ");
	
			//set buttonB back to 0 so that it is known the the diplay is not shown
			ButtonB = 0;
		}
	
		//if time change is prompted, this button is used to actually change the value with the increment functions and write_state
		else if (ButtonA == 1) {
			
			//if write state is 1 and ButtonB is pressed, increment seconds
			if (write_state == 1) {
				incrementSecond();
			}
			else if (write_state == 2) {
				incrementMinute();
			}
			else if (write_state == 3) {
				incrementHour();
			}
			else if (write_state == 4) {
				incrementDay();
			}
			else if (write_state == 5) {
				incrementMonth();
			}
			else if (write_state == 6) {
				incrementYear();
			}
		}
	
	} //end of if PIN_2	
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
 
	//RTC_TimeShow();
	
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
