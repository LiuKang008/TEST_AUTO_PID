/*
 * us_UART.c
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */
#include "us_UserConfig.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
uint8_t g_Uart_Flag = 0;    // 串口建立标志
// 环形缓冲区定义
#define BUFFER_SIZE 128
#define BUFFER_MASK (BUFFER_SIZE - 1)
volatile uint8_t bufferx[BUFFER_SIZE];
volatile int     buffer_head = 0;
volatile int     buffer_tail = 0;
int              g_SetValue  = 0;    // 设定值
int              g_ActValue  = 0;    // 实际值
uint8_t          Uart_timCnt;

void us_bUARTReceiveDecode(unsigned char bData);
void decode_command(uint16_t start, uint16_t end);

// 串口接收中断函数：仅接收数据并存储到缓冲区
void uart_ReceiveOneByte_Interrupt(void)
{
    static uint8_t ReceiveData;

    // 判断接收标志位，如果有接收数据则读取接收数据
    if (UART_GetFlagStatus(&UART_0, (XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)))
    {
        // 读取接收到的字节数据
        ReceiveData = XMC_UART_CH_GetReceivedData(UART_0.channel);
        UART_ClearFlag(&UART_0, (XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION));
        UART_Transmit(&UART_0, &ReceiveData, 1);
        // 将接收到的数据存储到环形缓冲区中
//        bufferx[buffer_head] = ReceiveData;
//        if (++buffer_head >= BUFFER_SIZE)
//        {
//            buffer_head = 0;
//        }
//        // 处理缓冲区溢出的情况
//        if (buffer_head == buffer_tail)
//        {
//            // 如果缓冲区已满，丢弃最老的数据
//            if (++buffer_tail >= BUFFER_SIZE)
//            {
//                buffer_tail = 0;
//            }
//        }
    }
}

// 用于处理跨越环形缓冲区边界的strncmp函数
int circular_strncmp(int tail, const char *command, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (bufferx[(tail + i) & BUFFER_MASK] != command[i])
        {
            return 1;    // 不相等
        }
    }
    return 0;    // 相等
}
// 提取数值（例如从"SET 100\r\n"中提取100）
int extract_value(int start, int end)
{
    char value_str[5] = {0};    // 最大支持4个字符的数值+1个终止符
    int  len          = 0;
    int  i;

    for (i = start; (i != ((end + 1) & BUFFER_MASK)) && (len < 4); i = (i + 1) & BUFFER_MASK)
    {
        if (isdigit(bufferx[i]))
        {
            value_str[len++] = bufferx[i];
        }
        else
        {
            return -1;    // 如果遇到非数字字符，返回-1表示错误
        }
    }
    // 检查是否为空字符串
    if (len == 0)
    {
        return -1;    // 空字符串，返回-1表示错误
    }
    // 检查字符串长度是否超过4个字符
    if (len == 4 && (i != ((end + 1) & BUFFER_MASK)))
    {
        return -1;    // 长度超过4个字符，返回-1表示错误
    }

    return atoi(value_str);    // 将字符串转换为整数
}

void UART_SendResponse(const char *response)
{
    UART_Transmit(&UART_0, (unsigned char *)response, strlen(response));
}
void process_uart_data(void)
{
    static int buffer_head_Pre;
    if (buffer_head_Pre != buffer_head)
    {
        while (buffer_tail != buffer_head)
        {
            // 检查缓冲区内至少有5个字节（最短命令长度）
            int available_data = (buffer_head >= buffer_tail) ?
                                     (buffer_head - buffer_tail) :
                                     (BUFFER_SIZE - buffer_tail + buffer_head);

            if (available_data >= 5)
            {
                // 遍历查找从buffer_tail到buffer_head的完整帧
                uint16_t current_index = buffer_tail;
                int      frame_found   = 0;

                while (current_index != buffer_head)
                {
                    // 检查是否找到 "\r\n"
                    if (bufferx[current_index] == '\n' &&
                        bufferx[(current_index - 1 + BUFFER_SIZE) & BUFFER_MASK] == '\r')
                    {
                        // 找到完整的一帧，从buffer_tail到current_index
                        frame_found = 1;

                        // 解码并处理命令帧
                        decode_command(buffer_tail, current_index);
                        g_Uart_Flag = 1;
                        Uart_timCnt = 0;

                        // 更新buffer_tail到当前帧结束的位置之后
                        buffer_tail = (current_index + 1) & BUFFER_MASK;

                        // 继续寻找下一帧
                        break;
                    }

                    // 移动到下一个字节
                    current_index = (current_index + 1) & BUFFER_MASK;
                }

                if (!frame_found)
                {
                    // 如果没有找到完整帧，退出循环等待更多数据
                    buffer_head_Pre = buffer_head;
                    break;
                }
            }
            else
            {
                // 可用数据不足5字节，不进行处理，退出循环
                buffer_head_Pre = buffer_head;
                break;
            }
        }
    }
}

// decode_command 函数：从buffer_tail到last_index解码并处理命令
void decode_command(uint16_t start, uint16_t end)
{
    int set_value;
    int start_pos;
    int end_pos;

    // 解析命令内容，这里仅示例解码逻辑
    // 处理 "SET " 命令
    if (circular_strncmp(start, "SET ", 4) == 0)
    {
        start_pos = (start + 4) & BUFFER_MASK;                // SET 后的值起始位置
        end_pos   = (end - 2 + BUFFER_SIZE) & BUFFER_MASK;    // \r\n 前的位置

        set_value = extract_value(start_pos, end_pos);

        if (set_value >= 0 && set_value <= 1023)
        {
            g_SetValue = set_value;
            char response[10];
            snprintf(response, sizeof(response), "%d\r\n", set_value);
            UART_SendResponse(response);
        }
        else if (set_value > 1023 && set_value < 9999)
        {
            UART_SendResponse("OUT OF RANG\r\n");
        }
        else
        {
            UART_SendResponse("UNKNOWN COMMAND\r\n");
        }
    }
    // 处理 "INC" 命令
    else if (circular_strncmp(start, "INC\r\n", 5) == 0)
    {
        if (g_SetValue + 2 > 1023)
        {
            g_SetValue = 1023;
        }
        else
        {
            g_SetValue += 2;
        }
        UART_SendResponse("mm\r\n");    // 回复 'm'+'m'+\r+\n
    }
    // 处理 "DEC" 命令
    else if (circular_strncmp(start, "DEC\r\n", 5) == 0)
    {
        if (g_SetValue <= 2)
        {
            g_SetValue = 0;
        }
        else
        {
            g_SetValue -= 2;
        }
        UART_SendResponse("mm\r\n");    // 回复 'm'+'m'+\r+\n
    }
    // 处理 "REQ" 命令
    else if (circular_strncmp(start, "REQ\r\n", 5) == 0)
    {
        char response[10];
        snprintf(response, sizeof(response), "%d\r\n", g_SetValue);
        UART_SendResponse(response);
    }
    // 处理 "MON" 命令
    else if (circular_strncmp(start, "MON\r\n", 5) == 0)
    {
        char response[10];
        snprintf(response, sizeof(response), "%d\r\n", g_ActValue);
        UART_SendResponse(response);
    }
    // 未知命令处理
    else
    {
        UART_SendResponse("UNKNOWN COMMAND\r\n");
    }
}
