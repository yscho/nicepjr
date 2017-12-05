/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"



//extern GPIO_InitTypeDef 		GPIO_Init_Struct;
extern ADC_HandleTypeDef	 	AdcHandler1;
extern ADC_HandleTypeDef	 	AdcHandler2;
//extern ADC_ChannelConfTypeDef 	sConfig;
extern TIM_HandleTypeDef 		TimHandle2;		// 타이머의 초기화용 구조체 변수를 선언
extern TIM_HandleTypeDef 		TimHandle3;
extern TIM_HandleTypeDef 		TimHandle5;
extern TIM_HandleTypeDef 		TimHandle4;
extern TIM_HandleTypeDef 		TimHandle12;

//extern TIM_OC_InitTypeDef 		TIM_OCInit;

void ADC_IRQHandler(void)
{
	HAL_ADC_IRQHandler(&AdcHandler1);
	HAL_ADC_IRQHandler(&AdcHandler2);
}

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle2);
}

void TIM3_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle3);
}

void TIM5_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle5);
}
void TIM4_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle4);
}


void TIM12_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle12);
}

void EXTI0_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); //jog UP
}

void EXTI1_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);//jog down
}

void EXTI2_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);//jog cent
}

void EXTI9_5_IRQHandler(void)
{
	 HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);//Tact UP
}

void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);//Tact DOWN
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);//jog L
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);//jog R
}


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles External interrupt request.
  * @param  None
  * @retval None
  */



/**
  * @brief  This function handles Timer interrupt request.
  * @param  None
  * @retval None
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
