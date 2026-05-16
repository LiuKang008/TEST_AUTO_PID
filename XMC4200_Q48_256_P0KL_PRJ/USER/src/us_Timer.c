/*
 * us_Timer.c
 *
 *  Created on: Apr 23, 2019
 *      Author: cosys SpartK
 */
#include "us_UserConfig.h"

uint32_t g_timerCount = 0;    // 定时器计数值

/***************************************************
 *   名称：      	timer_Interrupt_1us()
 *   功能：	定时器中断，用于延时计数
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void timer_Interrupt_1us(void)
{
    g_timerCount++;
}
