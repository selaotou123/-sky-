外设连接：
{
	陶晶驰X3串口屏（https://e.tb.cn/h.7CpCUOevx9QIt78?tk=mUTGUMJz9Ws）为串口连接，使用引脚为A00(UART4_TX) A01(UART4_RX)，
	陶晶驰X3串口屏支持读取SD卡内信息，SD卡为闪迪tf卡32G（https://e.tb.cn/h.7ycqx4AAehye248?tk=Z9ZkUMrZNdV）
    陶晶驰X3串口屏下载工具为CP2102芯片的USB转TTL串口模块（https://e.tb.cn/h.7CK3eCWcWyHAlWZ?tk=WNRwUMr44tc）
    USB转串口为串口连接，使用引脚为D02(UART5_RX) C12(UART5_TX)
    //MPU6050为IIC连接，使用引脚为B08(IIC1_SCL) B07(IIC_SDA)
    ATGM336H_5N31（https://e.tb.cn/h.7CpA5xMQ8zylveC?tk=9yUqUMr0SbQ）为串口连接，使用引脚为D05(USART2_TX) D06(USART2_RX)，使用延长天线（https://e.tb.cn/h.7CpwFsoKpSeSz3H?tk=OECeUMJy6H5）
}

电源供电：
{
    使用12V锂电池（https://e.tb.cn/h.7Ccezb8S8Odnsn9?tk=oC9HUMJDcCw）为外部接入供电
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

尽量在室外使用，初次启动搜星时间较长，大致在30秒到1分钟左右，地图资源采用QGIS 3.40.14（https://qgis.org/download/）的插件QTiles下载
地图源采用高德地图的街道图（https://webrd01.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=8&x={x}&y={y}&z={z}）
地图下载格式为xyz金字塔瓦片地图，像素大小为255*255，下载缩放等级分别为（16，14，12），下载区域为嘉兴，绍兴，宁波

