/*
 * us_HC595_3SEG.c
 *
 *  Created on: Jun 10, 2019
 *      Author: cosys engineer4
 */

#include "us_UserConfig.h"

#define HC595_CLK_L        // DIGITAL_IO_SetOutputLow(&IO_HC595_CLK) // 595 CS  SCLK
#define HC595_CLK_H        // DIGITAL_IO_SetOutputHigh(&IO_HC595_CLK)   //

#define HC595_DATA_IN_L    // DIGITAL_IO_SetOutputLow(&IO_HC595_DATA) // 595串行数据输入
#define HC595_DATA_IN_H    // DIGITAL_IO_SetOutputHigh(&IO_HC595_DATA)   //

#define HC595_CS_L         // DIGITAL_IO_SetOutputLow(&IO_HC595_CS) // 595 CLK LCLK
#define HC595_CS_H         // DIGITAL_IO_SetOutputHigh(&IO_HC595_CS)   //

#define LED1_ON            // DIGITAL_IO_SetOutputHigh(&IO_LED1)
#define LED1_OFF           // DIGITAL_IO_SetOutputLow(&IO_LED1)   //
#define LED2_ON            // DIGITAL_IO_SetOutputHigh(&IO_LED2)
#define LED2_OFF           // DIGITAL_IO_SetOutputLow(&IO_LED2)   //
#define LED3_ON            // DIGITAL_IO_SetOutputHigh(&IO_LED3)
#define LED3_OFF           // DIGITAL_IO_SetOutputLow(&IO_LED3)   //
#define LED4_ON            // DIGITAL_IO_SetOutputHigh(&IO_LED4)
#define LED4_OFF           // DIGITAL_IO_SetOutputLow(&IO_LED4)   //
#define LED(n, m) LED##n##_##m
td_LED_ShowData LED_ShowData, pre_LED_ShowData;
extern uint8_t  bMenU_number;    // 菜单页面

/***************************************************
 *   名称：      	DrvHc595_Led()
 *   功能：		将串口输入转换为并口输出
 *   函数参数：	uLedDat  输入的数据
 *   返回值：	Ret 	  返回状态
 ***************************************************/
uint8_t us_bDrvHc595_Led(uint8_t bLedDat)
{
    uint8_t i   = 0;
    uint8_t Ret = 0;
    HC595_CS_L;                // RCK  L
    for (i = 0; i < 8; i++)    // 8bit
    {
        if ((bLedDat) & (1 << (7 - i)))
        {
            HC595_DATA_IN_H;    // SI  H
        }
        else
        {
            HC595_DATA_IN_L;    // SI  L
        }
        HC595_CLK_L;            //
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        HC595_CLK_H;    // CS 上升沿 数据位移
    }
    HC595_CS_L;         // RCK  L
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    HC595_CS_H;    // RCK 上升沿  输出刷新
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    HC595_CS_L;
    return Ret;
}
/***************************************************
 *   名称：      	us_bLEDShowData()
 *   功能：		将输入的数据显示在数码管上
 *   函数参数：	uint16_t wData      整形数据输入
 *   			float flData   浮点型数据输入
 *				uint8_t blflag      最大小数点位数
 *   返回值：	Ret 	                   返回状态
 ***************************************************/
static uint8_t us_bLEDShowData(uint16_t wData, float flData, uint8_t blflag)
{
    uint8_t  Ret        = 0;
    uint16_t wData_temp = wData;
    if (wData > 999)
        wData = 999;
    if (flData > 999)
        flData = 999;
    if (wData > 999 || flData > 999)
    {
        goto Exit;
    }
    LED_ShowData.bdata_0 = 0;
    LED_ShowData.bdata_1 = 0;
    LED_ShowData.bdata_2 = 0;
    LED_ShowData.bdata_3 = 0;
    if (blflag != 0)
    {
        if (flData >= 100)    // 只可能出现在psi
        {
            LED_ShowData.bdata_2 = 0x80;
            flData               = flData * 10;
        }
        else if (flData < 100 && flData >= 10)    // 1位小数  用temp最高位来表示是否需要在该位显示小数点
        {
            flData += 0.05;
            LED_ShowData.bdata_2 = 0x80;
            flData               = flData * 10;
        }
        else if (flData < 10)    // 2位小数
        {
            if (blflag == 1)
            {
                if (flData > 0)
                    flData += 0.05;
                LED_ShowData.bdata_2 = 0x80;
                flData               = flData * 10;    // 106转换为int是会变为105 大0.01可以
            }
            else
            {
                if (flData > 0)
                    flData += 0.005;
                LED_ShowData.bdata_3 = 0x80;
                flData               = flData * 100;    // 106转换为int是会变为105 大0.01可以
            }
        }
        wData_temp = (uint16_t)(flData);
        if (flData > 999.9)
        {
            LED_ShowData.bdata_0 = bLED_Number[wData_temp % 10];
            LED_ShowData.bdata_1 = bLED_Number[(wData_temp % 100) / 10];
            LED_ShowData.bdata_2 = bLED_Number[(wData_temp % 1000) / 100];
            LED_ShowData.bdata_3 = bLED_Number[wData_temp / 1000];
            if (blflag == 1)
                LED_ShowData.bdata_1 |= 0x80;    // 小数点
        }
        else
        {
            LED_ShowData.bdata_0 = 0;
            LED_ShowData.bdata_1 = bLED_Number[wData_temp % 100 % 10];
            LED_ShowData.bdata_2 |= bLED_Number[(wData_temp / 10) % 10];
            LED_ShowData.bdata_3 |= bLED_Number[wData_temp / 100];
            if ((blflag == 1) && (wData_temp < 100))    // 如果是小于1的小数，小数点前保留一个0
                LED_ShowData.bdata_3 = 0;
        }
    }
    else
    {
        wData_temp           = (uint16_t)(flData);
        LED_ShowData.bdata_0 = 0;
        LED_ShowData.bdata_1 = bLED_Number[wData_temp % 100 % 10];     // 个
        LED_ShowData.bdata_2 = bLED_Number[(wData_temp / 10) % 10];    // 十
        LED_ShowData.bdata_3 = bLED_Number[wData_temp / 100];          // 百
        if (wData_temp < 10)
        {
            LED_ShowData.bdata_3 = 0;
            LED_ShowData.bdata_2 = 0;
        }
        else if (wData_temp < 100)
            LED_ShowData.bdata_3 = 0;
    }

    //	LED_ShowData.bdata_2 = bLED_Number[(wData_temp/10)%10];
    //	LED_ShowData.bdata_3 = bLED_Number[wData_temp/100];

    if (flData < 999.9)                    // 显示压力单位
    {
        if (KEY_MENU_Temp_Data.H01 < 3)    // 压力单位
            LED_ShowData.bdata_0 = bLED_Char[10 + KEY_MENU_Temp_Data.H01];
        else
            LED_ShowData.bdata_0 = bLED_Char[19];
    }

    if (bMenU_number == 4)    // 第4页
    {
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Number[1] | 0x80;
    }
    else if (bMenU_number == 5)    // 第5页
    {
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Number[2] | 0x80;
    }
    else if (bMenU_number == 6)    // 第5页
    {
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Number[KEY_MENU_Temp_Data.H06 + 1] | 0x80;
    }
    else if (bMenU_number == 7)    // 第7页
    {
        if (flData < 0)
        {
            if (KEY_MENU_Temp_Data.H01 == 3)    // Mpa
                wData_temp = (uint16_t)(-flData * 10);
            else
                wData_temp = (uint16_t)(-flData);
            LED_ShowData.bdata_1 = bLED_Number[wData_temp % 100 % 10];
            LED_ShowData.bdata_2 = bLED_Number[(wData_temp / 10) % 10];
            LED_ShowData.bdata_3 = bLED_Char[4];
        }
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Char[15];
    }
    else if (bMenU_number == 8)    // 第8页
    {
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Char[5];
    }
    else if (bMenU_number == 10)    // 第10页
    {
        LED_ShowData.bdata_0 = LED_ShowData.bdata_1;
        LED_ShowData.bdata_1 = LED_ShowData.bdata_2;
        LED_ShowData.bdata_2 = LED_ShowData.bdata_3;
        LED_ShowData.bdata_3 = bLED_Char[7] | 0x80;
    }
    LED_ShowData_K.bdata_0 = LED_ShowData.bdata_0;
    LED_ShowData_K.bdata_1 = LED_ShowData.bdata_1;
    LED_ShowData_K.bdata_2 = LED_ShowData.bdata_2;
    LED_ShowData_K.bdata_3 = LED_ShowData.bdata_3;
Exit:
    return Ret;
}
void us_bLEDShowProcess()
{
    static uint8_t  Num            = 0;
    uint16_t        bShowData      = 0;
    uint32_t        dwGetSysTime   = us_dwGetSystemTime();
    static uint32_t dwLEDShiftTime = 0;
    if (dwGetSysTime - dwLEDShiftTime < 4)    // 7ms扫描一次  3位要21ms 大约50Hz的扫描频率
    {
        goto Exit;
    }
    dwLEDShiftTime = dwGetSysTime;
    Num++;
    if (Num > 3)
    {
        Num = 0;
    }
    switch (Num)
    {
    case 0:
        LED(2, OFF);
        LED(3, OFF);
        LED(4, OFF);
        bShowData = LED_ShowData.bdata_0;
        us_bDrvHc595_Led(bShowData);
        LED(1, ON);
        break;
    case 1:
        LED(3, OFF);
        LED(4, OFF);
        LED(1, OFF);
        bShowData = LED_ShowData.bdata_1;
        us_bDrvHc595_Led(bShowData);
        LED(2, ON);
        break;
    case 2:
        LED(1, OFF);
        LED(2, OFF);
        LED(4, OFF);
        bShowData = LED_ShowData.bdata_2;
        us_bDrvHc595_Led(bShowData);
        LED(3, ON);
        break;
    case 3:
        LED(1, OFF);
        LED(2, OFF);
        LED(3, OFF);
        bShowData = LED_ShowData.bdata_3;
        us_bDrvHc595_Led(bShowData);
        LED(4, ON);
        break;
    default:
        break;
    }

Exit:;
}
/***************************************************
 *   名称：      	us_uLEDShow_IntData()
 *   功能：		将输入的数据显示在数码管上
 *   函数参数：	uint16_t wData      整形数据输入
 *   返回值：	Ret 	                   返回状态
 ***************************************************/
uint8_t us_bLEDShow_IntData(uint16_t wData)
{
    return us_bLEDShowData(wData, 0, 0);
}

/***************************************************
 *   名称：      	us_uLEDShow_FlData()
 *   功能：		将输入的数据显示在数码管上
 *   函数参数：    float flData   浮点型数据输入
 *   返回值：	Ret 	                   返回状态
 ***************************************************/
uint8_t us_bLEDShow_FlData(float flData, uint8_t bflag)
{
    return us_bLEDShowData(0, flData, bflag);
}

uint8_t us_bLEDShow_SetData(td_LED_ShowData td_Data)
{
    LED_ShowData.bdata_0 = td_Data.bdata_0;
    LED_ShowData.bdata_1 = td_Data.bdata_1;
    LED_ShowData.bdata_2 = td_Data.bdata_2;
    LED_ShowData.bdata_3 = td_Data.bdata_3;    // td_Data.bdata_3;
    return 0;
}

uint8_t us_bGetPage9_Data(uint16_t td_Data)
{
    if (us_bGet_ID3().ID3_Unit == 3)    // Mpa
        td_Data = td_Data / 10;
    LED_ShowData_K.bdata_2 = bLED_Number[td_Data / 100];
    LED_ShowData_K.bdata_1 = bLED_Number[td_Data % 100 / 10];
    LED_ShowData_K.bdata_0 = bLED_Number[td_Data % 10];
    if ((us_bGet_ID3().ID3_Unit == 0) || (us_bGet_ID3().ID3_Unit == 3))    // Bar Mpa
        LED_ShowData_K.bdata_2 |= 0x80;
    us_bLEDShow_SetData(LED_ShowData_K);
    return 0;
}
/***************************************************
 *   名称：  trans_to_psi(uint16_t uwLED_TempValue)
 *   功能：压力不同单位转换到psi下的值
 *   函数参数：uwLED_TempValue 压力数值
 *   返回值：    无
 ***************************************************/
float trans_to_psi(uint16_t uwLED_TempValue)
{
    float psi_set;
    switch (KEY_MENU_Temp_Data.H01)
    {
    case 0:                                            // psi
        psi_set = uwLED_TempValue * 0.1;               // uwLED_TempValue单位是0.1psi
        break;
    case 1:                                            // bar
    case 2:                                            // kpa
    case 3:                                            // Mpa
        psi_set = (float)(uwLED_TempValue * 0.145);    // uwLED_TempValue单位是kpa
        break;
    }
    return (psi_set);
}
