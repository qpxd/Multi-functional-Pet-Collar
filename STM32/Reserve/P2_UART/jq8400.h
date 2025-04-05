/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : jq8400.h
* Description: jq8400语音播报模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/05
*****************************************************************************/

#ifndef __JQ8400_H__
#define __JQ8400_H__



#define JQ8400_BAUDRATE    9600   /*jq8400的串口波特率*/

typedef struct __JQ8400 c_jq8400;

#define JQ8400_IDLE   0  /*空闲状态*/
#define JQ8400_PLAY   1  /*播放状态*/
#define JQ8400_PAUSE  2  /*暂停播放*/

#define JQ8400_VOLUME_MAX  30  /*音量最大*/

typedef struct __JQ8400
{
    void* this;

    /************************************************* 
    * Function: play 
    * Description: 播放曲目
    * Input : <this>  jq8400对象
    *         <loop>  true表示循环播放 false播放一次后停止
    *         <index> 曲目编号 1~65535 要求flash中有对应曲目，否则播放无效
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 如果在调用本接口时，其它音频没有播放完成，那么未完成
    *         的音频将立即停止，并立即开始播放新的音频。
    *         本接口为异步执行接口，调用完成后，语音模块将自行完成
    *         播放任务。
    * Demo  :
    *         ret = jq8400.play(&jq8400,false,1);
    *         if(E_OK != ret)
    *         {
    *             log_error("jq8400 play failed.");
    *         }
    *************************************************/ 
    int (*play)  (const c_jq8400* this,bool loop,u16 index);

    /************************************************* 
    * Function: stop 
    * Description: 停止播放
    * Input : <this>  jq8400对象
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = jq8400.stop(&jq8400);
    *         if(E_OK != ret)
    *         {
    *             log_error("jq8400 stop failed.");
    *         }
    *************************************************/ 
    int (*stop)  (const c_jq8400* this);

    /************************************************* 
    * Function: state 
    * Description: 查询jq8400当前状态
    * Input : <this>  jq8400对象
    * Output: <state> 当前状态
    *                 (JQ8400_IDLE )  空闲状态
    *                 (JQ8400_PLAY )  播放状态
    *                 (JQ8400_PAUSE)  暂停播放
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u8 state = 0;
    *
    *         ret = jq8400.state(&jq8400,&state);
    *         if(E_OK != ret)
    *         {
    *             log_error("jq8400 state get failed.");
    *         }
    *************************************************/ 
    int (*state) (const c_jq8400* this,u8* state);
    
    /************************************************* 
    * Function: volume 
    * Description: 设置播放音量
    * Input : <this>    jq8400对象
    *         <volume>  0~JQ8400_VOLUME_MAX
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = jq8400.volume(&jq8400,30);
    *         if(E_OK != ret)
    *         {
    *             log_error("jq8400 set failed.");
    *         }
    *************************************************/ 
    int (*volume)(const c_jq8400* this,u8 volume);
}c_jq8400;

/************************************************* 
* Function: jq8400_creat 
* Description: 创建一个jq8400对象
* Input : <uart_id>  jq8400所在的串口编号
*                    (MY_UART_1)  串口1
*                    (MY_UART_2)  串口2
*                    (MY_UART_3)  串口3
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_jq8400 jq8400 = {0};
*         
*	      jq8400 = jq8400_creat(MY_UART_1);
*         if(NULL == jq8400.this)
*         {
*             log_error("Jq8400 creat failed.");       
*         }
*************************************************/
c_jq8400 jq8400_creat(u8 uart_id);

#endif


