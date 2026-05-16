/*
 * us_Systimer.c
 *
 *  Created on: Apr 22, 2019
 *      Author: cosys SpartaK
 */
#include "us_UserConfig.h"
static uint32_t s_SysTimeCount = 0;
static void     us_SYSTIMER_TimeCount(void *Temp);

/***************************************************
 *   名称：      	us_SYSTIMER_Init()
 *   功能：		初始化系统时间
 *   函数参数：	初始化时设置的系统时间  单位us
 *   返回值：	void
 ***************************************************/
void us_SYSTIMER_Init(uint32_t dwInit_TimeCount)
{
    uint32_t TimerId;
    us_SetSystemTime(dwInit_TimeCount);
    TimerId = (uint32_t)SYSTIMER_CreateTimer(1000, SYSTIMER_MODE_PERIODIC, (void *)us_SYSTIMER_TimeCount, NULL);
    if (TimerId != 0)
    {
        SYSTIMER_StartTimer(TimerId);
    }
}
/***************************************************
 *   名称：      	us_SYSTIMER_TimeCount()
 *   功能：		滴答定时器中断回调函数
 *   函数参数：	void*
 *   返回值：	void
 ***************************************************/
static void us_SYSTIMER_TimeCount(void *Temp)
{
    s_SysTimeCount += 1;    // 每1ms都加1
}

/***************************************************
 *   名称：      	us_SetSystemTime()
 *   功能：		设置系统时间
 *   函数参数：	设置的系统时间  单位Ms
 *   返回值：	void
 ***************************************************/
void us_SetSystemTime(uint32_t dwSet_SYSTimeCount)
{
    s_SysTimeCount = dwSet_SYSTimeCount;
}

/***************************************************
 * 	  名称：		us_GetSystemTime()
 *   功能：		获取系统时间
 *   函数参数：	void
 *   返回值：	s_SysTimeCount
 *   			系统时间，单位  100us
 ***************************************************/
uint32_t us_dwGetSystemTime(void)
{
    uint32_t dwTime = 0;
    dwTime          = s_SysTimeCount;
    return dwTime;
}
