// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include <string.h>

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"
#include "temp_and_rh.h"

// Pin number define
#define TRIG		GPIO_PIN_0
#define ECHO		GPIO_PIN_1
#define LED1		GPIO_PIN_2
#define LED2		GPIO_PIN_3
#define JOG_UP		GPIO_PIN_0
#define JOG_DN		GPIO_PIN_1
#define JOG_CEN		GPIO_PIN_2
#define JOG_LT		GPIO_PIN_12
#define JOG_RT		GPIO_PIN_13
#define	PIEZO		GPIO_PIN_15
#define	MOTOR_P		GPIO_PIN_6
#define	MOTOR_N		GPIO_PIN_7
#define	MOTOR_PWM	GPIO_PIN_6
#define	CLCD_RS		GPIO_PIN_8
#define	CLCD_E		GPIO_PIN_9
#define	CLCD_DATA4	GPIO_PIN_12
#define	CLCD_DATA5	GPIO_PIN_13
#define	CLCD_DATA6	GPIO_PIN_14
#define	CLCD_DATA7	GPIO_PIN_15
#define USART1_TX	GPIO_PIN_9
#define USART1_RX	GPIO_PIN_10

// UART 통신을 위한 정의
#define TxBufferSize	(countof(UART_TxBuffer) - 1)	// 송신 버퍼 사이즈 정의
#define RxBufferSize	0xFF							// 수신 버퍼 사이즈를 0xFF로 정의
#define countof(a)		(sizeof(a) / sizeof(*(a)))		// 데이터 사이즈

// FND
#define FND_I2C_ADDR			0x20<<1
#define IN_PORT0				0x00
#define IN_PORT1				0x01
#define OUT_PORT0				0x02
#define OUT_PORT1				0x03
#define POLARITY_IVE_PORT0		0x04
#define POLARITY_IVE_PORT1		0x05
#define CONFIG_PORT0			0x06
#define CONFIG_PORT1			0x07

#define I2C_ADDRESS				FND_I2C_ADDR

// 전역변수 선언부
ADC_HandleTypeDef AdcHandler1, AdcHandler2, AdcHandler3;			// ADC의 초기화를 위한 구조체형의 변수를 선언
ADC_ChannelConfTypeDef sConfig;
int vr_val = 0, last_vr_val = 0, cds_val = 0, last_cds_val = 0, sound_val = 0;		// ADC값 저장 변수
unsigned long ultra_val =0;
// UART 통신용 변수 선언
UART_HandleTypeDef	UartHandle1;	// UART의 초기화를 위한 구조체형의 변수를 선언
char UART_TxBuffer[] = "/* ========= Workshop Test =========*/\n\r";
uint8_t UART_RxBuffer[RxBufferSize];

TIM_HandleTypeDef TimHandle2, TimHandle3, TimHandle5,TimHandle4, TimHandle12;
TIM_OC_InitTypeDef TIM_OCInit3, TIM_OCInit5, TIM_OCInit12;			// 타이머 Channel(OC)의 초기화용 구조체 변수를 선언
uint32_t LED_Pulse=10000, MOTOR_Pulse = 10000;
uint32_t timer_m, timer_s, timer_ms;
uint32_t cnt_state;
/*	Prescaler = 83로 설정(1us) 했을 때의 음계에 맞는 주파수를 계산한 값	*/
uint32_t PIEZO_Pulse[]	= {0,        3822,   3405,   3033,   2863,   2551,   2272,   2024,   1911}; //도레미파솔라시도
char* PIEZO_clcd[]		= {"mute", "DO4 ", "RE4 ", "MI4 ", "FA4 ", "SO4 ", "LA4 ", "SI4 ", "DO5 "};
uint32_t scale_val = 0, last_scale_val = 0;

GPIO_InitTypeDef GPIO_Init_Struct;
I2C_HandleTypeDef I2C2Handle;		// I2C의 초기화를 위한 구조체형의 변수를 선언
// FND 출력 데이터 선언
unsigned char FND_data[10] = {0xFC,0x60,0xDA,0xF2,0x66,0xB6,0x3E,0xE0,0xFE,0xF6};
//unsigned int FND_index[6] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB};
unsigned int FND_index[6] = {0xFB,0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
TIM_HandleTypeDef TimHandle4;
// I2C 통신용 변수 선언
uint8_t TxBuffer[5];

enum en_jog_st {
	YS_JOG_UP = 0, YS_JOG_DOWN, YS_JOG_CENT, YS_JOG_L, YS_JOG_R, YS_JOG_NON
};
enum en_motor_st {
	NOT_ING = 0, MOTOR_CCW, MOTOR_CW, MOTOR_SB, MOTOR_STOP
};
enum en_led_st {
	LED_ON=1, LED_OFF,
};
enum en_piezo_st {
	PIEZO_ON=1, PIEZO_OFF,
};

enum en_cnt_en {
	CNT_EN_ON =0,
	CNT_EN_OFF
};
uint32_t cnt_state = CNT_EN_OFF;
unsigned int state =YS_JOG_NON;
unsigned int motor_st =NOT_ING;
unsigned int led_st = LED_OFF;
unsigned int piezo_st = PIEZO_OFF;
unsigned int data ;





/*지연루틴*/
void ms_delay_int_count(volatile unsigned int nTime)
{
	nTime = (nTime * 14000);
	for(; nTime > 0; nTime--);
}

void us_delay_int_count(volatile unsigned int nTime)
{
	nTime = (nTime * 12);
	for(; nTime > 0; nTime--);
}



//;********************************** Ultra *****************************************
//PC0 = tirg ....... PC1 = Echo
int	open_Ultra(void)
{
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_Init_Struct.Pin = TRIG;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;	// Alternate Function Push Pull 모드
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_Init_Struct);

	GPIO_Init_Struct.Pin = ECHO; //PC1
	GPIO_Init_Struct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_Init_Struct.Pull = GPIO_PULLDOWN  ;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_Init_Struct);

	HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	return 0;
}

//;********************************** LED *****************************************
int	open_led(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_Init_Struct.Pin = LED1 | LED2;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;	// Alternate Function Push Pull 모드
	//GPIO_Init_Struct.Alternate = GPIO_AF2_TIM5;	// TIM5 Alternate Function mapping
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);
	return 0;
}

//;********************************** DC_MOTOR **************************************
int	open_dc_motor(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_Init_Struct.Pin = MOTOR_PWM;
	GPIO_Init_Struct.Mode = GPIO_MODE_AF_PP;	// Alternate Function Push Pull 모드
	GPIO_Init_Struct.Alternate = GPIO_AF2_TIM3;	// TIM3 Alternate Function mapping
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_Init_Struct.Pin = MOTOR_P | MOTOR_N;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);
	return 0;
}

int	dc_motor_cntl(uint8_t sel)
{
	switch (sel)
	{
	case 1:		// CCW
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 1);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 0);	// MOTOR_N
		//HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 2:		// CW
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 0);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 1);	// MOTOR_N
		//HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 3:		// Short break
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 1);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 1);	// MOTOR_N
		//HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 4:		// Stop
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 0);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 0);	// MOTOR_N
		//HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	}

	return 0;
}

//;********************************** PIEZO *****************************************
int	open_piezo(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_Init_Struct.Pin = PIEZO;
	GPIO_Init_Struct.Mode = GPIO_MODE_AF_PP;		// Alternate Function Push Pull 모드
	GPIO_Init_Struct.Alternate = GPIO_AF9_TIM12;	// TIM12 Alternate Function mapping
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);
	return 0;
}

//;********************************** JOG_SW  **************************************
int	open_jogsw(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_Init_Struct.Pin = JOG_UP | JOG_DN | JOG_CEN |JOG_LT |JOG_RT;
	GPIO_Init_Struct.Mode = GPIO_MODE_IT_RISING;
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);

	/* Enable and set EXTI Line2 Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	HAL_NVIC_SetPriority(EXTI1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);

	HAL_NVIC_SetPriority(EXTI2_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	return 0;
}

int	open_tactsw(void)
{	//PA11, PA12
	// TACT_SW가 연결된 GPIO 장치를 인터럽트 방식으로 초기화 한다.
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* Configure PB0~PB2 pin as input floating */
	GPIO_Init_Struct.Pin		= GPIO_PIN_8 | GPIO_PIN_11;
	GPIO_Init_Struct.Mode		= GPIO_MODE_IT_RISING; //RISING INT
	GPIO_Init_Struct.Pull		= GPIO_NOPULL;
	GPIO_Init_Struct.Speed	= GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	/* Enable and set EXTI Line0~2 Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	return 0;
}

//;********************************** CLCD  *****************************************

int CLCD_write(unsigned char rs, char data)
{
	HAL_GPIO_WritePin(GPIOC, CLCD_RS, rs);					// CLCD_RS
	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_RESET);		// CLCD_E = 0
	us_delay_int_count(2);

	HAL_GPIO_WritePin(GPIOC, CLCD_DATA4, (data>>4) & 0x1);	// CLCD_DATA = LOW_bit
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA5, (data>>5) & 0x1);
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA6, (data>>6) & 0x1);
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA7, (data>>7) & 0x1);

	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_SET);			// CLCD_E = 1
	us_delay_int_count(2);
	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_RESET);		// CLCD_E = 0
	us_delay_int_count(2);

	HAL_GPIO_WritePin(GPIOC, CLCD_DATA4, (data>>0) & 0x1);	// CLCD_DATA = HIGH_bit
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA5, (data>>1) & 0x1);
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA6, (data>>2) & 0x1);
	HAL_GPIO_WritePin(GPIOC, CLCD_DATA7, (data>>3) & 0x1);

	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_SET);			// CLCD_E = 1
	us_delay_int_count(2);
	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_RESET);		// CLCD_E = 0
	ms_delay_int_count(2);
	return 0;
}

int clcd_put_string(uint8_t *str)
{
	while(*str)
	{
		CLCD_write(1,(uint8_t)*str++);
	}
	return 0;
}

int CLCD_init()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	// CLCD_E = 0
	CLCD_write(0, 0x33);	// 4비트 설정 특수 명령
	CLCD_write(0, 0x32);	// 4비트 설정 특수 명령
	CLCD_write(0, 0x28);	// _set_function
	CLCD_write(0, 0x0F);	// _set_display
	CLCD_write(0, 0x01);	// clcd_clear
	CLCD_write(0, 0x06);	// _set_entry_mode
	CLCD_write(0, 0x02);	// Return home
	return 0;
}

int	CLCD_config(void)
{
	__HAL_RCC_GPIOC_CLK_ENABLE();

	// CLCD_RS(PC8), CLCD_E(PC9, DATA 4~5(PC12~15)
	GPIO_Init_Struct.Pin = CLCD_RS | CLCD_E | CLCD_DATA4 | CLCD_DATA5 | CLCD_DATA6 | CLCD_DATA7;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_FAST;
	HAL_GPIO_Init(GPIOC, &GPIO_Init_Struct);

	HAL_GPIO_WritePin(GPIOC, CLCD_RS, GPIO_PIN_RESET);	// CLCD_E = 0
	HAL_GPIO_WritePin(GPIOC, CLCD_E, GPIO_PIN_RESET);	// CLCD_RW = 0
	return 0;
}

int	open_clcd(void)
{
	CLCD_config();
	CLCD_init();
	//clcd_put_string((uint8_t*)"Initialization complete!!");
	return 0;
}

//;********************************** TIMER  *****************************************
int timer2_config(void)
{
	__HAL_RCC_TIM2_CLK_ENABLE();
	TimHandle2.Instance 			= TIM2;						// TIM2 사용
	TimHandle2.Init.Period 			= 300000 - 1;				// 업데이트 이밴드 발생시 ARR=999로 설정(100ms)
	TimHandle2.Init.Prescaler 		= 84 - 1;					// Prescaler = 83로 설정(0.001ms)
	TimHandle2.Init.ClockDivision	= TIM_CLOCKDIVISION_DIV1;	// division을 사용하지 않음
	TimHandle2.Init.CounterMode 	= TIM_COUNTERMODE_UP;		// Up Counter 모드 설정
	HAL_TIM_Base_Init(&TimHandle2);
	HAL_TIM_Base_Start_IT(&TimHandle2);	/* Start Channel1 */

	HAL_NVIC_SetPriority(TIM2_IRQn, 6, 0);	/* Set Interrupt Group Priority */
	HAL_NVIC_EnableIRQ(TIM2_IRQn);	/* Enable the TIMx global Interrupt */

	return 0;
}

int timer3_config(void)
{
	__HAL_RCC_TIM3_CLK_ENABLE();

	/* Set TIMx instance */
	TimHandle3.Instance 			= TIM3;						// TIM3 사용
	//TimHandle3.Init.Period 			= MOTOR_Pulse - 1;			// 업데이트 이밴드 발생시 ARR=9999로 설정
	TimHandle3.Init.Period 			= 3000 - 1;					// 업데이트 이밴드 발생시 ARR=2999로 설정(300ms)
	TimHandle3.Init.Prescaler 		= 8400 - 1;					// Prescaler = 8399로 설정(0.1ms)
	//TimHandle3.Init.Prescaler 		= 0;						// 최고치의 타이머 주파수를 얻기 위해 Prescaler = 0으로 설정
	TimHandle3.Init.ClockDivision 	= TIM_CLOCKDIVISION_DIV1;	// division을 사용하지 않음
	TimHandle3.Init.CounterMode 	= TIM_COUNTERMODE_UP;		// Up Counter 모드 설정

	/* Set TIMx PWM instance */
	TIM_OCInit3.OCMode 		= TIM_OCMODE_PWM1;					// PWM mode 1 동작 모드 설정
	TIM_OCInit3.Pulse 		= 0;								// CCR의 설정값
	HAL_TIM_PWM_Init(&TimHandle3);								// TIM PWM을 TimHandle에 설정된 값으로 초기화함
	// TIM PWM의 Channel을  TIM_OCInit에 설정된 값으로 초기화함
	HAL_TIM_PWM_ConfigChannel(&TimHandle3, &TIM_OCInit3, TIM_CHANNEL_1);

	return 0;
}

int timer5_config(void) //CDS 센서? -> LED
{
	__HAL_RCC_TIM5_CLK_ENABLE();

	/* Set TIMx instance */
	TimHandle5.Instance 			= TIM5;						// TIM2 사용
	TimHandle5.Init.Period 			= LED_Pulse - 1;			// 업데이트 이밴드 발생시 ARR=9999로 설정
	TimHandle5.Init.Prescaler 		= 0;						// 최고치의 타이머 주파수를 얻기 위해 Prescaler = 0으로 설정
	TimHandle5.Init.ClockDivision 	= TIM_CLOCKDIVISION_DIV1;	// division을 사용하지 않음
	TimHandle5.Init.CounterMode 	= TIM_COUNTERMODE_UP;		// Up Counter 모드 설정

	/* Set TIMx PWM instance */
	TIM_OCInit5.OCMode 		= TIM_OCMODE_PWM1;					// PWM mode 1 동작 모드 설정
	TIM_OCInit5.Pulse 		= 0;								// CCR의 설정값
	HAL_TIM_PWM_Init(&TimHandle5);								// TIM PWM을 TimHandle에 설정된 값으로 초기화함
	// TIM PWM의 Channel을  TIM_OCInit에 설정된 값으로 초기화함
	HAL_TIM_PWM_ConfigChannel(&TimHandle5, &TIM_OCInit5, TIM_CHANNEL_3);
	HAL_TIM_PWM_ConfigChannel(&TimHandle5, &TIM_OCInit5, TIM_CHANNEL_4);

	return 0;
}

int timer4_config(void) //stop watch
{
	__HAL_RCC_TIM4_CLK_ENABLE();

	/* Set TIMx instance */
	TimHandle4.Instance 			= TIM4;						// TIM4 사용
	TimHandle4.Init.Period 			= 10-1;			// 0.01ms * 10 = (0.1ms)
	TimHandle4.Init.Prescaler 		= 840 - 1;			// 0.01ms(10us)
	TimHandle4.Init.ClockDivision	= TIM_CLOCKDIVISION_DIV1;	// division을 사용하지 않음
	TimHandle4.Init.CounterMode 	= TIM_COUNTERMODE_UP;		// Up Counter 모드 설정
	HAL_TIM_Base_Init(&TimHandle4);
	HAL_TIM_Base_Start_IT(&TimHandle4);	/* Start Channel1 */

	HAL_NVIC_SetPriority(TIM4_IRQn, 7, 0);	/* Set Interrupt Group Priority */
	HAL_NVIC_EnableIRQ(TIM4_IRQn);	/* Enable the TIMx global Interrupt */

	return 0;
}

int timer12_config(void) //piezo에 사용
{
	__HAL_RCC_TIM12_CLK_ENABLE();

	/* Set TIMx instance */
	TimHandle12.Instance 			= TIM12;						// TIM12 사용
	TimHandle12.Init.Period 		= PIEZO_Pulse[scale_val] - 1;
	TimHandle12.Init.Prescaler 		= 84 - 1;						// Prescaler = 83로 설정(1us)
	TimHandle12.Init.ClockDivision 	= TIM_CLOCKDIVISION_DIV1;		// division을 사용하지 않음
	TimHandle12.Init.CounterMode 	= TIM_COUNTERMODE_UP;			// Up Counter 모드 설정

	/* Set TIMx PWM instance */
	TIM_OCInit12.OCMode 		= TIM_OCMODE_PWM1;					// PWM mode 1 동작 모드 설정
	TIM_OCInit12.Pulse 			= (PIEZO_Pulse[scale_val]/2) - 1;	// CCR의 설정값
	HAL_TIM_PWM_Init(&TimHandle12);									// TIM PWM을 TimHandle에 설정된 값으로 초기화함
	// TIM PWM의 Channel을  TIM_OCInit에 설정된 값으로 초기화함
	HAL_TIM_PWM_ConfigChannel(&TimHandle12, &TIM_OCInit12, TIM_CHANNEL_2);

	return 0;
}

int timer_initialize(void)
{
	timer2_config();	// ADC 갱신 주기 설정
	//timer3_config();	// MOTOR PWM 설정(CH1)
	//timer4_config();
	//timer5_config();	// LED PWM 설정(CH3~CH4)
	//timer12_config();	// PIEZO PWM 설정(CH2)

	return 0;
}

//;********************************** ADC  *****************************************
int	adc1_config(void)
{
	__HAL_RCC_ADC1_CLK_ENABLE();					// ADC1 클럭 활성화
	__HAL_RCC_GPIOA_CLK_ENABLE();					// ADC1 핀으로 사용할 GPIOx 활성화(CDS:PA0)

	GPIO_Init_Struct.Pin = GPIO_PIN_0;				// GPIO에서 사용할 PIN 설정
	GPIO_Init_Struct.Mode = GPIO_MODE_ANALOG; 		// Input Analog Mode 모드
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	AdcHandler1.Instance 				= ADC1;							// ADC1 설정
	AdcHandler1.Init.ClockPrescaler 	= ADC_CLOCK_SYNC_PCLK_DIV2;		// ADC1 clock prescaler
	AdcHandler1.Init.Resolution 		= ADC_RESOLUTION_12B;			// ADC1 resolution
	AdcHandler1.Init.DataAlign 			= ADC_DATAALIGN_RIGHT;			// ADC1 data alignment
	AdcHandler1.Init.ScanConvMode 		= DISABLE;						// ADC1 scan 모드 비활성화
	AdcHandler1.Init.ContinuousConvMode = DISABLE;						// ADC1 연속 모드 비활성화
	AdcHandler1.Init.NbrOfConversion 	= 1;							// ADC1 변환 개수 설정
	AdcHandler1.Init.ExternalTrigConv 	= ADC_SOFTWARE_START;			// ADC1 외부 트리거 OFF
	HAL_ADC_Init(&AdcHandler1);		// ADC1를 설정된 값으로 초기화

	sConfig.Channel 		= ADC_CHANNEL_0;			// ADC1 채널 설정(PA는 채널 0번)
	sConfig.Rank 			= 1;						// ADC1 채널 순위 설정
	sConfig.SamplingTime 	= ADC_SAMPLETIME_3CYCLES;	// ADC1 샘플링 타임 설정(3클럭)
	HAL_ADC_ConfigChannel(&AdcHandler1, &sConfig);		// 채널  설정

	return 0;
}

int	adc2_config(void)
{
	__HAL_RCC_ADC2_CLK_ENABLE();					// ADC2 클럭 활성화
	__HAL_RCC_GPIOA_CLK_ENABLE();					// ADC2 핀으로 사용할 GPIOx 활성화(VR:PA4, SOUND:PA5)

	GPIO_Init_Struct.Pin = GPIO_PIN_4 | GPIO_PIN_5;				// GPIO에서 사용할 PIN 설정
	GPIO_Init_Struct.Mode = GPIO_MODE_ANALOG; 		// Input Analog Mode 모드
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	AdcHandler2.Instance 				= ADC2;							// ADC2 설정
	AdcHandler2.Init.ClockPrescaler 	= ADC_CLOCK_SYNC_PCLK_DIV2;		// ADC2 clock prescaler
	AdcHandler2.Init.Resolution 		= ADC_RESOLUTION_12B;			// ADC2 resolution
	AdcHandler2.Init.DataAlign 			= ADC_DATAALIGN_RIGHT;			// ADC2 data alignment
	AdcHandler2.Init.ScanConvMode 		= DISABLE;						// ADC2 scan 모드 비활성화
	AdcHandler2.Init.ContinuousConvMode = DISABLE;						// ADC2 연속 모드 비활성화
	AdcHandler2.Init.NbrOfConversion 	= 1;							// ADC2 변환 개수 설정
	AdcHandler2.Init.ExternalTrigConv 	= ADC_SOFTWARE_START;			// ADC2 외부 트리거 OFF
	AdcHandler2.Init.EOCSelection		= ADC_EOC_SEQ_CONV;
	HAL_ADC_Init(&AdcHandler2);		// ADC2를 설정된 값으로 초기화

	return 0;
}

int	adc_initialize(void)
{
	adc1_config();		// ADC1을 이용하여 CDS값을 읽도록 설정
	adc2_config();		// ADC2을 이용하여 VR,SOUND값을 읽도록 설정

	/* NVIC configuration */
	HAL_NVIC_SetPriority(ADC_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(ADC_IRQn);

	return 0;
}

void I2C_config(void)		// I2C2_SCL(PB8), I2C2_SDA(PB9)
{
	// I2C의 클럭을 활성화
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_I2C2_CLK_ENABLE();	// I2C2

	// GPIO B포트 8,9번 핀을 I2C 전환기능으로 설정
	GPIO_Init_Struct.Pin		= GPIO_PIN_10 | GPIO_PIN_11;	// GPIO에서 사용할 PIN 설정
	GPIO_Init_Struct.Mode		= GPIO_MODE_AF_OD;				// Alternate Function Open Drain 모드
	GPIO_Init_Struct.Alternate	= GPIO_AF4_I2C2;				// I2C2 Alternate Function mapping
	GPIO_Init_Struct.Pull		= GPIO_PULLUP;					// Pull Up 모드
	GPIO_Init_Struct.Speed		= GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);

	// I2C의 동작 조건 설정
	I2C2Handle.Instance 			= I2C2;
	I2C2Handle.Init.ClockSpeed 		= 400000;
	I2C2Handle.Init.DutyCycle 		= I2C_DUTYCYCLE_16_9;
	I2C2Handle.Init.OwnAddress1 	= 0x40<<1;//I2C_ADDRESS;
	I2C2Handle.Init.AddressingMode	= I2C_ADDRESSINGMODE_7BIT;
	I2C2Handle.Init.DualAddressMode	= I2C_DUALADDRESS_DISABLE;
	I2C2Handle.Init.OwnAddress2 	= 0;
	I2C2Handle.Init.GeneralCallMode	= I2C_GENERALCALL_DISABLE;
	I2C2Handle.Init.NoStretchMode 	= I2C_NOSTRETCH_DISABLE;

	// I2C 구성정보를 I2CxHandle에 설정된 값으로 초기화 함
	HAL_I2C_Init(&I2C2Handle);
}
void FND_clear(void)
{
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xFF;		TxBuffer[2] = 0x00;
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	//HAL_Delay(0.01);
}

void i2cSendValue(void)
{
	unsigned int temp;
	//timer_m, timer_s, timer_ms;
	temp = timer_m/10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xFB;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();

	temp = timer_m%10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xBF;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();

	temp = timer_s/10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xDF;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();

	temp = timer_s%10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xEF;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();

	temp = timer_ms/10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xF7;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();

	temp = timer_ms%10;
	TxBuffer[0] = OUT_PORT0;	TxBuffer[1] = 0xFB;		TxBuffer[2] = FND_data[temp];
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 3, 100);
	FND_clear();
}

//;********************************** UART  *****************************************
int USART1_config(void)		// USART1_TX(PA9), USART1_RX(PA10)
{
	// UART1의 클럭을 활성화
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();

	// GPIO A포트 9번 핀을 USART Tx, 10번 핀을 USART Rx로 설정
	GPIO_Init_Struct.Pin	= USART1_TX | USART1_RX;
	GPIO_Init_Struct.Mode	= GPIO_MODE_AF_PP;
	GPIO_Init_Struct.Pull	= GPIO_NOPULL;
	GPIO_Init_Struct.Speed	= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_Init_Struct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	// UART의 동작 조건 설정
	UartHandle1.Instance		= USART1;
	UartHandle1.Init.BaudRate	= 9600;
	UartHandle1.Init.WordLength	= UART_WORDLENGTH_8B;
	UartHandle1.Init.StopBits	= UART_STOPBITS_1;
	UartHandle1.Init.Parity		= UART_PARITY_NONE;
	UartHandle1.Init.HwFlowCtl	= UART_HWCONTROL_NONE;
	UartHandle1.Init.Mode		= UART_MODE_TX_RX;
	UartHandle1.Init.OverSampling = UART_OVERSAMPLING_16;

	// UART 구성정보를 UartHandle에 설정된 값으로 초기화 함
	HAL_UART_Init(&UartHandle1);

	// TxBuffer에 저장되어 있는 내용을 PC로 보낸다.
	HAL_UART_Transmit(&UartHandle1, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);

	return 0;
}

//;*************************** board_initialize  ************************************
int	board_initialize(void)
{
	open_Ultra();
	open_led();			// PA2~PA3(AF Push Pull Mode)
	//open_dc_motor();	// PA6(AF Push Pull Mode), PB6~PB7 output
	//open_piezo();		// PB15(AF Push Pull Mode)
	//open_jogsw();		// PB2, PB12, PB13 input
	open_clcd();		// PC8~PC9, PC12~PC15 output
	I2C_config();

	return 0;
}

void timer_inc_set()
{
	//timer_m, timer_s, timer_ms
	timer_ms++;
	if(timer_ms >= 100){
		timer_ms =0;
		timer_s++;
	}

	if(timer_s >= 60){
		timer_s=0;
		timer_m++;
	}

	if(timer_m >= 60){
		timer_m =0;
	}
}

int main(int argc, char* argv[])
{
	double test_temp=0;
	uint32_t ul_time=0;
	board_initialize();
	adc_initialize();
	USART1_config();
	timer_initialize();

	char clcd_buf[] = "";
	HAL_GPIO_WritePin(GPIOA, LED2, 0 );
	//dc_motor_cntl(1);	// CCW

	// Infinite loop
	//init_temp_humi_sensor(&I2C2Handle);

	  char x[__SIZEOF_DOUBLE__];

	  //puts(x);

	while (1)
	{

		ul_time = __HAL_TIM_GET_COUNTER(&TimHandle2);
		//ul_time = HAL_TIM_Base_GetState(&TimHandle2);
		if(ul_time>10){
			HAL_GPIO_WritePin(GPIOC, TRIG, 0);
			HAL_GPIO_WritePin(GPIOA, LED1, 0);
		}

		/*
		ms_delay_int_count(300);
		HAL_GPIO_WritePin(GPIOC, TRIG, 1);
		us_delay_int_count(10);
		HAL_GPIO_WritePin(GPIOC, TRIG, 0);
		*/
		//==========LCD 1st line print
		CLCD_write(0, 0x80);
		//sprintf( clcd_buf, "ULTRA : %4lu", 1000) ;
		sprintf( clcd_buf, "ULTRA : %4lu", ultra_val) ;
		//sprintf( clcd_buf, "CDS:%4d,VR:%4d", cds_val, vr_val) ;
		clcd_put_string((uint8_t*)clcd_buf);

		//==========LCD 2nd line print
		CLCD_write(0, 0xC0);


		test_temp = get_temp_data(&I2C2Handle);
		sprintf(x, "%g", test_temp);
		//sprintf( clcd_buf, "SOUND:%4d", (int)TimHandle12.Init.Period) ;
		sprintf( clcd_buf, "TEMP:%s", x) ;
		clcd_put_string((uint8_t*)clcd_buf);






	}
}

/*인터럽트 루틴*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//HAL_GPIO_WritePin(GPIOA, LED2, 1);
	//GPIO_PinState pin_state;
	static uint32_t ult_time =0;

	if(GPIO_Pin == ECHO){//PC1

		if(HAL_GPIO_ReadPin(GPIOC, ECHO) == GPIO_PIN_SET)
		{
			ult_time =__HAL_TIM_GET_COUNTER(&TimHandle2);
			HAL_GPIO_WritePin(GPIOA, LED2, 1);
			//HAL_TIM_Base_Start_IT(&TimHandle2);	/* Start Channel1 */
		}
		else
		{
			HAL_GPIO_WritePin(GPIOA, LED2, 0 );
			ult_time = __HAL_TIM_GET_COUNTER(&TimHandle2)- ult_time ;
			ultra_val = (unsigned long)(((ult_time/2)/2.9));
		}
	}


	return;

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	//static int tic_count =0;
	if(htim->Instance == TIM2){
		// Interrupt PC1을 Rising 시킨다.
		HAL_GPIO_WritePin(GPIOC, TRIG, 1);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOA, LED1, 1);
	}
	/*if(htim->Instance == TIM4){
		if(cnt_state == CNT_EN_ON) tic_count++;
		i2cSendValue();
		if(tic_count == 10){ //0.1초 마다
			timer_inc_set();
			tic_count=0;
		}

	}*/
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
