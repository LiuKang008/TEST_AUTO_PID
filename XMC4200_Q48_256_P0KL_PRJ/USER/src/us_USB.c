///*
// * us_USB.c
// *
// *  Created on: 2015-3-25
// *      Author: Administrator
// */
//
#include "us_UserConfig.h"

extern uint8_t ubErrCode;
extern uint8_t AdjustADCFlag;    // 开始校准AD标志位

extern int16_t  DAC_Adj_Valu_Temp[2][32];
extern uint16_t wADC_Result_Avr_0[5];    // ADC采样结果数组

extern uint32_t ubMinScalePsi10;
extern uint32_t ubMaxScalePsi10;

extern eeprom_flash_data    EepFlashDat;
extern pressure_calibration calibration;
extern HandSetFlags         HandSet_Flag;

uint8_t Product_Type;
uint8_t Version[2]      = {26, 01};
int8_t  RxBuffer[64]    = {0};
uint8_t bReData[8]      = {0};
uint8_t ADJ_DAC_Flag[2] = {0, 0};    // DAC校准 标识  为0表明在进行校准  非零则完成校准
uint8_t bDACMoade       = DAC1C;
uint8_t bADCINMode      = ADCIN4_20mA;
uint8_t f_communicate   = 0;

uint16_t wOutPutValue[2] = {0, 0};    // 保存当前设置的DAC值
uint16_t wPWMFreg        = 5000;      // PWM频率
uint16_t wDutyCycle      = 5000;      // PWM占空比
uint16_t wPWMFreg2       = 5000;      // PWM频率
uint16_t wDutyCycle2     = 5000;      // PWM占空比
// 控制信号
// 0:4-20mA
// 1:0-20mA
// 2:0-5V
// 3:0-10V
// 4:-2-5V
// 5:-2-10V
// 6:1-10V

// 反馈信号
// 0:4-20mA
// 1:0-10V
// 2:1-10V
uint16_t controlType;              // 反馈信号类型
uint16_t FeedbackType;             // 反馈信号类型

float  fDeadBand_Percent = 0.1;    // 默认±0.1%,控制死区百分比
float  fZeroBand_Percent = 0.5;    // 默认0.5%
float  Out_Min;                    // 最小输出
float  Out_Max;                    // 最大输出
float  ubMaxScalePsi_PCSet;
float  ubMinScalePsi_PCSet;
float  DeadBand   = 0.2f;    // 死区值
float  ZeroOffset = 0.5f;    // 零点偏移值
double MCU_k[2]   = {0, 0};
double MCU_b[2]   = {0, 0};

ScalePam scalePam;    // KP\I\D参数
ID3      bmyID3;
mcu_type mcu_Data;
ADCAdjT  ADJ_ADC[2];

static void us_Decode(uint8_t *bpCommand, uint8_t bLen);
static void us_Respond(uint8_t bCommand1, error_Code error, uint16_t wReData);
static void us_PPCAdjust(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCGetADC(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCGetModel(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCOutPut(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCGetADC_Model_01(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCGetADC_Unit(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCE2_Clear(uint8_t *bpCommand2, uint8_t bLen);
static void us_MCUE2ID(uint8_t *bpCommand2, uint8_t bLen);
static void us_MCUE2Value(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPC_ADC_Adj(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPC_DAC_Adj(uint8_t *bpCommand2, uint8_t bLen);
static void us_bGetDACAdjFlag(void);
static void us_SetMCU_K_B(mcu_type mcu_temp, uint8_t bFlag);
static void us_PPCGetDACFlag(uint8_t *bpCommand2, uint8_t bLen);            // 上位机读写DAC校准标志位
static void us_PPCGetDAC_Adj_Value(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPCGetADCFlag(uint8_t *bpCommand2, uint8_t bLen);            // 上位机读取ADC校准标志位
static void us_PPCGetMCUFlag(uint8_t *bpCommand2, uint8_t bLen);            // 上位机读取气压校准标志位
static void us_PPCGet_Original_Value(uint8_t *bpCommand2, uint8_t bLen);    // 读取AD转换后的原始值。
static void us_Product_Selection(uint8_t *bpCommand2, uint8_t bLen);
static void us_D2_Parameter(uint8_t *bpCommand2, uint8_t bLen);
static void us_PPC_PID(uint8_t *bpCommand2, uint8_t bLen);                                                  // 上位机设置或读取KP、KI、KD比例参数
static void us_PPC_DeadBand(uint8_t *bpCommand2, uint8_t bLen);                                             // 上位机设置或读取死区参数
static void us_PPC_ZeroOffset(uint8_t *bpCommand2, uint8_t bLen);                                           // 上位机设置或读取零点偏移参数
static void us_Respond_Seting(uint8_t bCommand1, uint8_t bCommand2, error_Code error, uint16_t wReData);    // 为用户设置数据返回数据

double us_Adj_ADC_K_B(uint8_t bChnn, double dlData);
double us_Adj_ADC_To_Pc(uint8_t bChnn, double dlData);

/***************************************************
 *   名称：      	us_USBInit()
 *   功能：		usb模块的初始化
 *   函数参数：	void
 *   返回值：	错误代码
 ***************************************************/
uint8_t us_USBInit(void)
{
    uint8_t i             = 0;
    uint8_t temp[32]      = {0};
    uint8_t bWriteData[4] = {0xA5};    // 闁告劖鐟ラ崣鍡涘极閻楀牆绁�
    uint8_t bReadData[4]  = {0};       // 閻犲洩顕цぐ鍥极閻楀牆绁�
    /*上电检测EEPROM是否存在异常*/
    for (i = 0; i < 5; i++)
    {
        I2CWriteData(0xa0, TestEEPROM, bWriteData, 1);
        us_Delay_5ms();
        I2CReadData(0xa0, TestEEPROM, bReadData, 1);
        if (temp[0] == 0xA5)
        {
            ubErrCode |= 0x02;
            break;
        }
        else
        {
            ubErrCode = ubErrCode & (~0x02);
        }
    }

    us_bI2c_Read_Stream(mcu_range_max, (uint8_t *)(temp), 16);
    us_bI2c_Read_Stream(mcu_range_max, (uint8_t *)(temp), 16);
    for (i = 0; i < 4; i++)
    {
        if (wCRCCheck(temp + i * 4, 4) == 0)
        {
            *(((uint16_t *)&mcu_Data.range_max) + i) = (uint16_t)(temp[i * 4] | (temp[i * 4 + 1] << 8));
        }
    }
    us_bI2c_Read_Stream(mcu_id1, (uint8_t *)(temp), 20);
    for (i = 0; i < 5; i++)
    {
        if (wCRCCheck(temp + i * 4, 4) == 0)
        {
            *(((uint16_t *)&mcu_Data.id1) + i) = (uint16_t)(temp[i * 4] | (temp[i * 4 + 1] << 8));
        }
    }
    us_bI2c_Read_Stream(mcu_max_ADC_01, (uint8_t *)(temp), 16);
    if (wCRCCheck((uint8_t *)(temp), 16) == 0)
    {
        for (i = 0; i < 7; i++)
        {
            *(((uint16_t *)&mcu_Data.max_ADC_01) + i) = (uint16_t)(temp[i * 2] | (temp[i * 2 + 1] << 8));
        }
        if (mcu_Data.flag == 0xa5)
        {
            us_SetMCU_K_B(mcu_Data, 0);
        }
    }
    us_bI2c_Read_Stream(ADC_Adj_S, (uint8_t *)ADJ_ADC, sizeof(ADJ_ADC));
    for (i = 0; i < 2; i++)
    {
        if (wCRCCheck((uint8_t *)ADJ_ADC + i * sizeof(ADCAdjT), sizeof(ADCAdjT)) != 0)
        {
            ADJ_ADC[i].bFlag = 0;
        }
    }

    /*产品类型读取*/
    us_bI2c_Read_Stream(product_type, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        Product_Type = temp[0];
        HandSet_Flag.Hand_ProductFlag |= 0x01;
    }
    else
    {
        Product_Type = 2;    // 2;//设定一个默认的设备，或置0xFF表示无设备类型写入
    }
    us_bI2c_Read_Stream(E2_Control_DeadBand_Percent, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        fDeadBand_Percent = (temp[0] + (temp[1] << 8)) / 10.0f;
        HandSet_Flag.Hand_ProductFlag |= 0x02;
    }

    us_bI2c_Read_Stream(E2_Zero_DeadBand_Percent, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        fZeroBand_Percent = (temp[0] + (temp[1] << 8)) / 10.0f;
        HandSet_Flag.Hand_ProductFlag |= 0x04;

        if ((HandSet_Flag.Hand_ProductFlag & 0x07) == 0x07)
        {
            HandSet_Flag.Hand_ProductFlag = 0xA5;
        }
    }

    us_bI2c_Read_Stream(E2_Control_Signal_type, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        controlType                   = (uint16_t)(temp[0] + (temp[1] << 8));
        HandSet_Flag.Hand_ControlFlag = 0xA5;
    }
    else
    {
        controlType = 0xFF;
    }
    us_bI2c_Read_Stream(E2_Feedback_Signal_type, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        FeedbackType                   = (uint16_t)(temp[0] + (temp[1] << 8));
        HandSet_Flag.Hand_FeedbackFlag = 0xA5;
    }
    else
    {
        FeedbackType = 0xFF;
    }
    us_bI2c_Read_Stream(E2_Out_Min, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        Out_Min = (float)(temp[0] + (temp[1] << 8));

        ubMinScalePsi_PCSet = Out_Min * 0.145f;
    }
    us_bI2c_Read_Stream(E2_Out_Max, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
    {
        Out_Max                     = (float)(temp[0] + (temp[1] << 8));
        ubMaxScalePsi_PCSet         = Out_Max * 0.145f;
        HandSet_Flag.Hand_RangeFlag = 0xA5;
    }
    /****读取KP KI KD 死区 及零点偏移*****/
    us_bI2c_Read_Stream(SCALE_KP, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
        scalePam.Kp = (float)((temp[0] | (temp[1] << 8)) / 10.0f);
    else
        scalePam.Kp = 1;
    us_bI2c_Read_Stream(SCALE_KI, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
        scalePam.Ki = (float)((temp[0] | (temp[1] << 8)) / 10.0f);
    else
        scalePam.Ki = 1;
    us_bI2c_Read_Stream(SCALE_KD, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
        scalePam.Kd = (float)((temp[0] | (temp[1] << 8)) / 10.0f);
    else
        scalePam.Kd = 1;
    us_bI2c_Read_Stream(DEAD_BAND, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
        DeadBand = (float)((temp[0] | (temp[1] << 8)) / 10.0f);
    else
        DeadBand = 0.2f;    // 0.2%
    us_bI2c_Read_Stream(ZERO_OFFSET, (uint8_t *)(temp), 4);
    if (wCRCCheck(temp, 4) == 0)
        ZeroOffset = (float)((temp[0] | (temp[1] << 8)) / 10.0f);
    else
        ZeroOffset = 0.5f;    // 0.5%

    us_bGetDACAdjFlag();
    return 0;
}

///***************************************************
// *   名称：      	us_USB_Process()
// *   功能：		usb模块轮询程序
// *   函数参数：	void
// *   返回值：	void
// ***************************************************/
void us_USB_Process(void)
{
    uint16_t Bytes = 0;
    uint16_t Crc;
    Bytes = USBD_VCOM_BytesReceived();
    if (Bytes == 8)
    {
        USBD_VCOM_ReceiveData(RxBuffer, 8);
        //	memcpy(bData, RxBuffer, 8);
        if (RxBuffer[6] != 0x0D || RxBuffer[7] != 0x0A)
        {
            us_Respond((uint8_t)RxBuffer[0], UnknowCommand, 0);
        }
        else
        {
            Crc = (((uint8_t)RxBuffer[4]) | ((uint8_t)RxBuffer[5] << 8));
            if (wCRCCheck_Uart_Data((uint8_t *)RxBuffer, 4) != Crc)
            {
                us_Respond((uint8_t)RxBuffer[0], CRCFail, 0);
            }
            else
            {
                us_Decode((uint8_t *)RxBuffer, 4);
                f_communicate = 1;
            }
        }
    }
    else if (Bytes > 8)
    {
        USBD_VCOM_ReceiveData(RxBuffer, Bytes);
    }
    CDC_Device_USBTask(&USBD_VCOM_cdc_interface);
    // Eixt:
    //	;
}
/***************************************************
 *   名称：      	us_Decode()
 *   功能：		根据接收到的命令进行解码
 *   函数参数：	uint8_t *bpCommand  接收到的命令指针
 *   			uint8_t bLen		        接收到的命令长度
 *   返回值：	void
 ***************************************************/
static void us_Decode(uint8_t *bpCommand, uint8_t bLen)
{
    uint8_t bCommand1 = *bpCommand;
    if (bLen != 4)
    {
        us_Respond(bCommand1, UnknowCommand, 0);
        goto Exit;
    }
    switch (bCommand1)
    {
    case 0x01:
        break;
    case 0x02:
        break;
    case 0x03:
        break;
    case 0x04:
        break;
    case 0x05:
        us_PPCAdjust(bpCommand, 4);
        break;
    case 0x06:
        break;
    case 0x07:
        break;
    case 0x08:
        us_PPCGetADC(bpCommand, 4);
        break;
    case 0x09:
        us_PPCGetModel(bpCommand, 4);
        break;
    case 0x0a:
        us_PPCOutPut(bpCommand, 4);    // 强制输出4-20mA
        break;
    case 0x0b:
        break;
    case 0x0c:
        break;
    case 0x0d:
        break;
    case 0x0e:
        break;
    case 0x0f:
        break;
    case 0x10:
        us_PPCGetADC_Model_01(bpCommand, 4);
        break;
    case 0x11:
        us_PPCGetADC_Unit(bpCommand, 4);
        break;
    case 0x12:    // D2产品，上位机设定参数
        us_D2_Parameter(bpCommand, 4);
        break;
    case 0x13:
        break;
    case 0x14:
        break;
    case 0x15:
        us_PPCE2_Clear(bpCommand, 4);
        break;
    case 0x16:    // 设置KP KI KD参数
        us_PPC_PID(bpCommand, 4);
        break;
    case 0x17:    // 设置死区值
        us_PPC_DeadBand(bpCommand, 4);
        break;
    case 0x18:    // 设置零点偏移值
        us_PPC_ZeroOffset(bpCommand, 4);
        break;
    case 0x1c:
        us_Product_Selection(bpCommand, 4);
        break;
    case 0x1d:
        break;
    case 0x1e:
        break;
    case 0xf0:
        us_MCUE2Value(bpCommand, 4);    // 标传感器
        break;
    case 0xf1:
        us_MCUE2ID(bpCommand, 4);
        break;
    case 0x20:
        us_PPC_ADC_Adj(bpCommand, 4);    // 标ADC
        break;
    case 0x21:
        us_PPC_DAC_Adj(bpCommand, 4);    // 标DAC
        break;
    case 0x22:
        us_PPCGetDACFlag(bpCommand, 4);    // 读写DAC校准完成标志位
        break;
    case 0x23:
        us_PPCGetDAC_Adj_Value(bpCommand, 4);    // 读写DAC校准点的值
        break;
    case 0x24:
        us_PPCGetADCFlag(bpCommand, 4);    // 读写ADC校准完成标志位
        break;
    case 0x25:
        us_PPCGetMCUFlag(bpCommand, 4);    // 读写气压校准完成标志位
        break;
    case 0x30:                             // 读取AD通道的原始值
        us_PPCGet_Original_Value(bpCommand, 4);
        break;

    default:
        us_Respond(bCommand1, UnknowCommand, 0);
        break;
    }
Exit:;
}

/***************************************************
 *   名称：      	us_Respond()
 *   功能：		根据接收到的命令进行响应
 *   函数参数：	uint8_t bCommand1     	功能码1
 *   			error_Code error	错误代码
 *   			uint16_t wReData			需要传输的数据
 *   返回值：	void
 ***************************************************/
static void us_Respond(uint8_t bCommand1, error_Code error, uint16_t wReData)
{
    // uint8_t bReData[8]={0};
    uint16_t wReCRC = 0;
    bReData[0]      = bCommand1;
    if (error != Successful)
    {
        bReData[1] = 0xff;
        bReData[2] = (uint8_t)(error & 0xff);
        bReData[3] = (uint8_t)((error >> 8) & 0xff);
    }
    else
    {
        bReData[1] = 0x00;
        bReData[2] = (uint8_t)(wReData & 0xff);
        bReData[3] = (uint8_t)((wReData >> 8) & 0xff);
    }
    // wReCRC = wCRCCheck(bReData, 4);
    wReCRC     = wCRCCheck_Uart_Data(bReData, 4);
    bReData[4] = (uint8_t)(wReCRC & 0xff);
    bReData[5] = (uint8_t)((wReCRC >> 8) & 0xff);
    bReData[6] = 0x0D;
    bReData[7] = 0x0A;
    USBD_VCOM_SendData((const int8_t *)&bReData[0], 8);
}

/***************************************************
 *   名称：      	wCRCCheck()
 *   功能：		CRC-16校验
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
uint16_t wCRCCheck(uint8_t *bpData, uint8_t bLen)
{
    uint8_t  i, j;
    uint8_t  bTemp      = 0;
    uint16_t wResulrCrc = 0xffff;
    if (bLen == 0)
    {
        goto Exit;
    }
    for (i = 0; i < bLen; i++)
    {
        bTemp = *(bpData + i);
        for (j = 0; j < 8; j++)
        {
            if (((wResulrCrc ^ bTemp) & 0x0001) != 0)    // 该位不相等
            {
                wResulrCrc >>= 1;
                wResulrCrc ^= 0x1021;
            }
            else
            {
                wResulrCrc >>= 1;
            }
            bTemp >>= 1;
        }
    }
Exit:
    return wResulrCrc;    // 低位在前 高位在后
}

/***************************************************
 *   名称：      	us_PPCAdjust()
 *   功能：		读写PPC校正值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCAdjust(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t bChannel = (*(bpCommand2 + 1)) & 0x7f;
    //	uint8_t bData[2]={0};
    uint16_t wChnnValue = 0;

    if (bRorW == 0)    // 读命令
    {
        if (bChannel == 1)
        {
            // us_bI2c_Read_Stream(adPPCAdjust0,bData,2);//从320开始使用地址
            wChnnValue = us_wGet_Chnn_Adjust(1);
        }
        else if (bChannel == 2)
        {
            // us_bI2c_Read_Stream(adPPCAdjust1,bData,2);
            wChnnValue = us_wGet_Chnn_Adjust(2);
        }
        else if (bChannel == 10)
        {
            wChnnValue = us_wGet_Chnn_Adjust(10);
        }
        else if (bChannel == 11)
        {
            wChnnValue = us_wGet_Chnn_Adjust(11);
        }
        else if (bChannel == 12)
        {
            wChnnValue = us_wGet_Chnn_Adjust(12);
        }
        else if (bChannel == 13)
        {
            wChnnValue = us_wGet_Chnn_Adjust(13);
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, wChnnValue);    // 低字节在前
    }
    else                                                    // 写命令
    {
        if (bChannel == 1)
        {
            us_bI2c_Write_Stream(adPPCAdjust0, bpCommand2 + 2, 2);    // 从320开始使用地址

            wChnnValue = us_wGet_Chnn_Adjust(1);
            //			us_bI2c_Read_Stream(adPPCAdjust0,bData,2);//从320开始使用地址
        }
        else if (bChannel == 2)
        {
            us_bI2c_Write_Stream(adPPCAdjust1, bpCommand2 + 2, 2);

            wChnnValue = us_wGet_Chnn_Adjust(2);
            //			us_bI2c_Read_Stream(adPPCAdjust1,bData,2);//从320开始使用地址
        }
        else if (bChannel == 10)
        {
            us_bI2c_Write_Stream(adPPCADCAdjust0, bpCommand2 + 2, 2);

            wChnnValue = us_wGet_Chnn_Adjust(10);
        }
        else if (bChannel == 11)
        {
            us_bI2c_Write_Stream(adPPCADCAdjust1, bpCommand2 + 2, 2);

            wChnnValue = us_wGet_Chnn_Adjust(11);
        }
        else if (bChannel == 12)
        {
            us_bI2c_Write_Stream(adPPCADCAdjust2, bpCommand2 + 2, 2);

            wChnnValue = us_wGet_Chnn_Adjust(12);
        }
        else if (bChannel == 13)
        {
            us_bI2c_Write_Stream(adPPCADCAdjust3, bpCommand2 + 2, 2);

            wChnnValue = us_wGet_Chnn_Adjust(13);
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        if (wChnnValue != (*(bpCommand2 + 2) + ((*(bpCommand2 + 3)) << 8)))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPCGetADC()
 *   功能：		读各通道AD值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetADC(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint16_t wReValue = 0;
    if (bRorW != 0)    // 写命令
    {
        us_Respond(*bpCommand2, writeOnlyRead, 0);
        goto Exit;
    }
    else    // 读命令
    {
        switch (bChannel)
        {
        case 0:
            // 读取温度值
            wReValue = (uint16_t)us_Get_TempData();
            break;
        case 1:
            //			wReValue=(uint16_t)(us_wGet_Channel_Value(0)*1000);
            //	wReValue = (uint16_t)(us_ADS1118_GetADCData(0));
            //	wReValue = (uint16_t)((float)(us_ADS1118_GetADCData(7)+140)/10);
            // wReValue = (uint16_t)us_Adj_ADC_K_B(0,us_Get_ADC_Chnn(0));//181008
            wReValue = (uint16_t)us_Adj_ADC_To_Pc(0, us_Get_ADC_Chnn(0));
            break;
        case 2:
            //			wReValue=(uint16_t)(us_wGet_Channel_Value(1)*1000);
            // wReValue = (uint16_t)us_Adj_ADC_K_B(1,us_Get_ADC_Chnn(1));
            wReValue = (uint16_t)us_Adj_ADC_To_Pc(1, us_Get_ADC_Chnn(0));
            // wReValue = (uint16_t)(us_ADS1118_GetADCData(6));
            break;
        case 3:
            //			wReValue=(uint16_t)(us_wGet_Channel_Value(2)*1000);
            //	wReValue = (uint16_t)(us_ADS1118_GetADCData(6));
            wReValue = us_Get_ADC_Chnn(2);
            //			wReValue = (uint16_t)(us_ADS1118_GetADCData(8));
            break;
        case 4:
            //			wReValue=(uint16_t)(us_wGet_Channel_Value(3)*1000);
            //
            //			wReValue = us_Get_ADC_Chnn(1);
            wReValue = us_Get_ADC_Chnn(3);
            break;
        case 5:
            //			wReValue = wADC_Result_Avr_0;
            //			wReValue = (uint16_t)(us_ADS1118_GetTemp()*1000);
            wReValue = us_Get_ADC_Chnn(3);
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, wReValue);
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPCGetModel()
 *   功能：		读各通道模式值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetModel(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t bModel   = 0;
    if (bRorW == 0)    // 读命令
    {
        if (bChannel == 1)
        {
            us_Respond(*bpCommand2, Successful, us_bGet_Channel_Mode(1));
        }
        else if (bChannel == 2)
        {
            bModel = us_bGet_Channel_Mode(2);
            //			if(us_bGet_Channel_Mode(2)==0)
            //			{
            //				bModel |=0x00;
            //			}
            //			else
            //			{
            //				bModel |=0x01;
            //			}
            if (us_bGet_TTL_Mode() == 0)
            {
                bModel |= 0x00;
            }
            else
            {
                bModel |= 0x04;
            }
            if (us_bGet_Valid_Mode() == 0)
            {
                bModel |= 0x00;
            }
            else
            {
                bModel |= 0x08;
            }
            us_Respond(*bpCommand2, Successful, bModel);
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
        }
    }
    else    // 写命令
    {
        us_Respond(*bpCommand2, writeOnlyRead, 0);
    }
}

/***************************************************
 *   名称：      	us_PPCOutPut()
 *   功能：		读写各通道设置值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCOutPut(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW     = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bChannel  = (*(bpCommand2 + 1)) & 0x7f;
    uint16_t wOutvalue = (uint16_t)(*(bpCommand2 + 2) + (*(bpCommand2 + 3) << 8));
    // static uint8_t bCounter[2] = {0,0};
    if (bRorW == 0)    // 读命令
    {
        if (bChannel == 1)
        {
            wOutvalue = wOutPutValue[0];
        }
        else if (bChannel == 2)
        {
            wOutvalue = wOutPutValue[1];
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, wOutvalue);
    }
    else    // 写命令
    {
        if ((bChannel == 1) || (bChannel == 2))
        {
            us_Set_DAC_Temp(wOutvalue, us_bGet_Channel_Mode(bChannel), bChannel);
            //			if(ADJ_DAC_Flag[bChannel-1]==0)
            //			{
            //				if(wOutvalue==0)
            //				{
            //					bCounter[bChannel-1] = 0;
            //				}
            //				else
            //				{
            //					bCounter[bChannel-1]++;
            //				}
            //				if((bCounter[bChannel-1]>=2)&&(bCounter[bChannel-1]<=8))
            //				{
            //					if(bChannel==1)//电压
            //					{
            //						ADJ_DAC[bChannel-1][bCounter[bChannel-1]-2] = us_Get_ADC_Chnn(1);//电流 为通道0  电压为通道1  要与ADC通道对应
            //					}
            //					else//电流
            //					{
            //						ADJ_DAC[bChannel-1][bCounter[bChannel-1]-2] = us_Get_ADC_Chnn(0);//电流 为通道0  电压为通道1  要与ADC通道对应
            //					}
            //				}
            //				if(wOutvalue==1)//保存数据
            //				{
            //					us_bI2c_Write_Stream(DAC_Adj_S,(uint8_t *)ADJ_DAC,sizeof(ADJ_DAC));
            //					us_Set_DAC_Adj_Data((uint8_t *)ADJ_DAC,sizeof(ADJ_DAC));
            //				}
            //			}
            //			us_bSet_DACValue(wOutvalue,us_bGet_Channel_Mode(bChannel),bChannel,ADJ_DAC_Flag[bChannel-1]);
            wOutPutValue[bChannel - 1] = wOutvalue;
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPCGetADC_Model_01()
 *   功能：		读ADC通道1模式
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetADC_Model_01(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW  = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t bModel = 0;
    if (bRorW == 0)                                 // 读命令
    {
        bModel = us_bGetADCINMode();                // 获取当前ADC通道1的输入模式

                                                    //			if(ADC_Mode01==0)
        //			{
        //				bModel |=0x00;
        //			}
        //			else
        //			{
        //				bModel |=0x01;
        //			}
        //			if(ADC_Mode02==0)
        //			{
        //				bModel |=0x00;
        //			}
        //			else
        {
            // bModel |=0x02;
        }
        us_Respond(*bpCommand2, Successful, bModel);
    }
    else    // 写命令
    {
        us_Respond(*bpCommand2, writeOnlyRead, 0);
    }
}

/***************************************************
 *   名称：      	us_PPCGetADC_Unit()
 *   功能：		读ADC单位定义
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetADC_Unit(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW  = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t bModel = 0;
    if (bRorW == 0)                                 // 读命令
    {
        bModel = us_bGetADCFeedbackMode();          // 校准反馈模式
        //			if(Range_Mode01==0)
        //			{
        //				bModel |=0x00;
        //			}
        //			else
        //			{
        //				bModel |=0x01;
        //			}
        //			if(Range_Mode02==0)
        //			{
        //				bModel |=0x00;
        //			}
        //			else
        {
            // bModel |=0x02;
        }
        us_Respond(*bpCommand2, Successful, bModel);
    }
    else    // 写命令
    {
        us_Respond(*bpCommand2, writeOnlyRead, 0);
    }
}

/***************************************************
 *   名称：      	us_PPCE2_Clear()
 *   功能：		清除E2
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCE2_Clear(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW     = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t bData[32] = {0};

    if (bRorW == 0)    // 读命令
    {
        us_Respond(*bpCommand2, ReadOnlyWrite, 0);
    }
    else    // 写命令
    {
        if (((*(bpCommand2 + 2)) == 0x00) && ((*(bpCommand2 + 3)) == 0x00))
        {
            us_bI2c_Write_Stream((uint16_t)(mcu_range_max), bData, 32);
        }
        else
        {
            us_Respond(*bpCommand2, UnknowCommand, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);
    }
Exit:;
}

/***************************************************
 *   名称：      	us_MCUE2ID()
 *   功能：		读写MCU的E2中ID值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_MCUE2ID(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    if (bRorW == 0)    // 读命令
    {
        switch (bChannel)
        {
        case 0:
            bData[0] = Version[1];
            bData[1] = Version[0];
            crc      = wCRCCheck(bData, 2);
            bData[2] = (uint8_t)crc;
            bData[3] = (uint8_t)(crc >> 8);
            break;
        case 1:
            us_bI2c_Read_Stream(mcu_id2_h, bData, 4);
            break;
        case 2:
            us_bI2c_Read_Stream(mcu_id2_l, bData, 4);
            break;
        case 3:
            us_bI2c_Read_Stream(mcu_id3, bData, 4);
            break;
        case 4:
            us_bI2c_Read_Stream(mcu_id4, bData, 4);
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if (wCRCCheck(bData, 4) != 0)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
        }
        else
        {
            us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0] + (bData[1] << 8)));    // 低字节在前
        }
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        switch (bChannel)
        {
        case 0:
            us_bI2c_Write_Stream(mcu_id1, bData, 4);    //

            us_bI2c_Read_Stream(mcu_id1, bData, 4);     //
            break;
        case 1:
            us_bI2c_Write_Stream(mcu_id2_h, bData, 4);    //

            us_bI2c_Read_Stream(mcu_id2_h, bData, 4);     //
            break;
        case 2:
            us_bI2c_Write_Stream(mcu_id2_l, bData, 4);    //

            us_bI2c_Read_Stream(mcu_id2_l, bData, 4);     //
            break;
        case 3:
            us_bI2c_Write_Stream(mcu_id3, bData, 4);    //

            us_bI2c_Read_Stream(mcu_id3, bData, 4);     //
            *((uint8_t *)&bmyID3) = *bData;             // 获取ID3的值
            break;
        case 4:
            us_bI2c_Write_Stream(mcu_id4, bData, 4);    //

            us_bI2c_Read_Stream(mcu_id4, bData, 4);     //
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((*(bpCommand2 + 2) != bData[0]) || (*(bpCommand2 + 3) != bData[1]) || (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_MCUE2Value()
 *   功能：		读写MCU压力值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_MCUE2Value(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t  bData[4] = {0};
    uint8_t  bFlag    = 0;
    uint16_t crc      = 0;
    mcu_type mcu_temp;
    double   Key_MCU_Press = 0;
    if (bRorW == 0)    // 读命令
    {
        switch (bChannel)
        {
        case 0:
            us_bI2c_Read_Stream(mcu_range_min, bData, 4);
            break;
        case 1:
            us_bI2c_Read_Stream(mcu_range_max, bData, 4);
            break;
        case 2:
            us_bI2c_Read_Stream(mcu_min, bData, 2);
            bFlag = 0xff;
            break;
        case 3:
            us_bI2c_Read_Stream(mcu_max, bData, 2);
            bFlag = 0xff;
            break;
        case 4:
            us_bI2c_Read_Stream(mcu_flag, bData, 2);
            bFlag = 0xff;
            break;
        case 5:
            us_bI2c_Read_Stream(mcu_range_min2, bData, 4);
            break;
        case 6:
            us_bI2c_Read_Stream(mcu_range_max2, bData, 4);
            break;
        case 7:
            Key_MCU_Press = (us_bGet_K(0) * (us_Get_ADC_Chnn(2)) + us_bGet_B(0)) * 10;
            //			SEGGER_RTT_printf(0,"传给上位机的MCU_GetData0= %d\r\n",(uint16_t)Key_MCU_Press);
            us_Respond(*bpCommand2, Successful, (uint16_t)((s16)(Key_MCU_Press)));    // 低字节在前
            goto Exit;
        case 8:
            Key_MCU_Press = (us_bGet_K(1) * (us_Get_ADC_Chnn(3)) + us_bGet_B(1)) * 10;
            //			SEGGER_RTT_printf(0,"传给上位机的MCU_GetData1= %d\r\n",(uint16_t)Key_MCU_Press);
            us_Respond(*bpCommand2, Successful, (uint16_t)((s16)(Key_MCU_Press)));    // 低字节在前
            goto Exit;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((bFlag == 0) && (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
        }
        else
        {
            us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0] + (bData[1] << 8)));    // 低字节在前
        }
    }
    else    // 写命令
    {
        us_bI2c_Read_Stream(mcu_flag, bData, 2);
        if (bData[0] == 0xA5)
        {
            us_Respond(*bpCommand2, writeOnlyRead, 0);
            goto Exit;
        }
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        switch (bChannel)
        {
        case 0:
            us_bI2c_Write_Stream(mcu_range_min, bData, 4);    //
            us_bI2c_Read_Stream(mcu_range_min, bData, 4);     //
            break;
        case 1:
            us_bI2c_Write_Stream(mcu_range_max, bData, 4);                      //
            us_bI2c_Read_Stream(mcu_range_max, bData, 4);                       //
            ubMaxScalePsi10 = (uint32_t)((bData[0] + (bData[1] << 8))) * 58;    // 标定量程时，直接把量程赋给最大值
            switch (KEY_MENU_Temp_Data.H01)
            {
            case 0:    // psi
                KEY_MENU_Temp_Data.H08 = ubMaxScalePsi10 / 40;
                break;
            case 1:    // Bar
            case 2:    // Kpa
            case 3:    // Mpa
                KEY_MENU_Temp_Data.H08 = ubMaxScalePsi10 / 58;
                break;
            }
            EepFlashDat.present[2]         = (ubMaxScalePsi10 >> 16);
            EepFlashDat.present[26]        = (ubMaxScalePsi10 & 0x0000FFFF);
            EepFlashDat.present[3]         = ubMaxScalePsi10 / 58;
            calibration.pressure_scale_kpa = EepFlashDat.present[3];
            KEY_MENU_Data.H08              = KEY_MENU_Temp_Data.H08;
            KEY_MENU_Temp_Data.Max_H08     = KEY_MENU_Temp_Data.H08;
            ubMinScalePsi10                = 20;    // 标定时最小值设定0
            KEY_MENU_Data.H07              = 0;
            EepFlashDat.present[1]         = (ubMinScalePsi10 >> 16);
            EepFlashDat.present[25]        = (ubMinScalePsi10 & 0x0000FFFF);
            KEY_MENU_Temp_Data.H07         = KEY_MENU_Data.H07;
            Dat_Save_Eeprom();
            break;
        case 2:
            mcu_Data.min_ADC_01 = (uint16_t)(us_Get_ADC_Chnn(2));                  // 最小值AD
            calibration.min_AD  = mcu_Data.min_ADC_01;
            mcu_Data.min_ADC_02 = (uint16_t)(us_Get_ADC_Chnn(3));                  // 最小值AD
            calibration.min_AD2 = mcu_Data.min_ADC_02;
            mcu_Data.min        = *(bpCommand2 + 2) | (*(bpCommand2 + 3) << 8);    // 最小值
            if (mcu_Data.min >= 0x8000)                                            // 最小压力值为负，即为0
                mcu_Data.min = 0;
            calibration.min_pressure_psi = mcu_Data.min * 1.45;
            us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
            goto Exit;
        case 3:
            mcu_Data.max_ADC_01          = (uint16_t)(us_Get_ADC_Chnn(2));    // 最大值AD
            calibration.max_AD           = mcu_Data.max_ADC_01;
            mcu_Data.max_ADC_02          = (uint16_t)(us_Get_ADC_Chnn(3));
            calibration.max_AD2          = mcu_Data.max_ADC_02;
            mcu_Data.max                 = *(bpCommand2 + 2) | (*(bpCommand2 + 3) << 8);    // 最大值
            calibration.max_pressure_psi = mcu_Data.max * 1.45;
            us_SetMCU_K_B(mcu_Data, 0xff);
            us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
            goto Exit;
            // break;
        case 4:
            mcu_Data.flag = *(bpCommand2 + 2) | (*(bpCommand2 + 3) << 8);
            mcu_Data.crc  = wCRCCheck((uint8_t *)&mcu_Data.max_ADC_01, 14);
            us_bI2c_Write_Stream(mcu_max_ADC_01, (uint8_t *)&mcu_Data.max_ADC_01, 16);    //
            us_bI2c_Read_Stream(mcu_max_ADC_01, (uint8_t *)&mcu_temp.max_ADC_01, 16);
            if ((mcu_Data.crc != mcu_temp.crc) || (wCRCCheck((uint8_t *)&mcu_temp.max_ADC_01, 16) != 0))
            {
                us_Respond(*bpCommand2, E2Fail, 0);
            }
            else
            {
                us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
            }
            goto Exit;
            break;
        case 5:
            us_bI2c_Write_Stream(mcu_range_min2, bData, 4);    //
            us_bI2c_Read_Stream(mcu_range_min2, bData, 4);     //
            break;
        case 6:
            us_bI2c_Write_Stream(mcu_range_max2, bData, 4);    //
            us_bI2c_Read_Stream(mcu_range_max2, bData, 4);     //
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((*(bpCommand2 + 2) != bData[0]) || (*(bpCommand2 + 3) != bData[1]) || (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

static void us_SetMCU_K_B(mcu_type mcu_temp, uint8_t bFlag)
{
    uint16_t Key_MCU[4] = {0};
    if (mcu_temp.max_ADC_01 != mcu_temp.min_ADC_01)
    {
        MCU_k[0] = ((double)((s16)mcu_temp.max - (s16)mcu_temp.min) / (mcu_temp.max_ADC_01 - mcu_temp.min_ADC_01)) / 10;
        MCU_b[0] = (double)((s16)mcu_temp.min) / 10 - (double)MCU_k[0] * mcu_temp.min_ADC_01;
        if (bFlag != 0)
        {
            Key_MCU[0] = (uint16_t)((s16)(MCU_k[0] * 10000));
            Key_MCU[1] = wCRCCheck((uint8_t *)&Key_MCU[0], 2);
            Key_MCU[2] = (uint16_t)((s16)(MCU_b[0] * 10));
            Key_MCU[3] = wCRCCheck((uint8_t *)&Key_MCU[2], 2);
        }
    }
    if (mcu_temp.max_ADC_02 != mcu_temp.min_ADC_02)
    {
        MCU_k[1] = ((double)((s16)mcu_temp.max - (s16)mcu_temp.min) / (mcu_temp.max_ADC_02 - mcu_temp.min_ADC_02)) / 10;
        MCU_b[1] = (double)((s16)mcu_temp.min) / 10 - (double)MCU_k[1] * mcu_temp.min_ADC_02;
        if (bFlag != 0)
        {
            Key_MCU[0] = (uint16_t)((s16)(MCU_k[1] * 10000));
            Key_MCU[1] = wCRCCheck((uint8_t *)&Key_MCU[0], 2);
            Key_MCU[2] = (uint16_t)((s16)(MCU_b[1] * 10));
            Key_MCU[3] = wCRCCheck((uint8_t *)&Key_MCU[2], 2);
        }
    }
}

/***************************************************
 *   名称：      	us_bGet_ID3()
 *   功能：		读取当前ID3的各个位
 *   函数参数：    	void
 *   返回值：	ID3类型
 ***************************************************/
ID3 us_bGet_ID3()
{
    return bmyID3;
}

/***************************************************
 *   名称：      	us_bSet_ID3()
 *   功能：		设置当前ID3的各个位
 *   函数参数：    	bData ID3的值
 *   返回值：	错误类型
 ***************************************************/
uint8_t us_bSet_ID3(uint8_t bData)
{
    uint8_t  bReadData[4]  = {0};
    uint8_t  bWriteData[4] = {0};
    uint16_t crc           = 0;
    bWriteData[0]          = bData;
    crc                    = wCRCCheck(bWriteData, 2);
    bWriteData[2]          = (uint8_t)crc;
    bWriteData[3]          = (uint8_t)(crc >> 8);
    us_bI2c_Write_Stream(mcu_id3, bWriteData, 4);    //
    us_bI2c_Read_Stream(mcu_id3, bReadData, 4);      //
    if ((bWriteData[0] != bReadData[0]) || (wCRCCheck(bReadData, 4) != 0))
    {
        return 0xff;
    }
    *((uint8_t *)&bmyID3) = bReadData[0];    // 获取ID3的值
    return 0;
}

double us_bGet_K(uint8_t bChnn)
{
    if (bChnn > 2)
    {
        return 0;
    }
    return MCU_k[bChnn];
}

double us_bGet_B(uint8_t bChnn)
{
    if (bChnn > 2)
    {
        return 0;
    }
    return MCU_b[bChnn];
}

static void us_PPC_ADC_Adj(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    //	uint8_t bMode = ((*(bpCommand2+1))>>6)&0x01;//电压还是电流
    uint8_t bDataCount = ((*(bpCommand2 + 1)) >> 3) & 0x07;    // 校准的第几点
    uint8_t bChannel   = (*(bpCommand2 + 1)) & 0x07;           // 校准的通道号
    // uint8_t i = 0;
    //	uint16_t bTemp = 0;
    ADCAdjT temp[4];
    if (bRorW == 0)    // 读命令
    {
        us_Respond(*bpCommand2, writeOnlyRead, 0);
        goto Exit;
    }
    else    // 写命令
    {
        // bTemp = (uint16_t)(*(bpCommand2+2)+((*(bpCommand2+3))<<8));
        // if((bChannel>2)||(bDataCount>3)||(us_GetADC_Mode(bChannel)!=bMode))
        if ((bChannel > 2) || (bDataCount > 3))
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        switch (bDataCount)
        {
        case 0:
            ADJ_ADC[0].bFlag = 0;
            AdjustADCFlag    = 1;
            break;
        case 1:
            ADJ_ADC[0].dMin = (uint16_t)us_Get_AveInputADC_Chnn(0);
            ADJ_ADC[1].dMin = ADJ_ADC[0].dMin;
            // SEGGER_RTT_printf(0,"通道：%d  最小值:%d\r\n",bChannel,ADJ_ADC[bChannel].dMin);
            break;
        case 2:
            ADJ_ADC[0].dMid = (uint16_t)us_Get_AveInputADC_Chnn(0);
            ADJ_ADC[1].dMid = ADJ_ADC[0].dMid;
            // SEGGER_RTT_printf(0,"通道：%d  中间值:%d\r\n",bChannel,ADJ_ADC[bChannel].dMid);
            break;
        case 3:
            ADJ_ADC[0].bFlag = 0xA5;
            ADJ_ADC[1].bFlag = ADJ_ADC[0].bFlag;
            ADJ_ADC[0].dMax  = (uint16_t)us_Get_AveInputADC_Chnn(0);
            ADJ_ADC[1].dMax  = ADJ_ADC[0].dMax;
            // SEGGER_RTT_printf(0,"通道：%d  最大值:%d\r\n",bChannel,ADJ_ADC[bChannel].dMax);
            ADJ_ADC[0].crc = wCRCCheck((uint8_t *)&ADJ_ADC[0], sizeof(ADCAdjT) - 2);
            ADJ_ADC[1].crc = wCRCCheck((uint8_t *)&ADJ_ADC[1], sizeof(ADCAdjT) - 2);
            us_bI2c_Write_Stream(ADC_Adj_S + 0 * sizeof(ADCAdjT), (uint8_t *)&ADJ_ADC[0], sizeof(ADCAdjT));
            us_bI2c_Write_Stream(ADC_Adj_S + 1 * sizeof(ADCAdjT), (uint8_t *)&ADJ_ADC[1], sizeof(ADCAdjT));
            us_bI2c_Read_Stream(ADC_Adj_S + bChannel * sizeof(ADCAdjT), (uint8_t *)(temp + bChannel), sizeof(ADCAdjT));

            if ((ADJ_ADC[bChannel].bFlag != temp[bChannel].bFlag) || (ADJ_ADC[bChannel].dMin != temp[bChannel].dMin) || (ADJ_ADC[bChannel].dMax != temp[bChannel].dMax) || (wCRCCheck((uint8_t *)&ADJ_ADC[bChannel], sizeof(ADCAdjT)) != 0))
            {
                us_Respond(*bpCommand2, E2Fail, 0);    // 低字节在前
                goto Exit;
            }
            AdjustADCFlag = 0;
            break;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

double us_Adj_ADC_K_B(uint8_t bChnn, double dlData)
{
    double dlResult = 0;
    double maxY, minY;
    if (bChnn == 0)    // 电流
    {
        minY = 0;
        maxY = 20000;
    }
    else
    {
        minY = 0;
        maxY = 10000;
    }
    if (bChnn > 2)
    {
        return dlData;
    }
    // 判断,如果最大值等于最小值,且bFlag=0xA5表示,未校准完成,直接返回当前测量值
    if ((ADJ_ADC[bChnn].dMin == ADJ_ADC[bChnn].dMax) || (ADJ_ADC[bChnn].bFlag != 0xA5))
    {
        //		if(bChnn==0)//电流默认值
        //		{
        //			dlResult=dlData*0.60135*10;
        //		}else if(bChnn==1)//电压默认值
        //		{
        //			dlResult=dlData*3.0517;
        //		}
        //		return dlResult;
    }
    if ((ADJ_ADC[bChnn].dMin == ADJ_ADC[bChnn].dMax) || (ADJ_ADC[bChnn].bFlag != 0xA5))
    {
        return dlData;
    }
    //	if(dlData<ADJ_ADC[bChnn].dMin-10)
    //		dlData=ADJ_ADC[bChnn].dMin-10;
    dlResult = (double)((maxY - minY) * (dlData - ADJ_ADC[bChnn].dMin)) / (ADJ_ADC[bChnn].dMax - ADJ_ADC[bChnn].dMin) + minY;
    return dlResult;
}

double us_Adj_ADC_To_Pc(uint8_t bChnn, double dlData)
{
    double dlResult = 0;
    double maxY, minY;
    // 判断,如果最大值等于最小值,且bFlag=0xA5表示,未校准完成,直接返回当前测量值
    if ((ADJ_ADC[bChnn].dMin == ADJ_ADC[bChnn].dMax) || (ADJ_ADC[bChnn].bFlag != 0xA5))
    {
        return dlData;
    }
    if (us_bGetADCINMode() == ADCIN4_20mA)
    {
        if (bChnn == 0)
        {
            minY = 0;
            maxY = 20000;
        }
        else if ((bChnn == 1) && (us_bGetADCFeedbackMode() == ADCIN4_20mA))
        {
            minY = 0;
            maxY = 20000;
        }
        else if ((bChnn == 1) && (us_bGetADCFeedbackMode() == ADCIN0_10V))
        {
            minY = 0;
            maxY = 10000;
        }
    }
    else if (us_bGetADCINMode() == ADCIN0_10V)
    {
        if (bChnn == 1)
        {
            minY = 0;
            maxY = 10000;
        }
        else if ((bChnn == 0) && (us_bGetADCFeedbackMode() == ADCIN4_20mA))
        {
            minY = 0;
            maxY = 20000;
        }
        else if ((bChnn == 0) && (us_bGetADCFeedbackMode() == ADCIN0_10V))
        {
            minY = 0;
            maxY = 10000;
        }
    }
    else if (us_bGetADCINMode() == ADCIN0_5V)
    {
        if (bChnn == 1)
        {
            minY = 0;
            maxY = 5000;
        }
        else if ((bChnn == 0) && (us_bGetADCFeedbackMode() == ADCIN4_20mA))
        {
            minY = 0;
            maxY = 20000;
        }
        else if ((bChnn == 0) && (us_bGetADCFeedbackMode() == ADCIN0_10V))
        {
            minY = 0;
            maxY = 5000;
        }
    }

    dlResult = (double)((maxY - minY) * (dlData - ADJ_ADC[bChnn].dMin)) / (ADJ_ADC[bChnn].dMax - ADJ_ADC[bChnn].dMin) + minY;
    return dlResult;
}

static void us_PPC_DAC_Adj(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t         bRorW         = (*(bpCommand2 + 1)) & 0x80;           //
    uint8_t         bChannel      = ((*(bpCommand2 + 1)) & 0x70) >> 6;    //
    uint8_t         bPoint        = ((*(bpCommand2 + 1)) & 0x03f);        //
    uint8_t         i             = 0;
    uint8_t         bTempByte[64] = {0};
    uint16_t        bTemp         = 0;
    uint16_t        crc           = 0;
    uint16_t        bReadData     = 0;
    static uint16_t dacdata[32]   = {0};
    if (bRorW == 0)                                //
    {
        us_Respond(*bpCommand2, Successful, 0);    //
    }
    else                                           //
    {
        bTemp = (uint16_t)(*(bpCommand2 + 2) + ((*(bpCommand2 + 3)) << 8));
        if ((bChannel < 2) && (bPoint < 34))
        {
            if (bPoint == 0)    //
            {
                ADJ_DAC_Flag[bChannel] = 0;
                crc                    = wCRCCheck(bTempByte, 2);
                bTempByte[2]           = (uint8_t)crc;
                bTempByte[3]           = (uint8_t)(crc >> 8);
                us_bI2c_Write_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bTempByte, 4);
                us_bI2c_Read_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bTempByte, 4);
                if (wCRCCheck(bTempByte, 4) != 0)
                {
                    us_Respond(*bpCommand2, E2Fail, 0);
                    goto Exit;
                }
                else
                {
                    bTemp     = 0;
                    bReadData = (uint16_t)(bTempByte[0] | (bTempByte[1] << 8));
                }
                for (i = 0; i < 32; i++)
                {
                    dacdata[i] = 0;
                }
            }
            else if ((bTemp == 0xffff) && (bPoint == 33))    //
            {
                ADJ_DAC_Flag[bChannel] = 0xA5;
                bTempByte[0]           = ADJ_DAC_Flag[bChannel];
                crc                    = wCRCCheck(bTempByte, 2);
                bTempByte[2]           = (uint8_t)crc;
                bTempByte[3]           = (uint8_t)(crc >> 8);
                us_bI2c_Write_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bTempByte, 4);
                us_bI2c_Read_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bTempByte, 4);
                if (wCRCCheck(bTempByte, 4) != 0)
                {
                    us_Respond(*bpCommand2, E2Fail, 0);
                    goto Exit;
                }
                else
                {
                    bTemp     = 0xA5;
                    bReadData = (uint16_t)(bTempByte[0] | (bTempByte[1] << 8));
                }
                if (bReadData == 0xA5)
                {
                    us_bSet_DAC_Adjust(bChannel + 1);
                }
            }
            else
            {
                dacdata[bPoint - 1] = bTemp;
                /*定义一个两维全局数组，存储DAC校准值*/
                DAC_Adj_Valu_Temp[bChannel][bPoint - 1] = bTemp;    // 将上位机传递的指定点存储到全局变量中
                bReadData                               = dacdata[bPoint - 1];
                if (bPoint == 32)
                {
                    us_bI2c_Write_Stream(DAC_Adj_S + (bChannel) * 64, (uint8_t *)dacdata, 32);
                    us_bI2c_Write_Stream(DAC_Adj_S + (bChannel) * 64 + 32, (uint8_t *)&dacdata[16], 32);
                    crc = wCRCCheck((uint8_t *)dacdata, 64);
                    us_bI2c_Write_Stream(DAC_Adj_CRC_01 + (bChannel) * 4, (uint8_t *)&crc, 4);
                    us_bI2c_Read_Stream(DAC_Adj_S + (bChannel) * 64, (uint8_t *)bTempByte, 32);
                    us_bI2c_Read_Stream(DAC_Adj_S + (bChannel) * 64 + 32, (uint8_t *)&bTempByte[32], 32);
                    us_bI2c_Read_Stream(DAC_Adj_CRC_01 + (bChannel) * 4, (uint8_t *)&crc, 2);
                    if (crc != wCRCCheck(bTempByte, 64))
                    {
                        us_Respond(*bpCommand2, E2Fail, 0);
                        goto Exit;
                    }
                }
            }
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        if (bTemp != bReadData)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    //
    }
Exit:;
}

static void us_bGetDACAdjFlag(void)
{
    uint8_t i       = 0;
    uint8_t temp[8] = {0};
    us_bI2c_Read_Stream(DAC_Adj_SFlag_01, temp, 8);
    for (i = 0; i < 2; i++)
    {
        if (wCRCCheck(temp + i * 4, 4) == 0)
        {
            ADJ_DAC_Flag[i] = temp[i * 4];
        }
    }
}
/***************************************************
 *   名称：		us_bGetDACFlag()
 *   功能：
 *   函数参数：
 *   返回值：
 ***************************************************/
uint8_t us_bGetDACFlag(uint8_t bChnn)
{
    if ((bChnn > 3) || (bChnn == 0))
    {
        return 0;
    }
    else
    {
        return ADJ_DAC_Flag[bChnn - 1];
    }
}
/***************************************************
 *   名称：		us_bSetDACFlag()
 *   功能：
 *   函数参数：
 *   返回值：
 ***************************************************/
void us_bSetDACFlag(uint8_t bChnn, uint8_t bdata)
{
    if ((bChnn > 3) || (bChnn == 0))
    {
    }
    else
    {
        ADJ_DAC_Flag[bChnn - 1] = bdata;
    }
}
/***************************************************
 *   名称：		us_MemCpy()
 *   功能：		按字节复制内容   将源指针内容复制到目的指针中
 *   函数参数：	pDst_p     目的指针
 *   			pSrc_p     源指针
 *   返回值：	void
 ***************************************************/
void us_MemCpy(void *pDst_p, void *pSrc_p, uint32_t wSiz_p)    // 复制  只是对内容进行复制，不是将指针进行复制，源指针和目标指针是不一样的。
{
    uint8_t *dst = pDst_p;
    uint8_t *src = pSrc_p;
    while (wSiz_p > 0)
    {
        *dst = *src;
        src++;
        dst++;
        wSiz_p--;
    }
}

/***************************************************
 *   名称：		us_bGetDACMode()
 *   功能：
 *   函数参数：
 *   返回值：
 ***************************************************/
uint8_t us_bGetDACMode(void)
{
    return (uint8_t)((mcu_Data.id4 & 0x0c) | ((mcu_Data.id3 & 0x0c) >> 2));
}

/***************************************************
 *   名称：		us_bGetADCINMode()
 *   功能：
 *   函数参数：
 *   返回值：
 ***************************************************/
uint8_t us_bGetADCINMode(void)
{
    return (uint8_t)(((mcu_Data.id4 & 0x03) << 2) | (mcu_Data.id3 & 0x03));
}

uint8_t us_bGetADCFeedbackMode(void)
{
    return (uint8_t)(mcu_Data.id3 >> 6);
}

uint8_t us_bGet_D2_ADCINMode(void)
{
    return (uint8_t)(controlType);
}

uint8_t us_bGet_D2_DACFeedbackMode(void)
{
    return (uint8_t)(FeedbackType);
}

/********20180927增加传递双通道DAC校准状态函数以及DAC所有校准点函数***********/

/***************************************************
 *   名称：      	us_PPCGetDACFlag()
 *   功能：		读写两通道DAC校准完成标志位
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetDACFlag(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    if (bRorW == 0)    // 读命令
    {
        if (bChannel == 0)
        {
            bData[0] = ADJ_DAC_Flag[0];    // DAC通道1的校准完成标志位
        }
        else if (bChannel == 1)
        {
            bData[0] = ADJ_DAC_Flag[1];    // DAC通道2的校准完成标志位
        }
        else
        {
            // us_Respond(*bpCommand2,ObjectDnExist,0);
            goto Exit;
        }
        // SEGGER_RTT_printf(0,"接收到读DAC校准标志位命令,返回当前标志位为：%d\r\n",bData[0]);
        us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0]));
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = 0;
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        if (bChannel == 0)
        {
            ADJ_DAC_Flag[bChannel] = bData[0];
            us_bI2c_Write_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bData, 4);
            us_bI2c_Read_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bData, 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                us_Respond(*bpCommand2, E2Fail, 0);
                goto Exit;
            }
        }
        else if (bChannel == 1)
        {
            ADJ_DAC_Flag[bChannel] = bData[0];
            us_bI2c_Write_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bData, 4);
            us_bI2c_Read_Stream(DAC_Adj_SFlag_01 + bChannel * 4, bData, 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                us_Respond(*bpCommand2, E2Fail, 0);
                goto Exit;
            }
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPCGetDAC_Adj_Value()
 *   功能：		读写两通道DAC校准完成标志位
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetDAC_Adj_Value(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;           // R(0)/W(!0)
    uint8_t  bChannel = ((*(bpCommand2 + 1)) & 0x40) >> 6;    // 通道号
    uint8_t  bPoint   = (*(bpCommand2 + 1)) & 0x3F;           // 校准点
    uint16_t bData    = 0;
    if (bRorW == 0)                                           // 读命令
    {
        if (bPoint >= 0 && bPoint <= 31)
        {
            bData = DAC_Adj_Valu_Temp[bChannel][bPoint];
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        // SEGGER_RTT_printf(0,"读取的通道号为%d，校准点为：%d,值为：%d\r\n",bChannel,bPoint,bData);
        us_Respond(*bpCommand2, Successful, bData);
    }
    else    // 写命令
    {
    }
Exit:;
}

/********20181008增加传递双通道ADC校准状态函数***********/

/***************************************************
 *   名称：      	us_PPCGetADCFlag()
 *   功能：		读写两通道ADC校准完成标志位
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetADCFlag(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW    = (*(bpCommand2 + 1)) & 0x80;    // R(0)/W(!0)
    uint8_t bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t bData[4] = {0};
    if (bRorW == 0)    // 读命令
    {
        if (bChannel == 0)
        {
            if (us_bGetADCINMode() == ADCIN4_20mA)
            {
                bData[0] = ADJ_ADC[0].bFlag;    // ADC通道1的校准完成标志位
            }
            else
            {
                bData[0] = ADJ_ADC[1].bFlag;    // ADC通道1的校准完成标志位
            }
        }
        else if (bChannel == 1)
        {
            if (us_bGetADCINMode() == ADCIN4_20mA)
            {
                bData[0] = ADJ_ADC[1].bFlag;    // ADC通道2的校准完成标志位
            }
            else
            {
                bData[0] = ADJ_ADC[0].bFlag;    // ADC通道2的校准完成标志位
            }
        }
        else
        {
            // us_Respond(*bpCommand2,ObjectDnExist,0);
            goto Exit;
        }
        //		SEGGER_RTT_printf(0,"接收到读ADC校准标志位命令,返回当前标志位为：%d\r\n",bData[0]);
        us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0]));
    }
    else                                           // 写命令
    {
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPCGetADCFlag()
 *   功能：		读写气压校准完成标志位
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGetMCUFlag(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t bRorW    = (*(bpCommand2 + 1)) & 0x80;    // R(0)/W(!0)
    uint8_t bData[4] = {0};
    if (bRorW == 0)                                   // 读命令
    {
        bData[0] = mcu_Data.flag;                     // 气压校准完成标志位
        //		SEGGER_RTT_printf(0,"接收到读气压校准标志位命令,返回当前标志位为：%d\r\n",bData[0]);
        us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0]));
    }
    else                                           // 写命令
    {
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
}

/***************************************************
 *   名称：      	wCRCCheck_Uart_Data()
 *   功能：		CRC-16校验
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
uint16_t wCRCCheck_Uart_Data(uint8_t *bpData, uint8_t bLen)
{
    uint8_t  i, j;
    uint8_t  bTemp      = 0;
    uint16_t wResulrCrc = 0xffff;
    if (bLen == 0)
    {
        goto Exit;
    }

    for (i = 1; i < bLen; i++)
    {    // 从1开始，先偏移一位
        bTemp = *(bpData + i);
        for (j = 0; j < 8; j++)
        {
            if (((wResulrCrc ^ bTemp) & 0x0001) != 0)    // 该位不相等
            {
                wResulrCrc >>= 1;
                wResulrCrc ^= 0x1021;
            }
            else
            {
                wResulrCrc >>= 1;
            }
            bTemp >>= 1;
        }
    }
Exit:
    return wResulrCrc + 1;    // 低位在前 高位在后，最后返回值加1
}

/***************************************************
 *   名称：      	us_PPCGet_Original_Value(bpCommand,4)
 *   功能：		读各AD通道的原始AD值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_PPCGet_Original_Value(uint8_t *bpCommand2, uint8_t bLen)
{
    //	uint8_t bRorW = (*(bpCommand2 + 1)) & 0x80; //不为0 为写 R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint16_t wReValue = 0;
    switch (bChannel)
    {
    case 0:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(0);
        break;
    case 1:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(1);
        break;
    case 2:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(2);
        break;
    case 3:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(3);
        break;
    case 4:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(4);
        break;
    case 5:
        wReValue = (uint16_t)us_Get_Original_ADC_Chnn(5);
        break;
    default:
        us_Respond(*bpCommand2, ObjectDnExist, 0);
        goto Exit;
    }
    us_Respond(*bpCommand2, Successful, wReValue);
Exit:;
}

/***************************************************
 *   名称：      	us_Product_Selection()
 *   功能：		读写产品类型
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_Product_Selection(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // R(0)/W(!0)
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    if (bRorW == 0)    // 读命令
    {
        us_bI2c_Read_Stream(product_type, bData, 4);
        if (wCRCCheck(bData, 4) != 0)
        {
            bData[0] = 0xFF;
        }
        us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0]));
    }
    else    // 写命令
    {
        bData[0]     = *(bpCommand2 + 2);
        bData[1]     = 0;
        crc          = wCRCCheck(bData, 2);
        bData[2]     = (uint8_t)crc;
        bData[3]     = (uint8_t)(crc >> 8);
        Product_Type = bData[0];
        us_bI2c_Write_Stream(product_type, bData, 4);
        us_bI2c_Read_Stream(product_type, bData, 4);
        if (wCRCCheck(bData, 4) != 0)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_D2_Parameter()
 *   功能：		D2参数设定
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
static void us_D2_Parameter(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // R(0)/W(!0)
    uint8_t  bChannel = (*(bpCommand2 + 1)) & 0x7f;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    if (bRorW == 0)           // 读命令
    {
        if (bChannel == 0)    // PID死区（控制死区）
        {
            us_bI2c_Read_Stream(E2_Control_DeadBand_Percent, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else if (bChannel == 1)    // 输出量程最小值
        {
        }
        else if (bChannel == 2)    // 输出量程最大值
        {
        }
        else if (bChannel == 3)    // 控制信号
        {
            us_bI2c_Read_Stream(E2_Control_Signal_type, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else if (bChannel == 4)    // 反馈信号
        {
            us_bI2c_Read_Stream(E2_Feedback_Signal_type, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else if (bChannel == 5)    // 0点死区
        {
            us_bI2c_Read_Stream(E2_Zero_DeadBand_Percent, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else if (bChannel == 6)    // 最小输出
        {
            us_bI2c_Read_Stream(E2_Out_Min, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else if (bChannel == 7)    // 最大输出
        {
            us_bI2c_Read_Stream(E2_Out_Max, (uint8_t *)(bData), 4);
            if (wCRCCheck(bData, 4) != 0)
            {
                bData[0] = 0xFF;
                bData[1] = 0x00;
            }
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }

        us_Respond(*bpCommand2, Successful, (uint16_t)(bData[0] | bData[1] << 8));
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);

        if (bChannel == 0)    // PID死区（控制死区）
        {
            fDeadBand_Percent = (bData[0] + (bData[1] << 8)) / 10.0f;
            us_bI2c_Write_Stream(E2_Control_DeadBand_Percent, bData, 4);
        }
        else if (bChannel == 1)    // 输出量程最小值
        {
        }
        else if (bChannel == 2)    // 输出量程最大值
        {
        }
        else if (bChannel == 3)    // 控制信号
        {
            controlType = (uint16_t)(bData[0] + (bData[1] << 8));
            us_bI2c_Write_Stream(E2_Control_Signal_type, bData, 4);
        }
        else if (bChannel == 4)    // 反馈信号
        {
            FeedbackType = (uint16_t)(bData[0] + (bData[1] << 8));
            us_bI2c_Write_Stream(E2_Feedback_Signal_type, bData, 4);
        }
        else if (bChannel == 5)    // 0点死区
        {
            fZeroBand_Percent = (bData[0] + (bData[1] << 8)) / 10.0f;
            us_bI2c_Write_Stream(E2_Zero_DeadBand_Percent, bData, 4);
        }
        else if (bChannel == 6)    // 最小输出
        {
            Out_Min = (float)(bData[0] + (bData[1] << 8));
            us_bI2c_Write_Stream(E2_Out_Min, bData, 4);
        }
        else if (bChannel == 7)    // 最大输出
        {
            Out_Max = (float)(bData[0] + (bData[1] << 8));
            us_bI2c_Write_Stream(E2_Out_Max, bData, 4);
        }
        else
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        us_Respond(*bpCommand2, Successful, 0);    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPC_PID()
 *   功能：		读写KP、KI、KD参数，上位机放大10倍传输
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
void us_PPC_PID(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bSubCmd  = (*(bpCommand2 + 1)) & 0x7F;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    float    temp;
    if (bRorW == 0)    // 读命令
    {
        switch (bSubCmd)
        {
        case 0x01:    // 读Kp
            us_bI2c_Read_Stream(SCALE_KP, bData, 4);
            break;
        case 0x02:    // 读KI
            us_bI2c_Read_Stream(SCALE_KI, bData, 4);
            break;
        case 0x03:    // 读KD
            us_bI2c_Read_Stream(SCALE_KD, bData, 4);
            break;
        default:    // 无效参数
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if (wCRCCheck(bData, 4) != 0)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
        }
        else
        {
            us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
        }
        goto Exit;
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        temp     = (float)(bData[0] + (bData[1] << 8)) / 10.0f;
        /*1-对参数及写入数据容错0.1-100*/
        if ((bSubCmd != 0x01) && (bSubCmd != 0x02) && (bSubCmd != 0x03))
        {
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
        }
        else if (temp < 0.1)
        {
            us_Respond(*bpCommand2, SetDataTooSmall, 0);
            goto Exit;
        }
        else if (temp > 100)
        {
            if (temp == 6553.5)
            {
                bData[0] = 10;
                bData[1] = 0;
                crc      = wCRCCheck(bData, 2);
                bData[2] = (uint8_t)crc;
                bData[3] = (uint8_t)(crc >> 8);
                /*恢复默认值*/
                switch (bSubCmd)
                {
                case 0x01:    // 写Kp
                    scalePam.Kp = 1;
                    us_bI2c_Write_Stream(SCALE_KP, bData, 4);
                    us_bI2c_Read_Stream(SCALE_KP, bData, 4);
                    break;
                case 0x02:    // 写KI
                    scalePam.Ki = 1;
                    us_bI2c_Write_Stream(SCALE_KI, bData, 4);
                    us_bI2c_Read_Stream(SCALE_KI, bData, 4);
                    break;
                case 0x03:    // 写KD
                    scalePam.Kd = 1;
                    us_bI2c_Write_Stream(SCALE_KD, bData, 4);
                    us_bI2c_Read_Stream(SCALE_KD, bData, 4);
                    break;
                }
                us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));    // 低字节在前
            }
            else
            {
                us_Respond(*bpCommand2, SetDataTooBig, 0);
            }
            goto Exit;
        }
        /*对参数进行赋值存储*/
        switch (bSubCmd)
        {
        case 0x01:    // 写Kp
            scalePam.Kp = temp;
            us_bI2c_Write_Stream(SCALE_KP, bData, 4);
            us_bI2c_Read_Stream(SCALE_KP, bData, 4);
            break;
        case 0x02:    // 写KI
            scalePam.Ki = temp;
            us_bI2c_Write_Stream(SCALE_KI, bData, 4);
            us_bI2c_Read_Stream(SCALE_KI, bData, 4);
            break;
        case 0x03:    // 写KD
            scalePam.Kd = temp;
            us_bI2c_Write_Stream(SCALE_KD, bData, 4);
            us_bI2c_Read_Stream(SCALE_KD, bData, 4);
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((*(bpCommand2 + 2) != bData[0]) || (*(bpCommand2 + 3) != bData[1]) || (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));    // 低字节在前
    }
Exit:;
}

/***************************************************
 *   名称：      	us_PPC_DeadBand()
 *   功能：		读写死区值
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
void us_PPC_DeadBand(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bSubCmd  = (*(bpCommand2 + 1)) & 0x7F;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    float    temp;
    if (bRorW == 0)    // 读命令
    {
        switch (bSubCmd)
        {
        case 0x00:    // 读死区
            us_bI2c_Read_Stream(DEAD_BAND, bData, 4);
            break;
        default:    // 无效参数
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if (wCRCCheck(bData, 4) != 0)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
        }
        else
        {
            us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
        }
        goto Exit;
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        switch (bSubCmd)
        {
        case 0x00:    // 写死区
            temp = (float)(bData[0] + (bData[1] << 8)) / 10.0f;
            if (temp == 6553.5)
            {
                bData[0] = 2;
                bData[1] = 0;
                crc      = wCRCCheck(bData, 2);
                bData[2] = (uint8_t)crc;
                bData[3] = (uint8_t)(crc >> 8);
                DeadBand = 0.2;
                us_bI2c_Write_Stream(DEAD_BAND, bData, 4);
                us_bI2c_Read_Stream(DEAD_BAND, bData, 4);
                us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
                goto Exit;
            }
            else if (temp > 5)
            {
                us_Respond(*bpCommand2, SetDataTooBig, 0);
                goto Exit;
            }
            else if (temp < 0.1)
            {
                us_Respond(*bpCommand2, SetDataTooSmall, 0);
                goto Exit;
            }
            DeadBand = temp;
            us_bI2c_Write_Stream(DEAD_BAND, bData, 4);
            us_bI2c_Read_Stream(DEAD_BAND, bData, 4);
            Cal_DeadBand();
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((*(bpCommand2 + 2) != bData[0]) || (*(bpCommand2 + 3) != bData[1]) || (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
    }
Exit:;
}
/***************************************************
 *   名称：      	us_PPC_DeadBand()
 *   功能：		读写零点偏移参数
 *   函数参数：	uint8_t *bpData   源数据指针
 *   			uint8_t bLen      源数据长度
 *   返回值：	void
 ***************************************************/
void us_PPC_ZeroOffset(uint8_t *bpCommand2, uint8_t bLen)
{
    uint8_t  bRorW    = (*(bpCommand2 + 1)) & 0x80;    // 不为0 为写 R(0)/W(!0)
    uint8_t  bSubCmd  = (*(bpCommand2 + 1)) & 0x7F;
    uint8_t  bData[4] = {0};
    uint16_t crc      = 0;
    float    temp;
    if (bRorW == 0)    // 读命令
    {
        switch (bSubCmd)
        {
        case 0x00:    // 读零点偏移
            us_bI2c_Read_Stream(ZERO_OFFSET, bData, 4);
            break;
        default:    // 无效参数
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if (wCRCCheck(bData, 4) != 0)
        {
            us_Respond(*bpCommand2, E2Fail, 0);
        }
        else
        {
            us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
        }
        goto Exit;
    }
    else    // 写命令
    {
        bData[0] = *(bpCommand2 + 2);
        bData[1] = *(bpCommand2 + 3);
        crc      = wCRCCheck(bData, 2);
        bData[2] = (uint8_t)crc;
        bData[3] = (uint8_t)(crc >> 8);
        switch (bSubCmd)
        {
        case 0x00:    // 写零点偏移
            temp = (float)(bData[0] + (bData[1] << 8)) / 10.0f;
            if (temp == 6553.5)
            {
                bData[0]   = 5;
                bData[1]   = 0;
                crc        = wCRCCheck(bData, 2);
                bData[2]   = (uint8_t)crc;
                bData[3]   = (uint8_t)(crc >> 8);
                ZeroOffset = 0.5;
                us_bI2c_Write_Stream(ZERO_OFFSET, bData, 4);
                us_bI2c_Read_Stream(ZERO_OFFSET, bData, 4);
                us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
                goto Exit;
            }
            else if (temp > 10)
            {
                us_Respond(*bpCommand2, SetDataTooBig, 0);
                goto Exit;
            }
            else if (temp < 0.2)
            {
                us_Respond(*bpCommand2, SetDataTooSmall, 0);
                goto Exit;
            }
            ZeroOffset = temp;
            us_bI2c_Write_Stream(ZERO_OFFSET, bData, 4);
            us_bI2c_Read_Stream(ZERO_OFFSET, bData, 4);
            Cal_DeadBand();
            break;
        default:
            us_Respond(*bpCommand2, ObjectDnExist, 0);
            goto Exit;
            break;
        }
        if ((*(bpCommand2 + 2) != bData[0]) || (*(bpCommand2 + 3) != bData[1]) || (wCRCCheck(bData, 4) != 0))
        {
            us_Respond(*bpCommand2, E2Fail, 0);
            goto Exit;
        }
        us_Respond_Seting(*bpCommand2, *(bpCommand2 + 1), Successful, (uint16_t)(bData[0] + (bData[1] << 8)));
    }
Exit:;
}

/***************************************************
 *   名称：      	us_Respond_Seting()
 *   功能：		响应用户设置的KP、I、D、死区及零点偏移数据
 *   函数参数：	uint8_t bCommand1     	命令字
 *   			uint8_t bCommand2		子命令字
 *   			error_Code error	错误代码
 *   			uint16_t wReData			需要传输的数据
 *   返回值：	void
 ***************************************************/
void us_Respond_Seting(uint8_t bCommand1, uint8_t bCommand2, error_Code error, uint16_t wReData)
{
    uint16_t wReCRC = 0;
    bReData[0]      = bCommand1;
    if (error != Successful)
    {
        bReData[1] = 0xff;
        bReData[2] = (uint8_t)(error & 0xff);
        bReData[3] = (uint8_t)((error >> 8) & 0xff);
    }
    else
    {
        bReData[1] = bCommand2;
        bReData[2] = (uint8_t)(wReData & 0xff);
        bReData[3] = (uint8_t)((wReData >> 8) & 0xff);
    }
    wReCRC     = wCRCCheck_Uart_Data(bReData, 4);
    bReData[4] = (uint8_t)(wReCRC & 0xff);
    bReData[5] = (uint8_t)((wReCRC >> 8) & 0xff);
    bReData[6] = 0x0D;
    bReData[7] = 0x0A;
    USBD_VCOM_SendData((const char *)&bReData[0], 8);
}
