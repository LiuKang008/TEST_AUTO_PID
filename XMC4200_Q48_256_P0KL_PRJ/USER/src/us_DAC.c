/*
 * us_DAC.c
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */
#include "us_UserConfig.h"

uint8_t OutPut1_Mode = Mode_None;         // 通道模式值
uint8_t OutPut2_Mode = Mode_None;
uint8_t TTL_Mode     = 0;                 // 通道2输出模式值
uint8_t High_Mode    = 0;                 // 通道2输出有效电平模式值

uint16_t       wChannel1Adjust    = 0;    // 通道校正值
uint16_t       wChannel2Adjust    = 0;
uint16_t       wChannADCAdjust[4] = {0, 0, 0, 0};
const uint16_t DAC_Adj_Value_mA[] = {4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 8000, 8500, 9000, 9500, 10000, 10500, 11000, 12000, 12500, 13000, 13500, 14000, 14500, 15000, 15500, 16000, 16500, 17000, 17500, 18000, 18500, 19000, 19500, 20000};
const uint16_t DAC_Adj_Value_mV[] = {200, 500, 750, 1000, 1250, 1500, 1750, 2000, 2250, 2500, 2750, 3000, 3250, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500, 7750, 8000, 8250, 8500, 8750, 9000, 9250, 9500, 9750, 10000};
/*定义一个两维全局数组，存储DAC校准值*/
int16_t DAC_Adj_Valu_Temp[2][32] = {{0}, {0}};

static double DAC_Adj_k[2][64] = {{0}};

static double us_bGet_DAC_Adjust_k(uint16_t wValue, uint8_t bChnn, uint8_t bMode);

/***************************************************
 *   名称：      	us_DAC_Init()
 *   功能：		DAC 初始化函数
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_DAC_Init(void)
{
    DAC_Enable(&DAC_0);
    DAC_Enable(&DAC_1);
    DAC_SingleValue_SetValue_u16(&DAC_0, 4095);
    DAC_SingleValue_SetValue_u16(&DAC_1, 4095);
    switch (us_bGetDACMode())
    {
    case DAC1C:
        OutPut1_Mode = mA_Current;
        OutPut2_Mode = Mode_None;
        break;
    case DAC1V:
        OutPut1_Mode = mV_Voltage;
        OutPut2_Mode = Mode_None;
        break;
    case DAC12CC:
        OutPut1_Mode = mA_Current;
        OutPut2_Mode = mA_Current;
        break;
    case DAC12CV:
        OutPut1_Mode = mA_Current;
        OutPut2_Mode = mV_Voltage;
        break;
    case DAC12VC:
        OutPut1_Mode = mV_Voltage;
        OutPut2_Mode = mA_Current;
        break;
    case DAC12VV:
        OutPut1_Mode = mV_Voltage;
        OutPut2_Mode = mV_Voltage;
        break;
    default:
        OutPut1_Mode = Mode_None;
        OutPut2_Mode = Mode_None;
        break;
    }
    us_bSet_DAC_Adjust(1);
    us_bSet_DAC_Adjust(2);
    if (OutPut1_Mode == mA_Current)
    {
        us_Set_DAC_Temp(4000, mA_Current, OutPut1);
    }
    else
    {
        us_Set_DAC_Temp(0, mV_Voltage, OutPut1);
    }
    if (OutPut2_Mode == mA_Current)
    {
        us_Set_DAC_Temp(4000, mA_Current, OutPut2);
    }
    else
    {
        us_Set_DAC_Temp(0, mV_Voltage, OutPut2);
    }
}

/***************************************************
 *   名称：      	Set_DAC_Temp()
 *   功能：		根据1023的比例计算要设置的DA值
 *   函数参数：	uint16_t mA_Current  mA的一千倍
 *   			输入范围：2000~20000  输出精度可以是mA的一位小数点
 *   			bFlag  标识是电压还是电流
 *   返回值：	void
 ***************************************************/
void us_Set_DAC_Temp(uint16_t wValue, uint8_t bFlag, uint8_t bChannel)
{
    us_bSet_DACValue(wValue, bFlag, bChannel);
}
/***************************************************
 *   名称：      	Set_DACValue()
 *   功能：		设置输出电流或者电压
 *   函数参数：	uint16_t mA_Current  mA的一千倍
 *   			输入范围：3000~20000  输出精度可以是mA的一位小数点
 *   			flag  标识是电压还是电流
 *   返回值：	错误代码
 ***************************************************/
uint8_t us_bSet_DACValue(uint16_t wValue, uint8_t bFlag, uint8_t bChannel)
{
    // 减法放大
    // 电流计算：DAC.DATA = 4095*(Vout-0.3)/(2.5-0.3);
    // 由于是减法电路，2.5-Vout，所以实际输出电压为2.5-Vout,所以有Vo= (4095-DAC.DATA)*2.2/4095
    // I=Vo/500= (4095-DAC.DATA)*22*5/4095*5(mA);要乘以5倍
    //  DAC.DATA =4095*(1-IR/(2.2*5000));
    // 而电压计算为：DAC.DATA = 4095*(1-Vo/(2.2*5))其中5为放大倍数
    double flTemp = 0;
    if ((bChannel == 0) || (bChannel > 3))
    {
        goto Exit;
    }
    switch (bFlag)
    {
    case mA_Current:
        if (us_bGetDACFlag(bChannel) == 0xA5)
        {
            flTemp = us_bGet_DAC_Adjust_k(wValue, bChannel, mA_Current);
        }
        else
        {
            flTemp = wValue;
        }
        if (((flTemp < 1500) && (flTemp != 0)) || (flTemp > 22000))    // 允许有余量
        {
            goto Exit;
        }
        // flTemp = (double)4095*(((double)flTemp*120/(1000*2200*1.20)))+0.5;//按照公式 不进行调整 499为电阻   改为30/4.7倍
        flTemp = (double)4095 * (1 - (((double)flTemp * 120 / (1000 * 2200 * 1.20)))) + 0.5;
        break;
    case mV_Voltage:
        if (us_bGetDACFlag(bChannel) == 0xA5)
        {
            flTemp = us_bGet_DAC_Adjust_k(wValue, bChannel, mV_Voltage);
        }
        else
        {
            flTemp = wValue;
        }
        if (flTemp > 11000)    // 允许有2%的余量
        {
            goto Exit;
        }
        // flTemp = 4095*((double)(flTemp)/(2200*5.0))+0.5;//-37;//直接按照公式计算DAC的值
        flTemp = 4095 * (1 - ((double)(flTemp) / (2200 * 5.0))) + 0.5;    //-37;//直接按照公式计算DAC的值
        break;
    default:
        goto Exit;
    }
    if (flTemp > 4095)
    {
        flTemp = 4095;
    }
    if (bChannel == OutPut1)
    {
        DAC_SingleValue_SetValue_u16(&DAC_1, (uint16_t)flTemp);
    }
    else
    {
        DAC_SingleValue_SetValue_u16(&DAC_1, (uint16_t)flTemp);
    }

Exit:
    return 0;
}

/***************************************************
 *   名称：      	us_bGet_Channel_Mode()
 *   功能：		获取通道模式
 *   函数参数：	uint8_t bChnn 通道号
 *   返回值：	模式值
 ***************************************************/
uint8_t us_bGet_Channel_Mode(uint8_t bChnn)
{
    uint8_t bMode_Value = Mode_None;
    switch (bChnn)
    {
    case 1:
        bMode_Value = OutPut1_Mode;
        break;
    case 2:
        bMode_Value = OutPut2_Mode;
        break;
    default:
        bMode_Value = Mode_None;
        break;
    }
    return bMode_Value;
}

/***************************************************
 *   名称：      	us_bGet_TTL_Mode()
 *   功能：		获取通道2输出模式
 *   函数参数：	void
 *   返回值：	模式值
 ***************************************************/
uint8_t us_bGet_TTL_Mode(void)
{
    return TTL_Mode;
}

/***************************************************
 *   名称：      	us_bGet_Valid_Mode()
 *   功能：		获取通道2有效电平模式
 *   函数参数：	void
 *   返回值：	模式值
 ***************************************************/
uint8_t us_bGet_Valid_Mode(void)
{
    return High_Mode;
}

/***************************************************
 *   名称：      	us_wGet_Chnn_Adjust()
 *   功能：		获取通道 的校正值
 *   函数参数：	通道号
 *   返回值：	校正值
 ***************************************************/
uint16_t us_wGet_Chnn_Adjust(uint8_t bChnn)
{
    uint8_t bData[2] = {0};
    if (bChnn == 1)
    {
        us_bI2c_Read_Stream(adPPCAdjust0, bData, 2);
        wChannel1Adjust = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    else if (bChnn == 2)
    {
        us_bI2c_Read_Stream(adPPCAdjust1, bData, 2);
        wChannel2Adjust = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    else if (bChnn == 10)
    {
        us_bI2c_Read_Stream(adPPCADCAdjust0, bData, 2);
        wChannADCAdjust[0] = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    else if (bChnn == 11)
    {
        us_bI2c_Read_Stream(adPPCADCAdjust1, bData, 2);
        wChannADCAdjust[1] = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    else if (bChnn == 12)
    {
        us_bI2c_Read_Stream(adPPCADCAdjust2, bData, 2);
        wChannADCAdjust[2] = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    else if (bChnn == 13)
    {
        us_bI2c_Read_Stream(adPPCADCAdjust3, bData, 2);
        wChannADCAdjust[3] = (uint16_t)(bData[0] + (bData[1] << 8));
    }
    return (uint16_t)(bData[0] + (bData[1] << 8));
}

/***************************************************
 *   名称：      	us_wGet_Chnn_ADC_Adjust()
 *   功能：		获取ADC通道 的校正值
 *   函数参数：	通道号
 *   返回值：	校正值
 ***************************************************/
float us_wGet_Chnn_ADC_Adjust(uint8_t bChnn)
{
    if (bChnn < 4)
    {
        return (float)(wChannADCAdjust[bChnn] / 1000);
    }
    return 0xffff;
}

/*20180929*/
void us_bSet_DAC_Adjust(uint8_t bChnn)
{
    uint8_t  i         = 0;
    uint8_t  bTemp[64] = {0};
    uint16_t crc       = 0;
    if ((bChnn > 3) || (bChnn == 0))
    {
        goto Exit;
    }
    us_bI2c_Read_Stream(DAC_Adj_S + (bChnn - 1) * 64, (uint8_t *)bTemp, 32);
    us_bI2c_Read_Stream(DAC_Adj_S + (bChnn - 1) * 64 + 32, (uint8_t *)&bTemp[32], 32);
    us_bI2c_Read_Stream(DAC_Adj_CRC_01 + (bChnn - 1) * 4, (uint8_t *)&crc, 2);
    if (wCRCCheck((uint8_t *)bTemp, 64) != crc)
    {
        us_bSetDACFlag(bChnn, 0);
    }
    for (i = 0; i < 32; i++)
    {
        DAC_Adj_Valu_Temp[bChnn - 1][i] = (s16)(bTemp[i * 2] | (bTemp[i * 2 + 1] << 8));
    }
    if (us_bGet_Channel_Mode(bChnn) == mA_Current)
    {
        for (i = 0; i < 32; i++)
        {
            DAC_Adj_k[bChnn - 1][i * 2] = ((double)(DAC_Adj_Valu_Temp[bChnn - 1][i] - DAC_Adj_Value_mA[i])) / (DAC_Adj_Valu_Temp[bChnn - 1][i]);
        }
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            DAC_Adj_k[bChnn - 1][i * 2] = ((double)(DAC_Adj_Valu_Temp[bChnn - 1][i] - DAC_Adj_Value_mV[i])) / (DAC_Adj_Valu_Temp[bChnn - 1][i]);
        }
    }
    for (i = 0; i < 32; i++)
    {
        DAC_Adj_k[bChnn - 1][i * 2 + 1] = (DAC_Adj_k[bChnn - 1][i * 2] + DAC_Adj_k[bChnn - 1][(i + 1) * 2]) / 2;
    }
Exit:;
}

static double us_bGet_DAC_Adjust_k(uint16_t wValue, uint8_t bChnn, uint8_t bMode)
{
    uint8_t i      = 0;
    double  dlData = 0;
    if ((bChnn > 3) || (bChnn == 0) || (bMode > 1))
    {
        goto Exit;
    }
    if (bMode == mA_Current)
    {
        if (wValue <= DAC_Adj_Value_mA[0])
        {
            dlData = wValue - DAC_Adj_k[bChnn - 1][0] * wValue;
            goto Exit;
        }
        else if (wValue >= DAC_Adj_Value_mA[31])
        {
            dlData = wValue - DAC_Adj_k[bChnn - 1][62] * wValue;
            goto Exit;
        }
        for (i = 1; i < 32; i++)
        {
            if ((wValue > DAC_Adj_Value_mA[i - 1]) && (wValue <= (DAC_Adj_Value_mA[i - 1] + DAC_Adj_Value_mA[i]) / 2))
            {
                dlData = wValue - DAC_Adj_k[bChnn - 1][i * 2 - 1] * wValue;
                goto Exit;
            }
            else if ((wValue > (DAC_Adj_Value_mA[i - 1] + DAC_Adj_Value_mA[i]) / 2) && (wValue <= DAC_Adj_Value_mA[i]))
            {
                dlData = wValue - DAC_Adj_k[bChnn - 1][i * 2] * wValue;
                goto Exit;
            }
        }
    }
    else
    {
        if (wValue <= DAC_Adj_Value_mV[0])
        {
            dlData = wValue - DAC_Adj_k[bChnn - 1][0] * wValue;
            goto Exit;
        }
        else if (wValue >= DAC_Adj_Value_mV[31])
        {
            dlData = wValue - DAC_Adj_k[bChnn - 1][62] * wValue;
            goto Exit;
        }
        for (i = 1; i < 32; i++)
        {
            if ((wValue > DAC_Adj_Value_mV[i - 1]) && (wValue <= (DAC_Adj_Value_mV[i - 1] + DAC_Adj_Value_mV[i]) / 2))
            {
                dlData = wValue - DAC_Adj_k[bChnn - 1][i * 2 - 1] * wValue;
                goto Exit;
            }
            else if ((wValue > (DAC_Adj_Value_mV[i - 1] + DAC_Adj_Value_mV[i]) / 2) && (wValue <= DAC_Adj_Value_mV[i]))
            {
                dlData = wValue - DAC_Adj_k[bChnn - 1][i * 2] * wValue;
                goto Exit;
            }
        }
    }
Exit:
    return dlData;
}
