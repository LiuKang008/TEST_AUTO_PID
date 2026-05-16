/*
 * us_USB.h
 *
 *  Created on: 2015-3-25
 *      Author: Administrator
 */

#ifndef US_USB_H_
#define US_USB_H_

// 定义存储地址

/*从0x40开始*/
#define mcu_range_max  0x40    // 64
#define mcu_range_min  0x44    // 68
#define mcu_range_max2 0x48    // 72
#define mcu_range_min2 0x4c    // 76
#define mcu_max_ADC_01 0x50    // 80
#define mcu_min_ADC_01 0x52    // 82
#define mcu_max_ADC_02 0x54    // 84
#define mcu_min_ADC_02 0x56    // 86
#define mcu_max        0x58    // 88
#define mcu_min        0x5a    // 90
#define mcu_flag       0x5c    // 92
#define mcu_crc        0x5e    // 94
#define mcu_id1        0x60    // 96
#define mcu_id2_h      0x64    // 100
#define mcu_id2_l      0x68    // 104
#define mcu_id3        0x6c    // 108
#define mcu_id4        0x70    // 112
#define product_type   0x74    // 116

#define TestEEPROM 0x78

#define adPPCAdjust0 0x80         // 128
#define adPPCAdjust1 0x84         // 132

#define adPPCADCAdjust0 0x88      // 136
#define adPPCADCAdjust1 0x8c      // 140
#define adPPCADCAdjust2 0x90      // 144
#define adPPCADCAdjust3 0x94      // 148

#define ADC_Adj_S      0xA0       // 4* 7 个字节//160
#define DAC_Adj_S      0xE0       // 32*2 *2//224
#define DAC_Adj_CRC_01 0x180      // 384
#define DAC_Adj_CRC_02 0x184      // 388

#define DAC_Adj_SFlag_01 0x188    // 392
#define DAC_Adj_SFlag_02 0x18C    // 396

#define HandSetRange_Flag    0x190
#define HandSetControl_Flag  0x194
#define HandSetFeedback_Flag 0x198

#define E2_Control_DeadBand_Percent 0x19C
#define E2_Control_Signal_type      0x1A0
#define E2_Feedback_Signal_type     0x1A4
#define E2_Zero_DeadBand_Percent    0x1A8
#define E2_Out_Min                  0x1AC
#define E2_Out_Max                  0x1B0

#define SCALE_KP    0x1C0
#define SCALE_KI    0x1C4
#define SCALE_KD    0x1C8
#define DEAD_BAND   0x1CC
#define ZERO_OFFSET 0x1D0

/*定义KP、KI、KD比例参数*/
typedef struct
{
    float Kp;
    float Ki;
    float Kd;
} ScalePam;

uint8_t us_USBInit(void);
void    us_USB_Process(void);
typedef enum
{
    Successful      = 0x0000,
    SetDataTooBig   = 0x0001,
    SetDataTooSmall = 0x0002,
    SetDataDnEqual  = 0x0003,
    ReadOnlyWrite   = 0x0004,
    writeOnlyRead   = 0x0005,
    UnknowCommand   = 0x0006,
    CRCFail         = 0x0007,
    E2Fail          = 0x0008,
    ObjectDnExist   = 0x0009
} error_Code;

#define ADC_Mode01      // IO004_ReadPin(IO004_Handle18)
#define ADC_Mode02      // IO004_ReadPin(IO004_Handle13)

#define Range_Mode01    // IO004_ReadPin(IO004_Handle11)
#define Range_Mode02    // IO004_ReadPin(IO004_Handle12)

#define DAC1C   0x04
#define DAC1V   0x05
#define DAC12CC 0x08
#define DAC12CV 0x09
#define DAC12VC 0x0a
#define DAC12VV 0x0b

#define ADCIN4_20mA 0x00
#define ADCIN0_10V  0x01
#define ADCINSwitch 0x02
#define ADCIN0_20mA 0x0c
#define ADCIN0_5V   0x0d

// 控制信号
#define D2_ADCIN4_20mA    0x00
#define D2_ADCIN0_20mA    0x01
#define D2_ADCIN0_5V      0x02
#define D2_ADCIN0_10V     0x03
#define D2_ADCIN_Neg2_5V  0x04
#define D2_ADCIN_Neg2_10V 0x05
#define D2_ADCIN_1_10V    0x06

// 反馈信号
// 0:4-20mA
// 1:0-10V
// 2:1-10V
#define D2_DAC4_20mA 0x00
#define D2_DAC0_10V  0x01
#define D2_DAC1_10V  0x02

typedef struct
{
    uint8_t ID3_In : 2;
    uint8_t ID3_Out : 2;
    uint8_t ID3_Unit : 2;
    uint8_t ID3_FeedBack : 2;
} ID3;

typedef struct
{
    uint16_t range_max;
    uint16_t range_min;
    uint16_t range_max2;
    uint16_t range_min2;
    uint16_t id1;
    uint16_t id2_h;
    uint16_t id2_l;
    uint16_t id3;
    uint16_t id4;
    uint16_t max_ADC_01;
    uint16_t min_ADC_01;
    uint16_t max_ADC_02;
    uint16_t min_ADC_02;
    uint16_t max;
    uint16_t min;
    uint16_t flag;
    uint16_t crc;
} mcu_type;

// typedef struct
//{
//	uint8_t bFlag;
//	uint16_t dMin;
//	uint16_t dMax;
//	uint16_t crc;
// }ADCAdjT;

typedef struct
{
    uint8_t  bFlag;
    uint16_t dMin;
    uint16_t dMid;
    uint16_t dMax;
    uint16_t crc;
} ADCAdjT;

ID3      us_bGet_ID3();
uint8_t  us_bSet_ID3(uint8_t bData);
double   us_bGet_B(uint8_t bChnn);
double   us_bGet_K(uint8_t bChnn);
uint8_t  us_bGetDACFlag(uint8_t bChnn);
void     us_bSetDACFlag(uint8_t bChnn, uint8_t bdata);
uint16_t wCRCCheck(uint8_t *bpData, uint8_t bLen);
uint8_t  us_bGetDACMode(void);
uint8_t  us_bGetADCINMode(void);
uint8_t  us_bGetADCFeedbackMode(void);
double   us_Adj_ADC_K_B(uint8_t bChnn, double dlData);
uint16_t wCRCCheck_Uart_Data(uint8_t *bpData, uint8_t bLen);

uint8_t us_bGet_D2_ADCINMode(void);
uint8_t us_bGet_D2_DACFeedbackMode(void);

#endif /* US_USB_H_ */
