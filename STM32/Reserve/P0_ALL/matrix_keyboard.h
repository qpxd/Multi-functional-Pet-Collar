/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : matrix_keyboard.h
* Description: matrix_keyboard 
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#ifndef __MATRIX_KEYBOARD_H__
#define __MATRIX_KEYBOARD_H__

/*按键列表*/
typedef struct __MATRIX_IO
{
    GPIO_TypeDef* gpio    ;    /*按键所在GPIO分组*/
    uint32_t      pin     ;    /*按键所在PIN脚*/
}matrix_io_t;

typedef struct __C_MATRIX c_matrix;

typedef struct __C_MATRIX
{
    void* this;
	
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
//	int matrix_key_board_call(uint8_t row,u8 col)
//	{
//		log_inform("KEY:%d,%d",row,col);
//		return E_OK;
//	}
    
	ret = matrix.set_callback(&matrix,matrix_key_board_call);
	if(E_OK != ret)	log_error("matrix Callback set failed.");
    *************************************************/  
    int (*set_callback)(c_matrix* this,int (u8 row,u8 col));
}c_matrix;

	
/************************************************* 
* Function: matrix_keyboard_create 
* Description: 创建一个矩阵键盘对象
* Input : <this>      键盘对象
*         <callback>  回调函数
* Output: <无> 
* Return: <E_OK>     获取成功
*         <E_ERROR>  操作失败
* Others: 无
* Demo  :
	matrix_io_t	io_list[8] = {
		{GPIOB, GPIO_PIN_5},
		{GPIOB, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_1},
		{GPIOA, GPIO_PIN_7},
		
		{GPIOA, GPIO_PIN_6},
		{GPIOA, GPIO_PIN_1},
		{GPIOA, GPIO_PIN_3},
		{GPIOA, GPIO_PIN_2},
	};		
	c_matrix matrix = matrix_keyboard_create(io_list);
	if(NULL == matrix.this)	log_error("matrix keyboard create failed.");

*************************************************/  
c_matrix matrix_keyboard_create(matrix_io_t *io_list);

#endif

