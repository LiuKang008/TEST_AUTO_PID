/*
 * us_24AA64F_E2ROM.h
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */

#ifndef US_24AA64F_E2PROM_H_
#define US_24AA64F_E2PROM_H_

#define SaveDataNum  30
#define EEP_COMP_CNT 30

typedef struct
{
    /*
    present[0]=uwSaveUseIfo;
    present[1]=ubMinScalePsi10;
    present[2]=ubMaxScalePsi10;
    present[3]=calibration.scale; 量程最大值
    present[4]=calibration.min_pressure;
    present[5]=calibration.min_AD;
    present[6]=calibration.max_pressure;
    present[7]=calibration.max_AD;
    present[8]=ubLED_PageNum;
    present[9]= ubRelP1
    present[10]= ubRelP2
    present[11]= ubSegP1
    present[12]= ubSegP2
    present[13]= ubSegP3
    present[14]= ubSegP4
    present[15]= Man Set
    present[16]==0x5AA5  标志位
    present[17]= DeadBand_mode；
    present[18]=ubMaxScalePsi10_not_modify;
    present[19]=             // 反向设定 ｒ－０＝反向关闭，ｒ－１＝反向开启
    present[20]=             //控制曲线设定  ｃ－Ｓ＝标准，ｃ－Ｆ＝快速，ｃ－Ａ＝精确
    present[21]=             //压力显示设定  ｃ－Ｓ＝标准，ｃ－Ｆ＝快速，ｃ－Ａ＝精确
    present[22]
    present[23] 未定义
    present[24] 量程最小值 标定范围最小值，通常为0
    present[29]=sum;
     */
    uint16_t present[30];
    uint16_t eeprom[30];
    //	uint16_t  bufB[25];
    uint16_t EepSumCheck;
    uint16_t FlashSumCheck;
    uint8_t  fEepDatAOK;
    uint8_t  fEepDatBOK;
    uint8_t  fEepDatCOK;
    uint8_t  fDatChange;
    uint8_t  fFlashDatOK;
} eeprom_flash_data;
// eeprom_flash_data  EepFlashDat;

typedef struct
{
    uint16_t pressure_scale_kpa;
    uint16_t min_AD;
    uint16_t min_AD2;
    uint16_t min_pressure_kpa;
    uint16_t min_pressure_psi;
    uint16_t max_AD;
    uint16_t max_AD2;
    uint16_t max_pressure_kpa;
    uint16_t max_pressure_psi;
    uint8_t  fCalMin;
    uint8_t  fCalMax;
    uint8_t  fCalScale;
    uint8_t  fCalSave;
    uint8_t  fError;
    uint8_t  fCalADC;
    uint8_t  fCalDAC;
} pressure_calibration;
// pressure_calibration calibration;

typedef struct
{
    float    integration;
    float    error;
    float    delta_error;
    float    last_error;
    float    fact_psi;
    float    ref_psi;
    float    max_psi;
    float    min_psi;
    int32_t  control_value;
    uint16_t error_counter;
    uint16_t counter;
    uint8_t  f_overshoot;
    uint8_t  f_reach;
    uint8_t  mode;
} controller_struct;
// controller_struct 	global_controller;

uint8_t  Mem_Init(void);
uint8_t  decrypt(void);
void     encrypte(void);
uint8_t  Dat_Save_EepromC(void);
uint8_t  Dat_Save_EepromB(void);
uint8_t  Dat_Save_Eeprom(void);
void     EraseLed_Eeprom(void);
uint8_t  EEP_Write(uint16_t add, uint16_t dat);
void     read_eepromC(void);
void     read_eepromB(void);
void     read_eepromA(void);
void     EEP_Write_Data(uint16_t address, uint16_t input_word);
uint16_t EEP_Read_Data(uint16_t address);
void     EEP_Read(void);
uint8_t  us_bI2c_Test(void);
uint8_t  us_bI2c_Write_Byte(uint16_t wAddress, uint8_t bData);
uint8_t  us_bI2c_Read_Stream(uint16_t wAddress, uint8_t *bpData, uint8_t bLen);
uint8_t  us_bI2c_Write_Stream(uint16_t wAddress, uint8_t *bpData, uint8_t bLen);
uint8_t  I2CWriteData(uint8_t bCommand, uint16_t wAddress, uint8_t *bpData, uint8_t bLen);
uint8_t  I2CReadData(uint8_t bCommand, uint16_t wAddress, uint8_t *bpData, uint8_t bLen);
uint8_t  us_bI2c_Test(void);
void     us_Delay_5ms();
void     us_Delay_1us(uint32_t dwTime);
void     us_I2C_Init();
void     I2CTest();

#endif /* US_24AA64F_E2PROM_H_ */
