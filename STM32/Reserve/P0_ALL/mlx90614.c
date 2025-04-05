/***************************************************************************** 
* Copyright: 2019-2020
* File name: mlx90614.c
* Description: 红外温度传感器驱动模块实现
* Author: xzh
* Version: v1.0
* Date: xxx 
*****************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_MLX90614_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "mlx90614"

#define MLX90614_IIC_SPEED  5

//-- MLX90614 Command Mode --
#define RAM        0x00 //对RAM进行操作
#define EEPROM     0x20 //对EEPROM进行操作
#define MODE       0x60 //进入命令模式
#define EXITMODE   0x61 //退出命令模式
#define READFLAG   0xf0 //读标志
#define SLEEP      0xff //进入睡眠模式
#define RD         0x01 //读操作
#define WR         0x00 //写操作

//-- MLX90614 RAM Address --
#define AMBITEMPADDR 0x03 //Ta范围 PWM输出用
#define IR1ADDR      0x04 //原始数据 IR 通道 1 
#define IR2ADDR      0x05 //原始数据 IR 通道 2 
#define ENVITEMPADDR 0x06 //环境温度 -40 ~ 125 度
#define OBJ1TEMPADDR 0x07 //目标1温度，检测到的红外温度-70.01 ~ 382.19 度
#define OBJ2TEMPADDR 0x08 //目标2温度，检测到的红外温度-70.01 ~ 382.19 度

//-- MLX90614 EEPROM Address --
#define TOBJMAXADDR 0x00 //测量温度上限设定
#define TOBJMINADDR 0x01 //测量温度下限设定
#define PWMCTRLADDR 0x02 //PWM设定
#define TARANGEADDR 0x03 //环境温度设定
#define KEADDR      0x04 //频率修正系数
#define CONFIGADDR  0x05 //配置寄存器
#define SMBUSADDR   0x0e //器件地址设定
#define RES1ADDR    0x0f //保留
#define RES2ADDR    0x19 //保留
#define ID1ADDR     0x1c //ID地址1
#define ID2ADDR     0x1d //ID地址2
#define ID3ADDR     0x1e //ID地址3
#define ID4ADDR     0x1f //ID地址4

//-- Special Define --
#define ACK_FAIL 0x00 //没有收到应答信号
#define N 5

static u8 PEC_Cal(u8* pec,u16 n); 
static int m_set_addr(const c_mlx90614* this,u8 addr);
static int m_get(const c_mlx90614* this,float* environment,float* target);
static int m_read    (const c_mlx90614* this,u8 place,u8 cmd,u8* data);
static int m_write(const c_mlx90614* this,u8 cmd,u8* data);


typedef struct _M_MLX90614
{
    const c_my_iic*  m_iic    ;   /*所在的IIC总线*/
    u8               m_addr   ;   /*器件地址*/
}m_mlx90614;


c_mlx90614 mlx90614_creat(const c_my_iic* iic,u8 addr)
{
    c_mlx90614 new = {0};
    m_mlx90614* m_this  = NULL;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_mlx90614));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_mlx90614));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic   ;
    m_this->m_addr  = addr << 1;
    new.set_addr    = m_set_addr;
    new.get         = m_get ;
    
    /*返回新建的对象*/
    return new;
}

static int m_set_addr(const c_mlx90614* this,u8 addr)
{
    int ret = 0;
    u16 data = 0x0000;
    m_mlx90614* m_this = NULL;

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*先清空要写入的EEPROM区 否则写不进去*/
    data = 0x0000;
    ret = m_write(this,SMBUSADDR,(u8*)&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    vTaskDelay(15);  /*等待EEPROM 从页上搬运 测试过15ms是比较安全的 过小容易出现乱码*/
    
    /*向EEPROM写入新的数据*/
    data = addr;
    ret = m_write(this,SMBUSADDR,(u8*)&data);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    vTaskDelay(15);  /*等待EEPROM 从页上搬运 测试过15ms是比较安全的 过小容易出现乱码*/
    
    /*将写入的值再读取出来*/
    ret = m_read(this,EEPROM,SMBUSADDR,(u8*)&data);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }
    
    /*对比下 看看写入是否真实有效*/
    if(data != addr)
    {
        log_error("Addr set failed.");
        return E_ERROR;
    }
    
    log_inform("Addr set success,take effect after restart.");

    return E_OK;
}

static int m_get(const c_mlx90614* this,float* environment,float* target)
{
    int ret = 0;
    u8 env[3] = {0};
    u8 tar[3] = {0};

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == environment || NULL == target)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*获取环境温度*/
    ret = m_read(this,RAM,ENVITEMPADDR,env);
    if(E_OK != ret)
    {
        log_error("Enviroment temp get failed.");
        goto error_handle;
    }
    *environment = (float)(((u16)env[1] << 8) + (u16)env[0]) * 0.02f - 273.15f;

    /*获取目标温度*/
    ret = m_read(this,RAM,OBJ1TEMPADDR,tar);
    if(E_OK != ret)
    {
        log_error("Target temp get failed.");
        goto error_handle;
    }
    *target = (float)(((u16)tar[1] << 8) + (u16)tar[0]) * 0.02f - 273.15f;

    return E_OK;

    error_handle:

    *environment = *target = 0.0f;
    return E_ERROR;
}

static int m_read    (const c_mlx90614* this,u8 place,u8 cmd,u8* data)
{
    iic_cmd_handle_t  iic_cmd = NULL;
    const m_mlx90614* m_this = NULL;
    int ret = 0;
    u8 iic_addr_r  = 0;
    u8 iic_addr_w  = 0;
    u8 pec_data[6] = {0};
    u8 pec = 0;
    u8 data_buf[3] = {0};

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(RAM != place && EEPROM != place)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*创建新的命令链接*/
    iic_cmd = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写数据 写地址*/
    pec_data[5] = iic_addr_w = m_this->m_addr | I2C_MASTER_WRITE;
    ret = iic_write_bytes(iic_cmd,&iic_addr_w,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*操作位置 和 命令*/
    pec_data[4] = cmd = place | cmd;
    ret = iic_write_bytes(iic_cmd,&cmd,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*放入启动命令*/
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写数据 读地址*/
    pec_data[3] = iic_addr_r = m_this->m_addr | I2C_MASTER_READ;
    ret = iic_write_bytes(iic_cmd,&iic_addr_r,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*读数据*/
    ret = iic_read_bytes(iic_cmd,data_buf,3);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*停止命令*/
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    m_this->m_iic->clear(m_this->m_iic,iic_addr_w,I2C_ACK_WAIT_DEFAULT_TIME,MLX90614_IIC_SPEED);
    ret = m_this->m_iic->begin(m_this->m_iic,iic_cmd,MLX90614_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    /*PEC计算*/
    pec_data[2] = data_buf[0];
    pec_data[1] = data_buf[1];
    pec_data[0] = 0;
    pec = PEC_Cal(pec_data,6);

    if(pec != data_buf[2])
    {
        data[0] = 0;
        data[1] = 0;
        log_error("Pec error.");
        return E_ERROR;
    }

    data[0] = data_buf[0];
    data[1] = data_buf[1];

    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

static int m_write(const c_mlx90614* this,u8 cmd,u8* data)
{
    iic_cmd_handle_t  iic_cmd = NULL;
    const m_mlx90614* m_this = NULL;
    int ret = 0;
    u8 iic_addr = 0;
    u8 pec_data[6] = {0};
    u8 pec = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写数据*/
    pec_data[5] = 0;
    pec_data[4] = iic_addr = m_this->m_addr | I2C_MASTER_WRITE;
    ret = iic_write_bytes(iic_cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*目标寄存器地址*/
    pec_data[3] = cmd = EEPROM|cmd;
    ret = iic_write_bytes(iic_cmd,&cmd,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写数据*/
    pec_data[2] = data[0];
    pec_data[1] = data[1];
    pec_data[0] = 0;
    ret = iic_write_bytes(iic_cmd,data,2);      
    if(E_OK != ret)                               
    {                                             
        log_error("Add iic write byte failed.");  
        goto error_handle;
    }

    /*写pec*/
    pec = PEC_Cal(pec_data,6);
    ret = iic_write_bytes(iic_cmd,&pec,1);      
    if(E_OK != ret)                               
    {                                             
        log_error("Add iic write byte failed.");  
        goto error_handle;
    }

    /*停止命令*/
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    m_this->m_iic->clear(m_this->m_iic,iic_addr,I2C_ACK_WAIT_DEFAULT_TIME,MLX90614_IIC_SPEED);
    ret = m_this->m_iic->begin(m_this->m_iic,iic_cmd,MLX90614_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

/**
* @功能 计算PEC包裹校验码,根据接收的字节计算PEC码
* @说明 计算传入数据的PEC码$MLX90614.C
* @参数 pec[]：传入的数据
n：传入数据个数
* @返回值 pec[0]：计算得到的PEC值
*/
uint8_t PEC_Cal(uint8_t pec[],uint16_t n)
{
    unsigned char crc[6];
    unsigned char Bitposition=47;
    unsigned char shift;
    unsigned char i;
    unsigned char j;
    unsigned char temp;
    
    do
    {
        crc[5]=0; //载入 CRC数值 0x000000000107
        crc[4]=0;
        crc[3]=0;
        crc[2]=0;
        crc[1]=0x01;
        crc[0]=0x07;
        Bitposition=47; //设置Bitposition的最大值为47
        shift=0;
        //在传送的字节中找出第一个“1”
        
        i=5; //设置最高标志位 (包裹字节标志)
        j=0; //字节位标志，从最低位开始
        while((pec[i]&(0x80>>j))==0 && (i>0))
        {
            Bitposition--;
            if(j<7)
            {
                j++;
            }
            else
            {
                j=0x00;
                i--;
            }
        }//while语句结束，并找出Bitposition中为“1”的最高位位置
        shift=Bitposition-8;
        //得到CRC数值将要左移/右移的数值“shift”
        //对CRC数据左移“shift”位
        while(shift)
        {
            for(i=5;i<0xFF;i--)
            {
                if((crc[i-1]&0x80) && (i>0))
                //核对字节的最高位的下一位是否为"1"
                { //是 - 当前字节 + 1
                    temp=1; //否 - 当前字节 + 0
                } //实现字节之间移动“1”
                else
                {
                    temp=0;
                }
                crc[i]<<=1;
                crc[i]+=temp;
            }
            shift--;
        }
        //pec和crc之间进行异或计算
        for(i=0;i<=5;i++)
        {
            pec[i]^=crc[i];
        }
    }while(Bitposition>8);
    
    return pec[0]; //返回计算所得的crc数值
}
#endif