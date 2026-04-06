#include "tjc_uart4.h"
#include <string.h>
#include "debug_uart5.h"

// =============== 环形缓冲区 (STATIC封装，彻底避免全局冲突) ===============
#define RINGBUFF_UART4_LEN 500 // 独立缓冲区大小


typedef struct 
{
    uint16_t head;
    uint16_t tail;
    uint16_t len;
    uint8_t buf[RINGBUFF_UART4_LEN];
} uart4_ringbuff_t;

static uart4_ringbuff_t rx_ring; // 静态变量，仅本文件可见

// 静态函数：内部使用，不暴露给外部
static void ring_init(void)
{
    rx_ring.head = rx_ring.tail = rx_ring.len = 0;
}

static void ring_write(uint8_t data)
{
    if (rx_ring.len >= RINGBUFF_UART4_LEN) 
	{
		return;
	}
    rx_ring.buf[rx_ring.tail] = data;
    rx_ring.tail = (rx_ring.tail + 1) % RINGBUFF_UART4_LEN;
    rx_ring.len++;
}

static uint8_t ring_read(uint16_t pos)
{
    if (pos >= rx_ring.len) 
	{
		return 0;
	}
    uint16_t index = (rx_ring.head + pos) % RINGBUFF_UART4_LEN;
    return rx_ring.buf[index];
}

static void ring_delete(uint16_t size)
{
    if (size > rx_ring.len) 
	{
		size = rx_ring.len;
	}
    rx_ring.head = (rx_ring.head + size) % RINGBUFF_UART4_LEN;
    rx_ring.len -= size;
}

/********************************************************************************
 * @brief 初始化UART4
 * @param baudrate: 波特率
 * @note TX: PA0, RX: PA1
 *******************************************************************************/
void tjc_uart4_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 使能时钟 (GPIOA在AHB1, UART4在APB1)
    RCC_AHB1PeriphClockCmd(TJC_UART4_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(TJC_UART4_CLK, ENABLE);

    // 配置TX (PA0) - 复用推挽
    GPIO_InitStruct.GPIO_Pin = TJC_UART4_PIN_TX;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(TJC_UART4_PORT, &GPIO_InitStruct);

    // 配置RX (PA1) - 复用浮空输入
    GPIO_InitStruct.GPIO_Pin = TJC_UART4_PIN_RX;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(TJC_UART4_PORT, &GPIO_InitStruct);

    // 引脚复用映射 (AF8 for UART4 on PA0/PA1)
    GPIO_PinAFConfig(TJC_UART4_PORT, TJC_UART4_TX_SOURCE, TJC_UART4_AF);
    GPIO_PinAFConfig(TJC_UART4_PORT, TJC_UART4_RX_SOURCE, TJC_UART4_AF);

    // 配置UART4参数
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(UART4, &USART_InitStruct); // F407标准外设指针

    // 初始化接收缓冲区 & 使能中断
    ring_init();
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_Cmd(UART4, ENABLE);

    // 配置NVIC (UART4_IRQn)
    NVIC_InitStruct.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
	
	TJCPrintf("\x00\xff\xff\xff");
	debug_TJCPrintf("\x00\xff\xff\xff");
}

/********************************************************************************
 * @brief UART4中断服务函数
 * @note 接收数据并存入环形缓冲区
 *******************************************************************************/
void UART4_IRQHandler(void)
{
    if (USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
    {
        ring_write((uint8_t)USART_ReceiveData(UART4));
        USART_ClearITPendingBit(UART4, USART_IT_RXNE);
    }
}

/********************************************************************************
 * @brief 发送一个字节
 * @param data: 要发送的数据
 *******************************************************************************/
static void tjc_uart4_send_byte(uint8_t data)
{
    USART_SendData(UART4, (uint16_t)data);
    while (USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
}


/********************************************************************************
 * @brief 发送普通字符串
 * @param str: 要发送的字符串
 *******************************************************************************/
void tjc_uart4_send_string(char* str)
{
    if (!str) 
	{
		return;
	}
    while (*str) tjc_uart4_send_byte(*str++);
}

/********************************************************************************
 * @brief 串口屏专用：发送格式化字符串 + 帧尾 0xFF 0xFF 0xFF
 * @param cmd: 格式化字符串
 *******************************************************************************/
void TJCPrintf(const char *cmd, ...)
{
#define MAX_CMD_LEN 128
    char buffer[MAX_CMD_LEN];
    va_list args;
    va_start(args, cmd);
    int len = vsnprintf(buffer, MAX_CMD_LEN, cmd, args);
    va_end(args);
    if (len > 0)
    {
        for (int i = 0; i < len; i++)
            tjc_uart4_send_byte(buffer[i]);
        // 发送帧尾 (3字节0xFF)
        tjc_uart4_send_byte(0xFF);
        tjc_uart4_send_byte(0xFF);
        tjc_uart4_send_byte(0xFF);
    }
}

/********************************************************************************
 * @brief 测试用串口屏代码（使用串口5(debug_uart5,波特率为115200)）：
 *        发送格式化字符串 + 帧尾 0xFF 0xFF 0xFF
 * @param cmd: 格式化字符串
 * @example debug_TJCPrintf("exp0.path=\"sd0/%d/%d/%d.jpg\"", 8, 215, 105);
 *******************************************************************************/
void debug_TJCPrintf(const char *cmd, ...)
{
    char buffer[MAX_CMD_LEN];
    va_list args;
    va_start(args, cmd);
    int len = vsnprintf(buffer, MAX_CMD_LEN, cmd, args);
    va_end(args);
    if (len > 0)
    {
        for (int i = 0; i < len; i++)
            debug_uart5_send_byte(buffer[i]);
        // 发送帧尾 (3字节0xFF)
        debug_uart5_send_byte(0xFF);
        debug_uart5_send_byte(0xFF);
        debug_uart5_send_byte(0xFF);
    }
}

/*********************************************************************************
 * @brief 获取接收缓冲区数据长度
 * @return 缓冲区中有效数据的字节数
 ********************************************************************************/
uint16_t tjc_uart4_get_rx_len(void)
{
    return rx_ring.len;
}

/*********************************************************************************
 * @brief 读取指定位置字节
 * @param pos: 缓冲区中的偏移位置
 * @return 指定位置的字节数据
 ********************************************************************************/
uint8_t tjc_uart4_read_byte(uint16_t pos)
{
    return ring_read(pos);
}

/*********************************************************************************
 * @brief 清除指定长度数据
 * @param size: 要清除的数据长度
 ********************************************************************************/
void tjc_uart4_clear_rx(uint16_t size)
{
    ring_delete(size);
}

/*********************************************************************************
 * @brief 显式初始化接收缓冲区（通常init内自动调用）
 ********************************************************************************/
void tjc_uart4_init_rx_buffer(void)
{
    ring_init();
}

