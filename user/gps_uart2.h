#ifndef __GPS_UART2_H__
#define __GPS_UART2_H__

#include "stm32f4xx.h"
#include <stdbool.h>

// GPS缓冲区定义
#define GPS_RX_BUF_SIZE      256
#define GPS_FRAME_MAX_LEN    128

// GPS数据结构体
typedef struct 
{
    uint8_t raw_buffer[GPS_RX_BUF_SIZE];
    uint16_t raw_index;
    struct 
	{
        bool is_valid;
        bool has_fix;
        
        // === 新增：UTC 整数字段（关键！）===
        uint16_t utc_year;   // 如 2024
        uint8_t utc_month;   // 1-12
        uint8_t utc_day;     // 1-31
        uint8_t utc_hour;    // 0-23
        uint8_t utc_minute;  // 0-59
        uint8_t utc_second;  // 0-59
        
        // 原有字符串字段（保留用于调试）
        char utc_time_str[12]; // "hhmmss.ss"
        char date_str[7];      // "ddmmyy"
        
        // 原有GPS数据字段
        float latitude;
        char ns_indicator;
        float longitude;
        char ew_indicator;
        float speed_knots;
        float course_degrees;
        
        // 北京时间字段（新增）
        uint16_t bj_year;
        uint8_t bj_month;
        uint8_t bj_day;
        uint8_t bj_hour;
        uint8_t bj_minute;
        uint8_t bj_second;
    } data;
    bool new_data_available;
    uint32_t last_update_ms;
} gps_device_t;

// 声明外部系统滴答计数器（在main.c中定义）
extern volatile uint32_t system_tick;

// 函数声明
void gps_uart2_init(uint32_t baudrate);	//初始化
//void gps_configure_rmc_only(void);
void gps_uart2_send_byte(uint8_t data);	//发送字节
void gps_uart2_send_string(const char *str);	//发送字符串
void gps_parse_nmea(const char *nmea_str);	//解析NMEA语句（只解析RMC）
//void gps_convert_to_xyz(float lat, float lon, uint8_t zoom, uint32_t *x, uint32_t *y); 

// 全局GPS设备实例
extern gps_device_t gps_device;

// 数据获取函数
float gps_get_latitude_degree(void);	//获取纬度（十进制度）（原始数值）
float gps_get_longitude_degree(void);	//获取经度（十进制度）（原始数值）
float gps_get_speed_kmh(void);	//获取速度（公里/小时）
float gps_get_course_degrees(void);	//获取航向（度）
bool gps_has_fix(void);	//检查是否有定位

// 状态检查函数
bool gps_is_new_data_available(void);	//检查是否有新GPS数据可用
void gps_clear_new_data_flag(void);	//清除新数据标志
uint32_t gps_get_last_update_time(void);	//获取最后更新时间

// 中断服务函数原型
void USART2_IRQHandler(void);

#endif /* __GPS_UART2_H__ */

