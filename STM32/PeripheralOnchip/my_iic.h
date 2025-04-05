/*****************************************************************************
* Copyright:
* File name: my_iic.h
* Description: my iic公开头文件
* Author: 许少
* Version: V1.1
* Date: 2022/02/02
* log:  V1.0  2021/12/29
*       发布：
*
*       V1.2  2022/02/02
*       增加：clear接口，用于停止正在工作的器件。
* Instructions for use :
*
*第一步：使用iic_creat，创建一个 iic
*    例：c_my_iic iic = {0};
*   
*        iic = iic_creat(GPIOB,GPIO_PIN_8,GPIOB,GPIO_PIN_9);
*        if(NULL == iic.this)
*        {
*            log_error("IIC creat failed.");
*        }
*    你需要保存好已经创建的iic对象，在需要发起iic通信时，我们需要使用到它
*
*第二步：需要创建一个新的iic链接
*    例：cmd = iic_cmd_link_create();
*        if(NULL == cmd)
*        {
*            log_error("IIC cmd link create failed.");
*        }
*    你需要保存还已经创建的iic链接，在发起iic通信时，我们需要使用到它
*
*第三步：向iic链接中 添加需要执行的通信行为
*        注：添加的顺序，即为发起iic通信时所执行的顺序
*    例：添加启动命令
*        ret = iic_master_start(cmd);
*        if(E_OK != ret)
*        {
*            log_error("Add iic start failed.");
*        }
*        
*        添加写入数据命令
*        ret = iic_write_bytes(cmd,&data_addr,data_len);
*        if(E_OK != ret)
*        {
*            log_error("Add iic write bytes failed.");
*        }
*        
*        添加停止命令
*        ret = iic_master_stop(cmd);
*        if(E_OK != ret)
*        {
*            log_error("Add iic stop failed.");
*        }
*
*第四步：启动iic，该步骤 会使用到第一步创建的iic对象，已经第二步我们创建
*        好的iic链接。将iic链接传递给iic对象，其中iic链接包含要执行的iic
*        行为和顺序。
*    例：ret = iic->begin(&iic,cmd,I2C_SPEED_DEFAULT,1000);
*        if(E_OK != ret)
*        {
*            log_error("Add iic begin failed.");
*        }
*        序列返回后，代表所需要执行的iic通信已经完成，或失败
*
*第五步：删除iic链接。因为iic链接是被动态创建的，所以使用完成后，一定要释
*        放它
*    例：ret = iic_cmd_link_delete(cmd);
*        if(E_OK != ret)
*        {
*            log_error("Delete iic cmd link failed.");
*        }
*****************************************************************************/

#ifndef __MY_IIC_H__
#define __MY_IIC_H__

#define IIC_MAX_DATA_LEN  256  /*iic最大 单次发送/接收字节数*/
#define I2C_MASTER_WRITE    0  /*写*/
#define I2C_MASTER_READ     1  /*读*/

#define I2C_SPEED_DEFAULT              2
#define I2C_ACK_WAIT_DEFAULT_TIME     80     /*等待ack的最大时间*/

typedef struct __MY_IIC           c_my_iic    ;    /*iic对象*/
typedef            void*     iic_cmd_handle_t ;    /*iic命令句柄*/ 

/*iic对象定义*/
typedef struct __MY_IIC
{
	void* this ;/*this 指针 不允许进行任何操作*/

    /************************************************* 
    * Function: begin 
    * Description: 启动一次iic通信
    * Input : <this>         iic 对象
    *         <cmd_object>   iic链接句柄
    *         <speed>        iic速度   实指iic电平变化的时间间隔 单位us
    *         <TickType_t>   阻塞时间，单位ms。如果为0，则立即返回，
    *                        当为portMAX_DELAY的话就一直等，知道iic
    *                        空闲，并完成数据收发。
    * Output: 无
    * Return: <E_OK>     通信成功
    *         <E_NULL>   传递了空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 为了保证通信的有效性，本函数的执行会进入阻塞态
    * Demo  : 最多等待1s，如果1s后iic任然不空闲，将返回执行失败
    *         ret = iic->begin(&iic,cmd,I2C_SPEED_DEFAULT,1000);
    *         if(E_OK != ret)
    *         {
    *             log_error("Add iic begin failed.");
    *         }
    *************************************************/
	int (*begin)(const c_my_iic* this,iic_cmd_handle_t cmd_object,u8 speed,TickType_t time);

/************************************************* 
    * Function: clear 
    * Description: 清除指定地址器件的工作状态
    * Input : <this>         iic 对象
    *         <addr>         iic器件地址
    *         <ack_wait_time>等待ack的时长
    *         <speed>        iic速度   实指iic电平变化的时间间隔 单位us
    * Output: 无
    * Return: <E_OK>     通信成功
    *         <E_NULL>   传递了空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 在器件较多的iic总线上，如果器件会出现间歇性无响应，建议在begin前使用
    *         一次clear来清除器件的状态
    * Demo  : 
    *         ret = iic->clear(m_this->m_iic,0x55);
    *         if(E_OK != ret)
    *         {
    *             log_error("iic clear failed.");
    *         }
    *************************************************/
    int (*clear)(const c_my_iic* this,u8 addr,u16 ack_wait_time,u8 speed);
}c_my_iic;

/************************************************* 
* Function: iic_creat 
* Description: 创建一个iic对象
* Input : <sda_gpio>  iic sda脚  gpio分组
*         <sda_pin>   iic sda脚   pin 值
*         <scl_gpio>  iic scl脚   gpio分组
*         <scl_pin>   iic scl脚  pin 值  
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_my_iic iic = {0};
*	      iic = iic_creat(GPIOB,GPIO_PIN_7,GPIOB,GPIO_PIN_6);
*	      if(NULL == iic.this)
*	      {
*	      	 log_error("iic creat failed.");
*	      }
*************************************************/
c_my_iic iic_creat(GPIO_TypeDef* sda_gpio,uint32_t sda_pin,GPIO_TypeDef* scl_gpio,uint32_t scl_pin);

/************************************************* 
* Function: iic_cmd_link_create 
* Description: 创建一个iic命令链接
* Input : 无
* Output: 无
* Return: 返回命令链接句柄 如果为NULL 表示创建失败
* Others: 无
* Demo  :
*         iic_cmd_handle_t = NULL;
*         
*	      iic_cmd_handle_t = iic_cmd_link_create();
*	      if(NULL == iic_cmd_handle_t)
*	      {
*	      	 log_error("iic cmd handle creat failed.");haod
*	      }
*************************************************/
iic_cmd_handle_t iic_cmd_link_create(void);

/************************************************* 
* Function: iic_cmd_link_delete 
* Description: 删除一个iic命令连接
* Input : <object> 要删除的iic链接句柄
* Output: 无
* Return: <E_OK>     删除成功
*         <E_ERROR>  删除失败
* Others: 无
* Demo  :
*         ret = iic_cmd_link_delete(obj);
*	      if(E_OK != obj)
*	      {
*	      	 log_error("iic cmd handle delete failed.");
*	      }
*************************************************/
int iic_cmd_link_delete(iic_cmd_handle_t object);

/************************************************* 
* Function: iic_master_start 
* Description: 向IIC链接中添加一个启动信号
* Input : <object> 要添加的iic链接句柄
* Output: 无
* Return: <E_OK>     添加成功
*         <E_ERROR>  添加失败
* Others: 无
* Demo  :
*         ret = iic_master_start(obj);
*	      if(E_OK != obj)
*	      {
*	      	 log_error("Failed to add start signal.");
*	      }
*************************************************/
int iic_master_start(iic_cmd_handle_t object);

/************************************************* 
* Function: iic_master_stop 
* Description: 向IIC链接中添加一个停止信号
* Input : <object> 要添加的iic链接句柄
* Output: 无
* Return: <E_OK>     添加成功
*         <E_ERROR>  添加失败
* Others: 无
* Demo  :
*         ret = iic_master_stop(obj);
*	      if(E_OK != obj)
*	      {
*	      	 log_error("Failed to add stop signal.");
*	      }
*************************************************/
int iic_master_stop(iic_cmd_handle_t object);

/************************************************* 
* Function: iic_write_bytes 
* Description: 向IIC链接中添加一个发送多字节命令
* Input : <object>   要添加的iic链接句柄
*         <data>     要发送的数据地址  注：内部为引用，既在链接使用/释放之前，
*                    地址所指向的内存不可释放，否则将引发错误
*         <data_len> 数据长度  注：不可为0  不可超过IIC_MAX_DATA_LEN
* Output: 无
* Return: <E_OK>     添加成功
*         <E_ERROR>  添加失败
* Others: 无
* Demo  :
*         ret = iic_write_bytes(obj,data,data_len);
*	      if(E_OK != obj)
*	      {
*	      	 log_error("Failed to add send bytes signal.");
*	      }
*************************************************/
int iic_write_bytes(iic_cmd_handle_t object,const u8* data,u16 data_len);

/************************************************* 
* Function: iic_Read_bytes 
* Description: 向IIC链接中添加一个读取多字节命令
* Input : <object>   要添加的iic链接句柄
*         <data>     用于存放数据的地址  注：内部为引用，既在链接使用/释放之前，
*                    地址所指向的内存不可释放，否则将引发错误
*         <data_len> 数据长度  注：不可为0  不可超过IIC_MAX_DATA_LEN
* Output: 无
* Return: <E_OK>     添加成功
*         <E_ERROR>  添加失败
* Others: 无
* Demo  :
*         ret = iic_Read_bytes(obj,data,data_len);
*	      if(E_OK != obj)
*	      {
*	      	 log_error("Failed to add recv bytes signal.");
*	      }
*************************************************/
int iic_read_bytes(iic_cmd_handle_t object,u8* data,u16 data_len);



#endif



