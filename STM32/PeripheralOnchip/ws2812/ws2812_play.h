#ifndef _WS2812_PLAY_H
#define _WS2812_PLAY_H

#include "ws2812/ws2812.h"


#define NORMAL_MODE  0   //正常模式
#define HEATING_MODE 1   //加热模式
#define COOLING_MODE 2   //制冷模式

/*播放开机动画*/
int play_boot_animation (const c_ws2812* this);

/*显示当前温度*/
int play_current_temp   (const c_ws2812* this,float temp,int state);

/*显示设定温度*/
/*调用该函数后将闪烁10s 过程中再次调用将重置闪烁时间*/
int play_set_temp       (const c_ws2812* this,float* target_temp);

/*显示缺水图案*/
int lack_warn           (const c_ws2812* this);

/*显示补水图案*/
int fill_water          (const c_ws2812* this);

/*设置亮度*/
/*luminance范围为1-10*/
int set_luminance(const c_ws2812* this,u8 luminance);

#endif