#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include <stdio.h>

// 调试串口使用的硬件定义
#define DEBUG_USART              UART5
#define DEBUG_USART_CLK          RCC_APB1Periph_UART5
#define DEBUG_USART_GPIO_CLK     RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD
#define DEBUG_USART_PORT_TX      GPIOC
#define DEBUG_USART_PIN_TX       GPIO_Pin_12
#define DEBUG_USART_PORT_RX      GPIOD
#define DEBUG_USART_PIN_RX       GPIO_Pin_2
#define DEBUG_USART_TX_SOURCE    GPIO_PinSource12
#define DEBUG_USART_RX_SOURCE    GPIO_PinSource2
#define DEBUG_USART_AF           GPIO_AF_UART5


void debug_uart5_init(uint32_t baudrate);	//初始化
void debug_uart5_send_byte(uint8_t data); 	//发送字节
void debug_uart5_send_string(char* str);  	//发送字符串

#endif /* __USART_H */

