
#ifndef __SMBUS_H__
#define __SMBUS_H__



#define ACK	 0 //应答
#define	NACK 1 //无应答

void SMBus_Init();
void SMBus_StartBit(void);
void SMBus_StopBit(void);
u8 SMBus_SendByte(u8 Tx_buffer);
u8 SMBus_ReceiveByte(u8 ack_nack);

#endif