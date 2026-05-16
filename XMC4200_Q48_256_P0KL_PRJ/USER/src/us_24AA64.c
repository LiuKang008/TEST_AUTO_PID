/*
 * us_24AA64.c
 *
 *  Created on: Apr 22, 2019
 *      Author: cosys SpartaK
 */
#include "us_UserConfig.h"

#define I2C_CLK_H()    DIGITAL_IO_SetOutputHigh(&IO_24AA64_SCK)
#define I2C_CLK_L()    DIGITAL_IO_SetOutputLow(&IO_24AA64_SCK)
#define I2C_DAT_H()    DIGITAL_IO_SetOutputHigh(&IO_24AA64_SDA)
#define I2C_DAT_L()    DIGITAL_IO_SetOutputLow(&IO_24AA64_SDA)
#define I2C_DAT_Read() DIGITAL_IO_GetInput(&IO_24AA64_SDA)

extern KEY_MENU      KEY_MENU_Temp_Data;
extern ID3           bmyID3;
uint8_t              ubMinScalePsi;
float                ubMaxScalePsi;
uint16_t             uwSaveUseIfo;
uint8_t              ubErrCode;    //
uint8_t              RepairErr3Counter;
uint8_t              WriteCounter;
uint32_t             ubMinScalePsi10;
uint32_t             ubMaxScalePsi10;
controller_struct    global_controller;
eeprom_flash_data    EepFlashDat;
pressure_calibration calibration;

extern void Read_Zero(void);    // KEY_MENU_Temp_Data.H07
extern void Read_Rang(void);    // KEY_MENU_Temp_Data.H08

extern uint16_t Read_EEP_Show(uint16_t eep_dat, uint8_t unit);

const DIGITAL_IO_t IO_24AA64_SDA_IN =
    {
        .gpio_port   = XMC_GPIO_PORT1,
        .gpio_pin    = 1U,
        .gpio_config = {
            .mode = XMC_GPIO_MODE_INPUT_TRISTATE,

        },
        .hwctrl = XMC_GPIO_HWCTRL_DISABLED};

void us_I2C_Init()
{
    I2C_DAT_H();
    us_Delay_1us(4);
    I2C_CLK_H();
    us_Delay_1us(4);
}

void us_I2CStart()
{
    I2C_CLK_H();
    us_Delay_1us(4);
    I2C_DAT_H();
    us_Delay_1us(4);
    I2C_DAT_L();
    us_Delay_1us(4);
    I2C_CLK_L();
}

void us_I2CStop()
{
    I2C_DAT_L();
    us_Delay_1us(4);
    I2C_CLK_H();
    us_Delay_1us(4);
    I2C_DAT_H();
    us_Delay_1us(4);
}

uint8_t I2CWriteByte(uint8_t bData)
{
    uint8_t i = 0;
    I2C_CLK_L();
    for (i = 0; i < 8; i++)
    {
        if ((bData & 0x80) != 0)
        {
            I2C_DAT_H();
        }
        else
        {
            I2C_DAT_L();
        }
        bData <<= 1;
        us_Delay_1us(4);
        I2C_CLK_H();
        us_Delay_1us(4);
        I2C_CLK_L();
    }
    us_Delay_1us(4);
    I2C_DAT_H();
    us_Delay_1us(4);
    // DIGITAL_IO_Init(&IO_24AA64_SDA_IN);
    I2C_CLK_H();
    //	while(I2C_DAT_Read()!=0)
    //		{
    //			j++;
    //			if(j>65535)
    //				break;
    //		};
    I2C_CLK_L();
    us_Delay_1us(4);
    // DIGITAL_IO_Init(&IO_24AA64_SDA);
    return 0;
}

/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	I2CReadByte()
 *   闁告梻鍠曢崗姗�晬閿燂拷I2C閻犲洩顕ч悺褔鎳為敓锟�	 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹閻犲洩顕ч崵顓㈡儍閸曨剚娈堕柟鐧告嫹
 *   			闁哄嫷鍨伴幆浣虹磼閹惧瓨灏�  0 缂備焦鎸诲锟�	 *   					 1  缂備綀鍛暰
 *   閺夆晜鏌ㄥú鏍磹绾绐�闂佹寧鐟ㄩ銈嗙閿濆洨鍨�
 ***************************************************/
uint8_t I2CReadByte(uint8_t *bpData, bool bFlag)
{
    uint8_t i     = 0;
    uint8_t bRead = 0;
    I2C_CLK_L();
    us_Delay_1us(4);
    // 设置输入模式
    DIGITAL_IO_Init(&IO_24AA64_SDA_IN);
    for (i = 0; i < 8; i++)
    {
        I2C_CLK_H();
        us_Delay_1us(4);
        if (I2C_DAT_Read() != 0)
        {
            bRead |= 1;
        }
        if (i < 7)
        {
            bRead <<= 1;
        }
        I2C_CLK_L();
        us_Delay_1us(4);
    }
    DIGITAL_IO_Init(&IO_24AA64_SDA);
    if (bFlag != 1)
    {
        I2C_DAT_L();    // 閻犱礁澧介悿鍡涘炊閻愯尙瀹�
    }
    else
    {
        I2C_DAT_H();    // 閻犱礁澧介悿鍡涘籍閻樺弶绀�幖杈炬嫹
    }
    us_Delay_1us(4);
    I2C_CLK_H();
    us_Delay_1us(4);
    I2C_CLK_L();
    us_Delay_1us(4);
    if (bFlag != 1)
    {
        I2C_DAT_H();    // 閻犱礁澧介悿鍡涘炊閻愯尙瀹�
    }
    else
    {
        I2C_DAT_L();    // 閻犱礁澧介悿鍡涘籍閻樺弶绀�幖杈炬嫹
    }
    if (bpData != 0)
    {
        *bpData = bRead;
    }
    return 0;
}

/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	I2CWriteData()
 *   闁告梻鍠曢崗姗�晬閿燂拷I2C闁告劖鐟ラ悺銊╁磼閵婏附娈堕柟鐧告嫹
 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹闁告稒鍨濋幎銈囷拷閿燂拷
 *   			闁革附婢樺锟�	 *   			闁告劖鐟ラ崣鍡涘极閻楀牆绁﹂柟绋挎喘閹凤拷
 *   			闁告劖鐟ラ崣鍡涘极閻楀牆绁﹀☉鎿冧簼閺嗭拷   濞戞搫鎷烽濂稿嫉閿熷浜ｉ柛娆樹簻瑜板弶绂掗妷銉ユ櫢闁稿繈鍎扮粩瀛樸亜閿熶粙宕￠敓锟介悗娑欘殸婵★拷
 *   閺夆晜鏌ㄥú鏍磹绾绐�闂佹寧鐟ㄩ銈嗙閿濆洨鍨�
 ***************************************************/
uint8_t I2CWriteData(uint8_t bCommand, uint16_t wAddress, uint8_t *bpData, uint8_t bLen)
{
    uint8_t ret = 0;
    uint8_t i   = 0;
    if (bLen == 0)
    {
        ret = 1;
        goto Exit;
    }
    us_I2CStart();
    if (I2CWriteByte(bCommand) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    if (I2CWriteByte((uint8_t)(wAddress >> 8)) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    if (I2CWriteByte((uint8_t)(wAddress & 0xff)) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    for (i = 0; i < bLen; i++)
    {
        if (I2CWriteByte(*(bpData + i)) != 0)
        {
            ret = 0xff;
            goto Exit;
        }
    }
    us_I2CStop();
    us_Delay_5ms();
Exit:
    return ret;
}

/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	us_bI2c_Write_Stream()
 *   闁告梻鍠曢崗姗�晬閿燂拷闁告劖鐟﹂弳鐔煎箲椤旂晫銈�
 *				濞寸姴瀛╃�姘憋拷濮橆剚鍕鹃柛褝鎷风槐鎴炴叏閿熶粙宕樺▎蹇撳汲闁哄牞鎷烽妵锟�濞戞搩浜滈悺褔鎳為崒婊勭暠闁轰胶澧楀畵锟�	 *				婵炲鍔嶉崜锟藉┑鈥冲�閻忓顕ｉ敓绛嬬�闁革附婢樺锟絪wAddress%32闁挎冻鎷�闁告帗鐟﹂弳鐔煎箲椤旂厧璁查柤铏灊缁辨壆鎲伴崱娆愮０濞戞柨顑呮晶鐘绘儍閸曨偅鍕鹃柛褝鎷�
 *				闁告娅曞﹢鎵嫻閵娿倗鐟愰柡鍕靛灠閸熸挻銇勭喊澶岀濞达絽妫欏Σ鎼佸矗椤栨瑤绨伴柛鎰懃閻垱绂嶉敓锟藉☉鎿冧簻閻⊙囨嚍閸屾粍鐣遍柡浣哄瀹擄拷
 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹uint16_t wAddress   闁革附婢樺锟�	 *              uint8_t *bpData     闁轰胶澧楀畵渚�箰閸ヮ剚瀚�
 *              uint8_t bLen        闁轰胶澧楀畵渚�⒐閸喖顔�
 *   閺夆晜鏌ㄥú鏍磹绾绐�Ret            闁绘鍩栭敓锟�	 ***************************************************/
uint8_t us_bI2c_Write_Stream(uint16_t wAddress, uint8_t *bpData, uint8_t bLen)
{
    if (bLen > 32)
    {
        return 1;
    }
    I2CWriteData(0xa0, wAddress, bpData, bLen);

    return 0;
}
/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	I2CReadData()
 *   闁告梻鍠曢崗姗�晬閿燂拷I2C閻犲洨绮弳鐔煎箲閿燂拷
 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹闁告稒鍨濋幎锟�	 *   			闁革附婢樺锟�	 *   			閺夆晜鏌ㄥú鏍极閻楀牆绁﹂柟绋挎喘閹凤拷
 *   			閺夆晜鏌ㄥú鏍极閻楀牆绁﹂梻锟界仢鐎癸拷  濞戞搫鎷烽濂稿嫉閿熷浜�55濞戞搩浜滈悺褔鎳為敓锟�	 *   閺夆晜鏌ㄥú鏍磹绾绐�闂佹寧鐟ㄩ銈嗙閿濆洨鍨�
 ***************************************************/
uint8_t I2CReadData(uint8_t bCommand, uint16_t wAddress, uint8_t *bpData, uint8_t bLen)
{
    uint8_t ret = 0;
    uint8_t i   = 0;
    if (bLen == 0)
    {
        ret = 1;
        goto Exit;
    }
    us_I2CStart();
    if (I2CWriteByte(bCommand) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    if (I2CWriteByte((uint8_t)(wAddress >> 8)) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    if (I2CWriteByte((uint8_t)(wAddress & 0xff)) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    us_I2CStart();
    if (I2CWriteByte((bCommand | 0x01)) != 0)
    {
        ret = 0xff;
        goto Exit;
    }
    if (bLen > 1)
    {
        for (i = 0; i < bLen - 1; i++)
        {
            ret = I2CReadByte(bpData + i, 0);
        }
    }
    ret = I2CReadByte(bpData + bLen - 1, 1);
    us_I2CStop();
Exit:
    return ret;
}
/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	us_bI2c_Read_Stream()
 *   闁告梻鍠曢崗姗�晬閿燂拷閻犲洨绮弳鐔煎箲椤旂晫銈�  濞寸姴瀛╃�姘憋拷濮樿鲸鐣遍柡浣哄瀹撲線宕烽弶鎸庣祷鐎殿噯鎷烽‖濠勬嫚鐠囨彃绲�bLen濞戞搩浜滈悺褔鎳為崒婊勭暠闁轰胶澧楀畵锟�	 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹uint16_t wAddress   闁革附婢樺锟�	 *              uint8_t *bpData     閻犲洩顕цぐ鍥极閻楀牆绁﹂柟绋挎喘閹凤拷
 *              uint8_t bLen        闁轰胶澧楀畵渚�⒐閸喖顔�
 *   閺夆晜鏌ㄥú鏍磹绾绐�Ret            闁绘鍩栭敓锟�	 ***************************************************/
uint8_t us_bI2c_Read_Stream(uint16_t wAddress, uint8_t *bpData, uint8_t bLen)
{
    I2CReadData(0xa0, wAddress, bpData, bLen);
    return 0;
}
/***************************************************
 *   闁告艾绉惰ⅷ闁挎冻鎷�    	I2CTest()
 *   闁告梻鍠曢崗姗�晬閿燂拷I2C閻犲洩顕ч崯鎾趁圭�顓犳Ц
 *   闁告垼濮ら弳鐔煎矗閸屾稒娈堕柨娑虫嫹void
 *   閺夆晜鏌ㄥú鏍磹绾绐�void
 ***************************************************/
void I2CTest()
{
    uint8_t i              = 0;
    uint8_t bWriteData[10] = {0};    // 闁告劖鐟ラ崣鍡涘极閻楀牆绁�
    uint8_t bReadData[10]  = {0};    // 閻犲洩顕цぐ鍥极閻楀牆绁�
    for (i = 0; i < 10; i++)
    {
        bWriteData[i] = i * i;
    }
    // 閻犱礁澧介悿鍡樻綇閹惧啿姣夋俊顖楋拷缁憋拷
    // DIGITAL_IO_Init(&IO_24AA64_SCK);
    I2CWriteData(0xa0, 0x0, bWriteData, 10);
    us_Delay_5ms();
    I2CReadData(0xa0, 0x0, bReadData, 10);
    for (i = 0; i < 10; i++)
    {
        if (bWriteData[i] != bReadData[i])
        {
            us_Delay_5ms();
        }
    }
}

/***************************************************
 *   名称：      	us_Delay_5ms()
 *   功能：		延时1us
 *   函数参数：	延时1us的倍数
 *   返回值：	void
 ***************************************************/
void us_Delay_5ms(void)
{
    uint32_t i = 0;
    /*30500-5ms*/
    for (i = 0; i < 42700; i++)    // 7ms
    {
        __NOP();
    }
}

/***************************************************
 *   名称：      	us_Delay_1us()
 *   功能：		延时1us
 *   函数参数：	延时1us的倍数
 *   返回值：	void
 ***************************************************/
void us_Delay_1us(uint32_t dwTime)
{
    /*计数延时,dwTime=4,延时4us；dwTime=1，延时2us*/
    uint16_t i, j = 0;
    for (i = 0; i < dwTime; i++)
    {
        for (j = 0; j < 3; j++)
        {
            __NOP();
        }
    }
}

/***************************************************
 *   閸氬秶袨閿涳拷  EEP_Read_Data(uint16_t address)
 *   閸旂喕鍏橀敍锟�  鐠囪绔存稉顏勭摟閺佺増宓�
 *   閸戣姤鏆熼崣鍌涙殶閿涙瓫ddress 閸︽澘娼�
 *   鏉╂柨娲栭崐纭风窗    鐠囪鍤弫鐗堝祦閸婏拷
 ***************************************************/
uint16_t EEP_Read_Data(uint16_t address)
{
    uint16_t data;
    uint8_t  bpData[2];
    us_bI2c_Read_Stream(2 * address, bpData, 2);
    data = bpData[0];
    data = data << 8;
    data = data + bpData[1];
    return data;
}
/***************************************************
 *   閸氬秶袨閿涳拷EEP_Write_Data(uint16_t address, uint16_t input_word)
 *   閸旂喕鍏橀敍锟� 閸愭瑤绔存稉顏勭摟閺佺増宓�
 *   閸戣姤鏆熼崣鍌涙殶閿涙瓫ddress     閸︽澘娼�
 *         input_word  閺佺増宓�
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
void EEP_Write_Data(uint16_t address, uint16_t input_word)
{
    uint8_t bpData[2];
    uint8_t data_h;
    uint8_t data_l;
    data_h = (uint8_t)(input_word >> 8);
    data_l = (uint8_t)(input_word & 0xFF);
    //	us_bI2c_Write_Byte((address * 2), data_h);
    //	us_bI2c_Write_Byte((address * 2)+1, data_l);
    bpData[0] = data_h;
    bpData[1] = data_l;
    us_bI2c_Write_Stream(2 * address, bpData, 2);
}
/***************************************************
 *   閸氬秶袨閿涳拷   read_eepromA()
 *   閸旂喕鍏橀敍锟�   鐠囪缍嬮崜宥呬紣娴ｆ粌灏弫鐗堝祦
 *   閸戣姤鏆熼崣鍌涙殶閿涙碍妫�
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
void read_eepromA(void)
{
    uint8_t i;
    // read EEPROM_A
    for (i = 0; i < EEP_COMP_CNT; i++)
    {
        EepFlashDat.present[i] = EEP_Read_Data(i);
        EepFlashDat.eeprom[i]  = EepFlashDat.present[i];
    }
    EepFlashDat.EepSumCheck = 0x55;
    for (i = 0; i < EEP_COMP_CNT - 1; i++)
        EepFlashDat.EepSumCheck += EepFlashDat.eeprom[i];
    if (EepFlashDat.EepSumCheck == EepFlashDat.eeprom[EEP_COMP_CNT - 1])
        EepFlashDat.fEepDatAOK = 1;
    else
        EepFlashDat.fEepDatAOK = 0;
}

/***************************************************
 *   閸氬秶袨閿涳拷   EEP_Write(uint16_t add,uint16_t dat)
 *   閸旂喕鍏橀敍锟�   EEPROM娑擃厼鍟撴稉锟介嚋鐎涳拷
 *   閸戣姤鏆熼崣鍌涙殶閿涳拷add 閸︽澘娼�
 *          dat 閺佺増宓�
 *   鏉╂柨娲栭崐纭风窗  1 閸愭瑦鍨氶崝锟� *         0  閸愭瑥銇戠拹锟� ***************************************************/
uint8_t EEP_Write(uint16_t add, uint16_t dat)
{
    uint16_t temp, i;
    uint32_t j;
    uint8_t  success = 0;
    for (i = 0; i < 30; i++)
    {
        EEP_Write_Data(add, dat);
        for (j = 0; j < 90000; j++);    // 30000延时有点短，死机
        temp = EEP_Read_Data(add);
        if (temp == dat)
        {
            i       = 30;
            success = 1;
        }
    }
    return (success);
}
/***************************************************
 *   閸氬秶袨閿涳拷   EraseLed_Eeprom(void)
 *   閸旂喕鍏橀敍锟�   閹匡箓娅庤ぐ鎾冲瀹搞儰缍旈崠鐑樻殶閹癸拷
 *   閸戣姤鏆熼崣鍌涙殶閿涳拷閺冿拷
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
void EraseLed_Eeprom(void)
{
    //	uint8_t i=0;
    us_bI2c_Write_Stream(mcu_flag, 0, 2);
    /*	for(i=0;i<EEP_COMP_CNT;i++)
        {
            EepFlashDat.present[i]=0x0;
            EEP_Write(i,EepFlashDat.present[i]);
            EepFlashDat.eeprom[i]=EEP_Read_Data (i);
        }
        for(i=0;i<EEP_COMP_CNT;i++)
        {
            EepFlashDat.present[i]=0x0;
            EEP_Write(300+i,EepFlashDat.present[i]);
            EepFlashDat.eeprom[i]=EEP_Read_Data (300+i);
        }*/
}
/***************************************************
 *   閸氬秶袨閿涳拷  Dat_Save_Eeprom(void)
 *   閸旂喕鍏橀敍锟�   娣囨繃瀵旇ぐ鎾冲瀹搞儰缍旈崠鐑樻殶閹癸拷
 *   閸戣姤鏆熼崣鍌涙殶閿涳拷閺冿拷
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
uint8_t Dat_Save_Eeprom(void)
{
    uint8_t i          = 0;
    uint8_t tflgWr     = 0;    // 本次保存标志位
    uint8_t tErrRtn    = 0;
    bool    flgRepair  = 0;
    uint8_t flgSuccess = 1;
    if ((ubErrCode & 0x02) == 0x02)    // EEPROM读写有错误，需要修复
        flgRepair = 1;
    //	if(((ubErrCode&0x20)!=0x20)||(calibration.fCalScale==1)||(calibration.fCalMax==1)||(calibration.fCalMin==1))
    {
        for (i = 0; i < EEP_COMP_CNT - 1; i++)
        {
            if ((EepFlashDat.eeprom[i] != EepFlashDat.present[i]) || ((RepairErr3Counter < 10) && (flgRepair == 1)))
            {
                if (EepFlashDat.eeprom[i] != EepFlashDat.present[i] && (i != 8))
                {
                    EepFlashDat.fDatChange = 1;
                    tflgWr                 = 1;
                    flgSuccess             = EEP_Write(i, EepFlashDat.present[i]);
                    if (flgSuccess > 0)
                        EepFlashDat.eeprom[i] = EepFlashDat.present[i];
                }
            }
        }
        if ((tflgWr == 1) || (EepFlashDat.eeprom[EEP_COMP_CNT - 1] != EepFlashDat.present[EEP_COMP_CNT - 1]))
        {
            EepFlashDat.EepSumCheck = 0x55;
            for (i = 0; i < EEP_COMP_CNT - 1; i++)
                EepFlashDat.EepSumCheck += EepFlashDat.present[i];
            EepFlashDat.present[EEP_COMP_CNT - 1] = EepFlashDat.EepSumCheck;
            flgSuccess                            = EEP_Write(i, EepFlashDat.present[EEP_COMP_CNT - 1]);
            if (flgSuccess > 0)
                EepFlashDat.eeprom[EEP_COMP_CNT - 1] = EepFlashDat.present[EEP_COMP_CNT - 1];
            if (flgSuccess == 1)
            {
                WriteCounter = 0;
                // NVIC002_DisableIRQ(&NVIC002_Handle1);
                ubErrCode &= ~(0x02);
                // NVIC002_EnableIRQ(&NVIC002_Handle1);
            }
            else if (WriteCounter < 30)
                WriteCounter++;
            else if (WriteCounter >= 30)
            {
                // NVIC002_DisableIRQ(&NVIC002_Handle1);
                ubErrCode |= 0x02;
                // NVIC002_EnableIRQ(&NVIC002_Handle1);
                tErrRtn = 1;
            }
        }
        if (((ubErrCode & 0x02) == 0x02) && (RepairErr3Counter < 1000))
            RepairErr3Counter++;
        else
            RepairErr3Counter = 0;
    }
    return tErrRtn;
}

/***************************************************
 *   閸氬秶袨閿涳拷   encrypte()
 *   閸旂喕鍏橀敍锟�   閸旂姴鐦�
 *   閸戣姤鏆熼崣鍌涙殶閿涙碍妫�
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
void encrypte(void)
{
    /*	uint16_t password;
        uint16_t i;
        uint8_t success1=0;
        uint8_t success2=0;
        uint8_t success3=0;
        password=EepFlashDat.present[4]+EepFlashDat.present[5]+EepFlashDat.present[6]+EepFlashDat.present[7];
        success1=EEP_Write(100,password);
        success2=EEP_Write(101,0);
        if((success1==1)&&(success2==1))
            success3=EEP_Write(102,0xaa55);
        for(i=0;i<20;i++)
            EEP_Write(103+i,EepFlashDat.present[i]);
        for(i=0;i<20;i++)
            EEP_Write(200-i,EepFlashDat.present[i]);*/
}
/***************************************************
 *   閸氬秶袨閿涳拷   EEP_Read(void)
 *   閸旂喕鍏橀敍锟�  鐏忓挼EP娑擃厽鏆熼幑顕嗙礉鐠у绮伴惄绋跨安閸欐﹢鍣洪妴锟� *   閸戣姤鏆熼崣鍌涙殶閿涙碍妫�
 *   鏉╂柨娲栭崐纭风窗    閺冿拷
 ***************************************************/
void EEP_Read(void)
{
    calibration.pressure_scale_kpa = EepFlashDat.present[3];    // 閲忕▼鏈�ぇ鍊�kpa
    ubMaxScalePsi                  = calibration.pressure_scale_kpa * 0.145f;
    calibration.min_pressure_psi   = EepFlashDat.present[4];    // 鏍囧畾鐨勬渶灏忓帇鍔涳紝鏀惧ぇ10鍊�psi
    calibration.min_pressure_kpa   = calibration.min_pressure_psi / 14.5 + 0.5;
    calibration.min_AD             = EepFlashDat.present[5];
    calibration.max_pressure_psi   = EepFlashDat.present[6];    // 鏍囧畾鐨勬渶澶у帇鍔涳紝鏀惧ぇ10鍊�psi
    calibration.max_pressure_kpa   = calibration.max_pressure_psi / 14.5 + 0.5;
    calibration.max_AD             = EepFlashDat.present[7];
    if ((calibration.max_AD <= calibration.min_AD) || (calibration.max_pressure_psi <= calibration.min_pressure_psi))
        calibration.fError = 1;
    else
        calibration.fError = 0;
}

uint8_t Mem_Init(void)
{
    uint8_t tErrRtn = 0;
    uint8_t bData[2];
    us_bI2c_Read_Stream(mcu_range_min, bData, 2);    // 量程最小值
    EepFlashDat.present[24] = (uint16_t)(bData[0] + (bData[1] << 8));
    us_bI2c_Read_Stream(mcu_range_max, bData, 2);    // 量程最大值
    EepFlashDat.present[3] = (uint16_t)(bData[0] + (bData[1] << 8));
    // 标定后以下4句可去掉
    if (EepFlashDat.present[3] > 900)    // 标定量程最大值
        EepFlashDat.present[3] = 900;

    if (EepFlashDat.present[24] > EepFlashDat.present[3] / 2)    // 标定量程最小值
        EepFlashDat.present[24] = 0;

    /*读取标定的最小和最大压力值，转换为psi并放大100倍*/
    us_bI2c_Read_Stream(mcu_min, bData, 2);    // 标定的最小压力
    if ((bData[1] & 0x80) != 0)
        EepFlashDat.present[4] = 0;
    else
        EepFlashDat.present[4] = (uint16_t)(bData[0] + (bData[1] << 8)) * 1.45;
    us_bI2c_Read_Stream(mcu_max, bData, 2);    // 标定的最大压力
    EepFlashDat.present[6] = (uint16_t)(bData[0] + (bData[1] << 8)) * 1.45;

    /*读取标定的最小最大压力对应的AD值*/
    us_bI2c_Read_Stream(mcu_min_ADC_01, bData, 2);    // 最小压力对应AD值
    EepFlashDat.present[5] = (uint16_t)(bData[0] + (bData[1] << 8));
    us_bI2c_Read_Stream(mcu_max_ADC_01, bData, 2);    // 最大压力对应AD值
    EepFlashDat.present[7] = (uint16_t)(bData[0] + (bData[1] << 8));

    /*读取气压标定完成标志位*/
    us_bI2c_Read_Stream(mcu_flag, bData, 2);
    EepFlashDat.present[16] = (uint16_t)(bData[0] + (bData[1] << 8));
    if (EepFlashDat.present[16] == 0xA5)
        calibration.fCalSave = 1;
    else
        calibration.fCalSave = 0;

    us_bI2c_Read_Stream(mcu_id3, bData, 4);    //
    *((uint8_t *)&bmyID3) = *bData;            // 获取ID3的值

    return tErrRtn;
}
void EEP_Default(void)
{
    uint16_t uwTempData;
    EepFlashDat.present[0]  = 0x0010 | 0x0002;
    EepFlashDat.present[1]  = 0;
    EepFlashDat.present[25] = 20;
    //	EepFlashDat.present[8]=0;
    EepFlashDat.present[9]  = 0;
    EepFlashDat.present[10] = 0;
    EepFlashDat.present[11] = 0;
    EepFlashDat.present[12] = 0;
    EepFlashDat.present[13] = 0;
    EepFlashDat.present[14] = 0;
    EepFlashDat.present[15] = 0;
    EepFlashDat.present[17] = 5;
    Dat_Save_Eeprom();
    Mem_Init();
    EEP_Read();
    KEY_MENU_Temp_Data.H07 = 0;
    ubMinScalePsi10        = EepFlashDat.present[1] * 65536 + EepFlashDat.present[25];    // EepFlashDat.present[1];
    ubMinScalePsi          = (uint8_t)((float)ubMinScalePsi10 / 400);
    if (KEY_MENU_Temp_Data.H08 != KEY_MENU_Temp_Data.Max_H08)
    {
        KEY_MENU_Temp_Data.H08 = KEY_MENU_Temp_Data.Max_H08;
        switch (KEY_MENU_Temp_Data.H01)
        {
        case 0:                                                            // psi
            uwTempData      = ubMaxScalePsi10 - ((uint32_t)((ubMaxScalePsi10 / 40))) * 40;
            ubMaxScalePsi10 = 40 * KEY_MENU_Temp_Data.H08 + uwTempData;    // ubMaxScalePsi10=400*KEY_MENU_Temp_Data.H08;
            break;
        case 1:                                                            // Bar
        case 2:                                                            // Kpa
        case 3:                                                            // Mpa
            uwTempData      = ubMaxScalePsi10 - ((uint32_t)((ubMaxScalePsi10 / 58))) * 58;
            ubMaxScalePsi10 = 58 * KEY_MENU_Temp_Data.H08 + uwTempData;
            break;
        }
        EepFlashDat.present[2]  = (ubMaxScalePsi10 >> 16);
        EepFlashDat.present[26] = (ubMaxScalePsi10 & 0x0000FFFF);
    }
    //	ubMaxScalePsi10=EepFlashDat.present[2]*65536+EepFlashDat.present[26];//EepFlashDat.present[2];
    //	ubMaxScalePsi=(uint8_t)((float)ubMaxScalePsi10/400);
}
