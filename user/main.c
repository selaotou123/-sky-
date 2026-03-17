#include "stm32f4xx.h"
#include "board.h"
#include "debug_uart5.h"
#include "gps_uart2.h"
#include "gps_data_processing.h"
#include "tjc_uart4.h"
#include <stdio.h>
#include <string.h>

char Yes,No;

int main() 
{
    board_init();
    //debug_uart5_init(115200);
    tjc_uart4_init(9600);
    gps_uart2_init(9600);
    
    delay_ms(1000);
    
    while(1) 
	{
        delay_ms(100);
        if (gps_is_new_data_available()) 
		{
            // 获取滤波后的GPS数据
            float wgs_lat = get_filtered_latitude();
            float wgs_lon = get_filtered_longitude();
            
            // 检查是否有效定位
            if (gps_has_fix()) 
			{
				TJCPrintf("show_GPS.txt=\"Yes\"");
				TJCPrintf("show_GPS_max.txt=\"Yes\"");
				//时间矫正（加1秒）
				correct_gps_time(&gps_device);
                // WGS84 → GCJ-02
                float gcj_lat, gcj_lon;
                wgs84_to_gcj02(wgs_lat, wgs_lon, &gcj_lat, &gcj_lon);
                
                // GCJ-02转高德瓦片坐标
                uint32_t tile_x, tile_y, tile_x_1, tile_y_1, tile_x_max, tile_y_max;
                gcj02_to_gaode_tile(gcj_lat, gcj_lon, 17, &tile_x, &tile_y);
                gcj02_to_gaode_tile(gcj_lat, gcj_lon, 16, &tile_x_1, &tile_y_1);
                gcj02_to_gaode_tile(gcj_lat, gcj_lon, 14, &tile_x_max, &tile_y_max);
                
                // 经纬度符号动态生成
                char lat_dir = (gcj_lat >= 0) ? 'N' : 'S'; // 北纬/南纬
                char lon_dir = (gcj_lon >= 0) ? 'E' : 'W'; // 东经/西经
                
                // 获取北京时间
                convert_utc_to_beijing(&gps_device);
                
                // 判断方位
                uint8_t px, py;
                gcj02_to_tile_pixel_offset(gcj_lat, gcj_lon, 17, &px, &py);
                
                // === 判断方位（九宫格划分）===
                const char* position_str = "center";
                if (px < 85) 
				{
                    if (py < 85) 
						position_str = "topleft"; //左上
                    else if (py > 170) 
						position_str = "lowleft"; //左下
                    else 
						position_str = "left"; //左
                } 
				else if (px > 170) 
				{
                    if (py < 85) 
						position_str = "topright"; //右上
                    else if (py > 170) 
						position_str = "lowright"; //右下
                    else 
						position_str = "right"; //右
                } 
				else 
				{
                    if (py < 85) 
						position_str = "top"; //上
                    else if (py > 170) 
						position_str = "low"; //下
                    else 
						position_str = "center";
                }
                
                // 显示地图
                TJCPrintf("main_exp0.path=\"sd0/%d/%d/%d.jpg\"", 17, tile_x, tile_y);
                TJCPrintf("main_exp1.path=\"sd0/%d/%d/%d.jpg\"", 16, tile_x_1, tile_y_1);
                TJCPrintf("max_exp.path=\"sd0/%d/%d/%d.jpg\"", 14, tile_x_max, tile_y_max);
                
                // 显示时间
                TJCPrintf("show_time.txt=\"%04d-%02d-%02d %02d:%02d:%02d\"", 
                          gps_device.data.bj_year, gps_device.data.bj_month, gps_device.data.bj_day,
                          gps_device.data.bj_hour, gps_device.data.bj_minute, gps_device.data.bj_second);
                TJCPrintf("show_time_max.txt=\"%04d-%02d-%02d %02d:%02d:%02d\"", 
                          gps_device.data.bj_year, gps_device.data.bj_month, gps_device.data.bj_day,
                          gps_device.data.bj_hour, gps_device.data.bj_minute, gps_device.data.bj_second);
                
                // 显示经纬度
                TJCPrintf("show_latitude.txt=\"%.6f %c\"", gcj_lat, lat_dir);
                TJCPrintf("show_lat_max.txt=\"%.6f %c\"", gcj_lat, lat_dir);
                TJCPrintf("show_longitude.txt=\"%.6f %c\"", gcj_lon, lon_dir);
                TJCPrintf("show_lon_max.txt=\"%.6f %c\"", gcj_lon, lon_dir);
                
                // 显示速度
                float speed = get_filtered_speed_kmh();
                TJCPrintf("show_speed.txt=\"%d km/h\"", (int)speed);
                TJCPrintf("show_speed_max.txt=\"%d km/h\"", (int)speed);
                
                // 显示方位
                TJCPrintf("pos_indicator.txt=\"%s\"", position_str);
            }
			else
			{
				TJCPrintf("show_GPS.txt=\"No\"");
				TJCPrintf("show_GPS_max.txt=\"No\"");
			}
            gps_clear_new_data_flag();
        }

    }
}

