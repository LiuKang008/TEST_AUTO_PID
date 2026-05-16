#ifndef RTT_COMM_TEST_H_
#define RTT_COMM_TEST_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal SEGGER RTT communication test module for DAVE4 / XMC4200.
 *
 * Usage:
 *   1. Make sure SEGGER_RTT.c/.h are already included in your DAVE4 project.
 *   2. Add rtt_comm_test.c and rtt_comm_test.h to the project.
 *   3. Call RTT_CommTest_Init() once after board/app initialization.
 *   4. Call RTT_CommTest_Task_1ms() from a 1 ms task, SysTick hook, or main loop
 *      with approximately 1 ms cadence. If your loop is not exactly 1 ms, use
 *      RTT_CommTest_Task_Periodic(period_ms) instead.
 *
 * The module prints a CSV line that can be parsed by llm-pid-tuner:
 *   timestamp_ms,setpoint,input,pwm,error,p,i,d
 *
 * It also reads text commands from RTT down channel 0:
 *   PING
 *   STATUS
 *   SET P:1.2000 I:0.0300 D:0.0000
 *   PID 1.2000 0.0300 0.0000
 */

void RTT_CommTest_Init(void);
void RTT_CommTest_Task_1ms(void);
void RTT_CommTest_Task_Periodic(uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif /* RTT_COMM_TEST_H_ */
