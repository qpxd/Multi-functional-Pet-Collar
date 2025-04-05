/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : uln2003.h
* Description: uln2003步进电机驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/04
*****************************************************************************/

#ifndef __ULN2003_H__
#define __ULN2003_H__



#define SPEED_MIN   0.5f           /*最小转速 单位 转/min*/
#define SPEED_MAX   7.3242f        /*最大转速 单位 转/min*/
#define ANGLE_MAX  (30.0f*360.0f)  /*最大单次旋转角度 单位度*/

#define ULN2003_IDLE  0  /*空闲状态*/
#define ULN2003_BUSY  1  /*忙状态*/

/*（优点）简单，耗电低，精确性良好。
   (缺点）力矩小，振动大，每次励磁信号走的角度都是标称角度。  1相励磁法  A->B->C->D*/
#define TYPE_1_PHASE    0  /*每一瞬间只有一个线圈相通，其它休息*/

/*（优点）力矩大，震动小。
  （缺点）每励磁信号走的角度都是标称角度。2相励磁法  AB->BC->CD->DA*/
#define TYPE_2_PHASE    1  /*2相励磁法：每一瞬间有两个线圈导通*/

/*（优点）精度较高，运转平滑，每送一个励磁信号转动1/2标称角度，称为半步驱动。
  （前两种称为4相4拍，这一种称为4相8拍）1-2相励磁法  A-->AB-->B->BC->C-->CD->D-->DA
  （缺点）同样的转速 会更占用性能 耗电 发热大*/
#define TYPE_1_2_PHASE  2  /*1-2相励磁法：1相和2相交替导通*/

typedef struct __ULN2003 c_uln2003;

typedef struct __ULN2003
{
    void* this;

    /************************************************* 
    * Function: set 
    * Description: 设置电机的励磁类型
    * Input : <this>  uln2003对象
    *         <type>  励磁类型
    *                 (TYPE_1_PHASE)    1相励磁法
    *                 (TYPE_2_PHASE)    2相励磁法
    *                 (TYPE_1_2_PHASE)  1-2相励磁法
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 设置电机的励磁类型 要求必须在电机停止的状态
    *         下进行，否则将引发错误
    * Demo  :
    *         ret = uln2003.set(&uln2003,TYPE_1_2_PHASE);
    *         if(E_OK != ret)
    *         {
    *             log_error("uln2003 set failed.");
    *         }
    *************************************************/  
    int (*set)(const c_uln2003* this,u8 type);
    
    /************************************************* 
    * Function: rotate 
    * Description: 电机旋转一定角度
    * Input : <this>  uln2003对象
    *         <angle> 旋转角度  0~ANGLE_MAX 单位：度
    *         <speed> 旋转速度 SPEED_MIN~SPEED_MAX  单位：转/min
    *                 注：因底层的延时必须为整毫秒数，所以转速有一定误差
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 如果在调用本接口时，电机已经处于工作状态，那么
    *         旧的工作状态将被覆盖。
    *         本接口为异步执行接口，调用完成后，电机将自主完成旋转任务
    * Demo  :
    *         ret = uln2003.rotate(&uln2003,90.0f,7.3242f);
    *         if(E_OK != ret)
    *         {
    *             log_error("uln2003 rotate failed.");
    *         }
    *************************************************/ 
    int (*rotate)(const c_uln2003* this,float angle,float speed);
    
    /************************************************* 
    * Function: speed 
    * Description: 电机以一定速度保持旋转
    * Input : <this>  uln2003对象
    *         <speed> 旋转速度 SPEED_MIN~SPEED_MAX  单位：转/min
    *                 注：因底层的延时必须为整毫秒数，所以转速有一定误差
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 如果在调用本接口时，电机已经处于工作状态，那么
    *         旧的工作状态将被覆盖。
    * Demo  :
    *         ret = uln2003.speed(&uln2003,7.3242f);
    *         if(E_OK != ret)
    *         {
    *             log_error("uln2003 rotate failed.");
    *         }
    *************************************************/ 
    int (*speed)(const c_uln2003* this,float speed);
    
    /************************************************* 
    * Function: stop 
    * Description: 立即停止电机
    * Input : <this>  uln2003对象
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = uln2003.stop(&uln2003);
    *         if(E_OK != ret)
    *         {
    *             log_error("uln2003 stop failed.");
    *         }
    *************************************************/ 
    int (*stop)(const c_uln2003* this);
    
    /************************************************* 
    * Function: state 
    * Description: 获取电机状态
    * Input : <this>  uln2003对象
    * Output: <state> 电机当前状态
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u8 state = 0;
    *
    *         ret = uln2003.state(&uln2003,&state);
    *         if(E_OK != ret)
    *         {
    *             log_error("uln2003 state get failed.");
    *         }
    *************************************************/ 
    int (*state)(const c_uln2003* this,u8* state);
}c_uln2003;

/************************************************* 
* Function: uln2003_creat 
* Description: 创建一个uln2003对象
* Input : <type>   励磁类型
*                  (TYPE_1_PHASE)    1相励磁法
*                  (TYPE_2_PHASE)    2相励磁法
*                  (TYPE_1_2_PHASE)  1-2相励磁法
*         <a_gpio> a相GPIO分组
*         <a_pin>  a相PIN
*         <b_gpio> b相GPIO分组
*         <b_pin>  b相PIN
*         <c_gpio> c相GPIO分组
*         <c_pin>  c相PIN
*         <d_gpio> d相GPIO分组
*         <d_pin>  d相PIN
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_uln2003 uln2003 = {0};
*         
*         uln2003 = uln2003 = uln2003_creat(TYPE_1_PHASE,GPIOA,GPIO_PIN_4,GPIOA,GPIO_PIN_5,GPIOA,GPIO_PIN_6,GPIOA,GPIO_PIN_7);
*         if(NULL == uln2003.this)
*         {
*             log_error("uln2003 creat failed.");
*         }
*************************************************/
c_uln2003 uln2003_creat(u8 type,
                            GPIO_TypeDef* a_gpio,uint32_t a_pin,GPIO_TypeDef* b_gpio,uint32_t b_pin,
                           GPIO_TypeDef* c_gpio,uint32_t c_pin,GPIO_TypeDef* d_gpio,uint32_t d_pin);

#endif


