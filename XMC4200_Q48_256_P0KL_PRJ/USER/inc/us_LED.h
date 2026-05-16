/*
 * us_LED.h
 *
 *  Created on: May 24, 2024
 *      Author: cosys engineer4
 */

#ifndef USER_INC_US_LED_H_
#define USER_INC_US_LED_H_
typedef enum
{
    STATE_UNCALIBRATED,
    STATE_UNSET,
    STATE_NORMAL_OPERATION
} SystemState;

// 定义手动设定标志的结构体
typedef struct
{
    uint8_t Hand_RangeFlag;
    uint8_t Hand_ControlFlag;
    uint8_t Hand_FeedbackFlag;
    uint8_t Hand_ProductFlag;
} HandSetFlags;

void us_LED_Status_Process(void);

#endif /* USER_INC_US_LED_H_ */
