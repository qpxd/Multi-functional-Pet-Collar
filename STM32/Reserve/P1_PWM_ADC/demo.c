
float pm2_5 = 0.0f;
c_gp2y   gp2y  = {0};

/*创建gp2y对象*/
gp2y = gp2y_create(MY_ADC_0,GPIOA,GPIO_PIN_1);
if(NULL == gp2y.this)
{
    log_error("gp2y creat failed.");
}

/*获取当前pm2.5*/
ret = gp2y.get(&gp2y,&pm2_5);
if(E_OK != ret)
{
    log_error("pm2_5 get failed.");
}
