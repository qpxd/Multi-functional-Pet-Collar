/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : hlk_v40.h
* Description: hlk_v40 驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#ifndef __HLKV40_H__
#define __HLKV40_H__

#define HLKV40_BAUDRATE         115200       //串口通信波特率
#define HLKV40_VOL              100        //音量
#define HLKV40_VOER_TIME        1000        //命令响应超时时间


#define HLK_CMD_ENTER_AT     	"+++"
#define HLK_CMD_ENTER_AT2    	"a"
#define HLK_CMD_SET_VOL     	"at+play_vol=%d\r"
#define HLK_CMD_PALY_TEXT    	"at+play_text="

typedef struct __C_HLKV40
{
    void* this;
	
    /************************************************* 
    * Description: 复位初始化函数，创建设备时会默认初始化并设置默认音量为 100 无需额外调用此函数
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *		  int ret = 0; 
    *         ret = hlkv40.init(&hlkv40);
    *         if(ret != E_OK)
    *         {
    *             log_error("hlkv40 init failed.");
    *         }
    *************************************************/
    int(*init)          (const struct __C_HLKV40* this);
    /************************************************* 
    * Description: 设置播放音量，创建设备时会默认初始化并设置默认音量为 100 无需额外调用此函数
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *		  int ret = 0; 
    *         ret = hlkv40.set_volume(&hlkv40, 100);
    *         if(ret != E_OK)
    *         {
    *             log_error("hlkv40 set volume failed.");
    *         }
    *************************************************/
    int(*set_volume)    (const struct __C_HLKV40* this, unsigned char volume);
    /************************************************* 
    * Description: 语音播报文本内容，播放中文文本必须为 GB2312，需提前修改编码格式或者在notepad中转码后拷贝过来
	*			   英文文本只会按字母播报，不能播报单词
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *		  int ret = 0; 
    *         ret = hlkv40.play_text(&hlkv40, "ABC%d", 100);
    *         if(ret != E_OK)
    *         {
    *             log_error("hlkv40 play text failed.");
    *         }
    *************************************************/
    int(*play_text)     (const struct __C_HLKV40* this, char* format,...);
}c_hlkv40;

/************************************************* 
* Function: hlkv40_creat 
* Description: 创建 HLK_V40 语音合成设备
* Input : <uart_id>  串口ID
*         <rst_gpio>     复位脚Port
*		  <rst_gpio>     复位脚Pin
* Output: 无
* Return: <E_OK>     操作成功
*         <E_NULL>   空指针
*         <E_ERROR>  操作失败
* Others: 无
* Demo  :
*		  c_hlkv40 hlkv40 = {0};
*         hlkv40 = hlkv40_creat(MY_UART_1, GPIOB, GPIO_PIN_10);
*         if(NULL == hlkv40.this)
*         {
*             log_error("hlkv40 creat failed.");
*         }
*************************************************/
c_hlkv40 hlkv40_creat(unsigned char uart_id, GPIO_TypeDef* rst_gpio, uint32_t rst_pin);

#endif

