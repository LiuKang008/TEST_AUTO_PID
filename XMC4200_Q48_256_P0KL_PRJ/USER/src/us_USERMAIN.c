/*
 * us_USERMAIN.c
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */

#include "us_UserConfig.h"
/***************************************************
 *   名称：      	us_Init()
 *   功能：		用户初始化程序
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_Init(void)
{
    us_SYSTIMER_Init(0);    // 初始化定时器
    us_USBInit();
    us_DAC_Init();          // 初始化DAC
    us_Key_Init();
}
/***************************************************
 *   名称：      	us_Process()
 *   功能：		用户轮询程序
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_Process(void)
{
    us_bADC_Process();
    us_USB_Process();
    us_LED_Status_Process();
}
