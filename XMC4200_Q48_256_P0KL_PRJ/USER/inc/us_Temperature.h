/*
 * us_Temperature.h
 *
 *  Created on: 2015-4-5
 *      Author: pnzrk
 */

#ifndef US_TEMPERATURE_H_
#define US_TEMPERATURE_H_

void     us_Temp_Init(void);
void     us_Temp_Process(void);
uint16_t us_Get_TempData(void);

#endif /* US_TEMPERATURE_H_ */
