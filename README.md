外设连接：
{
	陶晶驰X3串口屏为串口连接，使用引脚为A00(UART4_TX) A01(UART4_RX)，
	陶晶驰X3串口屏支持读取SD卡内信息，SD卡为闪迪tf卡32G
    陶晶驰X3串口屏下载工具为CP2102芯片的USB转TTL串口模块
    USB转串口为串口连接，使用引脚为D02(UART5_RX) C12(UART5_TX)
    ATGM336H_5N31为串口连接，使用引脚为D05(USART2_TX) D06(USART2_RX)，使用延长天线
}

电源供电：
{
    使用12V锂电池为外部接入供电
    12V转5V使用DCDC转换器，使用芯片为LM2596SX
    5V转3.3V使用LDO(低压差线性稳压器)，使用芯片为AMS1117_3.3V
    5V为串口屏单独供电
    3.3V为单片机，ATGM336H_5N31，USB转串口，//MPU6050供电
}

主芯片：
{
    开发板为嘉立创天空星开发板(STM32F407VxT6开发板)
    https://wiki.lckfb.com/zh-hans/tkx/hardware/schematic.html
}


