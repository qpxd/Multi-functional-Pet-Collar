/*****************************************************************************
* Copyright:
* File name: track_car.h
* Description: 循迹车 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/
#ifndef __TRACKING_CAR__H__
#define __TRACKING_CAR__H__



#define WORK_TYPE_IDLE    0
#define WORK_TYPE_FOLLOW  1
#define WORK_TYPE_PATH    2

#define CAR_PASS  0  /*穿越*/
#define CAR_LEFT  1  /*左转*/
#define CAR_RIGHT 2  /*右转*/
#define CAR_TURN  3  /*掉头*/

#define CAR_PATH_MAX_LEN  50

typedef struct __TRACK_CAR
{
    int (*init)     (void);
    int (*pause)    (void);        /*暂停*/
    int (*cont)     (void);        /*继续*/
	int (*stop)     (void);        /*停止  如果小车之前在指向路径 那么路径会被清除*/
    
    /************************************************* 
    * Function: move 
    * Description: 启动循迹
    * Input : 无
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         
    *	      ret = track_car.move();
    *         if(E_OK != ret)
    *         {
    *             log_error("Move failed.");
    *         }
    *************************************************/
	int (*move)     (void);
    
    /************************************************* 
    * Function: move_by 
    * Description: 启动路径
    * Input : <path>  路径指针
    *         <len>   路径长
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  : 
    *         u8 path[4] = {CAR_PASS,CAR_LEFT,CAR_RIGHT,CAR_TURN};
    *         
    *	      ret = track_car.move_by(path,4);
    *         if(E_OK != ret)
    *         {
    *             log_error("Move failed.");
    *         }
    *************************************************/
    int (*move_by)  (u8* path,u16 len);
    int (*state)    (u8* );
	u8  (*getspeed) (void);
    int (*setspeed) (u8);
}c_track_car;

extern const c_track_car track_car;


#endif

