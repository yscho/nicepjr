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

// �½��� ����
#define HT_I2C_ADDR				0x40<<1
#define I2C_ADDRESS				HT_I2C_ADDR


// UART ����� ���� ����
#define TxBufferSize	0xFF	// �۽� ���� ������ ����
#define RxBufferSize	0xFF							// ���� ���� ����� 0xFF�� ����

// UART ��ſ� ���� ����
UART_HandleTypeDef	UartHandle1;	// UART�� �ʱ�ȭ�� ���� ����ü���� ������ ����
UART_HandleTypeDef	UartHandle4;	// Bluetooth
char UART_TxBuffer[TxBufferSize];
uint8_t UART_RxBuffer[RxBufferSize];
enum {LED_ON, LED_OFF, MOTOR_ON, MOTOR_OFF, HT_GET};
char *uartMsg[]={"LED_ON\r\n","LED_OFF\r\n","MOTOR_ON\r\n","MOTOR_OFF\r\n","HT_GET\r\n"};

TIM_HandleTypeDef TimHandle2;

GPIO_InitTypeDef GPIO_Init_Struct;

I2C_HandleTypeDef I2C2Handle;		// I2C�� �ʱ�ȭ�� ���� ����ü���� ������ ����

// I2C ��ſ� ���� ����
uint8_t TxBuffer[5];
uint8_t RxBuffer[5];

//�µ� ���� �� ���� ����
double curTemp; // Current Temp
double setTemp; //Configure Temp
//������
unsigned long ultra_val =0;

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


// -- I2C�� �ʱ⼳���� ���� �Լ�
void I2C_config(void)		// I2C2_SCL(PB10), I2C2_SDA(PB11)
{
	// I2C�� Ŭ���� Ȱ��ȭ
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_I2C2_CLK_ENABLE();	// I2C2

	// GPIO B��Ʈ 8,9�� ���� I2C ��ȯ������� ����
	GPIO_Init_Struct.Pin		= GPIO_PIN_10 | GPIO_PIN_11;	// GPIO���� ����� PIN ����
	GPIO_Init_Struct.Mode		= GPIO_MODE_AF_OD;				// Alternate Function Open Drain ���
	GPIO_Init_Struct.Alternate	= GPIO_AF4_I2C2;				// I2C2 Alternate Function mapping
	GPIO_Init_Struct.Pull		= GPIO_PULLUP;					// Pull Up ���
	GPIO_Init_Struct.Speed		= GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(GPIOB, &GPIO_Init_Struct);

	// I2C�� ���� ���� ����
	I2C2Handle.Instance 			= I2C2;
	I2C2Handle.Init.ClockSpeed 		= 400000;
	I2C2Handle.Init.DutyCycle 		= I2C_DUTYCYCLE_16_9;
	I2C2Handle.Init.OwnAddress1 	= I2C_ADDRESS;
	I2C2Handle.Init.AddressingMode	= I2C_ADDRESSINGMODE_7BIT;
	I2C2Handle.Init.DualAddressMode	= I2C_DUALADDRESS_DISABLE;
	I2C2Handle.Init.OwnAddress2 	= 0;
	I2C2Handle.Init.GeneralCallMode	= I2C_GENERALCALL_DISABLE;
	I2C2Handle.Init.NoStretchMode 	= I2C_NOSTRETCH_DISABLE;

	// I2C ���������� I2CxHandle�� ������ ������ �ʱ�ȭ ��
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
/*������ƾ*/
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
	GPIO_Init_Struct.Mode = GPIO_MODE_OUTPUT_PP;	// Alternate Function Push Pull ���
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
//	GPIO_Init_Struct.Mode = GPIO_MODE_AF_PP;	// Alternate Function Push Pull ���
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
	CLCD_write(0, 0x33);	// 4��Ʈ ���� Ư�� ���
	CLCD_write(0, 0x32);	// 4��Ʈ ���� Ư�� ���
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
	// UART1�� Ŭ���� Ȱ��ȭ
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();

	// GPIO A��Ʈ 9�� ���� USART Tx, 10�� ���� USART Rx�� ����
	GPIO_Init_Struct.Pin	= USART1_TX | USART1_RX;
	GPIO_Init_Struct.Mode	= GPIO_MODE_AF_PP;
	GPIO_Init_Struct.Pull	= GPIO_NOPULL;
	GPIO_Init_Struct.Speed	= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_Init_Struct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	// UART�� ���� ���� ����
	UartHandle1.Instance		= USART1;
	UartHandle1.Init.BaudRate	= 9600;
	UartHandle1.Init.WordLength	= UART_WORDLENGTH_8B;
	UartHandle1.Init.StopBits	= UART_STOPBITS_1;
	UartHandle1.Init.Parity		= UART_PARITY_NONE;
	UartHandle1.Init.HwFlowCtl	= UART_HWCONTROL_NONE;
	UartHandle1.Init.Mode		= UART_MODE_TX_RX;
	UartHandle1.Init.OverSampling = UART_OVERSAMPLING_16;

	// UART ���������� UartHandle�� ������ ������ �ʱ�ȭ ��
	HAL_UART_Init(&UartHandle1);

	// TxBuffer�� ����Ǿ� �ִ� ������ PC�� ������.
	return 0;
}

int UART4_config(void)		// UART2_TX(PA0), UART1_RX(PA1)
{
	// UART3�� Ŭ���� Ȱ��ȭ
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_UART4_CLK_ENABLE();

	// GPIO A��Ʈ 2�� ���� USART Tx, 3�� ���� USART Rx�� ����
	GPIO_Init_Struct.Pin	= GPIO_PIN_0 | GPIO_PIN_1 ;
	GPIO_Init_Struct.Mode	= GPIO_MODE_AF_PP;
	GPIO_Init_Struct.Pull	= GPIO_NOPULL;
	GPIO_Init_Struct.Speed	= GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_Init_Struct.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOA, &GPIO_Init_Struct);

	// UART�� ���� ���� ����
	UartHandle4.Instance		= UART4;
	UartHandle4.Init.BaudRate	= 9600;
	UartHandle4.Init.WordLength	= UART_WORDLENGTH_8B;
	UartHandle4.Init.StopBits	= UART_STOPBITS_1;
	UartHandle4.Init.Parity		= UART_PARITY_NONE;
	UartHandle4.Init.HwFlowCtl	= UART_HWCONTROL_NONE;
	UartHandle4.Init.Mode		= UART_MODE_TX_RX;
	UartHandle4.Init.OverSampling = UART_OVERSAMPLING_16;

	// UART ���������� UartHandle�� ������ ������ �ʱ�ȭ ��
	HAL_UART_Init(&UartHandle4);

	HAL_NVIC_SetPriority(UART4_IRQn,0,0);
	HAL_NVIC_EnableIRQ(UART4_IRQn);

	return 0;
}
int timer2_config(void)
{
	__HAL_RCC_TIM2_CLK_ENABLE();
	TimHandle2.Instance 			= TIM2;						// TIM2 ���
	TimHandle2.Init.Period 			= 300000 - 1;				// ������Ʈ �̹�� �߻��� ARR=999�� ����(100ms)
	TimHandle2.Init.Prescaler 		= 84 - 1;					// Prescaler = 83�� ����(0.001ms)
	TimHandle2.Init.ClockDivision	= TIM_CLOCKDIVISION_DIV1;	// division�� ������� ����
	TimHandle2.Init.CounterMode 	= TIM_COUNTERMODE_UP;		// Up Counter ��� ����
	HAL_TIM_Base_Init(&TimHandle2);
	HAL_TIM_Base_Start_IT(&TimHandle2);	/* Start Channel1 */

	HAL_NVIC_SetPriority(TIM2_IRQn, 6, 0);	/* Set Interrupt Group Priority */
	HAL_NVIC_EnableIRQ(TIM2_IRQn);	/* Enable the TIMx global Interrupt */

	return 0;
}

int timer_initialize(void)
{
	timer2_config();	//���� Timer TIM2
	return 0;
}

//;*************************** board_initialize  ************************************
int	board_initialize(void)
{
	open_Ultra();
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

	strcpy(UART_TxBuffer,"AT+NAMESW32A_YS");
	HAL_UART_Transmit(&UartHandle4, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);
	ms_delay_int_count(2000);

	strcpy(UART_TxBuffer,"AT+PIN1234");
	HAL_UART_Transmit(&UartHandle4, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);
}

int main(int argc, char* argv[])
{
	board_initialize();	//open_Ultra	open_led	open_dc_motor  open_clcd
	I2C_config();
	USART1_config();
	BT_config();
	timer_initialize();

	char clcd_buf[] = ""; //To char LED buffer

	uint32_t ul_time=0;
	//Temp Test for Char LCD
	curTemp = 10;
	setTemp = 20;

	strcpy(UART_TxBuffer,"Bluetooth Ready\r\n");
	HAL_UART_Transmit(&UartHandle1, (uint8_t*)UART_TxBuffer, strlen(UART_TxBuffer), 0xFFFF);

	while (1)
	{
		ul_time = __HAL_TIM_GET_COUNTER(&TimHandle2);
		if(ul_time>10){
			HAL_GPIO_WritePin(GPIOC, TRIG, 0);
		}

		HAL_UART_Receive_IT(&UartHandle4, UART_RxBuffer,1);
		//==========LCD 1st line print
		CLCD_write(0, 0x80);
		sprintf( clcd_buf, "ULTRA : %4d", curTemp) ;
		clcd_put_string((uint8_t*)clcd_buf);

		//==========LCD 2nd line print
		CLCD_write(0, 0xC0);
		sprintf(clcd_buf, "SETTEMP : %4d", setTemp);
		clcd_put_string((uint8_t*)clcd_buf);
	}
}

/*���ͷ�Ʈ ��ƾ*/
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
/*���ͷ�Ʈ ��ƾ*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static uint32_t ult_time =0;
	if(GPIO_Pin == ECHO){//PC1

		if(HAL_GPIO_ReadPin(GPIOC, ECHO) == GPIO_PIN_SET)
		{
			ult_time =__HAL_TIM_GET_COUNTER(&TimHandle2);
		}
		else
		{
			ult_time = __HAL_TIM_GET_COUNTER(&TimHandle2)- ult_time ;
			ultra_val = (unsigned long)(((ult_time/2)/2.9));
		}
	}
	return;

}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2){
		// Interrupt PC1�� Rising ��Ų��.
		HAL_GPIO_WritePin(GPIOC, TRIG, 1);	//
	}

}
