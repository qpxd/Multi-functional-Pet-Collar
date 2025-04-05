/*****************************************************************************
* Copyright:
* File name: key_board.h
* Description: 键盘模块公开头文件
* Author: 许少
* Version: V1.3
* Date: 2022/04/08
* log:  V1.0  2021/12/29
*       发布：
*
*       V1.1  2022/01/30
*       修复：377行KEY_PRESS_IS_ONE 写成KEY_PRESS_IS_ZERO 导致无法进入按下为1的处理。
*
*       V1.2  2022/03/08
*       新增：get_hw 通过硬件状态 获取按键状态
*
*       V1.3  2022/04/06
*       修复：get_hw 函数指针未赋值
*****************************************************************************/

#ifndef __KEY_BOARD_H__
#define __KEY_BOARD_H__



#define KEY_LONG_PRESS_TIME_MS   1000   /*按键长按时间*/
#define KEY_ELIMINATE_JITTER_MS    10   /*消抖时间*/
#define KEY_NAME_MAX_LEN           15   /*最长键名*/
#define KEY_BOARD_MAX_KEY_AMOUNT   10   /*一块键盘所包含的最大键数*/
#define KEY_BOARD_MAX_AMOUNT       10   /*最大可创建键盘数*/

#define KEY_PRESS_IS_ZERO  0 /*按下为零*/
#define KEY_PRESS_IS_ONE   1 /*按下为一*/

#define KEY_UP          0  /*弹起*/
#define KEY_PRESS       1  /*按下*/
#define KEY_LONG_PRESS  2  /*长按*/

typedef struct __KEY_BOARD c_key_board;

/*按键列表*/
typedef struct __KEY_LIST
{
    const char*   name    ;    /*按键名称*/
    GPIO_TypeDef* gpio    ;    /*按键所在GPIO分组*/
    uint32_t      pin     ;    /*按键所在PIN脚*/
    u8            polarity;    /*按键极性*/
}key_list;

/*键盘对象*/
typedef struct __KEY_BOARD
{
    void* this;

    /************************************************* 
    * Function: get 
    * Description: 获取当前按键状态
    * Input : <this>      键盘对象
    *         <key_name>  按键名
    * Output: <state>     按键当前状态
    *                     (KEY_UP)         弹起
    *                     (KEY_PRESS)      按下
    *                     (KEY_LONG_PRESS) 长按
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = board.get(&board,"UP",&state);
    *         if(E_OK != ret)
    *         {
    *             log_error("Key state get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_key_board* this,const char* key_name,u8* state);
    
    /************************************************* 
    * Function: get_hw 
    * Description: 通过硬件 获取按键的状态
    * Input : <this>      键盘对象
    *         <key_name>  按键名
    * Output: <state>     按键当前状态
    *                     (KEY_UP)         弹起
    *                     (KEY_PRESS)      按下
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = board.get_hw(&board,"UP",&state);
    *         if(E_OK != ret)
    *         {
    *             log_error("Key state get failed.");
    *         }
    *************************************************/  
    int (*get_hw)(const c_key_board* this,const char* key_name,u8* state);

    /************************************************* 
    * Function: set_callback 
    * Description: 设置键盘的回调函数
    * Input : <this>      键盘对象
    *         <callback>  回调函数
    * Output: <无> 
    * Return: <E_OK>     获取成功
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         int key_board_call(const char* key_name,u8 state)
    *         {
    *             log_inform("KEY:%s,state:%d",key_name,state);
    *             return E_OK;
    *         }
    *
    *         ret = board.set_callback(&board,key_board_call);
    *         if(E_OK != ret)
    *         {
    *             log_error("Callback set failed.");
    *         }
    *************************************************/  
    int (*set_callback)(c_key_board* this,int (*callback)(const char* key_name,u8 state));
}c_key_board;

/************************************************* 
* Function: key_board_create 
* Description: 创建一个键盘对象
* Input : <key_list>  按键列表
*         <list_len>  列表长
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 注：因本
* Demo  :     
*         c_key_board board = {0};
*
*         key_list board_list[] = {
*            {"UP"  ,GPIOB,GPIO_PIN_10,KEY_PRESS_IS_ZERO },
*            {"DOWN",GPIOB,GPIO_PIN_1 ,KEY_PRESS_IS_ONE  },
*         };
*        
*         board = key_board_create(board_list,sizeof(board_list)/sizeof(key_list));
*         if(NULL == board.this)
*         {
*             log_error("Board create failed.");
*         }

*************************************************/
c_key_board key_board_create(key_list* list,u8 list_len);


#endif



