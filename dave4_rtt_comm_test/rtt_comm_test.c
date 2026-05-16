#include "rtt_comm_test.h"

#include "SEGGER_RTT.h"
#include <stdio.h>
#include <string.h>

#ifndef RTT_COMM_TEST_CMD_BUF_SIZE
#define RTT_COMM_TEST_CMD_BUF_SIZE    (96u)
#endif

#ifndef RTT_COMM_TEST_TX_PERIOD_MS
#define RTT_COMM_TEST_TX_PERIOD_MS    (500u)
#endif

/*
 * Test variables. They intentionally look like PID variables so the existing
 * llm-pid-tuner CSV parser can verify the data path before the real control
 * loop is connected.
 */
static uint32_t g_ms = 0u;
static uint32_t g_last_tx_ms = 0u;
static uint32_t g_rx_count = 0u;
static uint32_t g_cmd_error_count = 0u;

static float g_setpoint = 100.0f;
static float g_input = 20.0f;
static float g_pwm = 0.0f;
static float g_kp = 1.0000f;
static float g_ki = 0.1000f;
static float g_kd = 0.0100f;

static char g_cmd_buf[RTT_COMM_TEST_CMD_BUF_SIZE];
static uint16_t g_cmd_len = 0u;

static void RTT_CommTest_SendStatus(void);
static void RTT_CommTest_SendCsv(void);
static void RTT_CommTest_ProcessCommand(const char *cmd);
static void RTT_CommTest_UpdateFakePlant(uint32_t period_ms);

void RTT_CommTest_Init(void)
{
    g_ms = 0u;
    g_last_tx_ms = 0u;
    g_rx_count = 0u;
    g_cmd_error_count = 0u;
    g_cmd_len = 0u;

    SEGGER_RTT_WriteString(0, "# DAVE4 XMC RTT communication test ready\r\n");
    SEGGER_RTT_WriteString(0, "# CSV: timestamp_ms,setpoint,input,pwm,error,p,i,d\r\n");
    SEGGER_RTT_WriteString(0, "# Commands: PING | STATUS | SET P:1.2 I:0.03 D:0 | PID 1.2 0.03 0\r\n");
}

void RTT_CommTest_Task_1ms(void)
{
    RTT_CommTest_Task_Periodic(1u);
}

void RTT_CommTest_Task_Periodic(uint32_t period_ms)
{
    char c;

    if (period_ms == 0u)
    {
        period_ms = 1u;
    }

    g_ms += period_ms;

    while (SEGGER_RTT_Read(0, &c, 1u) == 1u)
    {
        if (c == '\r')
        {
            continue;
        }

        if (c == '\n')
        {
            g_cmd_buf[g_cmd_len] = '\0';
            if (g_cmd_len > 0u)
            {
                RTT_CommTest_ProcessCommand(g_cmd_buf);
            }
            g_cmd_len = 0u;
            continue;
        }

        if (g_cmd_len < (RTT_COMM_TEST_CMD_BUF_SIZE - 1u))
        {
            g_cmd_buf[g_cmd_len++] = c;
        }
        else
        {
            g_cmd_len = 0u;
            g_cmd_error_count++;
            SEGGER_RTT_WriteString(0, "# ERR command buffer overflow\r\n");
        }
    }

    RTT_CommTest_UpdateFakePlant(period_ms);

    if ((g_ms - g_last_tx_ms) >= RTT_COMM_TEST_TX_PERIOD_MS)
    {
        g_last_tx_ms = g_ms;
        RTT_CommTest_SendCsv();
    }
}

static void RTT_CommTest_UpdateFakePlant(uint32_t period_ms)
{
    float error;
    float alpha;

    error = g_setpoint - g_input;
    g_pwm = (g_kp * error) + (g_ki * 10.0f) - (g_kd * 5.0f);

    if (g_pwm < 0.0f)
    {
        g_pwm = 0.0f;
    }
    if (g_pwm > 1000.0f)
    {
        g_pwm = 1000.0f;
    }

    /* Slow first-order fake plant, only for communication verification. */
    alpha = ((float)period_ms) * 0.0005f;
    if (alpha > 0.05f)
    {
        alpha = 0.05f;
    }
    g_input += error * alpha;
}

static void RTT_CommTest_SendCsv(void)
{
    float error = g_setpoint - g_input;

    SEGGER_RTT_printf(0,
        "%lu,%.3f,%.3f,%.3f,%.3f,%.5f,%.5f,%.5f\r\n",
        (unsigned long)g_ms,
        g_setpoint,
        g_input,
        g_pwm,
        error,
        g_kp,
        g_ki,
        g_kd);
}

static void RTT_CommTest_SendStatus(void)
{
    SEGGER_RTT_printf(0,
        "# STATUS ms=%lu rx=%lu err=%lu set=%.3f input=%.3f pwm=%.3f P=%.5f I=%.5f D=%.5f\r\n",
        (unsigned long)g_ms,
        (unsigned long)g_rx_count,
        (unsigned long)g_cmd_error_count,
        g_setpoint,
        g_input,
        g_pwm,
        g_kp,
        g_ki,
        g_kd);
}

static void RTT_CommTest_ProcessCommand(const char *cmd)
{
    float p;
    float i;
    float d;

    g_rx_count++;

    if (strcmp(cmd, "PING") == 0)
    {
        SEGGER_RTT_printf(0, "# ACK PONG ms=%lu rx=%lu\r\n", (unsigned long)g_ms, (unsigned long)g_rx_count);
        return;
    }

    if (strcmp(cmd, "STATUS") == 0)
    {
        RTT_CommTest_SendStatus();
        return;
    }

    if (sscanf(cmd, "SET P:%f I:%f D:%f", &p, &i, &d) == 3)
    {
        g_kp = p;
        g_ki = i;
        g_kd = d;
        SEGGER_RTT_printf(0, "# ACK SET P=%.5f I=%.5f D=%.5f\r\n", g_kp, g_ki, g_kd);
        return;
    }

    if (sscanf(cmd, "PID %f %f %f", &p, &i, &d) == 3)
    {
        g_kp = p;
        g_ki = i;
        g_kd = d;
        SEGGER_RTT_printf(0, "# ACK PID P=%.5f I=%.5f D=%.5f\r\n", g_kp, g_ki, g_kd);
        return;
    }

    g_cmd_error_count++;
    SEGGER_RTT_printf(0, "# ERR unknown command: %s\r\n", cmd);
}
