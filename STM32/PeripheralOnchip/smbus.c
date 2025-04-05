
#include "onchip_conf.h"
#ifdef ONCHIP_SMBUS_ENABLED

//IO方向设置
#define SDA_IN()  be_in(GPIOB,GPIO_PIN_8)
#define SDA_OUT() be_out(GPIOB,GPIO_PIN_8)
//IO操作
#define SMBUS_SCL   PBout(9) //SCL
#define SMBUS_SDA   PBout(8) //SDA
#define READ_SDA  PBin(8)  //输入SDA

#define SMBUS_SDA_H()   SMBUS_SDA = 1
#define SMBUS_SDA_L()   SMBUS_SDA = 0
#define SMBUS_SCK_H()   SMBUS_SCL = 1
#define SMBUS_SCK_L()   SMBUS_SCL = 0
#define SMBUS_SDA_PIN() READ_SDA

void SMBus_SendBit(u8 bit_out);
u8 SMBus_ReceiveBit(void);

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : SMBus_Delay
* Description    : 延时  一次循环约1us
* Input          : time
* Output         : None
* Return         : None
*******************************************************************************/
void SMBus_Delay(u16 time)
{
    u16 i, j;
    for (i=0; i<12; i++)
    {
        for (j=0; j<time; j++);
    }
}

/*******************************************************************************
* Function Name  : SMBus_StartBit
* Description    : Generate START condition on SMBus
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SMBus_StartBit(void)
{
    SDA_OUT();     //sda线输出
    SMBUS_SDA_H();		// Set SDA line
    SMBus_Delay(5);	    // Wait a few microseconds
    SMBUS_SCK_H();		// Set SCL line
    SMBus_Delay(5);	    // Generate bus free time between Stop
    SMBUS_SDA_L();		// Clear SDA line
    SMBus_Delay(5);	    // Hold time after (Repeated) Start
    // Condition. After this period, the first clock is generated.
    //(Thd:sta=4.0us min)在SCK=1时，检测到SDA由1到0表示通信开始（下降沿）
    SMBUS_SCK_L();	    // Clear SCL line
    SMBus_Delay(5);	    // Wait a few microseconds
}

/*******************************************************************************
* Function Name  : SMBus_StopBit
* Description    : Generate STOP condition on SMBus
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SMBus_StopBit(void)
{
    SDA_OUT();     //sda线输出
    SMBUS_SCK_L();		// Clear SCL line
    SMBus_Delay(5);	    // Wait a few microseconds
    SMBUS_SDA_L();		// Clear SDA line
    SMBus_Delay(5);	    // Wait a few microseconds
    SMBUS_SCK_H();		// Set SCL line
    SMBus_Delay(5);	    // Stop condition setup time(Tsu:sto=4.0us min)
    SMBUS_SDA_H();		// Set SDA line在SCK=1时，检测到SDA由0到1表示通信结束（上升沿）
}

/*******************************************************************************
* Function Name  : SMBus_SendByte
* Description    : Send a byte on SMBus
* Input          : Tx_buffer
* Output         : None
* Return         : None
*******************************************************************************/
u8 SMBus_SendByte(u8 Tx_buffer)
{
    u8	Bit_counter;
    u8	Ack_bit;
    u8	bit_out;

    SDA_OUT();     //sda线输出
    for(Bit_counter=8; Bit_counter; Bit_counter--)
    {
        if (Tx_buffer&0x80)
        {
            bit_out=1;   // If the current bit of Tx_buffer is 1 set bit_out
        }
        else
        {
            bit_out=0;  // else clear bit_out
        }
        SMBus_SendBit(bit_out);		// Send the current bit on SDA
        Tx_buffer<<=1;				// Get next bit for checking
    }

    SDA_IN();      //SDA设置为输入
    Ack_bit=SMBus_ReceiveBit();		// Get acknowledgment bit
    return	Ack_bit;
}

/*******************************************************************************
* Function Name  : SMBus_SendBit
* Description    : Send a bit on SMBus 82.5kHz
* Input          : bit_out
* Output         : None
* Return         : None
*******************************************************************************/
void SMBus_SendBit(u8 bit_out)
{
    if(bit_out==0)
    {
        SMBUS_SDA_L();
    }
    else
    {
        SMBUS_SDA_H();
    }
    SMBus_Delay(2);					// Tsu:dat = 250ns minimum
    SMBUS_SCK_H();					// Set SCL line
    SMBus_Delay(6);					// High Level of Clock Pulse
    SMBUS_SCK_L();					// Clear SCL line
    SMBus_Delay(3);					// Low Level of Clock Pulse
//	SMBUS_SDA_H();				    // Master release SDA line ,
    return;
}

/*******************************************************************************
* Function Name  : SMBus_ReceiveBit
* Description    : Receive a bit on SMBus
* Input          : None
* Output         : None
* Return         : Ack_bit
*******************************************************************************/
u8 SMBus_ReceiveBit(void)
{
    u8 Ack_bit;

    SMBUS_SDA_H();          //引脚靠外部电阻上拉，当作输入
	SMBus_Delay(2);			// High Level of Clock Pulse
    SMBUS_SCK_H();			// Set SCL line
    SMBus_Delay(5);			// High Level of Clock Pulse
    if (SMBUS_SDA_PIN())
    {
        Ack_bit=1;
    }
    else
    {
        Ack_bit=0;
    }
    SMBUS_SCK_L();			// Clear SCL line
    SMBus_Delay(3);			// Low Level of Clock Pulse

    return	Ack_bit;
}

/*******************************************************************************
* Function Name  : SMBus_ReceiveByte
* Description    : Receive a byte on SMBus
* Input          : ack_nack
* Output         : None
* Return         : RX_buffer
*******************************************************************************/
u8 SMBus_ReceiveByte(u8 ack_nack)
{
    u8 	RX_buffer;
    u8	Bit_Counter;

    SDA_IN();//SDA设置为输入
    for(Bit_Counter=8; Bit_Counter; Bit_Counter--)
    {
        if(SMBus_ReceiveBit())			// Get a bit from the SDA line
        {
            RX_buffer <<= 1;			// If the bit is HIGH save 1  in RX_buffer
            RX_buffer |=0x01;
        }
        else
        {
            RX_buffer <<= 1;			// If the bit is LOW save 0 in RX_buffer
            RX_buffer &=0xfe;
        }
    }
    SDA_OUT();      //SDA设置为输出
    SMBus_SendBit(ack_nack);			// Sends acknowledgment bit
    return RX_buffer;
}


/*******************************************************************************
* Function Name  : SMBus_Init
* Description    : SMBus初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SMBus_Init()
{
//    GPIO_InitTypeDef    GPIO_InitStructure;

//	/* Enable SMBUS_PORT clocks */
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SMBUS_PORT, ENABLE);

//    /*配置SMBUS_SCK、SMBUS_SDA为集电极开漏输出*/
//    GPIO_InitStructure.GPIO_Pin = SMBUS_SCK | SMBUS_SDA;
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(SMBUS_PORT, &GPIO_InitStructure);

    GPIO_InitTypeDef GPIO_Initure;
    
    //PC6,7初始化设置
    GPIO_Initure.Pin=GPIO_PIN_8|GPIO_PIN_9;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;     //快速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
    
    SMBUS_SCK_H();
    SMBUS_SDA_H();
}
#endif 