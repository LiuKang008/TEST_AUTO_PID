/*
 * DICT.h
 *
 *  Created on: 2014-8-11
 *      Author: dxm
 */

#ifndef DICT_H_
#define DICT_H_

#define CY033       1
#define P1K         0
#define P2K         1
// #define P1K_P2K 0
#define P3K         0
#define FIM         0    // FIM 1 FIM阀门快一点， 0正常阀门
#define UNIT_CHANGE 1
#define NO_DIS_ERR2 0    // 不显示Err2

#define period 22221     // 37500//40000//

void PWM1_SetCompare(uint32_t compare);
void PWM2_SetCompare(uint32_t compare);

#define Input_Valve_Open_ISR          PWM1_SetCompare(11111)
#define Input_Valve_Close_ISR         PWM1_SetCompare(period)
#define Output_Valve_Close_ISR        PWM2_SetCompare(period)
#define Output_Valve_Open_ISR         PWM2_SetCompare(11111)
#define Input_Valve_PWM_ISR(PwmDuty)  PWM1_SetCompare(PwmDuty)
#define Output_Valve_PWM_ISR(PwmDuty) PWM2_SetCompare(PwmDuty)

void Input_Valve_Open(void);
void Input_Valve_Close(void);
void Output_Valve_Close(void);
void Output_Valve_Open(void);
void Output_Valve_Open_EXH(void);
void Input_Valve_PWM(uint32_t Compare);
void Output_Valve_PWM(uint32_t Compare);

void PWM_DutyCon(void);
void PI_controller(float ref_psi, float fact_psi);
void Cal_DeadBand(void);

PWM_STATUS_t PWM_SetCompare(PWM_t *const handle_ptr, uint32_t compare);

#endif /* DICT_H_ */
