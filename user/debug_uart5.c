#include "debug_uart5.h"

/********************************************************************************
  * @brief  初始化调试串口（UART5）
  * @param  baudrate: 波特率，建议115200
  * @note   TX: PC12, RX: PD2
  *******************************************************************************/
void debug_uart5_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    // 1. 使能时钟（必须同时使能GPIOC、GPIOD和UART5时钟）
    RCC_AHB1PeriphClockCmd(DEBUG_USART_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(DEBUG_USART_CLK, ENABLE);
    
    // 2. 配置TX引脚(PC12)为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_PIN_TX;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;      // 复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 高速
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      // 上拉
    GPIO_Init(DEBUG_USART_PORT_TX, &GPIO_InitStructure);
    
    // 3. 配置RX引脚(PD2)为浮空输入
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_PIN_RX;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;      // 复用功能
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  // 浮空输入
    GPIO_Init(DEBUG_USART_PORT_RX, &GPIO_InitStructure);
    
    // 4. 将GPIO引脚连接到UART5复用功能
    GPIO_PinAFConfig(DEBUG_USART_PORT_TX, DEBUG_USART_TX_SOURCE, DEBUG_USART_AF);
    GPIO_PinAFConfig(DEBUG_USART_PORT_RX, DEBUG_USART_RX_SOURCE, DEBUG_USART_AF);
    
    // 5. 配置UART5参数
    USART_InitStructure.USART_BaudRate = baudrate;         // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 8位数据
    USART_InitStructure.USART_StopBits = USART_StopBits_1; // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;    // 无校验
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 收发模式
    USART_Init(DEBUG_USART, &USART_InitStructure);
    
    // 6. 使能UART5
    USART_Cmd(DEBUG_USART, ENABLE);
    
    // 7. 使能接收中断（如果需要接收数据，可取消注释）
    // USART_ITConfig(DEBUG_USART, USART_IT_RXNE, ENABLE);
    // NVIC_EnableIRQ(UART5_IRQn);
}

/********************************************************************************
  * @brief  发送一个字节
  * @param  data  要发送的数据
  *******************************************************************************/
void debug_uart5_send_byte(uint8_t data)
{
    USART_SendData(DEBUG_USART, data);
    while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET);
}

/********************************************************************************
  * @brief  发送字符串
  * @param  str  要发送的字符串
  *******************************************************************************/
void debug_uart5_send_string(char* str)
{
    while(*str != '\0') 
	{
        debug_uart5_send_byte(*str++);
    }
}

/********************************************************************************
  * @brief  printf重定向到串口5
  *******************************************************************************/
int fputc(int ch, FILE *f)
{
    // 直接发送字节，不进行编码转换
    debug_uart5_send_byte((uint8_t)ch);
    return ch;
}

/********************************************************************************
  * @brief  可选的scanf重定向
  *******************************************************************************/
int fgetc(FILE *f)
{
    while(USART_GetFlagStatus(DEBUG_USART, USART_FLAG_RXNE) == RESET);
    return (int)USART_ReceiveData(DEBUG_USART);
}

