#ifndef __TJC_UART4_H
#define __TJC_UART4_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>

// UART4 硬件配置 (PA0-TX / PA1-RX, AF8) - STM32F407专属
#define TJC_UART4_CLK          RCC_APB1Periph_UART4      // UART4在APB1总线
#define TJC_UART4_GPIO_CLK     RCC_AHB1Periph_GPIOA      // GPIOA在AHB1总线
#define TJC_UART4_PORT         GPIOA
#define TJC_UART4_PIN_TX       GPIO_Pin_0
#define TJC_UART4_PIN_RX       GPIO_Pin_1
#define TJC_UART4_TX_SOURCE    GPIO_PinSource0
#define TJC_UART4_RX_SOURCE    GPIO_PinSource1
#define TJC_UART4_AF           GPIO_AF_UART4             // AF8 (UART4)

#define MAX_CMD_LEN 128


// 用户调用接口
void tjc_uart4_init(uint32_t baudrate);      // 初始化UART4
void tjc_uart4_send_string(char* str);       // 发送普通字符串
void TJCPrintf(const char *cmd, ...);        // 串口屏专用格式化发送 (帧尾0xFFx3)
void debug_TJCPrintf(const char *cmd, ...);

// 环形缓冲区接口 (带前缀防冲突)
uint16_t tjc_uart4_get_rx_len(void);         // 获取接收缓冲区数据长度
uint8_t  tjc_uart4_read_byte(uint16_t pos);  // 读取指定位置字节
void     tjc_uart4_clear_rx(uint16_t size);  // 清除指定长度数据
void     tjc_uart4_init_rx_buffer(void);     // 显式初始化接收缓冲区（通常init内自动调用）

// 宏定义（按需使用，避免与gps_uart2冲突）
// #define UART4_RX_LEN()      tjc_uart4_get_rx_len()
// #define UART4_READ(pos)     tjc_uart4_read_byte(pos)
// #define UART4_CLEAR(size)   tjc_uart4_clear_rx(size)

#endif /* __TJC_UART4_H */

