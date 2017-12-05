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

// Pin number define
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

// 온습도 센서
#define HT_I2C_ADDR				0x40<<1
#define I2C_ADDRESS				HT_I2C_ADDR


// UART 통신을 위한 정의
#define TxBufferSize	0xFF	// 송신 버퍼 사이즈 정의
#define RxBufferSize	0xFF							// 수신 버퍼 사이즈를 0xFF로 정의

// UART 통신용 변수 선언
UART_HandleTypeDef	UartHandle1;	// UART의 초기화를 위한 구조체형의 변수를 선언
UART_HandleTypeDef	UartHandle4;	// Bluetooth
char UART_TxBuffer[TxBufferSize];
uint8_t UART_RxBuffer[RxBufferSize];
enum {LED_ON, LED_OFF, MOTOR_ON, MOTOR_OFF, HT_GET};
char *uartMsg[]={"LED_ON\r\n","LED_OFF\r\n","MOTOR_ON\r\n","MOTOR_OFF\r\n","HT_GET\r\n"};

GPIO_InitTypeDef GPIO_Init_Struct;

I2C_HandleTypeDef I2C2Handle;		// I2C의 초기화를 위한 구조체형의 변수를 선언

// I2C 통신용 변수 선언
uint8_t TxBuffer[5];
uint8_t RxBuffer[5];


/* FUNCTION PROTOTYPE */
void I2C_config(void);
int i2cGetTemp(void);
void ms_delay_int_count(volatile unsigned int nTime);
void ms_delay_int_count(volatile unsigned int nTime);
void us_delay_int_count(volatile unsigned int nTime);
int	open_led(void);
int	open_dc_motor(void);
int	dc_motor_cntl(uint8_t sel);
int CLCD_write(unsigned char rs, char data);
int clcd_put_string(uint8_t *str);
int CLCD_init();
int	CLCD_config(void);
int	open_clcd(void);
int USART1_config(void);
int UART4_config(void);
int	board_initialize(void);
void sprintfTemp(double v, int decimalDigits,char *buff);
void BT_config();


// -- I2C의 초기설정을 위한 함수
void I2C_config(void)		// I2C2_SCL(PB10), I2C2_SDA(PB11)
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
	I2C2Handle.Init.OwnAddress1 	= I2C_ADDRESS;
	I2C2Handle.Init.AddressingMode	= I2C_ADDRESSINGMODE_7BIT;
	I2C2Handle.Init.DualAddressMode	= I2C_DUALADDRESS_DISABLE;
	I2C2Handle.Init.OwnAddress2 	= 0;
	I2C2Handle.Init.GeneralCallMode	= I2C_GENERALCALL_DISABLE;
	I2C2Handle.Init.NoStretchMode 	= I2C_NOSTRETCH_DISABLE;

	// I2C 구성정보를 I2CxHandle에 설정된 값으로 초기화 함
	HAL_I2C_Init(&I2C2Handle);
}

int i2cGetTemp(void)
{
	int temp_val;

	TxBuffer[0] = 0xE3;
	HAL_I2C_Master_Transmit(&I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&TxBuffer, 1, 100);
	HAL_I2C_Master_Receive( &I2C2Handle, (uint16_t)I2C_ADDRESS, (uint8_t*)&RxBuffer, 3, 100);
	temp_val = RxBuffer[0]<<8|RxBuffer[1];

	return temp_val;

}
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

//;********************************** LED *****************************************
int	open_led(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	GPIO_Init_Struct.Pin = LED1 | LED2;
//	GPIO_Init_Struct.Mode = GPIO_MODE_AF_PP;	// Alternate Function Push Pull 모드
//	GPIO_Init_Struct.Alternate = GPIO_AF2_TIM5;	// TIM5 Alternate Function mapping
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;
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
//	GPIO_Init_Struct.Mode = GPIO_MODE_AF_PP;
//	GPIO_Init_Struct.Alternate = GPIO_AF2_TIM3;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init_Struct.Pull = GPIO_NOPULL;
	GPIO_Init_Struct.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_Init_Struct.Pin = MOTOR_P | MOTOR_N;
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);

	HAL_GPIO_WritePin(GPIOB,MOTOR_P | MOTOR_N, 0);
	return 0;
}

int	dc_motor_cntl(uint8_t sel)
{
	switch (sel)
	{
	case 1:		// CCW
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 1);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 0);	// MOTOR_N
		HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 2:		// CW
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 0);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 1);	// MOTOR_N
		HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 3:		// Short break
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 1);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 1);	// MOTOR_N
		HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 1);	// MOTOR_PWM
		break;
	case 4:		// Stop
		HAL_GPIO_WritePin(GPIOB, MOTOR_P, 0);	// MOTOR_P
		HAL_GPIO_WritePin(GPIOB, MOTOR_N, 0);	// MOTOR_N
		HAL_GPIO_WritePin(GPIOA, MOTOR_PWM, 0);	// MOTOR_PWM
		break;
	}

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
	return 0;
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
	return 0;
}

int UART4_config(void)		// UART2_TX(PA0), UART1_RX(PA1)
{
	// UART3의 클럭을 활성화
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_UART4_CLK_ENABLE();

	// GPIO A포트 2번 핀을 USART Tx, 3번 핀을 USART Rx로 설정
	GPIO_Init_Struct.Pin	= GPIO_PIN_0 | GPIO_PIN_1 ;
	GPIO_Init_Struct.Mode	= GPIO_MODE_AF_PP;
	GPIO_Init_Struct.Pull	= GPIO_NOPULL;
	GPIO_Init_Struct.Speed	= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_Init_Struct.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	// UART의 동작 조건 설정
	UartHandle4.Instance		= UART4;
	UartHandle4.Init.BaudRate	= 9600;
	UartHandle4.Init.WordLength	= UART_WORDLENGTH_8B;
	UartHandle4.Init.StopBits	= UART_STOPBITS_1;
	UartHandle4.Init.Parity		= UART_PARITY_NONE;
	UartHandle4.Init.HwFlowCtl	= UART_HWCONTROL_NONE;
	UartHandle4.Init.Mode		= UART_MODE_TX_RX;
	UartHandle4.Init.OverSampling = UART_OVERSAMPLING_16;

	// UART 구성정보를 UartHandle에 설정된 값으로 초기화 함
	HAL_UART_Init(&UartHandle4);

	HAL_NVIC_SetPriority(UART4_IRQn,0,0);
	HAL_NVIC_EnableIRQ(UART4_IRQn);

	return 0;
}

//;*************************** board_initialize  ************************************
int	board_initialize(void)
{
	open_led();			// PA2~PA3(AF Push Pull Mode)
	open_dc_motor();	// PA6(AF Push Pull Mode), PB6~PB7 output
	open_clcd();		// PC8~PC9, PC12~PC15 output
	return 0;
}

void sprintfTemp(double v, int decimalDigits,char *buff)
{
	int i = 1;
	int intPart, fractPart;
	for (;decimalDigits!=0; i*=10, decimalDigits--);
	intPart = (int)v;
	fractPart = (int)((v-(double)(int)v)*i);
	if(fractPart < 0) fractPart *= -1;
	sprintf(buff,"temp : %i.%i\r\n", intPart, fractPart);
}


void BT_config()
{
	UART4_config();

	strcpy(UART_TxBuffer,"AT+NAMESW32A_TEAM4");
	HAL_UART_Transmit(&UartHandle4, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);
	ms_delay_int_count(2000);

	strcpy(UART_TxBuffer,"AT+PIN1234");
	HAL_UART_Transmit(&UartHandle4, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);
}

int main(int argc, char* argv[])
{
	board_initialize();
	I2C_config();
	USART1_config();
	BT_config();


	strcpy(UART_TxBuffer,"Bluetooth Ready\r\n");
	HAL_UART_Transmit(&UartHandle1, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);

	while (1)
	{
		HAL_UART_Receive_IT(&UartHandle4, UART_RxBuffer,1);
	}
}

/*인터럽트 루틴*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	int rxData;
	double temp;
	int temp_int;

	if(huart->Instance==UART4)
	{
		rxData = UART_RxBuffer[0];
		if(0<=rxData && (unsigned int)rxData <= sizeof(uartMsg)/sizeof(uartMsg[0]))
			HAL_UART_Transmit(&UartHandle1, (uint8_t*)(uartMsg[rxData]),strlen(uartMsg[rxData]), 0xFFFF);


		if(rxData==LED_ON)
		{
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3, 1);
		}
		else if(rxData==LED_OFF)
		{
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3, 0);
		}
		else if(rxData==MOTOR_ON)
		{
			HAL_GPIO_WritePin(GPIOB,MOTOR_P, 0);
			HAL_GPIO_WritePin(GPIOB,MOTOR_N, 1);
			HAL_GPIO_WritePin(GPIOA,MOTOR_PWM, 1);
		}
		else if(rxData==MOTOR_OFF)
		{
			HAL_GPIO_WritePin(GPIOB,MOTOR_P, 0);
			HAL_GPIO_WritePin(GPIOB,MOTOR_N, 0);
			HAL_GPIO_WritePin(GPIOA,MOTOR_PWM, 0);
		}
		else if(rxData==HT_GET)
		{
			temp_int = i2cGetTemp();
			temp = (double)(temp_int)*175.72/65536-46.85;
			sprintfTemp(temp,2,UART_TxBuffer);
			HAL_UART_Transmit(&UartHandle1, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);
		}
	}
}
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
