#include "us_UserConfig.h"
#include "math.h"

extern uint8_t f_communicate;
extern uint8_t ubErrCode;

extern float controller_Error_Handled;
extern float psi_value_set1;
extern float psi_value_set3;
extern float Psi_Sensor1_Val;
extern float Psi_Sensor1_Val3;
extern float fDeadBand;
extern float ZeroDeadBand;
extern float ubMaxScalePsi_PCSet;
extern float ubMinScalePsi_PCSet;

extern SystemState          Sys_state;
extern pressure_calibration calibration;

volatile uint8_t f_start = 0;

float Psi_Sensor1_Val_Filter;

void    DAC_Output_Handle(void);
uint8_t us_PowerOn_Delay_SetFlag(void);
void    us_ReachLED_Status_Process(void);

int main(void)
{
    static uint32_t Sample_Time  = 0;
    static uint32_t DAC_Time     = 0;
    uint8_t Send_Data[] = "Infineon";
    uint32_t        dwGetSYSTime = 0;
    DAVE_Init();
    us_Init();
    Mem_Init();
    EEP_Read();
    Input_Valve_Close();
    Output_Valve_Close();
    Cal_DeadBand();
    WATCHDOG_Start();
    USBD_VCOM_Connect();
    while (1)
    {
#if 1
        if (f_start == 0)    // 上电延时200ms读取设定值
        {
            f_start = us_PowerOn_Delay_SetFlag();
        }
        dwGetSYSTime = us_dwGetSystemTime();
        if ((dwGetSYSTime - Sample_Time) >= 1)    // 1ms
        {
            ADC_Data_Handel();
            PsiShowData_Get();
            if (f_start == 1)
            {
                SetValue_Get();
                ErrorData_Get();
            }
            Sample_Time = dwGetSYSTime;
        }
        // 5ms刷新输出一次DAC
        if ((dwGetSYSTime - DAC_Time) >= 1)
        {
            DAC_Time = dwGetSYSTime;
            if ((f_start == 1) && (f_communicate == 0) && (calibration.fCalSave == 1))
            {
                DAC_Output_Handle();
            }
//            UART_Transmit(&UART_0, Send_Data, sizeof(Send_Data)-1);
        }
#endif
        us_bADC_Process();
        us_Read_Power_Voltage_Process();
        us_USB_Process();
        us_LED_Status_Process();
        us_ReachLED_Status_Process();
        WATCHDOG_Service();
    }
    return 0;
}

// uint32_t Debug_Input_Forward=22221;
// uint32_t Debug_Output_Forward=22221;
// 3600hz

void PWM_IN_ISR(void)
{
    static uint8_t timCnt_277us = 0;
    if ((f_start == 1) && (f_communicate == 0) && (calibration.fCalSave == 1))
    {
        timCnt_277us++;
        if (timCnt_277us >= 4)
        {
            timCnt_277us = 0;
            PWM_DutyCon();
        }
    }
    // Input_Valve_PWM_ISR(Debug_Input_Forward);
    // Output_Valve_PWM_ISR(Debug_Output_Forward);
}

void DAC_Output_Handle(void)
{
    float          tempf;
    uint16_t       temp;
    float          scaleRange;
    static uint8_t filterInitialized = 0;

    if ((us_bGetDACFlag(1) == 0xA5))
    {
        // 首次调用时用实际值初始化滤波器，避免从0开始的过渡问题
        if (!filterInitialized)
        {
            Psi_Sensor1_Val_Filter = Psi_Sensor1_Val;
            filterInitialized      = 1;
        }
        else
        {
            Psi_Sensor1_Val_Filter = Psi_Sensor1_Val_Filter * 0.95f + Psi_Sensor1_Val * 0.05f;
        }

        // 计算量程范围，防止除零
        scaleRange = ubMaxScalePsi_PCSet - ubMinScalePsi_PCSet;
        if (fabsf(scaleRange) < 0.1f)
        {
            return;
        }

        if (us_bGet_D2_DACFeedbackMode() == D2_DAC4_20mA)
        {
            tempf = 16000.0f * (Psi_Sensor1_Val_Filter - ubMinScalePsi_PCSet) / scaleRange + 4000.0f;
            if (tempf > 20500.0f)
                tempf = 20500.0f;
            else if (tempf < 4000.0f)
                tempf = 4000.0f;
            temp = (uint16_t)tempf;
            us_bSet_DACValue(temp, mA_Current, 1);
        }
        else if (us_bGet_D2_DACFeedbackMode() == D2_DAC0_10V)
        {
            tempf = 10000.0f * (Psi_Sensor1_Val_Filter - ubMinScalePsi_PCSet) / scaleRange;
            if (tempf > 10500.0f)
                tempf = 10500.0f;
            else if (tempf < 0.0f)
                tempf = 0.0f;
            temp = (uint16_t)tempf;
            us_Set_DAC_Temp(temp, mV_Voltage, 1);
        }
        else if (us_bGet_D2_DACFeedbackMode() == D2_DAC1_10V)
        {
            tempf = 9000.0f * (Psi_Sensor1_Val_Filter - ubMinScalePsi_PCSet) / scaleRange + 1000.0f;
            if (tempf > 10500.0f)
                tempf = 10500.0f;
            else if (tempf < 1000.0f)
                tempf = 1000.0f;
            temp = (uint16_t)tempf;
            us_Set_DAC_Temp(temp, mV_Voltage, 1);
        }
    }
}

/**
 * @brief 检查上电延迟是否完成
 *
 * 此函数用于实现上电后的固定延迟（默认为200ms）。
 * 首次调用时记录起始时间，后续调用检查是否已超过延迟时间。
 *
 * @return uint8_t 1表示延迟完成，0表示延迟未完成
 */
uint8_t us_PowerOn_Delay_SetFlag(void)
{
    static uint32_t powerOnStartTime = 0;
    static uint8_t  isInitialized    = 0;
    static uint8_t  isDelayComplete  = 0;
    uint32_t        currentTime;

    // 如果延迟已完成，直接返回
    if (isDelayComplete)
    {
        return 1;
    }
    // 首次调用时初始化起始时间
    if (!isInitialized)
    {
        powerOnStartTime = us_dwGetSystemTime();
        isInitialized    = 1;
        return 0;    // 首次调用，延迟肯定未完成
    }
    // 检查是否达到延迟时间（200ms）
    currentTime = us_dwGetSystemTime();
    if ((currentTime - powerOnStartTime) > 200)
    {
        isDelayComplete = 1;
    }
    return isDelayComplete;
}

void us_ReachLED_Status_Process(void)
{
    uint32_t        dwGetSysTime;
    static uint32_t dwLEDShowTime = 0;
    static float    error1        = 0.0f;

    dwGetSysTime = us_dwGetSystemTime();

    if ((Sys_state == STATE_NORMAL_OPERATION) && ((ubErrCode == 0) || (ubErrCode == 0x01)))
    {
        // 正常操作模式，每100ms检测一次
        if (dwGetSysTime - dwLEDShowTime < 100)
        {
            return;
        }
        dwLEDShowTime = dwGetSysTime;

        float currentError = psi_value_set1 - Psi_Sensor1_Val3;
        error1             = 0.8f * error1 + 0.2f * currentError;

        // 如果滤波值与实际误差符号相反，重置
        if ((error1 * currentError) < 0.0f)
        {
            error1 = currentError;
        }

        if ((fabsf(error1) < 3.0f * fDeadBand) ||
            ((psi_value_set1 <= ZeroDeadBand) && (Psi_Sensor1_Val <= ZeroDeadBand)))
        {
            DIGITAL_IO_SetOutputLow(&IO_LED_REACH);
            ubErrCode &= ~0x04;    // 清除Er2标志
        }
        else
        {
            DIGITAL_IO_SetOutputHigh(&IO_LED_REACH);
        }
    }
}
