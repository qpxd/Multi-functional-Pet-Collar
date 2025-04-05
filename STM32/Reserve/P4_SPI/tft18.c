/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : tft18.c
* Description: tft18屏幕 函数实现
* Author: xzh
* Version: v1.1
* Date: 2022/02/06
* log:  V1.0  2022/02/06
*       发布：
*
*       V1.1  2022/03/21
*       新增：rectangle 矩形绘制接口。
*****************************************************************************/

#include "driver_conf.h"
#include "tft18_font.h"

#ifdef DRIVER_TFT18_ENABLED

#define MODULE_NAME       "tft18"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define PIX_W  128   /*横向像素点数量*/
#define PIX_H  160   /*纵向像素点数量*/

#define DATA_DAT   0 /*数据类型*/
#define DATA_CMD   1 /*命令类型*/

typedef struct __M_TFT18
{
    c_switch      m_re          ;  /*复位脚*/
    c_switch      m_dc          ;  /*类型脚*/
    c_switch      m_cs          ;  /*片选脚*/
    u16           m_brush       ;  /*画笔颜色*/
    u16           m_background  ;  /*背景颜色*/
    u8            m_type        ;  /*横竖屏类型*/
    u8            m_spi_channal ;  /*spi通道*/
    u8            m_height      ;  /*屏幕高*/
    u8            m_width       ;  /*屏幕宽*/
    u16           m_buf[512]    ;  /*显示缓存*/
}m_tft18;

static int m_set(const c_tft18* this,u16 brush,u16 back);
static int m_clear(const c_tft18* this);
static int m_str(const c_tft18* this,u16 x,u16 y,u8 size, char* format,...);
static int m_char(const c_tft18* this,u16 x,u16 y,char cha,u8 size);
static int m_pic(const c_tft18* this,u16 x,u16 y,u16 length,u16 width,const u16* data);
static int m_init_cmd(const c_tft18* this);
static int m_set_address(const c_tft18* this,u16 x1,u16 x2,u16 y1,u16 y2);
static int m_send(const c_tft18* this,u8 type,u8 datasize,bool incremental,const u8* data,u16 len);
static int m_rectangle(const c_tft18* this,u16 x,u16 y,u16 weight,u16 height);
static int m_chinese_str(const c_tft18* this,u16 x,u16 y,u8 size, char* s);
static int m_chinese_char(const c_tft18* this,u16 x,u16 y,u8 size, char* s);
c_tft18 tft18_create(u8 spi_channal,u8 type,
                           GPIO_TypeDef* re_gpio,uint32_t re_pin,GPIO_TypeDef* dc_gpio,uint32_t dc_pin,
                           GPIO_TypeDef* cs_gpio,uint32_t cs_pin)
{
    int ret = 0;
    c_tft18  new = {0};
    m_tft18* m_this = NULL;  

    /*参数检查*/
    if(TFT18_DIR_V_POSITIVE != type && TFT18_DIR_V_REVERSE != type && TFT18_DIR_H_POSITIVE != type && TFT18_DIR_H_REVERSE != type)
    {
        log_error("Param error.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_tft18));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_tft18));
    m_this = new.this;

    /*初始化相应的SPI*/
    ret = my_spi.init(spi_channal);
    if(E_OK != ret)
    {
        log_error("Spi init failed.");
        goto error_handle;
    }

    /*引脚配置*/
    m_this->m_re = switch_create(re_gpio,re_pin);
    if(NULL == m_this->m_re.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_dc = switch_create(dc_gpio,dc_pin);
    if(NULL == m_this->m_dc.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_cs = switch_create(cs_gpio,cs_pin);
    if(NULL == m_this->m_cs.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }

    /*保存相关配置*/
    if(TFT18_DIR_V_POSITIVE == type || TFT18_DIR_V_REVERSE == type)
    {
        m_this->m_height = PIX_H;
        m_this->m_width  = PIX_W;
    }
    else 
    {
        m_this->m_height = PIX_W;
        m_this->m_width  = PIX_H;
    }
    m_this->m_spi_channal = spi_channal;
    m_this->m_type        = type       ;
    m_this->m_brush       = TFT18_BRUSH_DEFAULT_COLOR;
    m_this->m_background  = TFT18_BACK_DEFAULT_COLOR;
    new.clear             = m_clear    ;
    new.set               = m_set      ;
    new.str               = m_str      ;
    new.pic               = m_pic      ;
    new.rectangle         = m_rectangle;
		new.chinese_str				= m_chinese_str;
    /*发送初始化命令*/
    ret = m_init_cmd(&new);
    if(E_OK != ret)
    {
        log_error("Tft18 init cmd send failed.");
        goto error_handle;
    }

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_clear(const c_tft18* this)
{
    m_tft18* m_this = NULL;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*设置范围*/
    ret = m_set_address(this,0,m_this->m_width - 1,0,m_this->m_height - 1);
    if(E_OK != ret)
    {
        log_error("Address set failed.");
        return E_ERROR;
    }

    /*启动传输*/
    ret = m_send(this,DATA_DAT,16,false,(u8*)&m_this->m_background,m_this->m_width * m_this->m_height);
    if(E_OK != ret)
    {
        log_error("Send data failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_rectangle(const c_tft18* this,u16 x,u16 y,u16 weight,u16 height)
{
    m_tft18* m_this = NULL;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*设置范围*/
    ret = m_set_address(this,x,x + weight - 1,y,y + height - 1);
    if(E_OK != ret)
    {
        log_error("Address set failed.");
        return E_ERROR;
    }

    /*启动传输*/
    ret = m_send(this,DATA_DAT,16,false,(u8*)&m_this->m_background,weight * height);
    if(E_OK != ret)
    {
        log_error("Send data failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_str(const c_tft18* this,u16 x,u16 y,u8 size, char* format,...)
{
    u8 str_index = 0;
    const m_tft18*  m_this = NULL;
    va_list   argptr;
    char str_buf[25] = {0};
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*检查坐标*/
    if(x >= m_this->m_width || y >= m_this->m_height)
    {
        log_error("Param error");
        return E_ERROR;
    }
    
    va_start( argptr, format );  // 初始化argptr	
	memset(str_buf,0,25);
	vsnprintf(str_buf,25,format,argptr);
	va_end(argptr);
    
    /*循环输出字符*/
    for(str_index = 0;'\0' != str_buf[str_index];++str_index)
    {
        if(x + (str_index + 1) * size / 2 >= m_this->m_width)
        {
            log_warning("String is too long ");  /*超长直接舍弃*/
            return E_OK;
        }

        /*输出字符串*/
        ret = m_char(this,x + str_index * size / 2,y,str_buf[str_index],size);
        if(E_OK != ret)
        {
            log_error("Print char failed.");
            return E_ERROR;
        }
    }
    
    return E_OK;
}
static int m_chinese_str(const c_tft18* this,u16 x,u16 y,u8 size, char* s)
{
	const m_tft18*  m_this = NULL;
	int ret = 0;
	u8 cn_index = 0;
	/*检查参数*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	/*检查坐标*/
	if(x >= m_this->m_width || y >= m_this->m_height)
	{
			log_error("Param error");
			return E_ERROR;
	}
	for(cn_index=0;s[cn_index]!='\0';cn_index+=2)
	{
		if(x + (cn_index + 1) * size / 2 >= m_this->m_width)
		{
				log_warning("String is too long ");  /*超长直接舍弃*/
				return E_OK;
		}

		/*输出字符串*/
		ret = m_chinese_char(this,x + cn_index * size/2,y,size,s+cn_index);
		if(E_OK != ret)
		{
				log_error("Print char failed.");
				return E_ERROR;
		}
	}
}
static int m_chinese_char(const c_tft18* this,u16 x,u16 y,u8 size, char* s)
{
	m_tft18*  m_this = NULL;
	int ret = 0;
	
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x,data_count=0;
	TypefaceNum=(size/8+((size%8)?1:0))*size;
	
	/*检查参数*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	/*检查坐标*/
	if(x >= m_this->m_width || y >= m_this->m_height)
	{
			log_error("Param error");
			return E_ERROR;
	}
	
  /*设置光标位置*/
	ret = m_set_address(this , x , x + size - 1, y ,y + size - 1);
	if(E_OK != ret)
	{
			log_error("Addr set failed.");
			return E_ERROR;
	} 
	/*清空显存*/
  memset(m_this->m_buf,0,TypefaceNum * 8 * 2);
	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{ 	

			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(tfont12[k].Msk[i]&(0x01<<j)) 
						m_this->m_buf[data_count] = m_this->m_brush;
					else 
							m_this->m_buf[data_count] = m_this->m_background;
					m++;
					data_count++;
					if(m%size==0)
					{
						m=0;
						break;
					}
				}
			}
		}				  	
	}
	/*通过DMA 发送数据*/
	ret = m_send(this,DATA_DAT,16,true,(u8*)m_this->m_buf,data_count);
	if(E_OK != ret)
	{
			log_error("Send failed.");
			return E_ERROR;
	}
}
static int m_set(const c_tft18* this,u16 brush,u16 back)
{
    m_tft18*  m_this = NULL;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*设置颜色*/
    m_this->m_brush      = brush;
    m_this->m_background = back;

    return E_OK;
}


void delay(int t)
{
    while(t--);
}

static int m_send(const c_tft18* this,u8 type,u8 datasize,bool incremental,const u8* data,u16 len)
{
    m_tft18* m_this = NULL;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(DATA_CMD != type && DATA_DAT != type)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*设置数据位宽*/
    ret = my_spi.set_datasize(m_this->m_spi_channal,datasize);
    if(E_OK != ret)
    {
        log_error("Datasize set failed.");
        return E_ERROR;
    }

    /*数据类型设置电平*/
    if(DATA_DAT == type)
    {
        /*写数据*/
        ret = m_this->m_dc.set(&m_this->m_dc,GPIO_PIN_SET);
    }
    else
    {
        /*写命令*/
        ret = m_this->m_dc.set(&m_this->m_dc,GPIO_PIN_RESET);
    }
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }

    /*选中片选*/
    ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }

    /*启动传输*/
    ret = my_spi.transmission(m_this->m_spi_channal,data,NULL,len,incremental,3000);
    if(E_OK != ret)
    {
        log_error("Spi transmission failed.");
        return E_ERROR;
    }

    delay(1);
    
    /*复位片选*/
    ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_char(const c_tft18* this,u16 x,u16 y,char cha,u8 size)
{
    m_tft18* m_this = NULL;
    u8 temp,sizex,t,m=0 ;
    u16 i,TypefaceNum ;//一个字符所占字节大小
    u16 x0 = x,data_count = 0;
    int ret = 0;
    sizex = size / 2;
    TypefaceNum=(sizex / 8 + ((sizex % 8) ? 1 : 0)) * size;
    cha = cha -' ';    //得到偏移后的值

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(TFT18_FONT_12 != size && TFT18_FONT_16 != size && TFT18_FONT_24 != size && TFT18_FONT_32 != size)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*设置光标位置*/
    ret = m_set_address(this , x , x + sizex - 1, y ,y + size - 1);
    if(E_OK != ret)
    {
        log_error("Addr set failed.");
        return E_ERROR;
    }

    /*清空显存*/
    memset(m_this->m_buf,0,TypefaceNum * 8 * 2);
    
    for(i = 0 ;i < TypefaceNum;i++)
    { 
        if(size==12)
            temp = ascii_1206[cha][i];         //调用6x12字体
        else if(size==16)
            temp = ascii_1608[cha][i];         //调用8x16字体
//        else if(size==24)
//            temp = ascii_2412[cha][i];         //调用12x24字体
//        else 
//            temp = ascii_3216[cha][i];         //调用16x32字体

        for(t=0;t<8;t++)
        {
            /*数据保存到缓存*/
            if(temp&(0x01<<t))
                m_this->m_buf[data_count] = m_this->m_brush;
            else 
                m_this->m_buf[data_count] = m_this->m_background;
            m++;
            data_count++;
            if(m % sizex == 0)
            {
                m=0;
                break;
            }
        }
    }

    /*通过DMA 发送数据*/
    ret = m_send(this,DATA_DAT,16,true,(u8*)m_this->m_buf,data_count);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_pic(const c_tft18* this,u16 x,u16 y,u16 length,u16 width,const u16* data)
{
    m_tft18* m_this = NULL;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*设置光标位置*/
    ret = m_set_address(this , x , x + length - 1, y ,y + width - 1);
    if(E_OK != ret)
    {
        log_error("Addr set failed.");
        return E_ERROR;
    }

    /*通过DMA 发送数据*/
    ret = m_send(this,DATA_DAT,16,true,(u8*)data,length * width * 2);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    return E_OK;
}


static int m_set_address(const c_tft18* this,u16 x1,u16 x2,u16 y1,u16 y2)
{
    m_tft18* m_this = NULL;
    u8 data = 0;
    u8 send[8] = {0};
    u16 temp = 0;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    if(x1 > x2 || y1 > y2 || x2 >= m_this->m_width || y2 >= m_this->m_height)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*转换坐标*/
    if(TFT18_DIR_V_POSITIVE == m_this->m_type || TFT18_DIR_V_REVERSE == m_this->m_type)
    {
        temp = x1 + 2;
        send[0] = temp >> 8;
        send[1] = temp;
        temp = x2 + 2;
        send[2] = temp >> 8;
        send[3] = temp;
        temp = y1 + 1;
        send[4] = temp >> 8;
        send[5] = temp;
        temp = y2 + 1;
        send[6] = temp >> 8;
        send[7] = temp;
    }
    else
    {
        temp = x1 + 1;
        send[0] = temp >> 8;
        send[1] = temp;
        temp = x2 + 1;
        send[2] = temp >> 8;
        send[3] = temp;
        temp = y1 + 2;
        send[4] = temp >> 8;
        send[5] = temp;
        temp = y2 + 2;
        send[6] = temp >> 8;
        send[7] = temp;
    }
    
    /*横坐标*/
    data = 0x2A;
    ret = m_send(this,DATA_CMD,8,false,&data,1);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    ret = m_send(this,DATA_DAT,8,true,send,4);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    /*纵坐标*/
    data = 0x2B;
    ret = m_send(this,DATA_CMD,8,false,&data,1);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    ret = m_send(this,DATA_DAT,8,true,send + 4,4);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    data = 0x2C;
    ret = m_send(this,DATA_CMD,8,false,&data,1);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    
    return E_OK;
}

static int m_init_cmd(const c_tft18* this)
{   
    m_tft18* m_this = NULL;
    int ret = 0;
    u8 data = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*屏幕复位*/
    ret = m_this->m_re.set(&m_this->m_re,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }

    vTaskDelay(100);

    ret = m_this->m_re.set(&m_this->m_re,GPIO_PIN_SET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }
	vTaskDelay(500);

    data = 0x11; m_send(this,DATA_CMD,8,false,&data,1);
    vTaskDelay(100);
	data = 0x11; m_send(this,DATA_CMD,8,false,&data,1);
	vTaskDelay(100);
    
    //------------------------------------ST7735S Frame Rate-----------------------------------------// 
    data = 0xB1; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x05; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xB2; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x05; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xB3; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x05; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x05; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3C; m_send(this,DATA_DAT,8,false,&data,1);
    //------------------------------------End ST7735S Frame Rate---------------------------------// 
    data = 0xB4; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x03; m_send(this,DATA_DAT,8,false,&data,1);
    //------------------------------------ST7735S Power Sequence---------------------------------// 
    data = 0xC0; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x28; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x08; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x04; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xC1; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0xC0; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xC2; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x0D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x00; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xC3; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x8D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2A; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xC4; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x8D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xEE; m_send(this,DATA_DAT,8,false,&data,1);
    //---------------------------------End ST7735S Power Sequence-------------------------------------// 
    data = 0xC5; m_send(this,DATA_CMD,8,false,&data,1);  //VCOM 
    data = 0x1A; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x36; m_send(this,DATA_CMD,8,false,&data,1);  //MX, MY, RGB mode 
    if(m_this->m_type == TFT18_DIR_V_POSITIVE)
    {
        data = 0x00; m_send(this,DATA_DAT,8,false,&data,1);
    }
    else if(m_this->m_type == TFT18_DIR_V_REVERSE)
    {
        data = 0xC0; m_send(this,DATA_DAT,8,false,&data,1);
    }
    else if(m_this->m_type == TFT18_DIR_H_POSITIVE)
    {
        data = 0x70; m_send(this,DATA_DAT,8,false,&data,1);
    }
    else 
    {
        data = 0xA0; m_send(this,DATA_DAT,8,false,&data,1);
    }
    //------------------------------------ST7735S Gamma Sequence---------------------------------// 
    data = 0xE0; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x04; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x22; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x07; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x0A; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2E; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x30; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x25; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2A; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x28; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x26; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2E; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3A; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x00; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x01; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x03; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x13; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0xE1; m_send(this,DATA_CMD,8,false,&data,1);
    data = 0x04; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x16; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x06; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x0D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x26; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x23; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x27; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x27; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x25; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x2D; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x3B; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x00; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x01; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x04; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x13; m_send(this,DATA_DAT,8,false,&data,1);
    //------------------------------------End ST7735S Gamma Sequence-----------------------------// 
    data = 0x3A; m_send(this,DATA_CMD,8,false,&data,1);  //65k mode 
    data = 0x05; m_send(this,DATA_DAT,8,false,&data,1);
    data = 0x29; m_send(this,DATA_CMD,8,false,&data,1);  //Display on 
    
    return E_OK;
}


#endif
