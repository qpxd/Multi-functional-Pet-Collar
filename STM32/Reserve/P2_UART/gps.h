/*****************************************************************************
* Copyright:
* File name: gps.h
* Description: gps模块 公开头文件
* Author: 许少
* Version: V1.1
* Date: 2022/04/08
* log:  V1.0  2022/02/17
*       发布：
*       V1.1  2022/04/08
*       修复：gnvta获取 地址m_this->m_gga 少写了o_gnvtg成员问题
*****************************************************************************/

#ifndef __GPS_H__
#define __GPS_H__



#define GPS_BAUDRATE 9600

typedef struct __NMEA_GNGGA
{
    /*GNGGA 卫星定位信息*/
    float o_utc                  ;  /*UTC时间*/
    float o_latitude             ;  /*纬度 */
    char  o_latitude_hemisphere  ;  /*纬度半球*/
    float o_longitude            ;  /*经度*/
    char  o_longitude_hemisphere ;  /*经度半球*/
    u32   o_state                ;  /*定位状态 0=未定位，1=非差分定位，2=差分定位*/
    u32   o_num                  ;  /*卫星数量*/
    float o_hdop                 ;  /*水平精确度因子（0.5~99.9）*/
    float o_altitude             ;  /*海拔高度（-9999.9 到 9999.9 米）*/
}nmea_gngga;

typedef struct __NMEA_GNVTG
{   
    /*GNVTG 地面速度信息*/
    float o_heading_due_north      ; /*以真北为参考基准的地面航向（000~359 度，前面的 0 也将被传输）*/
    float o_heading_magnetic_north ; /*以磁北为参考基准的地面航向（000~359 度，前面的 0 也将被传输）*/
    float o_ground_speed_knots     ; /*地面速率（000.0~999.9 节，前面的 0 也将被传输）*/
    float o_ground_speed_kmh       ; /*地面速率（000.0~999.9 节，前面的 0 也将被传输）*/
    char  o_mode                   ; /*模式指示（仅 NMEA0183 3.00 版本输出，A=自主定位，D=差分，E=估算，N=数据无效）*/
}nmea_gnvtg;

/*gga信息*/
typedef struct __NMEA_GGA
{
    /*GNGGA 卫星定位信息*/
    nmea_gngga o_gngga;
    
    /*GNVTG 地面速度信息*/
    nmea_gnvtg o_gnvtg;
}nmea_gga;



typedef struct __C_GPS c_gps;

typedef struct __C_GPS
{
    void* this;

    /************************************************* 
    * Function: get 
    * Description: 获取当前GPS信息
    * Input : <this>      gps对象
    * Output: <msg>       nmea gga信息 成员见 struct nmea_gga
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         nmea_gga data = {0};
    *         
    *         ret = gps.get(&gps,&data);
    *         if(E_OK != ret)
    *         {
    *             log_error("gps get failed.");
    *         }
    *         log_inform("num:%d,a:%.2fm,%c:%.5f,%c:%.5f",data.o_num,data.o_altitude,data.o_latitude_hemisphere,data.o_latitude,data.o_longitude_hemisphere,data.o_longitude);
    *************************************************/  
    int (*get)(const c_gps* this,nmea_gga* msg);
}c_gps;

/************************************************* 
* Function: gps_creat 
* Description: 创建一个gps对象
* Input : <uart_id>  gps所在的串口编号
*                    (MY_UART_1)  串口1
*                    (MY_UART_2)  串口2
*                    (MY_UART_3)  串口3
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_gps gps = {0};
*         
*	      gps = gps_creat(MY_UART_1);
*         if(NULL == gps.this)
*         {
*             log_error("gps creat failed.");       
*         }
*************************************************/
c_gps gps_creat(u8 uart_id);

#endif


