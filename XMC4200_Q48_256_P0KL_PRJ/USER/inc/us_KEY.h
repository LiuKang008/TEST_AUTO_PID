/*
 * KEY.h
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */

#ifndef us_KEY_H_
#define us_KEY_H_

#define ReadKey_Up   DIGITAL_IO_GetInput(&IO_KEY_UP)
#define ReadKey_Down DIGITAL_IO_GetInput(&IO_KEY_DOWN)
#define ReadKey_Ok   DIGITAL_IO_GetInput(&IO_KEY_OK)

typedef enum Key_Handle
{
    Key_Idle,    // 无
    Key_Up,
    Key_Down,
    Key_Ok
} Key_Handle_t;

typedef struct Key_Event
{
    uint8_t (*Key_Up_Event)(uint8_t *bpdata);
    uint8_t (*Key_Down_Event)(uint8_t *bpdata);
    uint8_t (*Key_Ok_Event)(uint8_t *bpdata);
} Key_Event_t;

typedef uint8_t (*KeyCall_Event)(uint8_t *bpdata);

typedef struct
{
    uint8_t  H01;
    uint16_t H02;
    uint16_t Max_H02;
    s16      H03;
    uint16_t H04;
    uint16_t H05;
    uint8_t  H06;
    uint16_t H061;
    uint16_t H062;
    uint16_t H063;
    uint16_t H064;
    s16      H07;
    uint16_t H08;
    uint16_t Max_H08;
    uint16_t H081;
    uint16_t H082;
    uint8_t  H09;
    uint16_t H10;
    uint8_t  H11;
    uint8_t  H12;
    uint8_t  H13;
    uint8_t  H14[4];
    uint8_t  H15;
} KEY_MENU;

uint8_t us_bReadKey_Process(uint8_t *bpdata);
uint8_t us_bGetShowData();
void    us_Key_Init(void);

KEY_MENU KEY_MENU_Data;         // 设定好的各个参数
KEY_MENU KEY_MENU_Temp_Data;    // 各个参数的中间值
uint8_t  bButtonDeep_OK;        // 确定按键的深度
#endif                          /* KEY_H_ */
