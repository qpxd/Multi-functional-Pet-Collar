
#ifndef __TOUCH_H__
#define __TOUCH_H__



#define Adujust      1        //1��ʹ�ó���Ԥ��У׼��Ϣ  0�ֶ�У׼

#define TP_PRES_DOWN 0x80  		//����������	  
#define TP_CATH_PRES 0x40  		//�а��������� 

typedef struct __C_TOUCH c_touch;

typedef struct __C_TOUCH
{
	    void* this;
	u8  (*scan)(const c_touch* this);
	int (*get_state)(const c_touch* this,u8* state);
  int (*get_xy)(const c_touch* this,u16* x,u16* y);
}c_touch;

c_touch touch_create(u8 spi_channal,u8 type,GPIO_TypeDef* pen_gpio,uint32_t pen_pin,GPIO_TypeDef* cs_gpio,uint32_t cs_pin);
#endif
