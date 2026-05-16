/*
 * us_Systimer.h
 *
 *  Created on: Apr 22, 2019
 *      Author: cosys SpartaK
 */

#ifndef US_SYSTIMER_H_
#define US_SYSTIMER_H_
#include "us_UserConfig.h"
void     us_SYSTIMER_Init(uint32_t dwInit_TimeCount);
void     us_SetSystemTime(uint32_t dwSet_SYSTimeCount);
uint32_t us_dwGetSystemTime(void);

#endif /* US_SYSTIMER_H_ */
