#include "gps_data_processing.h"
#include "gps_uart2.h"
#include <math.h>  

// 用于存储纬度滤波的历史数据
static float latitude_history[GPS_FILTER_WINDOW_SIZE] = {0};
static uint8_t latitude_index = 0;

// 用于存储经度滤波的历史数据
static float longitude_history[GPS_FILTER_WINDOW_SIZE] = {0};
static uint8_t longitude_index = 0;

// 用于存储速度滤波的历史数据
static float speed_history[GPS_FILTER_WINDOW_SIZE] = {0};
static uint8_t speed_index = 0;

/*******************************************************************************
 * @brief  闰年判断函数
 *******************************************************************************/
static uint8_t is_leap_year(uint16_t year) 
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/*******************************************************************************
 * @brief  获取月天数
 *******************************************************************************/
static uint8_t get_days_in_month(uint16_t year, uint8_t month) 
{
    const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) 
	{
		return 29;
	}
    return (month >= 1 && month <= 12) ? days[month - 1] : 31;
}

/*******************************************************************************
 * @brief  UTC转北京时间
 *******************************************************************************/
void convert_utc_to_beijing(gps_device_t *dev) 
{
    if (!dev || !dev->data.has_fix) 
	{
        return;
    }
    
    // 复制UTC时间
    dev->data.bj_year = dev->data.utc_year;
    dev->data.bj_month = dev->data.utc_month;
    dev->data.bj_day = dev->data.utc_day;
    dev->data.bj_hour = dev->data.utc_hour + 8; // UTC+8
    dev->data.bj_minute = dev->data.utc_minute;
    dev->data.bj_second = dev->data.utc_second;
    
    // 处理跨日
    if (dev->data.bj_hour >= 24) 
	{
        dev->data.bj_hour -= 24;
        dev->data.bj_day++;
        
        // 处理跨月/跨年
        while (dev->data.bj_day > get_days_in_month(dev->data.bj_year, dev->data.bj_month)) 
		{
            dev->data.bj_day -= get_days_in_month(dev->data.bj_year, dev->data.bj_month);
            dev->data.bj_month++;
            if (dev->data.bj_month > 12) 
			{
                dev->data.bj_month = 1;
                dev->data.bj_year++;
            }
        }
    }
}

/*******************************************************************************
 * @brief  时间矫正
 *******************************************************************************/
/*void correct_gps_time(gps_device_t *dev) 
{
    // GPS接收器通常会返回上一秒的时间（处理延迟）
    // UTC时间 + 1秒
    dev->data.utc_second++;
    
    // 处理秒级进位
    if (dev->data.utc_second >= 60) 
	{
        dev->data.utc_second = 0;
        dev->data.utc_minute++;
        
        if (dev->data.utc_minute >= 60) 
		{
            dev->data.utc_minute = 0;
            dev->data.utc_hour++;
            
            if (dev->data.utc_hour >= 24) 
			{
                dev->data.utc_hour = 0;
                dev->data.utc_day++;
                
                // 处理跨月/跨年
                while (dev->data.utc_day > get_days_in_month(dev->data.utc_year, dev->data.utc_month)) 
				{
                    dev->data.utc_day -= get_days_in_month(dev->data.utc_year, dev->data.utc_month);
                    dev->data.utc_month++;
                    if (dev->data.utc_month > 12) 
					{
                        dev->data.utc_month = 1;
                        dev->data.utc_year++;
                    }
                }
            }
        }
    }
    
    // 重新转换为北京时间
    convert_utc_to_beijing(dev);
}*/

/*******************************************************************************
 * @brief 判断是否在中国范围外
 * @param lat  WGS84纬度
 * @param lon  WGS84经度
 *******************************************************************************/
static int out_of_china(float lat, float lon) 
{
    if (lon < 72.004f || lon > 137.8347f) 
	{
		return 1;
	}
    if (lat < 0.8293f || lat > 55.8271f) 
	{
		return 1;
	}
    return 0;
}

/*******************************************************************************
 * @brief GPS 坐标（WGS84）纬度纠偏
 *******************************************************************************/
static float transform_lat(float x, float y) 
{
    float ret = -100.0f + 2.0f * x + 3.0f * y + 0.2f * y * y + 0.1f * x * y + 0.2f * sqrtf(fabsf(x));
    ret += (20.0f * sinf(6.0f * x * M_PI) + 20.0f * sinf(2.0f * x * M_PI)) * 2.0f / 3.0f;
    ret += (20.0f * sinf(y * M_PI) + 40.0f * sinf(y / 3.0f * M_PI)) * 2.0f / 3.0f;
    ret += (150.0f * sinf(y / 12.0f * M_PI) + 300.0f * sinf(y / 30.0f * M_PI)) * 2.0f / 3.0f;
    return ret;
}


/*******************************************************************************
 * @brief GPS 坐标（WGS84）经度纠偏
 *******************************************************************************/
static float transform_lon(float x, float y) 
{
    float ret = 300.0f + x + 2.0f * y + 0.1f * x * x + 0.1f * x * y + 0.1f * sqrtf(fabsf(x));
    ret += (20.0f * sinf(6.0f * x * M_PI) + 20.0f * sinf(2.0f * x * M_PI)) * 2.0f / 3.0f;
    ret += (20.0f * sinf(x * M_PI) + 40.0f * sinf(x / 3.0f * M_PI)) * 2.0f / 3.0f;
    ret += (150.0f * sinf(x / 12.0f * M_PI) + 300.0f * sinf(x / 30.0f * M_PI)) * 2.0f / 3.0f;
    return ret;
}



/********************************************************************************
 * @brief WGS84 转 GCJ-02（火星坐标）
 * @param wgs_lat  WGS84纬度 (十进制度，-85.0511 ~ 85.0511)
 * @param wgs_lon  WGS84经度 (十进制度，-180 ~ 180)
 * @param gcj_lat  GCJ-02纬度 (十进制度，-85.0511 ~ 85.0511)
 * @param gcj_lon  GCJ-02经度 (十进制度，-180 ~ 180)
 *******************************************************************************/
void wgs84_to_gcj02(float wgs_lat, float wgs_lon, float *gcj_lat, float *gcj_lon) 
{
    if (out_of_china(wgs_lat, wgs_lon)) // 非中国区域直接返回
	{ 
        *gcj_lat = wgs_lat;
        *gcj_lon = wgs_lon;
        return;
    }
    
    float dlat = transform_lat(wgs_lon - 105.0f, wgs_lat - 35.0f);
    float dlon = transform_lon(wgs_lon - 105.0f, wgs_lat - 35.0f);
    float rad_lat = wgs_lat * M_PI / 180.0f;
    float magic = sinf(rad_lat);
    magic = 1.0f - EE * magic * magic;
    float sqrt_magic = sqrtf(magic);
    
    // 单精度计算偏移量
    dlat = (dlat * 180.0f) / ((A * (1.0f - EE)) / (magic * sqrt_magic) * M_PI);
    dlon = (dlon * 180.0f) / (A / sqrt_magic * cosf(rad_lat) * M_PI);
    
    *gcj_lat = wgs_lat + dlat;
    *gcj_lon = wgs_lon + dlon;
    
    // 边界保护（高德地图有效范围）
    if (*gcj_lat > 55.8271f) 
	{
		*gcj_lat = 55.8271f;
	}
    if (*gcj_lat < 0.8293f) 
	{
		*gcj_lat = 0.8293f;
	}
    if (*gcj_lon > 137.8347f) 
	{
		*gcj_lon = 137.8347f;
	}
    if (*gcj_lon < 72.004f) 
	{
		*gcj_lon = 72.004f;
	}
}



/*******************************************************************************
 * @brief GCJ-02 坐标转高德瓦片坐标 (zoom: 0~18)
 * @param gcj_lat	GCJ纬度 (十进制度，-85.0511 ~ 85.0511)
 * @param gcj_lon	GCJ经度 (十进制度，-180 ~ 180)
 * @param zoom		缩放级别
 * @param x    		输出：瓦片X坐标
 * @param y			输出：瓦片Y坐标
 *******************************************************************************/
void gcj02_to_gaode_tile(float gcj_lat, float gcj_lon, uint8_t zoom, uint32_t *x, uint32_t *y) 
{
    // 1. 限制纬度（高德有效范围）
    if (gcj_lat > 85.0511f) 
	{
		gcj_lat = 85.0511f;
	}
    if (gcj_lat < -85.0511f) 
	{
		gcj_lat = -85.0511f;
	}
    
    // 2. 计算瓦片总数
    float n = (float)(1U << zoom); // 2^zoom
    
    // 3. X 坐标：经度线性映射
    float xtile = (gcj_lon + 180.0f) / 360.0f * n;
    
    // 4. Y 坐标：墨卡托投影（全程单精度！）
    float lat_rad = gcj_lat * M_PI / 180.0f;
    float ytile = (1.0f - logf(tanf(lat_rad) + 1.0f / cosf(lat_rad)) / M_PI) * 0.5f * n;
    
    // 5. 安全取整（防浮点误差越界）
    *x = (uint32_t)(xtile < 0 ? 0 : (xtile >= n ? (uint32_t)(n - 1.0f) : xtile));
    *y = (uint32_t)(ytile < 0 ? 0 : (ytile >= n ? (uint32_t)(n - 1.0f) : ytile));
}


/*******************************************************************************
 * @brief 计算 GCJ-02 坐标在指定缩放级别下，位于当前瓦片内的像素位置 (0~255)
 * @param gcj_lat   GCJ纬度 (十进制度，-85.0511 ~ 85.0511)
 * @param gcj_lon   GCJ经度 (十进制度，-180 ~ 180)
 * @param zoom		缩放级别
 * @param px  		输出：X方向像素偏移 [0, 255]
 * @param py  		输出：Y方向像素偏移 [0, 255]
 *******************************************************************************/
void gcj02_to_tile_pixel_offset(float gcj_lat, float gcj_lon, uint8_t zoom, uint8_t *px, uint8_t *py)
{
    // 限制纬度
    if (gcj_lat > 85.0511f) 
	{
		gcj_lat = 85.0511f;
	}
    if (gcj_lat < -85.0511f) 
	{
		gcj_lat = -85.0511f;
	}

    float n = (float)(1U << zoom); // 2^zoom

    // 全局像素X（连续值）
    float global_pixel_x = (gcj_lon + 180.0f) / 360.0f * n * 256.0f;
    // 全局像素Y（墨卡托投影）
    float lat_rad = gcj_lat * M_PI / 180.0f;
    float global_pixel_y = (1.0f - logf(tanf(lat_rad) + 1.0f / cosf(lat_rad)) / M_PI) * 0.5f * n * 256.0f;

    // 当前瓦片内的像素偏移
    uint16_t pixel_x = (uint16_t)(global_pixel_x) % 256;
    uint16_t pixel_y = (uint16_t)(global_pixel_y) % 256;

    *px = (uint8_t)(pixel_x);
    *py = (uint8_t)(pixel_y);
}



/*******************************************************************************
 * @brief  纬度滤波函数
 *******************************************************************************/
float filter_latitude(float raw_latitude) 
{
    // 将新数据加入历史数据
    latitude_history[latitude_index] = raw_latitude;
    latitude_index = (latitude_index + 1) % GPS_FILTER_WINDOW_SIZE;
    
    // 计算滑动平均值
    float sum = 0.0f;
    for (uint8_t i = 0; i < GPS_FILTER_WINDOW_SIZE; i++) 
	{
        sum += latitude_history[i];
    }
    return sum / GPS_FILTER_WINDOW_SIZE;
}


/*******************************************************************************
 * @brief  经度滤波函数
 *******************************************************************************/
float filter_longitude(float raw_longitude) 
{
    // 将新数据加入历史数据
    longitude_history[longitude_index] = raw_longitude;
    longitude_index = (longitude_index + 1) % GPS_FILTER_WINDOW_SIZE;
    
    // 计算滑动平均值
    float sum = 0.0f;
    for (uint8_t i = 0; i < GPS_FILTER_WINDOW_SIZE; i++) 
	{
        sum += longitude_history[i];
    }
    return sum / GPS_FILTER_WINDOW_SIZE;
}


/*******************************************************************************
 * @brief  速度滤波函数
 *******************************************************************************/
float filter_speed(float raw_speed) 
{
    // 将新数据加入历史数据
    speed_history[speed_index] = raw_speed;
    speed_index = (speed_index + 1) % GPS_FILTER_WINDOW_SIZE;
    
    // 计算滑动平均值
    float sum = 0.0f;
    for (uint8_t i = 0; i < GPS_FILTER_WINDOW_SIZE; i++) 
	{
        sum += speed_history[i];
    }
    return sum / GPS_FILTER_WINDOW_SIZE;
}


/*******************************************************************************
 * @brief  获取滤波后的维度
 *******************************************************************************/
float get_filtered_latitude(void) 
{
    float lat = gps_get_latitude_degree();
    return filter_latitude(lat);
}


/*******************************************************************************
 * @brief  获取滤波后的经度
 *******************************************************************************/
float get_filtered_longitude(void) 
{
    float lon = gps_get_longitude_degree();
    return filter_longitude(lon);
}


/*******************************************************************************
 * @brief  获取滤波后的速度
 *******************************************************************************/
float get_filtered_speed_kmh(void) 
{
    return filter_speed(gps_get_speed_kmh());
}


