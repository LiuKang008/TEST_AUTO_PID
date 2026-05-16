/*
 * us_HC595_3SEG.h
 *
 *  Created on: Jun 10, 2019
 *      Author: cosys engineer4
 */

#ifndef USER_INC_US_HC595_3SEG_H_
#define USER_INC_US_HC595_3SEG_H_
typedef struct
{
    uint8_t bdata_0;
    uint8_t bdata_1;
    uint8_t bdata_2;
    uint8_t bdata_3;
} td_LED_ShowData;

td_LED_ShowData LED_ShowData_K;    // 要显示的字符
uint8_t         us_bLEDShow_IntData(uint16_t wData);
uint8_t         us_bLEDShow_FlData(float flData, uint8_t bflag);
void            us_bLEDShowProcess();
uint8_t         us_bLEDShow_SetData(td_LED_ShowData td_Data);
float           trans_to_psi(uint16_t uwLED_TempValue);

static const uint8_t bLED_Number[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};    // 0~9数码管显示
static const uint8_t bLED_Char[20]   = {0x76, 0x54, 0x5E, 0x71, 0x40, 0x50, 0x58, 0xED, 0x77, 0x3E, 0x73, 0x7C, 0x5C, 0x30, 0x38, 0x39, 0x79, 0x36, 0x6D, 0x48};
// H n d F -   r c S. A U    P b o I L    C E M S =
uint8_t us_bDrvHc595_Led(uint8_t bLedDat);
#endif /* USER_INC_US_HC595_3SEG_H_ */
