#ifndef __OLED_H
#define __OLED_H
		  

#if 1
#define OLED_MODE 	0
#define SIZE 		8
#define XLevelL		0x00
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xFF 
#define X_WIDTH 	128
#define Y_WIDTH 	64	    						  
//-----------------OLED IIC端口定义----------------  					   

#define OLED_SCLK_Clr() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); //GPIO_ResetBits(GPIOA,GPIO_Pin_5)//SCL
#define OLED_SCLK_Set() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); 	//GPIO_SetBits(GPIOA,GPIO_Pin_5)
 
#define OLED_SDIN_Clr() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);// GPIO_ResetBits(GPIOA,GPIO_Pin_7)//SDA
#define OLED_SDIN_Set() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); //GPIO_SetBits(GPIOA,GPIO_Pin_7)

 		     
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据


//OLED控制用函数
void OLED_WR_Byte(unsigned dat,unsigned cmd);  
void OLED_Display_On(void);
void OLED_Display_Off(void);	   							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
void OLED_ShowString(u8 x,u8 y, u8 *p,u8 Char_Size);	 
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(u8 x,u8 y,u8 no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
void Delay_50ms(unsigned int Del_50ms);
void Delay_1ms(unsigned int Del_1ms);
void fill_picture(unsigned char fill_Data);
void Picture();
void IIC_Start();
void IIC_Stop();
void Write_IIC_Command(unsigned char IIC_Command);
void Write_IIC_Data(unsigned char IIC_Data);
void Write_IIC_Byte(unsigned char IIC_Byte);

void IIC_Wait_Ack();
#endif

#if 0					    					
#define OLED_TYPE_96  0 /*0.96寸oled*/
#define OLED_TYPE_91  1 /*0.91寸oled*/
 		     
typedef struct __C_OLED c_oled;

/*OLED对象*/
typedef struct __C_OLED
{
    void*   this;

    /************************************************* 
    * Function: display_clear 
    * Description: 清空屏幕
    * Input : <this>  OLED对象
    * Output: 在屏幕首次初始化时，或者需要清楚全屏时 使用
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_clear(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled clear failed.");
    *         }
    *************************************************/ 
    int (*display_clear)(const c_oled* this); 

    /************************************************* 
    * Function: display_str 
    * Description: 显示字符串
    * Input : <this>  OLED对象
    *         <x>     字符串的x坐标  0~127
    *         <y>     字符串的y坐标0~3
    *         <format>格式化字符串
    *         <...>   可变参
    * Output: 注单次输入的字符数量不可超过16个 否则会多出的字符串会覆盖本行显示
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float a = 10.0f;
    *
    *         oled.display_str(&oled,0,1,"dis2:%.2fcm ",a);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled display failed.");
    *         }
    *************************************************/ 
    int (*display_str)  (const c_oled* this,u8 x,u8 y, char* format,...);

    /************************************************* 
    * Function: display_on 
    * Description: 打开屏幕显示
    * Input : <this>  OLED对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_on(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled on failed.");
    *         }
    *************************************************/ 
    int (*display_on)   (const c_oled* this); 

    /************************************************* 
    * Function: display_off 
    * Description: 关闭屏幕显示
    * Input : <this>  OLED对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_off(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled off failed.");
    *         }
    *************************************************/ 
    int (*display_off)  (const c_oled* this); 
}c_oled;

/************************************************* 
* Function: oled_create 
* Description: 创建一个oled对象
* Input : <iic>  oled所在的iic总线
*         <type> oled屏幕类型
*                (OLED_TYPE_96)   0.96寸oled
*                (OLED_TYPE_91)   0.91寸oled
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_bh1750 bh1750 = {0};
*         
*	      bh1750 = bh1750_creat(&iic,BH1750_ADDR_H);
*        if(NULL == bh1750.this)
*        {
*            log_error("bh1750 creat failed.");
*        }
*************************************************/
c_oled oled_create(c_my_iic* iic,u8 type);

#endif

#endif  
	 



