/*
 * us_LED.c
 *
 *  Created on: May 24, 2024
 *      Author: cosys engineer4
 */
#include "us_UserConfig.h"

#define LED_ON  1
#define LED_OFF 0

extern uint8_t intakeCoilStatus;
extern uint8_t exhaustCoilStatus;
extern uint8_t ubErrCode;
extern uint8_t ADJ_DAC_Flag[2];

extern mcu_type mcu_Data;
extern ADCAdjT  ADJ_ADC[2];

SystemState  Sys_state = STATE_UNCALIBRATED;
HandSetFlags HandSet_Flag;
// 模拟的控制LED的函数
void Set_Red_LED(uint8_t state);
void Set_Green_LED(uint8_t state);

// 标定状态和其他条件的模拟函数
uint8_t is_sensor_calibrated();
uint8_t is_control_signal_calibrated();
uint8_t is_feedback_signal_calibrated();
uint8_t is_coil_detected();

uint8_t is_Hand_Set_Range();
uint8_t is_Hand_Set_Control_Signal_Type();
uint8_t is_Hand_Set_Feedback_Signal_Type();
uint8_t is_Hand_Set_Product_Type();
void    upData_System_Status(void);

uint16_t StartFrame = 1500;

void us_LED_Status_Process(void)
{
    uint32_t        dwGetSysTime;
    static uint32_t dwLEDShowTime = 0;
    // 记录时间的变量
    static uint32_t uncalibrated_timer = 0;
    uint32_t        local_time;
    dwGetSysTime = us_dwGetSystemTime();
    if (dwGetSysTime - dwLEDShowTime < 50)    // 50ms进入一次
    {
        goto Exit;
    }
    dwLEDShowTime = dwGetSysTime;

    upData_System_Status();
    // 每50ms调用一次该函数
    switch (Sys_state)
    {
    case STATE_UNCALIBRATED:
        // 处理未标定状态
        local_time = uncalibrated_timer % 6300;    // 一个周期为6000ms
        StartFrame = 2000;
        if (local_time < StartFrame)
        {
            if ((local_time < 150) ||
                (local_time >= 300 && local_time < 450) ||
                (local_time >= 900 && local_time < 1050) ||
                (local_time >= 1200 && local_time < 1350))
            {
                Set_Green_LED(LED_ON);
            }
            else
            {
                Set_Green_LED(LED_OFF);
            }
        }
        else
        {
            // 状态显示
            if (local_time == StartFrame)
            {
                // 判断是否标定传感器
                if (is_sensor_calibrated())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 1000))
            {
                // 判断是否标定控制信号
                if (is_control_signal_calibrated())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 1800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 2000))
            {
                // 判断是否标定反馈信号
                if (is_feedback_signal_calibrated())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 2800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 3000))
            {
                // 判断是否检测到线圈
                if (is_coil_detected())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 3800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
        }

        break;
    case STATE_UNSET:
        // 处理未标定状态
        StartFrame = 2600;
        local_time = uncalibrated_timer % 6750;    // 一个周期为7000ms
        if (local_time < StartFrame)
        {
            if ((local_time < 150) || (local_time >= 300 && local_time < 450) || (local_time >= 600 && local_time < 750) || (local_time >= 1200 && local_time < 1350) || (local_time >= 1500 && local_time < 1650) || (local_time >= 1800 && local_time < 1950))
            {
                Set_Green_LED(LED_ON);
            }
            else
            {
                Set_Green_LED(LED_OFF);
            }
        }
        else
        {
            // 状态显示
            if (local_time == StartFrame)
            {
                // 判断是否手动设置量程
                if (is_Hand_Set_Range())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 1000))
            {
                // 判断是否手动设置控制信号
                if (is_Hand_Set_Control_Signal_Type())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 1800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 2000))
            {
                // 判断是否手动设置反馈信号
                if (is_Hand_Set_Feedback_Signal_Type())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 2800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
            else if (local_time == (StartFrame + 3000))
            {
                // 判断是否手动设置产品功能码
                if (is_Hand_Set_Product_Type())
                {
                    Set_Green_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_ON);
                }
            }
            else if (local_time == (StartFrame + 3800))
            {
                Set_Green_LED(LED_OFF);
                Set_Red_LED(LED_OFF);
            }
        }
        break;
    case STATE_NORMAL_OPERATION:
        if (ubErrCode != 0)
        {
            if ((ubErrCode & 0x01) != 0)    // 模拟量超量程
            {
                local_time = uncalibrated_timer % 2000;
                if (local_time < 1000)
                {
                    Set_Red_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_OFF);
                }
            }
            else if ((ubErrCode & 0x04) != 0)    // 无法到达设定值
            {
                local_time = uncalibrated_timer % 1000;
                if (local_time < 500)
                {
                    Set_Red_LED(LED_ON);
                }
                else
                {
                    Set_Red_LED(LED_OFF);
                }
                Set_Green_LED(LED_OFF);
            }
            else if ((ubErrCode & 0x02) != 0)    // EEPROM错误
            {
                Set_Red_LED(LED_ON);
                Set_Green_LED(LED_OFF);
            }
            else if ((ubErrCode & 0x08) != 0)    // 线圈故障
            {
                local_time = uncalibrated_timer % 1000;
                if (local_time < 500)
                {
                    Set_Red_LED(LED_ON);
                    Set_Green_LED(LED_OFF);
                }
                else
                {
                    Set_Green_LED(LED_ON);
                    Set_Red_LED(LED_OFF);
                }
            }
        }
        break;
    default:
        break;
    }
    uncalibrated_timer += 50;
Exit:;
}

void Set_Red_LED(uint8_t state)
{
    if (state == LED_ON)
    {
        DIGITAL_IO_SetOutputLow(&IO_LED_STATUS);
    }
    else
    {
        DIGITAL_IO_SetOutputHigh(&IO_LED_STATUS);
    }
}
void Set_Green_LED(uint8_t state)
{
    if (state == LED_ON)
    {
        DIGITAL_IO_SetOutputLow(&IO_LED_REACH);
    }
    else
    {
        DIGITAL_IO_SetOutputHigh(&IO_LED_REACH);
    }
}

// 标定状态和其他条件的模拟函数
uint8_t is_sensor_calibrated()
{
    // 判断传感器是否标定
    uint8_t status;
    if (mcu_Data.flag == 0xa5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}
uint8_t is_control_signal_calibrated()
{
    // 判断控制信号是否标定
    uint8_t status;
    if (ADJ_ADC[0].bFlag == 0xA5 || ADJ_ADC[1].bFlag == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }

    return status;
}
uint8_t is_feedback_signal_calibrated()
{
    // 判断反馈信号是否标定
    uint8_t status;
    if (ADJ_DAC_Flag[0] == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}
uint8_t is_coil_detected()
{
    // 判断是否检测到线圈
    uint8_t status;
    if ((intakeCoilStatus == 1) || (exhaustCoilStatus == 1))
    {
        status = 0;
    }
    else
    {
        status = 1;
    }
    return status;
}

uint8_t is_Hand_Set_Range()
{
    // 判断是否已手动设定量程
    uint8_t status;
    if (HandSet_Flag.Hand_RangeFlag == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}

uint8_t is_Hand_Set_Control_Signal_Type()
{
    // 判断是否已手动设定控制信号类型
    uint8_t status;
    if (HandSet_Flag.Hand_ControlFlag == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}

uint8_t is_Hand_Set_Feedback_Signal_Type()
{
    // 判断是否已手动设定反馈信号类型
    uint8_t status;
    if (HandSet_Flag.Hand_FeedbackFlag == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}

uint8_t is_Hand_Set_Product_Type()
{
    // 判断是否已手动设定产品功能码
    uint8_t status;
    if (HandSet_Flag.Hand_ProductFlag == 0xA5)
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    return status;
}

void upData_System_Status(void)
{
    if (is_sensor_calibrated() == 0 ||
        is_control_signal_calibrated() == 0 ||
        is_feedback_signal_calibrated() == 0 ||
        is_coil_detected() == 0)
    {
        Sys_state = STATE_UNCALIBRATED;
    }
    else if (is_Hand_Set_Range() == 0 || is_Hand_Set_Control_Signal_Type() == 0 || is_Hand_Set_Feedback_Signal_Type() == 0 || is_Hand_Set_Product_Type() == 0)
    {
        Sys_state = STATE_UNSET;
    }
    else
    {
        Sys_state = STATE_NORMAL_OPERATION;
    }
}
