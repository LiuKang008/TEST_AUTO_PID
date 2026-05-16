/*
 * us_ADC.c
 *
 *  Created on: Apr 22, 2019
 *      Author: cosys SpartaK
 */

#include "us_UserConfig.h"
#include <math.h>
#include "us_ADC.h"
#define CURRENT_THRESHOLD 15

extern uint8_t ubErrCode;

extern float ubMaxScalePsi_PCSet;
extern float ubMinScalePsi_PCSet;
extern float psi_value_set_Filter;
extern float Psi_Sensor1_Val;
extern float Psi_Sensor1_Val3;
extern float psi_value_set;
extern float psi_value_set1;
extern float psi_value_set2;
extern float psi_value_set3;
extern float controller_Error_Handled;
extern float fDeadBand;
extern float ZeroDeadBand;

extern pressure_calibration calibration;

volatile uint8_t  bADC_finsh = 0;    // 标识ADC转换已经完成
volatile uint16_t ADC_PWM1;
volatile uint16_t ADC_PWM2;
volatile uint16_t ADC_4_20mA;
volatile uint16_t ADC_Sensor;
volatile uint16_t ADC_In_Power;
volatile uint8_t  intakeValveCoilCheckFlag = 0;    // 进气阀线圈检测标志位
volatile uint8_t  intakeValveDamageCount   = 0;    // 进气阀线圈损坏计次
volatile uint16_t intake_Cnt;

volatile uint8_t  exhaustValveCoilCheckFlag = 0;    // 排气阀线圈检测标志位
volatile uint8_t  exhaustValveDamageCount   = 0;    // 排气阀线圈损坏计次
volatile uint16_t exhaust_Cnt;

uint8_t AdjustADCFlag = 0;    // 开始校准AD标志位
uint8_t Psi_Show_Count;       // 用于滤波的计数
uint8_t mode;
uint8_t pre_mode;
uint8_t intakeCoilStatus;
uint8_t exhaustCoilStatus;
uint8_t sensor_filter_initialized = 0;    // 传感器滤波器初始化标志

uint16_t wADC_Temp[5][20];
uint16_t ad_value_voltage = 0;
uint16_t ad_value_current = 0;
uint16_t wADC_Result_Avr_0[5];
uint16_t wADC_Adjust_Temp[2][20];    // 20个采样点
uint16_t wADC_Adjust_Input[2];       // 0:输入的mA值对应AD，1：输入的mV值对应AD
uint16_t Voltage_Filter_Buf[Filter_Num];
uint16_t Current_Filter_Buf[Filter_Num];
uint16_t Sensor1_Filter_Buf[Filter_Num];
uint16_t Input_Power[Filter_Num];
uint16_t AD_Voltage_Value_Filtered;
uint16_t AD_Current_Value_Filtered;
uint16_t AD_Sensor1_Value_Filtered;
uint16_t AD_Power_Value_Filtered;
uint16_t sample_cnt;

float current_ref;
float volt_ref;
float current_ref1;
float volt_ref1;
float x1, x2, dot_ref;
float g_Power_Voltage     = 24;
float g_Power_Voltage_Adj = 24;
float g_Power_Scale       = 1.0f;

uint8_t  us_Get_ADCResult(uint16_t *wpResult, uint8_t bLen);
uint16_t us_Get_ADC_Chnn(uint8_t bChnn);

void adc_measurement_adv_callback_in_coil(void)
{
    static uint32_t PWM1_adcCnt;
    PWM1_adcCnt++;
    // ADC_PWM1 = ADC_MEASUREMENT_ADV_GetResult(&ADC_MEASUREMENT_ADV_0_Channel_A);//group_ptrs[1]->res[3]
    ADC_PWM1 = ADC_MEASUREMENT_ADV_0_Channel_In_Coil_RES & 0xFFF;
    CheckIntakeValveCoil(ADC_PWM1);
}

void adc_measurement_adv_callback_out_coil(void)
{
    static uint32_t PWM2_adcCnt;

    PWM2_adcCnt++;
    // ADC_PWM2 = ADC_MEASUREMENT_ADV_GetResult(&ADC_MEASUREMENT_ADV_1_Channel_A);//group_ptrs[0]-》res[4];
    ADC_PWM2 = ADC_MEASUREMENT_ADV_1_Channel_Out_Coil_RES & 0xFFF;
    CheckexhaustValveCoil(ADC_PWM2);
    ADC_4_20mA   = ADC_MEASUREMENT_ADV_1_Channel_4_20mA_RES & 0xFFF;
    ADC_Sensor   = ADC_MEASUREMENT_ADV_1_Channel_Sensor_Pressure_RES & 0xFFF;
    ADC_In_Power = ADC_MEASUREMENT_ADV_1_Channel_In_Power_RES & 0xFFF;
    bADC_finsh   = 1;
}

/***************************************************
 *   名称：      	us_bADC_Process()
 *   功能：		ADC轮询函数
 *   函数参数：	void
 *   返回值：	void
 ***************************************************/
void us_bADC_Process(void)
{
    static uint8_t bCount[5]         = {0, 0, 0, 0, 0};
    static uint8_t bSampleCnt[2]     = {0};
    static uint8_t bSampleTimeCnt[2] = {0};
    if (bADC_finsh != 0)
    {
        sample_cnt++;
        // 输入电流或电压AD值
        wADC_Temp[0][bCount[0]] = ADC_4_20mA;
        bCount[0]++;
        if (bCount[0] > 19)
        {
            bCount[0] = 0;
            us_Get_ADCResult(wADC_Temp[0], 20);
            wADC_Result_Avr_0[0] = wADC_Temp[0][0];
            /*标定输入电流，每30ms采集一个点，20个点取平均值，wADC_Adjust_Input[0]作为标定AD值*/
            if ((AdjustADCFlag == 1) && ((bSampleTimeCnt[0]++) >= 5))
            {
                bSampleTimeCnt[0]                  = 0;    // 计数清零
                wADC_Adjust_Temp[0][bSampleCnt[0]] = wADC_Result_Avr_0[0];
                bSampleCnt[0]++;
                if (bSampleCnt[0] >= 20)
                {
                    bSampleCnt[0] = 0;
                    us_Get_ADCResult(wADC_Adjust_Temp[0], 20);
                    wADC_Adjust_Input[0] = wADC_Adjust_Temp[0][0];
                }
            }
        }

        // 输入Power 24V
        wADC_Temp[1][bCount[1]] = ADC_In_Power;
        bCount[1]++;
        if (bCount[1] > 19)
        {
            bCount[1] = 0;
            us_Get_ADCResult(wADC_Temp[1], 20);
            wADC_Result_Avr_0[1] = wADC_Temp[1][0];
        }

        // 气压传感器
        wADC_Temp[2][bCount[2]] = ADC_Sensor;
        bCount[2]++;
        if (bCount[2] > 19)
        {
            bCount[2] = 0;
            us_Get_ADCResult(wADC_Temp[2], 20);
            wADC_Result_Avr_0[2] = wADC_Temp[2][0];
        }
        // 进气电磁阀电流
        wADC_Temp[3][bCount[3]] = ADC_PWM1;
        bCount[3]++;
        if (bCount[3] > 19)
        {
            bCount[3] = 0;
            us_Get_ADCResult(wADC_Temp[3], 20);
            wADC_Result_Avr_0[3] = wADC_Temp[3][0];
        }
        // 排气电磁阀电流
        wADC_Temp[4][bCount[4]] = ADC_PWM2;
        bCount[4]++;
        if (bCount[4] > 19)
        {
            bCount[4] = 0;
            us_Get_ADCResult(wADC_Temp[4], 20);
            wADC_Result_Avr_0[4] = wADC_Temp[4][0];
        }

        bADC_finsh = 0;
    }
}

/***************************************************
 *   名称：      	us_Get_ADCResult()
 *   功能：		将ADC结果数组进行排序，取平均值，并通过指针pResult返回结果
 *   函数参数：	uint16_t *pResult  返回采样平均值   精度可以是0.01V  单位为mV
 *   返回值：	错误代码
 ***************************************************/
uint8_t us_Get_ADCResult(uint16_t *wpResult, uint8_t bLen)
{
    uint16_t temp       = 0;    // 用于对ADC数组中的数据进行排序
    float    tempResult = 0;
    s8       i = 0, j = 0;      // j一定要是有符号数，否则j<=0会判断失败，进入死循环
    if (bLen < 5)
    {
        goto Exit;
    }
    for (i = 1; i < bLen; i++)    // 将数组中的数据进行排序
    {
        for (j = i - 1; j >= 0; j--)
        {
            if (*(wpResult + j + 1) < *(wpResult + j))
            {
                temp                = *(wpResult + j + 1);
                *(wpResult + j + 1) = *(wpResult + j);
                *(wpResult + j)     = temp;
            }
        }
    }
    for (i = 5; i < bLen - 5; i++)
    {
        tempResult = tempResult + ((float)*(wpResult + i)) / (bLen - 10);
    }
    *wpResult = (uint16_t)(tempResult + 0.5);
Exit:
    return 0;
}
//
//
uint16_t us_Get_ADC_Chnn(uint8_t bChnn)
{
    if (bChnn >= 5)    // wADC_Result_Avr_0数组大小为5
    {
        return 0;
    }
    return wADC_Result_Avr_0[bChnn];
}

/***************************************************
 *   名称：       Voltage_Current_Filter(uint16_t filter[Filter_Num])
 *   功能：       去除最大最小值后，均值滤波
 *   函数参数：filter[Filter_Num] 待滤波数据
 *   返回值：	滤波后均值
 ***************************************************/
uint16_t Voltage_Current_Filter(uint16_t filter[Filter_Num])
{
    unsigned char count;
    uint16_t      sum = 0, max, min;
    max               = filter[0];
    min               = filter[0];
    for (count = 0; count < Filter_Num; count++)
    {
        if (filter[count] > max)
            max = filter[count];
        if (filter[count] < min)
            min = filter[count];
    }
    for (count = 0; count < Filter_Num; count++)
        sum = sum + filter[count];
    sum = (sum - max - min) / (Filter_Num - 2);
    return (sum);
}
/***************************************************
 *   名称：       ADC_Data_Handel()
 *   功能：       采样数据送入缓冲区，并进行滤波，并判断设定压力值是否超出范围，若超出，标记错误
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void ADC_Data_Handel(void)
{
    // 滤波,因为采样是1ms一次，而PID是10ms一次，所以把10次的采样值平均
    Current_Filter_Buf[Psi_Show_Count] = wADC_Result_Avr_0[0];    // ad_value_current;
    Sensor1_Filter_Buf[Psi_Show_Count] = wADC_Result_Avr_0[2];
    Input_Power[Psi_Show_Count]        = wADC_Result_Avr_0[1];
    Psi_Show_Count                     = Psi_Show_Count + 1;
    if (Psi_Show_Count >= Filter_Num)
    {
        Psi_Show_Count = 0;
    }
    AD_Current_Value_Filtered = Voltage_Current_Filter(Current_Filter_Buf);
    AD_Voltage_Value_Filtered = AD_Current_Value_Filtered;
    AD_Sensor1_Value_Filtered = wADC_Result_Avr_0[2];
    AD_Power_Value_Filtered   = Voltage_Current_Filter(Input_Power);
}

/***************************************************
 *   名称：       Ref_Filter()
 *   功能：       设定值滤波（气压设定值的平滑处理）
 *   函数参数：   无
 *   返回值：     无
 *   相关变量：   float x1, x2, dot_ref;
 ***************************************************/
void Ref_Filter(void)
{
    // 定义阻尼系数r0，用于控制滤波的平滑程度，值越大，系统变化越快
    float r0 = 20.0;     // 阻尼系数，原代码注释掉了值为20.0
    float h  = 0.001;    // 时间步长，决定每次迭代的变化幅度
    float fh1;           // 滤波加速度，用于计算设定值变化率

    // 计算设定值滤波的加速度，基于当前设定气压的误差和设定气压的变化率
    fh1 = -r0 * r0 * (psi_value_set3 - psi_value_set1) - 2 * r0 * dot_ref;

    // 更新平滑后的设定气压值（psi_value_set3）和设定气压的变化率（dot_ref）
    psi_value_set3 = psi_value_set3 + h * dot_ref;    // 平滑后的设定值根据当前的变化率更新
    dot_ref        = dot_ref + h * fh1;               // 变化率根据加速度更新

    // 检查 psi_value_set1 和 psi_value_set3 的关系，并进行约束调整，防止过度平滑导致的不合理设定值
        if (psi_value_set1 >= psi_value_set3)  // 如果目标设定值大于或等于平滑设定值
        {
            // 若 psi_value_set1 大于传感器读数，且传感器值介于 psi_value_set1 和 psi_value_set3 之间，则调整平滑值
            if ((psi_value_set1 > Psi_Sensor1_Val) && (Psi_Sensor1_Val > psi_value_set3))
                psi_value_set3 = Psi_Sensor1_Val;  // 将平滑值调整为传感器的实际读数
            // 否则，如果目标设定值小于或等于传感器读数，则直接将平滑设定值调整为目标值
            else if (psi_value_set1 <= Psi_Sensor1_Val)
                psi_value_set3 = psi_value_set1;
        }
//        else  // 如果目标设定值小于平滑设定值
//        {
//            // 若 psi_value_set1 小于传感器读数，且传感器值介于 psi_value_set1 和 psi_value_set3 之间，则调整平滑值
//            if ((psi_value_set1 < Psi_Sensor1_Val) && (Psi_Sensor1_Val < psi_value_set3))
//                psi_value_set3 = Psi_Sensor1_Val;  // 将平滑值调整为传感器的实际读数
//            // 否则，如果目标设定值大于或等于传感器读数，则直接将平滑设定值调整为目标值
//            else if (psi_value_set1 >= Psi_Sensor1_Val)
//                psi_value_set3 = psi_value_set1;
//        }
}

/***************************************************
 *   名称：       Sensor_Filter()
 *   功能：     传感器滤波
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void Sensor_Filter(void)
{
    float r0, h = 0.001, fh1;

    // 首次调用时初始化滤波器状态，避免启动时产生大的输出脉冲
    if (sensor_filter_initialized == 0)
    {
        x1                        = Psi_Sensor1_Val3;    // 将平滑值初始化为当前传感器值
        x2                        = 0;                   // 变化速率初始化为0
        sensor_filter_initialized = 1;
    }

    r0  = 40.0;            // 阻尼系数,值越大，系统变化越快
    fh1 = -r0 * r0 * (x1 - Psi_Sensor1_Val3) - 2 * r0 * x2;
    x1  = x1 + h * x2;     // 平滑值
    x2  = x2 + h * fh1;    // 变化速率
}

/***************************************************
 *   名称：     PsiShowData_Get()
 *   功能：     计算当前反馈的压力值
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void PsiShowData_Get(void)
{
//    calibration.min_AD           = 362;
//    calibration.max_AD           = 3748;
//    calibration.min_pressure_psi = 0;
//    calibration.max_pressure_psi = 2900.7;
    if ((calibration.max_pressure_psi != 0) && (calibration.max_AD > calibration.min_AD))
    {
        // 1-计算滤波气压值（滤波AD）
        if (AD_Sensor1_Value_Filtered >= calibration.min_AD)
            Psi_Sensor1_Val = (float)(AD_Sensor1_Value_Filtered - calibration.min_AD) * (calibration.max_pressure_psi - calibration.min_pressure_psi) / (calibration.max_AD - calibration.min_AD) / 100;
        else
            Psi_Sensor1_Val = 0;
        // 2-计算瞬时气压值（实时AD）
        if (ADC_Sensor >= calibration.min_AD)
            Psi_Sensor1_Val3 = (float)(ADC_Sensor - calibration.min_AD) * (calibration.max_pressure_psi - calibration.min_pressure_psi) / (calibration.max_AD - calibration.min_AD) / 100;
        else
            Psi_Sensor1_Val3 = 0;
        Sensor_Filter();
    }
}
/***************************************************
 *   名称：       Judge_Input_Error()
 *   功能：       判断输入错误, 判断设定压力值是否超出范围，若超出，标记错误
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void Judge_Input_Error(void)
{
    if ((us_bGet_D2_ADCINMode() == D2_ADCIN0_5V) || (us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_5V))
    {
        if (volt_ref > 5500)    // 输入错误
            ubErrCode |= (1 << 0);
        else
            ubErrCode &= ~(1 << 0);
    }
    else if ((us_bGet_D2_ADCINMode() == D2_ADCIN0_10V) || (us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_10V) || (us_bGet_D2_ADCINMode() == D2_ADCIN_1_10V))
    {
        if (volt_ref > 10500)    // 输入错误
            ubErrCode |= (1 << 0);
        else
            ubErrCode &= ~(1 << 0);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN4_20mA)
    {
        if (((current_ref > 1000) && (current_ref < 3500)) || (current_ref > 20500))    // 输入错误
            ubErrCode |= (1 << 0);
        else
            ubErrCode &= ~(1 << 0);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN0_20mA)
    {
        if (current_ref > 20500)    // 输入错误
            ubErrCode |= (1 << 0);
        else
            ubErrCode &= ~(1 << 0);
    }
    else
    {
        ubErrCode &= ~(1 << 0);
    }
}

/***************************************************
 *   名称：       SetValue_Get()
 *   功能：     计算设定压力值
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void SetValue_Get(void)
{
    static uint8_t timcnt  = 0;
    static uint8_t timcnt2 = 0;
    // 1-根据输入模式，计算当前输入信号
    if (us_bGet_D2_ADCINMode() == D2_ADCIN0_20mA)
    {
        current_ref = us_Adj_ADC_K_B(0, AD_Current_Value_Filtered);
        if (current_ref < 0)
            current_ref = 0;
        else if (current_ref >= 20000)
            current_ref = 20000;
        psi_value_set = (float)(current_ref / 200.0f);    // 计算设定信号百分比
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN4_20mA)
    {
        current_ref = us_Adj_ADC_K_B(0, AD_Current_Value_Filtered);
        if (current_ref < 4000)
            current_ref = 4000;
        else if (current_ref >= 20000)
            current_ref = 20000;
        psi_value_set = (float)((current_ref - 4000) / 160.0f);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN0_5V)
    {
        volt_ref = us_Adj_ADC_K_B(1, AD_Voltage_Value_Filtered);
        if (volt_ref > 5000)
            volt_ref = 5000;
        psi_value_set = (float)(volt_ref / 50.0f);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN0_10V)
    {
        volt_ref = us_Adj_ADC_K_B(1, AD_Voltage_Value_Filtered);
        if (volt_ref > 10000)
            volt_ref = 10000;
        psi_value_set = (float)(volt_ref / 100.0f);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN_1_10V)
    {
        volt_ref = us_Adj_ADC_K_B(1, AD_Voltage_Value_Filtered);
        if (volt_ref < 1000)
        {
            volt_ref = 1000;
        }
        else if (volt_ref > 10000)
        {
            volt_ref = 10000;
        }
        psi_value_set = (float)((volt_ref - 1000) / 90.0f);
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_10V)
    {
        volt_ref = us_Adj_ADC_K_B(1, AD_Voltage_Value_Filtered);
        if (volt_ref > 10000)
        {
            volt_ref = 10000;
        }
        psi_value_set = (float)(volt_ref + 2000) / 120.0;    // 2/12=0.16666
    }
    else if (us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_5V)
    {
        volt_ref = us_Adj_ADC_K_B(1, AD_Voltage_Value_Filtered);
        if (volt_ref > 5000)
        {
            volt_ref = 5000;
        }
        psi_value_set = (float)(volt_ref + 1000) / 60.0;    // 2/12=0.16666
    }

    else
    {
        psi_value_set = 0;
    }
    // 2-检测输入信号是否存在异常
    Judge_Input_Error();
    // 3-计算设定值
    if ((us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_10V) || (us_bGet_D2_ADCINMode() == D2_ADCIN_Neg2_5V))
    {
        // psi_value_set1=psi_value_set/100*ubMaxScalePsi*1.2-ubMaxScalePsi*0.2f;
        psi_value_set1 = (ubMaxScalePsi_PCSet - ubMinScalePsi_PCSet) * (psi_value_set / 120 - 0.2f) + ubMinScalePsi_PCSet;
    }
    else
    {
        psi_value_set1 = psi_value_set / 100 * (ubMaxScalePsi_PCSet - ubMinScalePsi_PCSet) + ubMinScalePsi_PCSet;
    }
    if (psi_value_set1 > 143.6f)
    {
        psi_value_set1 = 143.6f;
    }
    psi_value_set2 = psi_value_set1;
    Ref_Filter();
    if ((ADC_PWM1 < 140) && (ADC_PWM2 < 140) && (psi_value_set1 > psi_value_set_Filter))
    {
        timcnt++;
        if (timcnt > 3)
        {
            psi_value_set_Filter = KalmanFilter(psi_value_set1, 0.005f, 1, 0);
            timcnt               = 3;
        }
    }
    else if (psi_value_set1 < (psi_value_set_Filter - 0.04))
    {
        timcnt = 0;
        timcnt2++;
        if (timcnt2 > 50)
        {
            psi_value_set_Filter = KalmanFilter(psi_value_set1, 0.01f, 1, 0);
            timcnt2              = 0;
        }
    }
    if ((((fabsf(psi_value_set_Filter - psi_value_set3)) / ubMaxScalePsi_PCSet) > 0.03f) || psi_value_set3 < ZeroDeadBand / 2)
    {
        psi_value_set_Filter = psi_value_set3;
        // psi_value_set_Filter=KalmanFilter(psi_value_set1, 0.01f,1, 0);
    }
}
/***************************************************
 *   名称：   MinMaxPsiAdjust(uint8_t ubMin,uint8_t ubMax,float fSet)
 *   功能：    将设定压力值fSet，按比例调整到[ubMin ubMax]之间
 *   函数参数：  fSet  设定压力值
 *          ubMin 最小压力值
 *          ubMax 最大压力值
 *   返回值：    调整后的设定压力值
 ***************************************************/
float MinMaxPsiAdjust1(uint32_t ubMin, uint32_t ubMax, float fSet)
{
    float bias, k;
    if (ubMin <= 20)
    {
        bias = 20.0 - ubMin;
        if (fSet < bias)
            fSet = 0;
        else
        {
            k    = 100.0 / (100.0 - bias);
            fSet = (fSet - bias) * k * ubMax / 40000.0;
        }
    }
    else
        fSet = (ubMin - 20.0) / 400.0 + fSet * (ubMax - ubMin + 20.0) / 40000.0;
    return (fSet);
}
/***************************************************
 *   名称：    Error_Calculator(float Psi_Value_Compared)
 *   功能：     设定压力与反馈压力差
 *   函数参数：Psi_Value_Compared当前反馈压力
 *   返回值：    压力差
 ***************************************************/
float Error_Calculator(float Psi_Value_Compared)
{
    float controller_Error;
    controller_Error = psi_value_set3 - Psi_Value_Compared;
    return (controller_Error);
}
/***************************************************
 *   名称：    ErrorData_Get()
 *   功能：     设定压力与反馈压力差
 *   函数参数：无
 *   返回值：    无
 ***************************************************/
void ErrorData_Get(void)
{
    controller_Error_Handled = Error_Calculator(Psi_Sensor1_Val);
}
// 获取AD转换后的原始值,不进行滤波
uint16_t us_Get_Original_ADC_Chnn(uint8_t bChnn)
{
    if (bChnn >= 5)    // wADC_Result_Avr_0数组大小为5
    {
        return 0;
    }
    return wADC_Result_Avr_0[bChnn];
}
uint16_t us_Get_AveInputADC_Chnn(uint8_t bChnn)
{
    if (bChnn > 3)
    {
        return 0;
    }
    return wADC_Adjust_Input[bChnn];
}

// 进气阀线圈电流的函数
void CheckIntakeValveCoil(uint16_t adcValue)
{
    if (intakeValveCoilCheckFlag == 1)
    {
        intake_Cnt++;
        if (adcValue > CURRENT_THRESHOLD)
        {
            intakeCoilStatus       = 0;    // 线圈无问题
            intakeValveDamageCount = 0;    // 清空错误计次
            intake_Cnt             = 0;
        }
        if (intake_Cnt > 3000)    // 超过1S
        {
            intakeValveCoilCheckFlag = 0;
            intake_Cnt               = 0;
            intakeValveDamageCount++;
            if (intakeValveDamageCount > 3)
            {
                intakeCoilStatus = 1;
            }
        }
    }
}

// 排气气阀线圈电流的函数
void CheckexhaustValveCoil(uint16_t adcValue)
{
    if (exhaustValveCoilCheckFlag == 1)
    {
        exhaust_Cnt++;
        if (adcValue > CURRENT_THRESHOLD)
        {
            exhaustCoilStatus       = 0;    // 线圈无问题
            exhaustValveDamageCount = 0;    // 清空错误计次
            exhaust_Cnt             = 0;
        }
        if (exhaust_Cnt > 3000)    // 超过1S
        {
            exhaustValveCoilCheckFlag = 0;
            exhaust_Cnt               = 0;
            exhaustValveDamageCount++;
            if (exhaustValveDamageCount > 3)
            {
                exhaustCoilStatus = 1;
            }
        }
    }
}

float us_Calculate_Power_Voltage(void)
{
    float Power_Voltage;
    float Decoid_V = 1.1f;    // 压降
#if 0
    // 计算电源电压，参考电源电压为3V，分压电阻为100K和10K
    // 电压计算公式：Vout = (AD值 / 4095) * Vref * (R1 + R2) / R2
    // 其中，Vref = 3.3V, R1 = 10K, R2 = 1K

    float Vref = 3.0f;       // ADC参考电压
    float R1 = 10000.0f;    // 分压电阻R1值（100K）
    float R2 = 1000.0f;     // 分压电阻R2值（10K）

    Power_Voltage = ((float)g_AD_Power / 4095.0f) * Vref * (R1 + R2) / R2+ Decoid_V; // 计算电源电压
#else
    Power_Voltage = ((float)wADC_Result_Avr_0[1] / 4095.0f) * 33.0f + Decoid_V;    // 计算电源电压
#endif
    return Power_Voltage;
}

void us_Read_Power_Voltage_Process(void)
{
    static uint32_t dwLastReadTime = 0;
    static float    AIData[10]     = {0};
    static uint8_t  bIndex         = 0;
    static bool     bBufferFilled  = false;

    uint32_t dwCurrentTime = us_dwGetSystemTime();

    if (dwCurrentTime - dwLastReadTime < 50)    // 50ms读取一次
    {
        return;                                 // 未到读取时间，直接返回
    }

    dwLastReadTime = dwCurrentTime;
    SEGGER_RTT_printf(0,"E2-Write g_Adjust_Flag=%d\r\n",dwCurrentTime);
    // 读取并存储数据
    AIData[bIndex] = us_Calculate_Power_Voltage();
    // 移动索引
    bIndex++;

    // 如果索引到达数组末尾
    if (bIndex >= 10)
    {
        bIndex        = 0;
        bBufferFilled = true;    // 标记缓冲区已填充
    }

    // 只有在缓冲区已填充后才计算平均值
    // 或者可以计算实际有效数据的平均值（使用bIndex作为计数）
    if (bBufferFilled)
    {
        float fSum = 0.0f;

        // 计算10个数据的平均值
        for (uint8_t i = 0; i < 10; i++)
        {
            fSum += AIData[i];
        }

        g_Power_Voltage = fSum / 10.0f;
    }
    else if (bIndex > 0)
    {
        float fSum = 0.0f;
        for (uint8_t i = 0; i < bIndex; i++)
        {
            fSum += AIData[i];
        }
        g_Power_Voltage = fSum / (float)bIndex;
    }
    if (g_Power_Voltage > 30)
    {
        g_Power_Voltage = 30;
    }
    else if (g_Power_Voltage < 19)
    {
        g_Power_Voltage = 19;
    }
    // g_Power_Voltage_Adj = g_Power_Voltage * g_Power_Scale;
}
