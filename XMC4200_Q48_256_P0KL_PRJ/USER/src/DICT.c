#include "DAVE.h"
#include "us_UserConfig.h"
#include "math.h"
PWM_STATUS_t PWM_SetCompare(PWM_t *const handle_ptr, uint32_t compare);

extern volatile uint8_t  intakeValveCoilCheckFlag;     // 进气阀线圈检测标志位
extern volatile uint8_t  exhaustValveCoilCheckFlag;    // 排气阀线圈检测标志位
extern volatile uint16_t intake_Cnt;
extern volatile uint16_t exhaust_Cnt;

extern uint8_t ubErrCode;
extern uint8_t mode;
extern uint8_t pre_mode;

extern float DeadBand;      // 死区值
extern float ZeroOffset;    // 零点偏移值
extern float ubMaxScalePsi;
extern float g_Power_Voltage;

extern ScalePam             scalePam;    // KP\I\D参数
extern controller_struct    global_controller;
extern pressure_calibration calibration;

float psi_value_set;
float psi_value_set1;
float psi_value_set2;
float psi_value_set3;
float psi_value_set_Filter;
float fDeadBand;
float ZeroDeadBand;
float controller_Error_Handled = 0;
float pre_controller_Error_Handled;
float Psi_Sensor1_Val;
float Psi_Sensor1_Val3;

#if 1                            // 版本1
#define MAX_INTEGRATION 3500     // 积分上限，避免积分累加过大
#define MIN_INTEGRATION -3500    // 积分下限，避免积分累加过小
#define PERIOD          8000     // 控制周期，用于限制输出范围
#define PWM_PERIOD      22221    // PWM周期，用于控制阀门

float Set_KP           = 150;    // PID控制器的增益系数：比例150，小量程使用500
float Set_KI           = 2.5;    // PID控制器的增益系数：积分2.5，小量程使用5
float Set_KD           = 800;    // PID控制器的增益系数：微分800，小量程使用1400
float Integration_ctrl = 0;
float up_value;
float ui_value;
float ud_value;
float u_value;                   // PID控制器中的各项控制值
float Input_forward  = 2600;     // 前馈值，用于进气阀的控制
float Output_forward = -2600;    // 前馈值，用于排气阀的控制

int32_t control_value;           // 控制输出值，用于电磁阀的开度调节

/**
 * 名称：PID_Adjust_Control_Value
 * 功能：将当前计算的控制输出量适配到PWM周期值
 * 参数：
 *    - controlValue: 当前的控制输出量
 * 返回值：
 *    - 适配后的PWM控制值
 */
float PID_Adjust_Control_Value(float controlValue)
{
    // 将当前控制信号转换为 PWM 周期内的控制值
    if (controlValue > 10000)
    {
        controlValue = 10000;
    }
    else if (controlValue < 0)
    {
        controlValue = 0;
    }
    return PWM_PERIOD - controlValue * 2.2221f;
}

/**
 * 名称：PI_controller
 * 功能：PID控制器，用于调节系统输出
 * 参数：
 *    - ref_psi: 设定气压
 *    - fact_psi: 实际气压
 *    - controller: 控制器结构体，包含积分值等状态信息
 * 返回值：
 *    - 更新后的控制器结构体
 */
void PI_controller(float ref_psi, float fact_psi)
{
    static float last_error = 0;    // 上一周期的误差值

    float error;                    // 当前误差
    float error_Filter;             // 当前误差
    float scale;
    float period_scale;
    if (g_Power_Voltage < 18.0f)
        scale = 0.75f;    // 最小限制
    else
        scale = g_Power_Voltage / 24.0f;

    // 计算误差值：设定气压和实际气压的差值
    error        = ref_psi - fact_psi;
    error_Filter = ref_psi - Psi_Sensor1_Val;

    // 更新积分值，积分控制部分
    // 如果误差在死区外，才更新积分值，防止积分累积导致控制信号饱和
    if ((error > fDeadBand/2) || error < (-fDeadBand/2))
    {
        Integration_ctrl = Integration_ctrl + error * Set_KI;
    }
//    else                           // if(fabsf(error)<=2*fDeadBand)
//    {
//        Input_Valve_Close_ISR;     // 关闭进气阀
//        Output_Valve_Close_ISR;    // 关闭排气阀
//        if (fabsf(error_Filter) <= fDeadBand)
//        {
//            Integration_ctrl = 0;
//        }
//        return;
//    }

    // 限制积分值在积分上下限之间，避免积分饱和
    if (Integration_ctrl > MAX_INTEGRATION)
        Integration_ctrl = MAX_INTEGRATION;
    else if (Integration_ctrl < MIN_INTEGRATION)
        Integration_ctrl = MIN_INTEGRATION;

    // 计算比例、积分、微分控制量
    up_value = scalePam.Kp * Set_KP * error;                   // 比例控制部分
    ui_value = scalePam.Ki * Integration_ctrl;                 // 积分控制部分
    ud_value = scalePam.Kd * Set_KD * (error - last_error);    // 原始微分项

    // 更新前一周期的误差值，用于微分计算
    last_error = error;

    // 计算总控制信号
    u_value = up_value + ui_value + ud_value;

    // 限制总控制信号在周期范围内
    period_scale = (float)PERIOD / scale;
    // if (u_value > period_scale)
    //     u_value = period_scale;
    // else if (u_value < -period_scale)
    //     u_value = -period_scale;

    // 判断控制信号的正负，决定使用进气阀还是排气阀
    if (error > 0)
    //if((up_value+ui_value)>=0)
    {
        u_value = u_value + Input_forward;    // 加上前馈值
        if (Integration_ctrl < -500)
        {
            Integration_ctrl = -500;
        }
    }
    else                                       // if(u_value<0)
    {
        u_value = u_value + Output_forward;    // 加上前馈值
        if (Integration_ctrl > 500)
        {
            Integration_ctrl = 500;
        }
    }
    if (((u_value >= 0)))    //&&(error>0)))
    {
        u_value = u_value / scale;
        if (u_value > period_scale)
            u_value = period_scale;

        // 对control_value进行处理，适配PWM周期
        control_value = PID_Adjust_Control_Value(u_value);

        Output_Valve_Close_ISR;                // 关闭排气阀
        Input_Valve_PWM_ISR(control_value);    // 使用 PWM 控制进气阀开度
    }
    else                                       // if(((u_value<0)&&(error<0)))
    {
        u_value = u_value / scale;
        if (u_value < -period_scale)
            u_value = -period_scale;
        // 对control_value进行处理，适配PWM周期
        control_value = PID_Adjust_Control_Value(-u_value);
        Input_Valve_Close_ISR;                  // 关闭进气阀
        Output_Valve_PWM_ISR(control_value);    // 使用 PWM 控制排气阀开度
    }

//     if (error_Filter >= fDeadBand)
//     {
//         Output_Valve_Close_ISR;    // 关闭排气阀
//     }
//     else if (error_Filter <= -fDeadBand)
//     {
//         Input_Valve_Close_ISR;    // 关闭进气阀
//     }
    return;
}

#endif

void Cal_DeadBand(void)
{
    fDeadBand = DeadBand * ubMaxScalePsi / 100;
    if (us_bGet_D2_ADCINMode() == D2_ADCIN0_10V)
    {
        if (fabsf(ZeroOffset - 0.5f) < 0.001f)
        {
            ZeroOffset = 0.6;    // 这个地方发现仍存在个别板子突破55mV，针对0-10V信号做特殊处理
        }
    }
    ZeroDeadBand = ZeroOffset * ubMaxScalePsi / 100.0f;
}

void PWM_DutyCon(void)
{
    float temp_Set;
    if (psi_value_set3 < 3 * ZeroDeadBand)
    {
        temp_Set = psi_value_set_Filter;
    }
    else
    {
        temp_Set = psi_value_set3;
    }
    if (temp_Set <= ZeroDeadBand)
    {
        Input_Valve_Close_ISR;
        if ((-controller_Error_Handled) > ZeroDeadBand / 2)
        {
            if (controller_Error_Handled < pre_controller_Error_Handled + 0.1)
            {
                Output_Valve_Open_ISR;
            }
            else
            {
                global_controller.counter++;
                if ((global_controller.counter > 2000))
                    global_controller.counter = 0;
                if (global_controller.counter > 1000)
                    Output_Valve_Close_ISR;
                else
                    Output_Valve_Open_ISR;
            }
            pre_controller_Error_Handled = controller_Error_Handled;
            mode                         = 1;
        }
        else if (global_controller.counter < 1500)
        {
            Output_Valve_Open_ISR;
            global_controller.counter++;
            mode = 2;
        }
        else
        {
            Output_Valve_Close_ISR;
            ubErrCode = ubErrCode & (~0x04);
            mode      = 3;
        }
        Integration_ctrl = 0;
    }
    else
    {
        mode = 4;
        if (mode != pre_mode)
            global_controller.counter = 0;
        PI_controller(temp_Set, Psi_Sensor1_Val3);
    }
    pre_mode = mode;
}

void Input_Valve_Open(void)
{
    PWM1_SetCompare(0);
}

void Input_Valve_Close(void)
{
    PWM1_SetCompare(period);
}
void Output_Valve_Close(void)
{
    PWM2_SetCompare(period);
}
void Output_Valve_Open(void)
{
    PWM2_SetCompare(0);
}

void Output_Valve_Open_EXH(void)
{
    PWM2_SetCompare(15000);
}

void Input_Valve_PWM(uint32_t Compare)
{
    PWM1_SetCompare(Compare);
}
void Output_Valve_PWM(uint32_t Compare)
{
    PWM2_SetCompare(Compare);
}

PWM_STATUS_t PWM_SetCompare(PWM_t *const handle_ptr, uint32_t compare)
{
    uint32_t temp_period;
    temp_period = (uint32_t)handle_ptr->period_value + 1U;
    if (compare > temp_period)
    {
        compare = temp_period;
    }
    handle_ptr->ccu4_slice_ptr->CRS   = (uint32_t)compare;
    handle_ptr->ccu4_kernel_ptr->GCSS = (uint32_t)handle_ptr->shadow_mask;
    return (0);
}

/*Sets the frequency and duty cycle for CCU4_CC4 slice Symmetric Mode. */
PWM_CCU4_STATUS_t PWM_CCU4_SetCompare(PWM_CCU4_t *handle_ptr, uint32_t compare)
{
    uint32_t temp_period;
    temp_period = handle_ptr->config_ptr->period_value + 1U;
    if (compare > temp_period)
    {
        compare = temp_period;
    }
    handle_ptr->ccu4_slice_ptr->CRS   = (uint32_t)compare;
    handle_ptr->ccu4_module_ptr->GCSS = (uint32_t)handle_ptr->shadow_txfr_msk;

    return (0);
}

void PWM1_SetCompare(uint32_t compare)
{
    uint32_t compare_tmp;
    compare_tmp = (period - compare) * 0.5f;
    PWM_SetCompare(&PWM_0, compare);
    PWM_CCU4_SetCompare(&PWM_CCU4_0, compare_tmp);
    if (compare < 16000)
    {
        // 启动线圈检测功能
        intakeValveCoilCheckFlag = 1;
    }
    else
    {
        intakeValveCoilCheckFlag = 0;
        intake_Cnt               = 0;
    }
}

void PWM2_SetCompare(uint32_t compare)
{
    uint32_t compare_tmp;
    compare_tmp = (period - compare) * 0.5f;
    PWM_SetCompare(&PWM_1, compare);
    PWM_CCU4_SetCompare(&PWM_CCU4_1, compare_tmp);
    if (compare < 16000)
    {
        // 启动线圈检测功能
        exhaustValveCoilCheckFlag = 1;
    }
    else
    {
        exhaustValveCoilCheckFlag = 0;
        exhaust_Cnt               = 0;
    }
}
