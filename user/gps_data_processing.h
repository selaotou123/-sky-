#ifndef __GPS_DATA_PROCESSING_H__
#define __GPS_DATA_PROCESSING_H__

#include "stm32f4xx.h"
#include "gps_uart2.h"

#define M_PI 3.14159265358979323846f //圆周率
#define EE 0.00669342162296594323f // 地球椭球偏心率平方（单精度）
#define A 6378137.0f // 地球赤道半径（米）

// =============== 新增：滤波器配置 ===============
// 滑动平均滤波器窗口大小（越大越平滑，响应越慢）
#define GPS_FILTER_WINDOW_SIZE 5

static int out_of_china(float lat, float lon);//判断是否在中国范围外（简化版，避免无效纠偏）
static float transform_lat(float x, float y);//GPS 坐标（WGS84）纬度纠偏
static float transform_lon(float x, float y);//GPS 坐标（WGS84）经度纠偏
void gcj02_to_tile_pixel_offset(float gcj_lat, float gcj_lon, uint8_t zoom, uint8_t *px, uint8_t *py);
void wgs84_to_gcj02(float wgs_lat, float wgs_lon, float *gcj_lat, float *gcj_lon);//WGS84 转 GCJ-02（火星坐标）
void gcj02_to_gaode_tile(float gcj_lat, float gcj_lon, uint8_t zoom, uint32_t *x, uint32_t *y);//GCJ-02 坐标转高德瓦片坐标 (zoom: 0~18)
static uint8_t is_leap_year(uint16_t year);// 判断闰年
static uint8_t get_days_in_month(uint16_t year, uint8_t month);// 获取当月天数
void convert_utc_to_beijing(gps_device_t *dev);// UTC 转 北京时间（UTC+8）
//void correct_gps_time(gps_device_t *dev);

// 滤波函数声明 
float filter_latitude(float raw_latitude);	// 纬度滤波
float filter_longitude(float raw_longitude);	// 经度滤波
float filter_speed(float raw_speed);	// 速度滤波

// 获取滤波后的数据
float get_filtered_latitude(void);
float get_filtered_longitude(void);
float get_filtered_speed_kmh(void);

#endif /* __GPS_DATA_PROCESSING_H__ */

