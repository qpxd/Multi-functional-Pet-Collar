/*****************************************************************************
* Copyright:
* File name: my_uart.h
* Description: 串口模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/03
* Instructions for use :
*
*第一步：你必须使用uart_init来初始化需要的串口，未初始化的调用将会导致失败
*第二步：初始化完成后，使用uart_send来发送需要发送的数据 注：数据长度不可超过UART_MAX_SEND_DATA_LEN限定的大小
*第三步：如果需要通过串口接收数据，使用uart_set_callback来设置相应的回调函数。请注意，回调函数的作用在于通知
*        和传递相应的数据，不可在回调函数内定义过多局部变量、做数据处理、同步消息等耗时耗内存的工作
*第四步：如果需要清空接收缓存 调用uart_clean_recv_buf
*****************************************************************************/

#ifndef __UART_H__
#define __UART_H__



#define UART_MAX_RECV_BUF_SIZE     256  /*接收缓存大小*/
#define UART_MAX_SEND_DATA_LEN     512  /*单条最大发送数据长度*/

#define MY_UART_1   0   /*串口1*/
#define MY_UART_2   1   /*串口2*/
#define MY_UART_3   2   /*串口3*/

typedef enum
{
    UART_MODE_POLL = 0, //轮询模式 （暂未启用）
    UART_MODE_ISR,      //中断模式
    UART_MODE_DMA,      //DMA模式
}uart_mode_t;

typedef struct __C_UART
{
    /************************************************* 
    * Function: init 
    * Description: 初始化一个串口
    * Input : <uart>        初始化哪个uart  
    *         <baud_rate>   串口波特率
    *         <buf_size>    串口接收缓存大小 按使用需求来设计
    *                       注：该缓存为自动覆盖机制 既缓存满后，会自动覆盖老的数据
    *         <mode>        串口工作模式： DMA模式 / 中断模式 / 轮询模式 参见 uart_mode_t 定义
    * Output: 无
    * Return: <E_OK>             初始化成功
    *         <E_PARAM>          参数错误
    *         <E_OUT_OF_MEMORY>  内存不足
    *         <E_ERROR>          初始化失败
    * Others: 无
    * Demo  :
    *         int ret = 0;
    *         
    *         ret = my_uart.uart_init(MY_UART_1,115200,64,UART_MODE_DMA);
    *         if(E_OK != ret)
    *         {
    *            log_error("Uart init failed.");
    *         }
    *************************************************/
    int (*init)           (u8 uart, u32 baud_rate, u16 buf_size, uart_mode_t mode);

    /************************************************* 
    * Function: send 
    * Description: 通过一个串口 发送串口消息
    * Input : <uart>        使用哪个uart  
    *         <data>        数据地址
    *         <data_len>    数据长度 注：不可超过UART_MAX_SEND_DATA_LEN
    *                       注：该缓存为自动覆盖机制 既缓存满后，会自动覆盖老的数据
    * Output: 无
    * Return: <E_OK>             发送成功
    *         <E_PARAM>          参数错误
    *         <E_OUT_OF_MEMORY>  内存不足
    *         <E_ERROR>          发送失败
    * Others: 该接口的使用，必须建立在初始化成功的基础上，否则调用将失败
    *         本发送为异步的，数据为拷贝传递，不需要保留原来的数据
    * Demo  :
    *         int ret = 0;
    *         
    *         ret = my_uart.send(MY_UART_1,data,data_len);
    *         if(E_OK != ret)
    *         {
    *            log_error("Uart send failed.");
    *         }
    *************************************************/
    int (*send)           (u8 uart,const u8* data     ,u16 data_len);

    /************************************************* 
    * Function: set_callback 
    * Description: 设置串口回调
    * Input : <uart>            设置哪个串口的回调
    *         <param>           回调触发 需要带入的参数
    *         <callback>        回调函数
    *           Input ：<param>       传参
    *                 ：<data>        接收缓存数据首地址
    *                 ：<data_len>    接收缓存数据长度
    * Output: 无
    * Return: <E_OK>             设置成功
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          设置失败
    * Others: 无
    * Demo  :
    *         int uart3_callback(void* param,const u8* data,u16 data_len)
    *         {
    *             ret = my_uart.send(MY_UART_3,NULL,data,data_len);
    *             if(E_OK != ret)
    *             {
    *                log_error("Uart send failed.");
    *             }
    *         }
    *         
    *         my_uart.set_callback(MY_UART_3,NULL,uart3_callback);
    *         if(E_OK != ret)
    *         {
    *            log_error("Callback set failed.");
    *         }
    *************************************************/
    int (*set_callback)   (u8 uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
}c_uart;

extern const c_uart my_uart;  /*单例*/


#endif



