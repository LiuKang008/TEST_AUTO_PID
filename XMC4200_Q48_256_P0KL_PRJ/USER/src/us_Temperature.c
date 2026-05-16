/*
 * us_Temperature.c
 *
 *  Created on: 2015-4-5
 *      Author: pnzrk
 */

#include "us_UserConfig.h"

uint32_t dwTempData = 0;

/***************************************************
 *   名称：      	us_Temp_Init()
 *   功能：		初始化获取芯片温度
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_Temp_Init(void)
{
    // TMPS001_Enable();
}

/***************************************************
 *   名称：      	us_Temp_Process()
 *   功能：		轮询获取芯片温度
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_Temp_Process(void)
{
    //	static uint32_t dwTempTime = 0;
    //	uint32_t dwGetSYSTime = us_dwGetSystemTime();
    //	if((dwGetSYSTime-dwTempTime)<1000)
    //	{
    //		goto Exit;
    //	}
    //	dwTempTime = dwGetSYSTime;
    //	if(TMPS001_GetStatus() == TMPS001_READY)
    //	{
    //		TMPS001_StartMeasurement();
    //		while (TMPS001_GetStatus() != TMPS001_READY)
    //		{}
    //		dwTempData = TMPS001_ReadTemp();
    //	}
    // Exit:
    //	;
}

/***************************************************
 *   名称：      	us_Get_TempData()
 *   功能：		获取当前芯片温度
 *   函数参数：	void
 *   返回值：	当前温度
 ***************************************************/
uint16_t us_Get_TempData(void)
{
    return (uint16_t)dwTempData;
}
