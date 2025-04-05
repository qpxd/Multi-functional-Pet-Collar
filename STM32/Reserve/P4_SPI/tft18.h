/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : tft18.h
* Description: tft18屏幕 公开头文件
* Author: xzh
* Version: v1.1
* Date: 2022/02/06
* log:  V1.0  2022/02/06
*       发布：
*
*       V1.1  2022/03/21
*       新增：rectangle 矩形绘制接口。
*****************************************************************************/

#ifndef __TFT_18_H__
#define __TFT_18_H__



#define TFT18_COLOR_WHITE        0xFFFF
#define TFT18_COLOR_BLACK        0x0000   
#define TFT18_COLOR_BLUE         0x001F  
#define TFT18_COLOR_BRED         0XF81F
#define TFT18_COLOR_GRED         0XFFE0
#define TFT18_COLOR_GBLUE        0X07FF
#define TFT18_COLOR_RED          0xF800
#define TFT18_COLOR_MAGENTA      0xF81F
#define TFT18_COLOR_GREEN        0x07E0
#define TFT18_COLOR_CYAN         0x7FFF
#define TFT18_COLOR_YELLOW       0xFFE0
#define TFT18_COLOR_BROWN        0XBC40 //棕色
#define TFT18_COLOR_BRRED        0XFC07 //棕红色
#define TFT18_COLOR_GRAY         0X8430 //灰色
#define TFT18_COLOR_DARKBLUE     0X01CF //深蓝色
#define TFT18_COLOR_LIGHTBLUE    0X7D7C //浅蓝色  
#define TFT18_COLOR_GRAYBLUE     0X5458 //灰蓝色
#define TFT18_COLOR_LIGHTGREEN   0X841F //浅绿色
#define TFT18_COLOR_LGRAY        0XC618 //浅灰色(PANNEL),窗体背景色
#define TFT18_COLOR_LGRAYBLUE    0XA651 //浅灰蓝色(中间层颜色)
#define TFT18_COLOR_LBBLUE       0X2B12 //浅棕蓝色(选择条目的反色)

#define TFT18_DIR_V_POSITIVE  1  /*竖屏正*/
#define TFT18_DIR_V_REVERSE   0  /*竖屏反*/
#define TFT18_DIR_H_POSITIVE  2  /*横屏正*/
#define TFT18_DIR_H_REVERSE   3  /*横屏反*/

#define TFT18_FONT_12     12  /*12字宽*/
#define TFT18_FONT_16     16  /*16字宽*/
#define TFT18_FONT_24     24  /*24字宽*/
#define TFT18_FONT_32     32  /*32字宽*/

#define TFT18_BRUSH_DEFAULT_COLOR  TFT18_COLOR_BLACK  /*画笔默认黑色*/
#define TFT18_BACK_DEFAULT_COLOR   TFT18_COLOR_WHITE  /*背景默认白色*/

typedef struct __C_TFT18 c_tft18;

typedef struct __C_TFT18
{
    void* this;

    /************************************************* 
    * Function: set 
    * Description: 设置画笔
    * Input : <this>  tft18对象
    *         <brush> 画笔颜色
    *         <back>  背景颜色
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         tft18.set(&tft18,TFT18_COLOR_WHITE,TFT18_COLOR_BLACK);
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 set failed.");
    *         }
    *************************************************/      
    int (*set)(const c_tft18* this,u16 brush,u16 back);
    
    /************************************************* 
    * Function: clear 
    * Description: 清除屏幕
    * Input : <this>  tft18对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         tft18.clear(&tft18);
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 clear failed.");
    *         }
    *************************************************/        
    int (*clear)(const c_tft18* this);
    
    /************************************************* 
    * Function: rectangle 
    * Description: 绘制矩形
    * Input : <this>   tft18对象
    *         <x>      矩形左上角x坐标
    *         <y>      矩形右上角y坐标
    *         <weight> 矩形宽
    *         <height> 矩形高
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         tft18.clear(&tft18);
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 clear failed.");
    *         }
    *************************************************/        
    int (*rectangle)(const c_tft18* this,u16 x,u16 y,u16 weight,u16 height);
    
    /************************************************* 
    * Function: str 
    * Description: 显示字符串
    * Input : <this>  tft18对象
    *         <x>     字符串的x坐标  
    *         <y>     字符串的y坐标  
    *         <format>格式化字符串
    *         <...>   可变参
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 如果字符超出屏幕能够显示的区域，过多的字符会被丢弃
    * Demo  :
    *         int a = 9999999;
    *
    *         tft18.str(&tft18,0,0 ,12,"%d",a);
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 display failed.");
    *         }
    *************************************************/     
    int (*str)(const c_tft18* this,u16 x,u16 y,u8 size, char* format,...);

	/************************************************* 
    * Function: chinese_str 
    * Description: 显示汉字
    * Input : <this>  tft18对象
    *         <x>     字符串的x坐标  
    *         <y>     字符串的y坐标  
	*		  <s>     汉字字符串
    * Output: 无
     * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 如果字符超出屏幕能够显示的区域，过多的字符会被丢弃
    * Demo  :
    *         tft18.chinese_str(&tft18,0,0 ,12,"密码错误");
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 display failed.");
    *         }
    *************************************************/   
    int (*chinese_str)(const c_tft18* this,u16 x,u16 y,u8 size, char* s);
	
    /************************************************* 
    * Function: pic 
    * Description: 显示图片
    * Input : <this>  tft18对象
    *         <x>     图片左上角的x坐标  
    *         <y>     图片左上角的y坐标  
    *         <length>图片宽
    *         <width> 图片高
    *         <data>  图片数据 RGB565
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 禁止超出显示区域的显示行为
    * Demo  :
    *         u16 pic[250] = {xxx};
    *
    *         tft18.pic(&tft18,0,0 ,50,50,pic);
    *         if(E_OK != ret)
    *         {
    *             log_error("Tft18 display failed.");
    *         }
    *************************************************/   
    int (*pic)(const c_tft18* this,u16 x,u16 y,u16 length,u16 width,const u16* data);
}c_tft18;

/************************************************* 
* Function: tft18_create 
* Description: 创建一个tft18对象
* Input : <spi_channal>  tft18所在的spi编号
*         <type>         tft18的屏幕类型     
*                        (TFT18_DIR_V_POSITIVE)  竖屏正
*                        (TFT18_DIR_V_REVERSE )  竖屏反
*                        (TFT18_DIR_H_POSITIVE)  横屏正
*                        (TFT18_DIR_H_REVERSE )  横屏反
*         <re_gpio>      复位脚GPIO分组
*         <re_pin>       复位脚PIN
*         <dc_gpio>      数据脚GPIO分组
*         <dc_pin>       数据脚PIN
*         <cs_gpio>      片选脚GPIO分组
*         <cs_pin>       片选脚PIN
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_tft18 tft18 = {0};
*         
*	      tft18 = tft18_create(MY_SPI_1,TFT18_DIR_V_REVERSE,GPIOA,GPIO_PIN_1,GPIOA,GPIO_PIN_2,GPIOA,GPIO_PIN_3);
*         if(NULL == tft18.this)
*         {
*             log_error("tft18 creat failed."); 
*         }
*************************************************/
c_tft18 tft18_create(u8 spi_channal,u8 type,
                           GPIO_TypeDef* re_gpio,uint32_t re_pin,GPIO_TypeDef* dc_gpio,uint32_t dc_pin,
                           GPIO_TypeDef* cs_gpio,uint32_t cs_pin);


#endif

