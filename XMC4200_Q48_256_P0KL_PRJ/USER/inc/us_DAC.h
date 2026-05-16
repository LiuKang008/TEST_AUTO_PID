/*
 * us_DAC.h
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */

#ifndef US_DAC_H_
#define US_DAC_H_

#define mA_Current 0x00
#define mV_Voltage 0x01
#define Mode_None  0x02
#define OutPut1    0x01
#define OutPut2    0x02

void     us_DAC_Init(void);
void     us_Set_DAC_Temp(uint16_t wValue, uint8_t bFlag, uint8_t bChannel);
uint8_t  us_bGet_Channel_Mode(uint8_t bChnn);
uint8_t  us_bGet_TTL_Mode(void);
uint8_t  us_bGet_Valid_Mode(void);
uint16_t us_wGet_Chnn_Adjust(uint8_t bChnn);
uint8_t  us_bSet_DACValue(uint16_t wValue, uint8_t bFlag, uint8_t bChannel);
float    us_wGet_Chnn_ADC_Adjust(uint8_t bChnn);
void     us_bSet_DAC_Adjust(uint8_t bChnn);

#endif /* US_DAC_H_ */
