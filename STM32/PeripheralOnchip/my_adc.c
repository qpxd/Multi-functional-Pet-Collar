/*****************************************************************************
* Copyright:
* File name: my_adc.c
* Description: adc模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/19
*****************************************************************************/

#include "onchip_conf.h"

#ifdef ONCHIP_ADC_ENABLED

#define MODULE_NAME       "my adc"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

static int init(u8 ch);
static int get(u8 ch,float* v);

const struct {u8 channal;GPIO_TypeDef* gpio;uint32_t pin;} g_my_adc_cfg[] = {
       {MY_ADC_0,GPIOA,GPIO_PIN_0},
       {MY_ADC_1,GPIOA,GPIO_PIN_1},
       {MY_ADC_2,GPIOA,GPIO_PIN_2},
       {MY_ADC_3,GPIOA,GPIO_PIN_3},
       {MY_ADC_4,GPIOA,GPIO_PIN_4},
       {MY_ADC_5,GPIOA,GPIO_PIN_5},
       {MY_ADC_6,GPIOA,GPIO_PIN_6},
       {MY_ADC_7,GPIOA,GPIO_PIN_7},
       {MY_ADC_8,GPIOB,GPIO_PIN_0},
       {MY_ADC_9,GPIOB,GPIO_PIN_1},
};

const c_adc my_adc = {init,get};

static ADC_HandleTypeDef        g_adc_handler     ;
static RCC_PeriphCLKInitTypeDef g_adc_clock       ;
static bool                     g_base_init       ;
static bool                     g_channal_init[10];

static int init(u8 ch)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_ADC1_CLK_ENABLE();

    /*检查基础初始化*/
    if(!g_base_init)
    {
        g_adc_clock.PeriphClockSelection=RCC_PERIPHCLK_ADC;         //ADC外设时钟
        g_adc_clock.AdcClockSelection=RCC_ADCPCLK2_DIV6;            //分频因子6时钟为72M/6=12MHz
        HAL_RCCEx_PeriphCLKConfig(&g_adc_clock);                    //设置ADC时钟
        
        g_adc_handler.Instance                    = ADC1;
        g_adc_handler.Init.DataAlign              = ADC_DATAALIGN_RIGHT; //右对齐
        g_adc_handler.Init.ScanConvMode           = DISABLE;             //非扫描模式
        g_adc_handler.Init.ContinuousConvMode     = DISABLE;             //关闭连续转换
        g_adc_handler.Init.NbrOfConversion        = 1;                   //1个转换在规则序列中 也就是只转换规则序列1 
        g_adc_handler.Init.DiscontinuousConvMode  = DISABLE;             //禁止不连续采样模式
        g_adc_handler.Init.NbrOfDiscConversion    = 0;                   //不连续采样通道数为0
        g_adc_handler.Init.ExternalTrigConv       = ADC_SOFTWARE_START;  //软件触发
        HAL_ADC_Init(&g_adc_handler);                                    //初始化 
        
        HAL_ADCEx_Calibration_Start(&g_adc_handler);                  //校准ADC

        g_base_init = true;
    }

    /*检查参数*/
    if(ch > MY_ADC_9)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*初始化相应IO*/
    GPIO_Initure.Pin  = g_my_adc_cfg[ch].pin; 
    GPIO_Initure.Mode = GPIO_MODE_ANALOG;     //模拟
    GPIO_Initure.Pull = GPIO_NOPULL;          //不带上下拉
    HAL_GPIO_Init(g_my_adc_cfg[ch].gpio,&GPIO_Initure);

    /*对应的通道标记为初始化成功*/
    g_channal_init[ch] = true;

    return E_OK;
}

static int get(u8 ch,float* v)
{
    ADC_ChannelConfTypeDef ADC1_ChanConf;
    u16 adc_value = 0;
    float adc_v   = 0.0f;

    /*检查参数*/
    if(NULL == v)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(ch > MY_ADC_9)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(false == g_base_init || false == g_channal_init[ch])
    {
        log_error("Need init first.");
        return E_ERROR;
    }

    /*配置ADC*/
    ADC1_ChanConf.Channel=ch;                                   //通道
    ADC1_ChanConf.Rank=1;                                       //第1个序列，序列1
    ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_239CYCLES_5;      //采样时间               
    HAL_ADC_ConfigChannel(&g_adc_handler,&ADC1_ChanConf);        //通道配置
    
    HAL_ADC_Start(&g_adc_handler);                               //开启ADC
    
    HAL_ADC_PollForConversion(&g_adc_handler,10);                //轮询转换
 
    adc_value = (u16)HAL_ADC_GetValue(&g_adc_handler);
    adc_v     = (float)adc_value * (3.3f / 4096.0f);
    *v        = adc_v;

    return E_OK;
}
#endif
