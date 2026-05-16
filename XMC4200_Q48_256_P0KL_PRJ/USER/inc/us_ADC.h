/*
 * us_ADC.h
 *
 *  Created on: Apr 22, 2019
 *      Author: cosys SpartaK
 */

#ifndef USER_INC_US_ADC_H_
#define USER_INC_US_ADC_H_
#define Filter_Num 3    // 滤波器数值，几个求平均
/*
#define MAX_10V    0x3C4		//10V 由6.2K与3.9K分压，基准电压4.096V
#define Err_10V    0x3F4		//10.5V为0~10V输出的上限，超过此值，"Err1"
#define MAX_5V     0x339		// 5V 由2K与3.9K分压，基准电压4.096V
#define Err_5V     0x212		// (5.5*3.9/(6.2+3.9))*1023/4.096=0x212 5.5V为0~5V输出的上限，超过此值， "Err1"
#define Err_20mA   0x383		// 5.5V为0~5V输出的上限，超过此值， "Err1"
#define Err_4mAH   0x09c		//3.5mA为4~20mA输入下限，低于此值， "Err1"
#define Err_4mAL   0x016		//0.5mA为4~20mA输入错误识别值，超过此值则为有效信号

#define max_ad_current              0x383				   //3.6V
#define min_ad_current              0x02				   //0.01V---0.5psi
#define valve_controller_step_period 10
#define	min_ad_voltage              0x04				   //0.05V---0.5psi
 */
#define MAX_10V    0x3C4    // 10V 由6.2K与3.9K分压，基准电压4.096V
#define Err_10V    0x3F4    // 10.5V为0~10V输出的上限，超过此值，"Err1"
#define MAX_5V     0x339    // 5V 由2K与3.9K分压，基准电压4.096V
#define Err_5V     0x212    // (5.5*3.9/(6.2+3.9))*1023/4.096=0x212 5.5V为0~5V输出的上限，超过此值， "Err1"
#define Err_20mA   680      // 674//940//988//682		    // 5.5V为0~5V输出的上限，超过此值， "Err1"
#define Err_4mAH   117      // 162//167//119		//3.5mA为4~20mA输入下限，低于此值， "Err1"
#define Err_4mAL   16       // 23//17		//0.5mA为4~20mA输入错误识别值，超过此值则为有效信号

// #define min_ad_current             40// 3				   //0.01V---0.5psi
// 参考电压2.048
// #define ad_current_4mA              793//702//789//646//640//136//134//186//191//135                //100*0.004/3*1023=136  实测135
// #define max_ad_current             3981//3521//3981//4000//3246//680//674//940//964// 682				   //2V
// 参考电压2.5v
// #define ad_current_4mA              527//417//652//645
// #define max_ad_current              2175//2077//3268//3248

// #define valve_controller_step_period 10
// #define	min_ad_voltage              13//0//3//6//40//6//0x04				   //0.05V---0.5psi
// #define max_ad_voltage              2052//2043//3969//3254//3847//1983//949//1307//709//0x3C4
#define adjust_range1 0.5
#define adjust_range2 1
#define adjust_k      (adjust_range2 - adjust_range1) / adjust_range2;
//****************************************************************************
// @Global Variables
//****************************************************************************
//****************************************************************************
// @Prototypes Of Global Functions
//****************************************************************************
void     ADC_Get_Data(void);
uint16_t Voltage_Current_Filter(uint16_t filter[10]);
void     ADC_Data_Handel(void);
void     SetValue_Get(void);
void     PsiShowData_Get(void);
// void FBVI_DataGet(void);
void     ErrorData_Get(void);
float    MinMaxPsiAdjust1(uint32_t ubMin, uint32_t ubMax, float fSet);
void     us_bADCInit(void);
void     us_bADC_Process(void);
uint8_t  us_Get_ADCResult(uint16_t *wpResult, uint8_t bLen);
uint16_t us_Get_ADC_Chnn(uint8_t bChnn);
uint16_t us_Get_AveInputADC_Chnn(uint8_t bChnn);
uint16_t us_Get_Original_ADC_Chnn(uint8_t bChnn);
// 进气阀线圈电流的函数
void CheckIntakeValveCoil(uint16_t adcValue);
// 排气气阀线圈电流的函数
void CheckexhaustValveCoil(uint16_t adcValue);
void us_Read_Power_Voltage_Process(void);
#endif /* USER_INC_US_ADC_H_ */
