#include "gps_uart2.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "board.h"
#include <math.h>

// GPS设备全局实例
gps_device_t gps_device = {0};

// 私有函数声明
static bool gps_check_nmea_checksum(const char *nmea_str);
static void gps_parse_gprmc(const char *data);
static void gps_extract_field(const char *src, char *dest, uint8_t field_num);

/*******************************************************************************
 * @brief  初始化GPS串口（USART2）
 *******************************************************************************/
void gps_uart2_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);
    
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART2, ENABLE);
    
    memset(&gps_device, 0, sizeof(gps_device_t));
    gps_device.raw_index = 0;
    gps_device.data.is_valid = false;
    gps_device.new_data_available = false;
    
    //printf("[GPS] USART2 initialized at %lu bps\r\n", baudrate);
    
    //gps_configure_rmc_only();
}

/*******************************************************************************
 * @brief  配置GPS模块只输出RMC语句
 *******************************************************************************/
/*
void gps_configure_rmc_only(void)
{
    delay_ms(500);
    
    const char *cmd_disable_all = "$PCAS03,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
    printf("[GPS] Config: Disable all NMEA outputs\r\n");
    gps_uart2_send_string(cmd_disable_all);
    delay_ms(100);
    
    const char *cmd_enable_rmc = "$PCAS03,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*27\r\n";
    printf("[GPS] Config: Enable RMC only (1Hz)\r\n");
    gps_uart2_send_string(cmd_enable_rmc);
    delay_ms(100);
    
    const char *cmd_save_config = "$PCAS10,3*1E\r\n";
    printf("[GPS] Config: Save configuration\r\n");
    gps_uart2_send_string(cmd_save_config);
    delay_ms(100);
    
    gps_device.raw_index = 0;
    memset(gps_device.raw_buffer, 0, sizeof(gps_device.raw_buffer));
    
    printf("[GPS] Configuration complete - RMC only mode\r\n");
}
*/

/*******************************************************************************
 * @brief  USART2中断服务函数
 *******************************************************************************/
void USART2_IRQHandler(void)
{
    uint8_t received_char;
    
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        received_char = USART_ReceiveData(USART2);
        
        if(received_char == '$')
        {
            gps_device.raw_index = 0;
            gps_device.raw_buffer[gps_device.raw_index++] = received_char;
        }
        else if(gps_device.raw_index > 0 && gps_device.raw_index < GPS_RX_BUF_SIZE - 1)
        {
            gps_device.raw_buffer[gps_device.raw_index++] = received_char;
            
            if(received_char == '\n')
            {
                gps_device.raw_buffer[gps_device.raw_index] = '\0';
                
                char *nmea_str = (char*)gps_device.raw_buffer;
                if(strstr(nmea_str, "$GPRMC") != NULL || strstr(nmea_str, "$GNRMC") != NULL)
                {
                    gps_parse_nmea(nmea_str);
                }
                
                gps_device.raw_index = 0;
            }
        }
        else
        {
            gps_device.raw_index = 0;
        }
        
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

/*******************************************************************************
 * @brief  发送一个字节到GPS模块
 *******************************************************************************/
void gps_uart2_send_byte(uint8_t data)
{
    USART_SendData(USART2, data);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/*******************************************************************************
 * @brief  发送字符串到GPS模块
 *******************************************************************************/
void gps_uart2_send_string(const char *str)
{
    while(*str) 
	{
        gps_uart2_send_byte(*str++);
    }
}

/*******************************************************************************
 * @brief  校验NMEA语句的校验和
 *******************************************************************************/
static bool gps_check_nmea_checksum(const char *nmea_str)
{
    uint8_t checksum = 0;
    const char *p = nmea_str + 1;
    
    while(*p && *p != '*' && *p != '\r' && *p != '\n')
    {
        checksum ^= *p++;
    }
    
    if(*p == '*')
    {
        char hex[3] = {0};
        uint8_t received_checksum;
        
        hex[0] = *(p + 1);
        hex[1] = *(p + 2);
        received_checksum = (uint8_t)strtoul(hex, NULL, 16);
        
        return (checksum == received_checksum);
    }
    
    return false;
}

/*******************************************************************************
 * @brief  提取NMEA字段
 *******************************************************************************/
static void gps_extract_field(const char *src, char *dest, uint8_t field_num)
{
    uint8_t comma_count = 0;
    const char *start = src;
    
    while(*src)
    {
        if(*src == ',')
        {
            comma_count++;
            if(comma_count == field_num)
            {
                start = src + 1;
            }
            else if(comma_count == field_num + 1)
            {
                uint8_t len = src - start;
                if(len > 0)
                {
                    strncpy(dest, start, len);
                    dest[len] = '\0';
                }
                else
                {
                    dest[0] = '\0';
                }
                return;
            }
        }
        src++;
    }
    
    dest[0] = '\0';
}

/*******************************************************************************
 * @brief  解析GPRMC语句
 *******************************************************************************/
static void gps_parse_gprmc(const char *data)
{
    char field[20];
    
	gps_extract_field(data, field, 1);
	if (strlen(field) >= 6) 
	{
		// 复制原始字符串
		strncpy(gps_device.data.utc_time_str, field, sizeof(gps_device.data.utc_time_str)-1);
		
		// 拆分为整数（hhmmss）
		gps_device.data.utc_hour = (field[0] - '0') * 10 + (field[1] - '0');
		gps_device.data.utc_minute = (field[2] - '0') * 10 + (field[3] - '0');
		gps_device.data.utc_second = (field[4] - '0') * 10 + (field[5] - '0');
	} 
	else 
	{
		// 无效时间，清零
		gps_device.data.utc_hour = gps_device.data.utc_minute = gps_device.data.utc_second = 0;
	}
    
    gps_extract_field(data, field, 2);
    gps_device.data.has_fix = (field[0] == 'A');
    
    gps_extract_field(data, field, 3);
    if(strlen(field) > 0)
    {
        gps_device.data.latitude = atof(field);
    }
    
    gps_extract_field(data, field, 4);
    if(strlen(field) > 0)
    {
        gps_device.data.ns_indicator = field[0];
    }
    
    gps_extract_field(data, field, 5);
    if(strlen(field) > 0)
    {
        gps_device.data.longitude = atof(field);
    }
    
    gps_extract_field(data, field, 6);
    if(strlen(field) > 0)
    {
        gps_device.data.ew_indicator = field[0];
    }
    
    gps_extract_field(data, field, 7);
    if(strlen(field) > 0)
    {
        gps_device.data.speed_knots = atof(field);
    }
    
    gps_extract_field(data, field, 8);
    if(strlen(field) > 0)
    {
        gps_device.data.course_degrees = atof(field);
    }
    
	gps_extract_field(data, field, 9);
	if (strlen(field) >= 6) 
	{
		strncpy(gps_device.data.date_str, field, sizeof(gps_device.data.date_str)-1);
		
		// 拆分为整数（ddmmyy）
		gps_device.data.utc_day = (field[0] - '0') * 10 + (field[1] - '0');
		gps_device.data.utc_month = (field[2] - '0') * 10 + (field[3] - '0');
		gps_device.data.utc_year = 2000 + (field[4] - '0') * 10 + (field[5] - '0'); // 转为四位年
	} 
	else 
	{
		// 无效日期，设为安全值（避免后续计算溢出）
		gps_device.data.utc_year = 2000;
		gps_device.data.utc_month = 1;
		gps_device.data.utc_day = 1;
	}
    
    gps_device.data.is_valid = gps_device.data.has_fix;
    gps_device.new_data_available = true;
    //gps_device.last_update_ms = system_tick;
}

/*******************************************************************************
 * @brief  解析NMEA语句（只解析RMC）
 *******************************************************************************/
void gps_parse_nmea(const char *nmea_str)
{
    if(!gps_check_nmea_checksum(nmea_str))
    {
        printf("[GPS] Checksum error\r\n");
        return;
    }
    
    gps_parse_gprmc(nmea_str);
}

/*******************************************************************************
 * @brief  将度分格式转换为十进制度格式
 *******************************************************************************/
static void gps_convert_to_degree(float *dm_coord, char indicator)
{
    int degrees = (int)(*dm_coord / 100);
    float minutes = *dm_coord - degrees * 100;
    *dm_coord = degrees + minutes / 60.0f;
    
    if(indicator == 'S' || indicator == 'W')
        *dm_coord = -*dm_coord;
}

/*******************************************************************************
 * @brief  获取纬度（十进制度）（原始数值）
 *******************************************************************************/
float gps_get_latitude_degree(void)
{
    float lat = gps_device.data.latitude;
    gps_convert_to_degree(&lat, gps_device.data.ns_indicator);
    return lat;
}

/*******************************************************************************
 * @brief  获取经度（十进制度）（原始数值）
 *******************************************************************************/
float gps_get_longitude_degree(void)
{
    float lon = gps_device.data.longitude;
    gps_convert_to_degree(&lon, gps_device.data.ew_indicator);
    return lon;
}

/*******************************************************************************
 * @brief  获取速度（公里/小时）
 *******************************************************************************/
float gps_get_speed_kmh(void)
{
    return gps_device.data.speed_knots * 1.852f;
}

/*******************************************************************************
 * @brief  获取航向（度）
 *******************************************************************************/
float gps_get_course_degrees(void)
{
    return gps_device.data.course_degrees;
}

/*******************************************************************************
 * @brief  检查是否有定位
 *******************************************************************************/
bool gps_has_fix(void)
{
    return gps_device.data.has_fix;
}

/*******************************************************************************
 * @brief  检查是否有新GPS数据可用
 *******************************************************************************/
bool gps_is_new_data_available(void)
{
    return gps_device.new_data_available;
}

/*******************************************************************************
 * @brief  清除新数据标志
 *******************************************************************************/
void gps_clear_new_data_flag(void)
{
    gps_device.new_data_available = false;
}

/*******************************************************************************
 * @brief  获取最后更新时间
 *******************************************************************************/
uint32_t gps_get_last_update_time(void)
{
    return gps_device.last_update_ms;
}


