#ifndef _WS2812_H
#define _WS2812_H



typedef struct __WS2812 c_ws2812;

typedef struct __WS2812
{
	void*      this;
	
	/************************************************* 
	* Function: write_24Bits 
	* Description: 发送24位数据,连续发送多条将点亮多个灯
	* Input : <this>  ws2812对象
	* 				green   8位绿色数据
	* 				red   	8位红色数据
	* 				blue  	8位蓝色数据
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.write_24Bits(&ws2812,0x01,0x01,0x01);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*write_24Bits)(const c_ws2812* this,uint8_t green,uint8_t red,uint8_t blue);
	
	/************************************************* 
	* Function: reset 
	* Description: 重置数据队列
	* Input : <this>  ws2812对象
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.reset(&ws2812);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*reset)(const c_ws2812* this);
	
	/************************************************* 
	* Function: clear 
	* Description: 清空所有灯
	* Input : <this>  ws2812对象
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.clear(&ws2812);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*clear)(const c_ws2812* this);
	
	/************************************************* 
	* Function: play_num 
	* Description: 显示数字,只显示十位、个位、小数点和十分位
	* Input : <this>  ws2812对象
	*					x				x轴位移
	*					y				y轴位移
	* 				green   8位绿色数据
	* 				red   	8位红色数据
	* 				blue  	8位蓝色数据
	*					num     需要显示的数字
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.play_num(&ws2812,1,1,0x01,0x01,0x01,26.7);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*play_num)(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue,float num);
	
	/************************************************* 
	* Function: play_logo 
	* Description: 显示logo
	* Input : <this>  ws2812对象
	*					x				x轴位移
	*					y				y轴位移
	* 				green   8位绿色数据
	* 				red   	8位红色数据
	* 				blue  	8位蓝色数据
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.play_logo(&ws2812,1,1,0x01,0x01,0x01);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*play_logo)(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue);
	
	/************************************************* 
	* Function: play_drop 
	* Description: 显示水滴
	* Input : <this>  ws2812对象
	*					x				x轴位移
	*					y				y轴位移
	* 				green   8位绿色数据
	* 				red   	8位红色数据
	* 				blue  	8位蓝色数据
	* Output: 
	* Return: <E_OK>     操作成功
	*         <E_NULL>   空指针
	*         <E_ERROR>  操作失败
	* Others: 无
	* Demo  :
	*         ret=ws2812.play_drop(&ws2812,1,1,0x01,0x01,0x01);
	*         if(E_OK != ret)
	*         {
	*             log_error("ws2812 write failed.");
	*         }
	*************************************************/ 
	int (*play_drop)(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue);
}c_ws2812;

/************************************************* 
* Function: ws2812_create 
* Description: 创建ws2812对象
* Input : gpio  				数据输入所在的gpio
*					pin						引脚
*					light_num			灯珠数量
* Output: 
* Return: <E_OK>     操作成功
*         <E_NULL>   空指针
*         <E_ERROR>  操作失败
* Others: 无
* Demo  :
*         ret=ws2812.play_drop(&ws2812,1,1,0x01,0x01,0x01);
*         if(E_OK != ret)
*         {
*             log_error("ws2812 write failed.");
*         }
*************************************************/ 
c_ws2812 ws2812_create(GPIO_TypeDef* gpio,uint32_t pin,int light_num);

#endif