/*****************************************************************************
* Copyright:
* File name: tcs34725.h
* Description: tcs34725颜色传感器公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/28
*****************************************************************************/

#ifndef __TCS34725_H__
#define __TCS34725_H__

/*积分时间 精度选择*/
#define TCS34725_INTEGRATIONTIME_2_4MS   0xFF   /**<  2.4ms - 1 cycle    - Max Count: 1024  */
#define TCS34725_INTEGRATIONTIME_24MS    0xF6   /**<  24ms  - 10 cycles  - Max Count: 10240 */
#define TCS34725_INTEGRATIONTIME_50MS    0xEB   /**<  50ms  - 20 cycles  - Max Count: 20480 */
#define TCS34725_INTEGRATIONTIME_101MS   0xD5   /**<  101ms - 42 cycles  - Max Count: 43008 */
#define TCS34725_INTEGRATIONTIME_154MS   0xC0   /**<  154ms - 64 cycles  - Max Count: 65535 */
#define TCS34725_INTEGRATIONTIME_240MS   0x9C   /**<  240ms - 100 cycles - Max Count: 65535 */
#define TCS34725_INTEGRATIONTIME_700MS   0x00   /**<  700ms - 256 cycles - Max Count: 65535 */

/*增益选择*/
#define TCS34725_GAIN_1X                 0x00   /**<  No gain  */
#define TCS34725_GAIN_4X                 0x01   /**<  4x gain  */
#define TCS34725_GAIN_16X                0x02   /**<  16x gain */
#define TCS34725_GAIN_60X                0x03   /**<  60x gain */


typedef struct __TCS34725 c_tcs34725;

typedef struct __TCS34725
{
    void* this;

    /************************************************* 
    * Function: get_rgbc 
    * Description: 获取当前颜色的rgbc值
    * Input : <this>  tcs34725对象
    * Output: <r>     红色值  值域由积分设置决定
    *         <g>     绿色值  值域由积分设置决定
    *         <b>     蓝色值  值域由积分设置决定
    *         <c>     白色值  值域由积分设置决定
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u16 r = 0,g = 0,b = 0,c = 0;
    *
    *         tcs34725.get_rgbc(&tcs34725,&r,&g,&b,&c);
    *         if(E_OK != ret)
    *         {
    *             log_error("tcs34725 get failed.");
    *         }
    *************************************************/  
    int (*get_rgbc)(const c_tcs34725* this,u16* r,u16* g,u16* b,u16* c);
    
    /************************************************* 
    * Function: get_hsl 
    * Description: 获取当前颜色的hsl值
    * Input : <this>  tcs34725对象
    * Output: <h>     当前色调，0~360°一个圆来表示。  0°为红  60°为黄  120°为绿  180°为青  240°为蓝  300°为品红
    *         <s>     饱和度    0~100%
    *         <l>     亮度      0~100%
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u16 h = 0;
    *         u8 s = 0,l = 0;
    *
    *         ret = tcs34725.get_hsl(&tcs34725,&h,&s,&l);
    *         if(E_OK != ret)
    *         {
    *             log_error("tcs34725 get failed.");
    *         }
    *************************************************/  
    int (*get_hsl)(const c_tcs34725* this,u16* h,u8* s,u8* l);
    
    /************************************************* 
    * Function: start 
    * Description: 启动颜色转换
    * Input : <this>           tcs34725对象
    *         <integral_time>  积分时间  时间越长精度越高
    *                          (TCS34725_INTEGRATIONTIME_2_4MS)   2.4ms -  1 cycle   - Max Count: 1024  
    *                          (TCS34725_INTEGRATIONTIME_24MS )   24ms  - 10 cycles  - Max Count: 10240 
    *                          (TCS34725_INTEGRATIONTIME_50MS )   50ms  - 20 cycles  - Max Count: 20480 
    *                          (TCS34725_INTEGRATIONTIME_101MS)   101ms - 42 cycles  - Max Count: 43008 
    *                          (TCS34725_INTEGRATIONTIME_154MS)   154ms - 64 cycles  - Max Count: 65535 
    *                          (TCS34725_INTEGRATIONTIME_240MS)   240ms - 100 cycles - Max Count: 65535 
    *                          (TCS34725_INTEGRATIONTIME_700MS)   700ms - 256 cycles - Max Count: 65535 
    *         <gain>           增益倍率  可理解为放大倍率，在环境过暗 或者颜色区分不明显的时候 适当增大 增益过大会会导致结果容易被干扰
    *                          TCS34725_GAIN_1X                   No  gain  
    *                          TCS34725_GAIN_4X                   4x  gain  
    *                          TCS34725_GAIN_16X                  16x gain 
    *                          TCS34725_GAIN_60X                  60x gain 
    * Output: 无
    * Return: <E_OK>     启动成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = tcs34725.start(&tcs34725,TCS34725_INTEGRATIONTIME_50MS,TCS34725_GAIN_1X);
    *         if(E_OK != ret)
    *         {
    *             log_error("tcs34725 start failed.");
    *         }
    *************************************************/   
    int (*start)(const c_tcs34725* this,u8 integral_time,u8 gain);
}c_tcs34725;

/************************************************* 
* Function: tcs34725_creat 
* Description: 创建一个tcs34725对象
* Input : <iic>  tcs34725所在的iic总线
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_tcs34725 tcs34725 = {0};
*         
*	      tcs34725 = tcs34725_creat(&iic);
*         if(NULL == tcs34725.this)
*         {
*             log_error("tcs34725 creat failed.");
*         }
*************************************************/
c_tcs34725 tcs34725_creat(const c_my_iic* iic);


#endif



