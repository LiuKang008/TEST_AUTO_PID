/*
 * us_KEY.c
 *
 *  Created on: 2015-3-23
 *      Author: Administrator
 */
#include "us_UserConfig.h"

extern uint8_t      Product_Type;
uint8_t             bFlash = 0, bLock = 0, bFlash1 = 0;
extern float        psi_value_set;
extern float        current_ref, volt_ref, current_ref1, volt_ref1;
extern uint8_t      ubErrCode;    //
extern uint8_t      ubErrCode1;
static Key_Handle_t us_ReadKey(void);
static uint8_t      us_bKey_Up_Func(uint8_t *bpdata);
static uint8_t      us_bKey_Down_Func(uint8_t *bpdata);
static uint8_t      us_bKey_Ok_Func(uint8_t *bpdata);
static uint8_t      us_bKeyCall_Func(KeyCall_Event Key_Func, uint8_t *bpdata);
extern void         Read_Rang(void);
extern void         EEP_Default(void);
Key_Event_t         Key_Function =
    {
        us_bKey_Up_Func,
        us_bKey_Down_Func,
        us_bKey_Ok_Func};
uint8_t         bCount_Ok = 0;                 // 鐢ㄤ簬璁＄畻闀挎寜璁℃暟
td_LED_ShowData LED_ShowData_K;                // 瑕佹樉绀虹殑瀛楃

uint8_t                     bFlag_OK   = 0;    // 鐢ㄤ簬鏍囪瘑杩涘叆 bButtonDeep_OK=3 浠ヤ究鍙互閫�嚭鍒�bButtonDeep_OK=1
uint8_t                     Unit_Flag  = 0;
uint8_t                     sg_cnt     = 0;
uint8_t                     sw_cnt     = 1;
uint8_t                     key_type   = 1;    // 1 ok 2 up 3 down
uint16_t                    key_up_cnt = 0, key_down_cnt = 0;
double                      Key_MCU_k = 0;
double                      Key_MCU_b = 0;
extern float                Psi_Sensor1_Val;
extern uint16_t             uwSaveUseIfo;
extern float                ubMaxScalePsi;
extern uint8_t              ubMinScalePsi;
extern uint8_t              bMenU_number;    // 鑿滃崟椤甸潰
extern eeprom_flash_data    EepFlashDat;
extern float                psi_value_set1;
extern pressure_calibration calibration;
extern uint32_t             ubMinScalePsi10;
extern uint32_t             ubMaxScalePsi10;
extern td_LED_ShowData      LED_ShowData, pre_LED_ShowData;
/***************************************************
 *   鍚嶇О锛� Read_Rang()
 *   鍔熻兘锛�        璇诲彇鍘嬪姏鑼冨洿
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Read_Rang(void)
{
    ubMaxScalePsi10 = EepFlashDat.present[2] * 65536 + EepFlashDat.present[26];
    switch (KEY_MENU_Temp_Data.H01)
    {
    case 0:                                                            // psi
        KEY_MENU_Temp_Data.H08     = ubMaxScalePsi10 / 40;             // ubMaxScalePsi10/400;//EepFlashDat.present[2]/40;//閺�儳銇�0閸婏拷
        KEY_MENU_Temp_Data.Max_H08 = EepFlashDat.present[3] * 1.45;    // EepFlashDat.present[3]*0.145;
        break;
    case 1:                                                            // Bar
    case 2:                                                            // Kpa
    case 3:                                                            // Mpa
        KEY_MENU_Temp_Data.H08     = ubMaxScalePsi10 / 58;             // EepFlashDat.present[2]/58;
        KEY_MENU_Temp_Data.Max_H08 = EepFlashDat.present[3];
        break;
    }
}
/***************************************************
 *   鍚嶇О锛� Read_Zero()
 *   鍔熻兘锛�    璇诲彇闆剁偣鍊�
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Read_Zero(void)
{
    ubMinScalePsi10 = EepFlashDat.present[1] * 65536 + EepFlashDat.present[25];
    if (ubMinScalePsi10 <= 20)
        KEY_MENU_Temp_Data.H07 = ubMinScalePsi10 - 20;
    else
    {
        switch (KEY_MENU_Temp_Data.H01)
        {
        case 0:                                                      // psi
            KEY_MENU_Temp_Data.H07 = (ubMinScalePsi10 - 20) / 40;    //(ubMinScalePsi10-20)/400;
            break;
        case 1:                                                      // Bar
        case 2:                                                      // Kpa
        case 3:                                                      // Mpa
            KEY_MENU_Temp_Data.H07 = (ubMinScalePsi10 - 20) / 58;
            break;
        }
    }
    ubMinScalePsi = (uint16_t)((float)ubMinScalePsi10 / 400);
}
/***************************************************
 *   鍚嶇О锛� Read_EEP_Show(uint16_t show_dat,uint16_t eep_dat,uint8_t unit)
 *   鍔熻兘锛� 璇诲彇eep涓殑鍊硷紝鏄剧ず鐢�
 *   鍑芥暟鍙傛暟锛歴how_dat鏄剧ず鐨勫�锛宔ep_dat eep涓殑鍊�  unit鍗曚綅
 *   杩斿洖鍊硷細    鏄剧ず鏁版嵁
 ***************************************************/
uint16_t Read_EEP_Show(uint16_t eep_dat, uint8_t unit)
{
    uint16_t show_dat;
    switch (unit)
    {
    case 0:                         // psi
        show_dat = eep_dat / 40;    // 0;
        break;
    case 1:                         // Bar
    case 2:                         // Kpa
    case 3:                         // Mpa
        show_dat = eep_dat / 58;
        break;
    }
    return (show_dat);
}
/***************************************************
 *   鍚嶇О锛� Save_Show_Dat(uint16_t show_dat uint16_t eep_dat uint8_t unit)
 *   鍔熻兘锛�淇濆瓨鏄剧ず鐨勬暟鎹�
 *   鍑芥暟鍙傛暟锛歴how_dat鏄剧ず鐨勫�锛宔ep_dat eep涓殑鍊�  unit鍗曚綅
 *   杩斿洖鍊硷細    淇濆瓨鏁版嵁
 ***************************************************/
uint16_t Save_Show_Dat(uint16_t show_dat, uint16_t eep_dat, uint8_t unit)
{
    uint16_t uwTempData;
    if (unit == 0)                                                   // psi
    {
        uwTempData = eep_dat - ((uint16_t)((eep_dat / 40))) * 40;    // 鍗曚綅杞崲涓嶄涪鏁版嵁 400  eep_dat宸茬粡鏀惧ぇ10鍊嶄簡锛屽啀鏀惧ぇ40鍊嶏紝鍗虫斁澶�00鍊�
        eep_dat    = show_dat * 40 + uwTempData;
    }
    else
        eep_dat = show_dat * 58;
    return (eep_dat);
}
/***************************************************
 *   鍚嶇О锛� Save_Press_Unit()
 *   鍔熻兘锛� 淇濆瓨鍘嬪姏鍗曚綅
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Save_Press_Unit(void)
{
    switch (KEY_MENU_Temp_Data.H01)
    {
    case 0:    // psi 00
        uwSaveUseIfo = uwSaveUseIfo & 0xFFDE;
        uwSaveUseIfo = uwSaveUseIfo | 0x0000;
        break;
    case 1:    // bar 01
        uwSaveUseIfo = uwSaveUseIfo & 0xFFDE;
        uwSaveUseIfo = uwSaveUseIfo | 0x0001;
        break;
    case 2:    // kpa 10
        uwSaveUseIfo = uwSaveUseIfo & 0xFFDE;
        uwSaveUseIfo = uwSaveUseIfo | 0x0020;
        break;
    case 3:    // Mpa 11
        uwSaveUseIfo = uwSaveUseIfo & 0xFFDE;
        uwSaveUseIfo = uwSaveUseIfo | 0x0021;
        break;
    }
}
/***************************************************
 *   鍚嶇О锛� Save_Decimal()
 *   鍔熻兘锛� 淇濆瓨鍘嬪姏鍗曚綅
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Save_Decimal(void)
{
    switch (KEY_MENU_Temp_Data.H02)
    {
    case 0:    // no Decimal
        uwSaveUseIfo = uwSaveUseIfo & 0xFFFD;
        break;
    case 1:    // Decimal
        uwSaveUseIfo = uwSaveUseIfo & 0xFFFD;
        uwSaveUseIfo = uwSaveUseIfo | 0x0002;
        break;
    }
}
/***************************************************
 *   鍚嶇О锛� Save_Input_Mode()
 *   鍔熻兘锛� 淇濆瓨杈撳叆鏂瑰紡
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Save_Input_Mode(void)
{
    switch (KEY_MENU_Temp_Data.H03)
    {
    case 0:    // 4~20mA/0~10V
        uwSaveUseIfo = uwSaveUseIfo & 0xFFF3;
        break;
    case 1:    // 0~20mA/0~5V
        uwSaveUseIfo = uwSaveUseIfo & 0xFFF3;
        uwSaveUseIfo = uwSaveUseIfo | 0x0004;
        break;
    case 2:    // 4娈垫帶鍒�
        uwSaveUseIfo = uwSaveUseIfo & 0xFFF3;
        uwSaveUseIfo = uwSaveUseIfo | 0x0008;
        break;
    }
}

void Save_Show_Precision(void)
{
    switch (KEY_MENU_Temp_Data.H12)
    {
    case 0:    // real time display
        uwSaveUseIfo = uwSaveUseIfo & 0xFFEF;
        break;
    case 1:    // chang for 1psi
        uwSaveUseIfo = uwSaveUseIfo & 0xFFEF;
        uwSaveUseIfo = uwSaveUseIfo | 0x0010;
        break;
    }
}
/***************************************************
 *   鍚嶇О锛� Save_Zero()
 *   鍔熻兘锛� 淇濆瓨闆剁偣鍊�
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Save_Zero(void)
{
    uint16_t uwTempData;
    if (KEY_MENU_Data.H07 < 0)
        ubMinScalePsi10 = KEY_MENU_Data.H07 + 20;
    else
    {
        switch (KEY_MENU_Temp_Data.H01)
        {
        case 0:    // psi
            //		   		   uwTempData=ubMinScalePsi10-20-((uint32_t)(((ubMinScalePsi10-20)/400)))*400;//鍗曚綅杞崲涓嶄涪鏁版嵁
            uwTempData = ubMinScalePsi10 - 20 - ((uint32_t)(((ubMinScalePsi10 - 20) / 40))) * 40;
            //		   		   ubMinScalePsi10=KEY_MENU_Data.H07*400+20+uwTempData;
            ubMinScalePsi10 = KEY_MENU_Data.H07 * 40 + 20 + uwTempData;
            break;
        case 1:    // Bar
        case 2:    // Kpa
        case 3:    // Mpa
            //				   KEY_MENU_Data.H03-=KEY_MENU_Data.H03%10;//kpa娌℃硶鏄剧ず锛屽幓鎺変釜浣嶆暟
            uwTempData      = ubMinScalePsi10 - 20 - ((uint32_t)(((ubMinScalePsi10 - 20) / 58))) * 58;    // 鍗曚綅杞崲涓嶄涪鏁版嵁
            ubMinScalePsi10 = KEY_MENU_Data.H07 * 58 + 20 + uwTempData;
            break;
        }
    }
    //		ubMinScalePsi10=EepFlashDat.present[1]*65536+EepFlashDat.present[25];
    EepFlashDat.present[1]  = (ubMinScalePsi10 >> 16);
    EepFlashDat.present[25] = (ubMinScalePsi10 & 0x0000FFFF);
    ubMinScalePsi           = (uint16_t)((float)ubMinScalePsi10 / 400);
}
/***************************************************
 *   鍚嶇О锛� Save_Rang()
 *   鍔熻兘锛� 淇濆瓨鑼冨洿
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void Save_Rang(void)
{
    uint16_t uwTempData;
    switch (KEY_MENU_Temp_Data.H01)
    {
    case 0:    // psi
        //	   		   uwTempData=ubMaxScalePsi10-((uint32_t)((ubMaxScalePsi10/400)))*400;//鍗曚綅杞崲涓嶄涪鏁版嵁
        uwTempData = ubMaxScalePsi10 - ((uint32_t)((ubMaxScalePsi10 / 40))) * 40;
        //	   		   ubMaxScalePsi10=KEY_MENU_Data.H08*400+uwTempData;
        ubMaxScalePsi10 = KEY_MENU_Data.H08 * 40 + uwTempData;
        break;
    case 1:    // Bar
    case 2:    // Kpa
    case 3:    // Mpa
        //			   KEY_MENU_Data.H02-=KEY_MENU_Data.H02%10;//kpa娌℃硶鏄剧ず锛屽幓鎺変釜浣嶆暟
        uwTempData      = ubMaxScalePsi10 - ((uint32_t)((ubMaxScalePsi10 / 58))) * 58;    // 鍗曚綅杞崲涓嶄涪鏁版嵁
        ubMaxScalePsi10 = KEY_MENU_Data.H08 * 58 + uwTempData;
        break;
    }
    if (ubMaxScalePsi10 > calibration.pressure_scale_kpa * 58)
        ubMaxScalePsi10 = calibration.pressure_scale_kpa * 58;
    EepFlashDat.present[2]  = (ubMaxScalePsi10 >> 16);
    EepFlashDat.present[26] = (ubMaxScalePsi10 & 0x0000FFFF);
    //	   ubMaxScalePsi10=EepFlashDat.present[2]*65536+EepFlashDat.present[26];
    //       ubMaxScalePsi=(uint16_t)((float)ubMaxScalePsi10/400+0.55);
    ubMaxScalePsi = (uint16_t)((float)ubMaxScalePsi10 / 400);
}
/***************************************************
 *   鍚嶇О锛� LED_Act(void)
 *   鍔熻兘锛氭牴鎹甃CD鐨勫姩浣滀繚瀛樻暟鎹�
 *   鍑芥暟鍙傛暟锛氭棤
 *   杩斿洖鍊硷細    鏃�
 ***************************************************/
void LED_Act(void)
{
    static uint8_t pre_H13 = 0;
    switch (bMenU_number)
    {
    // Page1
    case 1:    // 淇濆瓨鍘嬪姏鍗曚綅
        Save_Press_Unit();
        // 鍒锋柊璺熷帇鍔涘崟浣嶇浉鍏冲弬鏁�
        Read_Rang();
        Read_Zero();
        KEY_MENU_Temp_Data.H04  = Read_EEP_Show(EepFlashDat.present[9], KEY_MENU_Temp_Data.H01);     // Read_P1
        KEY_MENU_Temp_Data.H05  = Read_EEP_Show(EepFlashDat.present[10], KEY_MENU_Temp_Data.H01);    // Read_P2
        KEY_MENU_Temp_Data.H10  = Read_EEP_Show(EepFlashDat.present[15], KEY_MENU_Temp_Data.H01);    // Read_Manual_Ouput
        KEY_MENU_Temp_Data.H061 = Read_EEP_Show(EepFlashDat.present[11], KEY_MENU_Temp_Data.H01);
        KEY_MENU_Temp_Data.H062 = Read_EEP_Show(EepFlashDat.present[12], KEY_MENU_Temp_Data.H01);
        KEY_MENU_Temp_Data.H063 = Read_EEP_Show(EepFlashDat.present[13], KEY_MENU_Temp_Data.H01);
        KEY_MENU_Temp_Data.H064 = Read_EEP_Show(EepFlashDat.present[14], KEY_MENU_Temp_Data.H01);
        break;
    case 2:    // 淇濆瓨灏忔暟鐐�
        Save_Decimal();
        break;
    case 3:    // 淇濆瓨杈撳叆鏂瑰紡
        Save_Input_Mode();
        break;
    case 4:    // 淇濆瓨璁惧畾P1鍘嬪姏鍊�
        EepFlashDat.present[9] = Save_Show_Dat(KEY_MENU_Temp_Data.H04, EepFlashDat.present[9], KEY_MENU_Temp_Data.H01);
        break;
    case 5:    // 淇濆瓨璁惧畾P2鍘嬪姏鍊�
        EepFlashDat.present[10] = Save_Show_Dat(KEY_MENU_Temp_Data.H05, EepFlashDat.present[10], KEY_MENU_Temp_Data.H01);
        break;
    case 6:
        EepFlashDat.present[11] = Save_Show_Dat(KEY_MENU_Temp_Data.H061, EepFlashDat.present[11], KEY_MENU_Temp_Data.H01);
        EepFlashDat.present[12] = Save_Show_Dat(KEY_MENU_Temp_Data.H062, EepFlashDat.present[12], KEY_MENU_Temp_Data.H01);
        EepFlashDat.present[13] = Save_Show_Dat(KEY_MENU_Temp_Data.H063, EepFlashDat.present[13], KEY_MENU_Temp_Data.H01);
        EepFlashDat.present[14] = Save_Show_Dat(KEY_MENU_Temp_Data.H064, EepFlashDat.present[14], KEY_MENU_Temp_Data.H01);
        if ((KEY_MENU_Temp_Data.H06 == 4) || (bFlash1 == 1) || (bButtonDeep_OK == 1))
            KEY_MENU_Temp_Data.H06 = 0;
        else
            KEY_MENU_Temp_Data.H06++;
        bFlash1                 = 0;
        EepFlashDat.present[20] = KEY_MENU_Temp_Data.H06;
        break;
    // Page2
    case 7:
        Save_Zero();
        break;
    case 8:
        Save_Rang();
        break;
    case 9:    // 姝诲尯妯″紡锛屾病鏈変娇鐢�
        EepFlashDat.present[17] = KEY_MENU_Temp_Data.H09;
        break;
    case 10:    // 淇濆瓨鎵嬪姩杈撳嚭
        EepFlashDat.present[15] = Save_Show_Dat(KEY_MENU_Temp_Data.H10, EepFlashDat.present[15], KEY_MENU_Temp_Data.H01);
        break;
    case 12:
        Save_Show_Precision();
        break;
    //*********************************************************************
    // Page17	 Recover
    //*********************************************************************
    case 13:
        if (KEY_MENU_Temp_Data.H13 == 1)
        {
            EEP_Default();
            KEY_MENU_Temp_Data.H13 = 0;
        }
        break;
    }
    EepFlashDat.present[0] = uwSaveUseIfo;
    EepFlashDat.present[8] = bMenU_number;
}

void us_Key_Init(void)
{
    memcpy(&KEY_MENU_Temp_Data, &KEY_MENU_Data, sizeof(KEY_MENU));
}
/*
 * 1銆乥ButtonDeep_OK=0涓洪攣瀹氱姸鎬�瑕侀暱鎸�杩涘叆Hnd 姝ゆ椂 涓� 鑿滃崟椤甸潰涓�  濡傛灉鎸塷k 鍒欓�鍑篐nd鑿滃崟 椤甸潰涓� 娣卞害涓�
 * 2
 */
/***************************************************
 *   鍚嶇О锛�     	us_ReadKey()
 *   鍔熻兘锛�	璇诲彇鎸夐敭
 *   鍑芥暟鍙傛暟锛�void
 *   杩斿洖鍊硷細	Key_Handle_t  杩斿洖褰撳墠鎸変笅鐨勬寜閿�
 ***************************************************/
static Key_Handle_t us_ReadKey(void)
{
#if 0
	Key_Handle_t Key = Key_Idle;
	static uint8_t bFlag = 0; //浣嶆爣璇�鐢ㄤ簬璁板綍鍝釜鎸夐敭鎸変笅
	uint8_t i=0;
	static uint8_t bNum[4] = {0};//璁℃椂璁℃暟     涓暟*鎵弿鏃堕棿鍗充负 鎬绘椂闂�
	static uint32_t dwReadKeyTime = 0;
	uint32_t dwGetSysTime = us_dwGetSystemTime();
	if(dwGetSysTime-dwReadKeyTime<60)//6ms鎵弿涓�
	{
		goto Exit;
	}
	dwReadKeyTime = dwGetSysTime;
	if(ReadKey_Up==0)//Up
	{
		bFlag |= 0x01;
		bNum[1]++;//璁＄畻鎸変笅璇ユ寜閿椂闂磋鏁�
	}
	if(ReadKey_Down==0)//Down璁℃暟
	{
		bFlag |= 0x02;
		bNum[2]++;//
	}
	if(ReadKey_Ok==0)//Ok璁℃暟
	{
		bFlag |= 0x04;
		bNum[3]++;//
	}
	else
	{
		bCount_Ok = 0;
	}
	if(bFlag!=0)//瓒呮椂璁℃暟
	{
		bNum[0]++;//璁＄畻鏄惁瓒呮椂璁℃暟
	}
	for(i=0;i<3;i++)
	{
		if(bNum[i+1]>15)//娑堟姈鏃堕棿  NUM*60 = 120ms
		{
			bFlag&=(~(1<<i));
			bNum[i+1]= 0 ;
			Key = i+1;//杩斿洖褰撳墠鎸変笅鐨勬寜閿� 涓庡畾涔夊尮閰�
			goto Exit;
		}
	}
	if(bNum[0]>50)//瓒呮椂
	{
		bFlag=0;
		for(i=0;i<4;i++)//娓呴櫎璁℃暟
		{
			bNum[i]= 0 ;
		}
	}
Exit:
	return Key;
#endif
}
/***************************************************
 *   鍚嶇О锛�     	Show_flData(uint16_t dat,uint8_t unit)
 *   鍔熻兘锛�	鏄剧ず娴偣鏁�
 *   鍑芥暟鍙傛暟锛�u16 dat鏁版嵁锛寀nit 鍗曚綅
 *   杩斿洖鍊硷細	鏃�
 ***************************************************/
void Show_flData(s16 dat, uint8_t unit)
{
    if (unit == 0)    // psi
    {
        if (dat < 1000)
            us_bLEDShow_FlData(dat * 0.1, 1);
        else
            us_bLEDShow_FlData(dat * 0.1 + 0.5, 0);    // 4鑸�鍏�
    }
    if (unit == 1)                                     // bar
        us_bLEDShow_FlData(dat * 0.01, 2);
    else if (unit == 2)                                // kpa
        us_bLEDShow_FlData(dat, 0);
    else if (unit == 3)                                // Mpa
        us_bLEDShow_FlData(dat * 0.001, 2);
}
/***************************************************
 *   鍚嶇О锛�     	Show_Input(void)
 *   鍔熻兘锛�	鐩戞帶杈撳叆淇″彿
 *   鍑芥暟鍙傛暟锛�鏃�
 *   杩斿洖鍊硷細	鏃�
 ***************************************************/
void Show_Input(void)
{
    //	 if(us_bGetADCINMode()==ADCIN0_10V)
    //	 {
    //		 KEY_MENU_Temp_Data.H11=psi_value_set/1;//鏀惧ぇ10鍊�
    //		 LED_ShowData_K.bdata_3 =bLED_Char[9]|0x80;//U.
    //	 }
    //	 else if(us_bGetADCINMode()==ADCIN0_5V)
    //	 {
    //		 KEY_MENU_Temp_Data.H11=psi_value_set/2;	//鏀惧ぇ10鍊�
    //		 LED_ShowData_K.bdata_3 =bLED_Char[9]|0x80;//U.
    //	 }
    //	 else if((AD_Current_Value_Filtered>mA4_ad_current)&&(us_bGetADCINMode()==ADCIN4_20mA))
    //	 {
    //		 KEY_MENU_Temp_Data.H11=4+psi_value_set*16/10;//鏀惧ぇ10鍊�
    //		 LED_ShowData_K.bdata_3 =bLED_Char[13]|0x80;//I.
    //	 }
    //	 else if((AD_Current_Value_Filtered>mA0_ad_current)&&(us_bGetADCINMode()==ADCIN0_20mA))
    //	 {
    //   	 KEY_MENU_Temp_Data.H11=psi_value_set*2;//鏀惧ぇ10鍊�
    //   	 LED_ShowData_K.bdata_3 =bLED_Char[13]|0x80;//I.
    //	 }
    if ((us_bGetADCINMode() == ADCIN0_10V) || (us_bGetADCINMode() == ADCIN0_5V))
    {
        KEY_MENU_Temp_Data.H11 = volt_ref1 / 100;
        LED_ShowData_K.bdata_3 = bLED_Char[9] | 0x80;    // U.
    }
    else
    {
        KEY_MENU_Temp_Data.H11 = current_ref1 / 100;
        LED_ShowData_K.bdata_3 = bLED_Char[13] | 0x80;    // I.
    }
    if (KEY_MENU_Temp_Data.H11 < 100)
    {
        LED_ShowData_K.bdata_2 = 0;
        if (KEY_MENU_Temp_Data.H11 < 10)
        {
            LED_ShowData_K.bdata_1 = bLED_Number[0] | 0x80;
            LED_ShowData_K.bdata_0 = bLED_Number[KEY_MENU_Temp_Data.H11];
        }
        else
        {
            LED_ShowData_K.bdata_1 = bLED_Number[KEY_MENU_Temp_Data.H11 / 10] | 0x80;
            LED_ShowData_K.bdata_0 = bLED_Number[KEY_MENU_Temp_Data.H11 % 10];
        }
    }
    else
    {
        LED_ShowData_K.bdata_2 = bLED_Number[KEY_MENU_Temp_Data.H11 / 100];
        LED_ShowData_K.bdata_1 = bLED_Number[(KEY_MENU_Temp_Data.H11 / 10) % 10] | 0x80;
        LED_ShowData_K.bdata_0 = bLED_Number[KEY_MENU_Temp_Data.H11 % 10];
    }
}
/***************************************************
 *   鍚嶇О锛�     	Dis_Err()
 *   鍔熻兘锛�	鏄剧ず閿欒
 *   鍑芥暟鍙傛暟锛�鏃�
 *   杩斿洖鍊硷細	鏃�
 ***************************************************/
void Dis_Err(void)
{
    LED_ShowData.bdata_3 = bLED_Char[16];                          //'E';
    LED_ShowData.bdata_2 = bLED_Char[5];                           //'r'
    LED_ShowData.bdata_1 = bLED_Char[5];                           //'r'
    if ((ubErrCode & 0x01) == 0x01)
        LED_ShowData.bdata_0 = bLED_Number[1];                     // 1 鎺у埗淇″彿瓒呴噺绋�
    else if ((ubErrCode & 0x04) == 0x04)
        LED_ShowData.bdata_0 = bLED_Number[2];                     // 2 杈撳嚭鍘嬪姏涓嶈兘杈惧埌璁惧畾鍊�
    else if (((ubErrCode & 0x02) == 0x02) || (ubErrCode1 == 1))    // ubErrCode1==1 EEP鏁版嵁璇讳笉鍑烘潵
        LED_ShowData.bdata_0 = bLED_Number[3];                     // 3 EEPROM璇诲啓閿欒
}

/***************************************************
 *   鍚嶇О锛�     	LED_OFF()
 *   鍔熻兘锛�	LED鐔勭伃
 *   鍑芥暟鍙傛暟锛�鏃�
 *   杩斿洖鍊硷細	鏃�
 ***************************************************/
void LED_OFF(void)
{
    LED_ShowData.bdata_0 = 0;
    LED_ShowData.bdata_1 = 0;
    LED_ShowData.bdata_2 = 0;
    LED_ShowData.bdata_3 = 0;
}
/***************************************************
 *   鍚嶇О锛�     	us_bReadKey_Process()
 *   鍔熻兘锛�	鎵弿鎸夐敭浜嬩欢 骞舵墽琛岀浉搴旂殑浜嬩欢鍑芥暟
 *   鍑芥暟鍙傛暟锛�u8 *bpdata
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
uint8_t us_bReadKey_Process(uint8_t *bpdata)
{
#if 0
	uint8_t Ret=0;
	uint8_t bShowPage = 0;
	static uint8_t first=1,flash_cnt,pre_bButtonDeep_OK,bDis=0;
	static uint16_t err_cnt=0,dis_cnt=0;
	Key_Handle_t Key=Key_Idle;
	static uint32_t dwReadTime = 0;//鏄剧ず1s LoC
	static uint32_t dwIdletime = 0;//鏃犳帶鍒�鑷姩閿佸畾鐣岄潰
	static uint32_t dwShowDataTime = 0,dwKeyTime=0;//
	static float pre_Psi_Sensor1_Val;
	uint32_t dwGetSysTime = us_dwGetSystemTime();

	if(dwIdletime==0)
		dwIdletime=dwGetSysTime;
	Key=us_ReadKey();
	if(bLock==1)//瑙ｉ攣 ok閿寜3绉�
	{
		if(ReadKey_Ok!=0)
			dwReadTime = dwGetSysTime;
		if(dwGetSysTime-dwReadTime>30000)
		{
			bLock=0;
			bButtonDeep_OK = 0;
		}
	}

//	if((dwGetSysTime-dwReadTime>10000)&&(bMenU_number == 0xff))//涓轰簡鏄剧ず1s鐨凩oC
//	{
//		bMenU_number = 0;
//		bLock=0;
//	}
	if((dwGetSysTime-dwIdletime>600000))//&&(bMenU_number!=10))//1鍒嗛挓 涓嶆搷浣�鐩存帴閿佸畾锛岄〉闈�0瑕佷笉瑕侀攣瀹氾紵
	{
		bMenU_number = 0;//1鍒嗛挓涓嶆寜閿紝璋冨埌0椤�
		bButtonDeep_OK = 0;
		bFlag_OK = 0;
		bLock=1;
	}
    if(bMenU_number!=10)
    {
		KEY_MENU_Temp_Data.H10=0;
		KEY_MENU_Data.H10=0;
    }
	if((dwGetSysTime-dwKeyTime>100))//瀹炴椂鏄剧ず  10ms鍒锋柊
	{
		dwKeyTime=dwGetSysTime;//
		if(ReadKey_Up==0)
		{
			if(key_up_cnt<1000)
				key_up_cnt++;
			key_down_cnt=0;
		}
		else if(ReadKey_Down==0)
		{
			if(key_down_cnt<1000)
				key_down_cnt++;
			key_up_cnt=0;
		}
		else
		{
			key_up_cnt=0;
			key_down_cnt=0;
		}
	}
	switch((uint8_t)Key)
	{
		case Key_Up:
			Ret=us_bKeyCall_Func(Key_Function.Key_Up_Event,bpdata);
			break;
		case Key_Down:
			Ret=us_bKeyCall_Func(Key_Function.Key_Down_Event,bpdata);
			break;
		case Key_Ok:
			Ret=us_bKeyCall_Func(Key_Function.Key_Ok_Event,bpdata);
			break;
		default://濡傛灉娌℃湁鎸夐敭锛屽仛浠ヤ笅鏄剧ず
			Ret=1;
			if((dwGetSysTime-dwShowDataTime>3000))//瀹炴椂鏄剧ず 0.5s鍒锋柊
			{
				dwShowDataTime = dwGetSysTime;
				if((bMenU_number==0)&&(bButtonDeep_OK!=1))
				{
					    if((Psi_Sensor1_Val<1)||(KEY_MENU_Temp_Data.H12==0)||(bDis==0)
					    	||(((fabsf(pre_Psi_Sensor1_Val-Psi_Sensor1_Val)>0.1f)||(bLock==1)||(dis_cnt>0))&&(KEY_MENU_Temp_Data.H12==1)))
//						if((Psi_Sensor1_Val<1)||(KEY_MENU_Temp_Data.H12==0)||(bDis==0)
//						    ||(((fabsf(pre_Psi_Sensor1_Val-Psi_Sensor1_Val)>0.1f)||(dis_cnt>0))&&(KEY_MENU_Temp_Data.H12==1)))

					    {
							if(KEY_MENU_Temp_Data.H01==0)//psi
							{
								if((KEY_MENU_Temp_Data.H02==0))//||(Product_Type==6))//LS不要小数点
									us_bLEDShow_FlData(Psi_Sensor1_Val+0.5,0);//4鑸�鍏�
								else
									us_bLEDShow_FlData(Psi_Sensor1_Val,1);
							}
							else if(KEY_MENU_Temp_Data.H01==1)//bar
							{
								us_bLEDShow_FlData(Psi_Sensor1_Val*0.0689655,2);
							}
							else if(KEY_MENU_Temp_Data.H01==2)//kpa
								us_bLEDShow_FlData(Psi_Sensor1_Val*6.89655,0);
							else if(KEY_MENU_Temp_Data.H01==3)//Mpa
								us_bLEDShow_FlData(Psi_Sensor1_Val*0.00689655,2);
							pre_Psi_Sensor1_Val=Psi_Sensor1_Val;
							bDis=1;
					    }
				}
				else if((bMenU_number==11)&&(bButtonDeep_OK==2))
				{
					Show_Input();
					us_bLEDShow_SetData(LED_ShowData_K);
					bDis=0;
				}
				else
				{
					bDis=0;
					us_bLEDShow_SetData(LED_ShowData_K);
				}
				if(bFlash==1)
				{
//					if(bButtonDeep_OK==0)
//						bFlash=0;
					flash_cnt++;
					if(flash_cnt==5)
					{
						flash_cnt=0;
						bFlash=0;
						if(bButtonDeep_OK==1)
						{
							if(bMenU_number==0)
								bButtonDeep_OK=0;
							else
								bButtonDeep_OK=2;
						}
						else if(bButtonDeep_OK==2)
						{
							bFlash1=1;
							bButtonDeep_OK=1;
						}
					}
					if(flash_cnt%2==1)
						LED_OFF();
					else
						LED_ShowData=pre_LED_ShowData;
				}
#if NO_DIS_ERR2 || NO_NC    // 常开不报ERR2
					if(((ubErrCode>0)||(ubErrCode1>0))&&((ubErrCode&0x04)!=0x04))
#else
					if((((ubErrCode>0)||(ubErrCode1>0))&&(Product_Type!=6))
					  ||((((ubErrCode>0)||(ubErrCode1>0))&&((ubErrCode&0x04)!=0x04))&&(Product_Type==6)))
#endif
				{
					dis_cnt++;
					if(dis_cnt>12)
					{
						err_cnt++;
						Dis_Err();
						if(err_cnt%2==1)
							LED_OFF();
						if(dis_cnt>=18)
							dis_cnt=0;
					}
				}
			}
			if((first==1)&&(bMenU_number==10))
				first=0;
			else if(pre_bButtonDeep_OK!=bButtonDeep_OK)
				pre_bButtonDeep_OK=bButtonDeep_OK;
			else// if(bFlash==0)
				goto Exit;//濡傛灉娌℃湁鎸夐敭杩斿洖
			break;
	}
//  鏈夐敭鎸変笅
	EepFlashDat.present[8]=bMenU_number;
	dwIdletime = dwGetSysTime;//鏈夋寜閿�閲嶇疆鏃犳搷浣滄椂闂�
//	if((bButtonDeep_OK==0)&&(bMenU_number==0xff))
	if(ubErrCode>0)
	{
		if((dwGetSysTime-dwShowDataTime>5000))//瀹炴椂鏄剧ず 0.5s鍒锋柊
		{
			dwShowDataTime = dwGetSysTime;
			dis_cnt++;
			if(dis_cnt>12)
			{
				err_cnt++;
				Dis_Err();//閿欒鏄剧ず
				if(err_cnt%2==1)
					LED_OFF();//鐔勭伅
				if(dis_cnt>=18)
					dis_cnt=0;
			}
		}
	}

	if((ubErrCode==0)||(dis_cnt<=12))// 鎸夐敭鏄剧ず
	{
		if(bLock==1)//鏄剧ずLoC
		{
	//		bMenU_number = 0xff;
			LED_ShowData_K.bdata_3=0;
			LED_ShowData_K.bdata_2 = bLED_Char[14];
			LED_ShowData_K.bdata_1 = bLED_Char[12];
			LED_ShowData_K.bdata_0 = bLED_Char[15];
		}
		else
		if(bButtonDeep_OK==1)//鏄剧ず鑿滃崟
		{
				bShowPage = bMenU_number;
				LED_ShowData_K.bdata_3 = bLED_Char[10];
				LED_ShowData_K.bdata_2 = 0; //鍏ㄧ伃
				LED_ShowData_K.bdata_1 = bLED_Number[bShowPage/10];
				LED_ShowData_K.bdata_0 = bLED_Number[(bShowPage%10)];
		}
		else if(bButtonDeep_OK==2)//鏄剧ず娆＄骇鑿滃崟
		{
			switch(bMenU_number)
			{
				case 1:
					if(KEY_MENU_Temp_Data.H01 <=3)
					{
						LED_ShowData_K.bdata_3 = bLED_Char[9];
	//					LED_ShowData_K.bdata_2 = bLED_Char[3];
	//					LED_ShowData_K.bdata_1 = bLED_Number[0];
						LED_ShowData_K.bdata_2 = 0;
						LED_ShowData_K.bdata_1 = 0;
						if(KEY_MENU_Temp_Data.H01<3)
							LED_ShowData_K.bdata_0 =bLED_Char[10+KEY_MENU_Temp_Data.H01];
						else
							LED_ShowData_K.bdata_0 =bLED_Char[19];
					}
					if(KEY_MENU_Temp_Data.H01==2)
						sg_cnt=1;
					break;
				case 2:
					LED_ShowData_K.bdata_0 =bLED_Number[0];
					LED_ShowData_K.bdata_1 =bLED_Number[0];
					LED_ShowData_K.bdata_2 =bLED_Number[0];
					LED_ShowData_K.bdata_3 = bLED_Char[2];//'d'
					if(KEY_MENU_Temp_Data.H02==1)
						LED_ShowData_K.bdata_1 |= 0x80;
				break;
				case 3:
					LED_ShowData_K.bdata_0 =bLED_Number[KEY_MENU_Temp_Data.H03+1];
					LED_ShowData_K.bdata_1 =0;
					LED_ShowData_K.bdata_2 =0;
					LED_ShowData_K.bdata_3 = bLED_Char[3];//'F'
					break;
				case 4:
					Show_flData(KEY_MENU_Temp_Data.H04,KEY_MENU_Temp_Data.H01);
					break;
				case 5:
					Show_flData(KEY_MENU_Temp_Data.H05,KEY_MENU_Temp_Data.H01);
					break;
				case 6:
					switch(KEY_MENU_Temp_Data.H06)
					{
						case 0:
							Show_flData(KEY_MENU_Temp_Data.H061,KEY_MENU_Temp_Data.H01);
							break;
						case 1:
							Show_flData(KEY_MENU_Temp_Data.H062,KEY_MENU_Temp_Data.H01);
							break;
						case 2:
							Show_flData(KEY_MENU_Temp_Data.H063,KEY_MENU_Temp_Data.H01);
							break;
						case 3:
							Show_flData(KEY_MENU_Temp_Data.H064,KEY_MENU_Temp_Data.H01);
							break;
					}
					break;
				case 7:
					Show_flData(KEY_MENU_Temp_Data.H07,KEY_MENU_Temp_Data.H01);
					break;
				case 8:
					Show_flData(KEY_MENU_Temp_Data.H08,KEY_MENU_Temp_Data.H01);
					break;
				case 9:
					switch (Product_Type)
					{
					case 0://显示：__P0
						LED_ShowData_K.bdata_0 =bLED_Number[0];//0
						LED_ShowData_K.bdata_1 =bLED_Char[10];//P
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					case 1:
						LED_ShowData_K.bdata_0 =bLED_Number[1];//1
						LED_ShowData_K.bdata_1 =bLED_Char[10];//P
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					case 2:
						LED_ShowData_K.bdata_0 =bLED_Number[2];//2
						LED_ShowData_K.bdata_1 =bLED_Char[10];//P
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					case 3:
						LED_ShowData_K.bdata_0 =bLED_Number[3];//3
						LED_ShowData_K.bdata_1 =bLED_Char[10];//P
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					case 4:
						LED_ShowData_K.bdata_0 =bLED_Number[2];//2
						LED_ShowData_K.bdata_1 =bLED_Number[0];//0
						LED_ShowData_K.bdata_2 =bLED_Number[2];//G
						LED_ShowData_K.bdata_3 =0;
						break;
					case 5:
						LED_ShowData_K.bdata_0 =bLED_Number[3];//3
						LED_ShowData_K.bdata_1 =bLED_Number[3];//3
						LED_ShowData_K.bdata_2 =bLED_Number[0];//0
						LED_ShowData_K.bdata_3 =0;
						break;
					case 6://朗仕LS
						LED_ShowData_K.bdata_0 =bLED_Number[5];//3
						LED_ShowData_K.bdata_1 =bLED_Char[14];//3
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					case 7://1200Kpa比例阀：当前显示70
						LED_ShowData_K.bdata_0 =bLED_Number[0];//0
						LED_ShowData_K.bdata_1 =bLED_Number[7];//7
						LED_ShowData_K.bdata_2 =0;
						LED_ShowData_K.bdata_3 =0;
						break;
					default:
						LED_ShowData_K.bdata_0 =bLED_Number[0];//0
						LED_ShowData_K.bdata_1 =bLED_Number[0];//0
						LED_ShowData_K.bdata_2 =bLED_Number[0];//0
						LED_ShowData_K.bdata_3 =bLED_Number[0];//0;
						break;
					}
					break;
				case 10:
					Show_flData(KEY_MENU_Temp_Data.H10,KEY_MENU_Temp_Data.H01);
					break;
				case 11:
					Show_Input();
					break;
				case 12:
					LED_ShowData_K.bdata_3 = bLED_Char[14];//'L'
					LED_ShowData_K.bdata_2 = bLED_Char[16];//'E'
					LED_ShowData_K.bdata_1 = bLED_Char[2];//'d'
					LED_ShowData_K.bdata_0 = bLED_Number[KEY_MENU_Temp_Data.H12];
					break;
				case 13:
					if(Key!=Key_Ok)
					{
						if((KEY_MENU_Temp_Data.H13==0))//&&(bFlash==0))
							LED_ShowData_K.bdata_3 = bLED_Char[1];//'n'
						else
							LED_ShowData_K.bdata_3 = 0;//
					}
					LED_ShowData_K.bdata_2 = bLED_Char[5];//'r'
					LED_ShowData_K.bdata_1 = bLED_Char[16];//'E'
					LED_ShowData_K.bdata_0 = bLED_Char[15];//'C';
					break;
			}

		}
		us_bLEDShow_SetData(LED_ShowData_K);
		pre_LED_ShowData=LED_ShowData;
	}
Exit:
	return Ret;
#endif
}

/***************************************************
 *   鍚嶇О锛�     	us_bKeyCall_Func()
 *   鍔熻兘锛�	璋冪敤鎸夐敭浜嬩欢鍑芥暟
 *   鍑芥暟鍙傛暟锛�KeyCall_Event Key_Func 璋冪敤鐨勫嚱鏁�
 *   			uint8_t *bpdata             浼犻�鐨勫弬鏁�
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
static uint8_t us_bKeyCall_Func(KeyCall_Event Key_Func, uint8_t *bpdata)
{
    uint8_t Ret = 0;
    if (Key_Func != NULL)
    {
        Ret = Key_Func(bpdata);
    }
    else
    {
        Ret = 1;
    }
    return Ret;
}
/***************************************************
 *   鍚嶇О锛�     	bKey_Up(uint16_t dat,uint16_t max,uint8_t unit)
 *   鍔熻兘锛�	Up浜嬩欢
 *   鍑芥暟鍙傛暟锛�    dat鏁版嵁 max鏈�ぇ鍊�speed_cnt up閫熷害  unit鍗曚綅
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
uint16_t bKey_up(uint16_t dat, uint16_t max, uint16_t speed_cnt, uint8_t unit)
{
    if (dat < max)
    {
        if (unit == 3)    // Mpa
            dat = dat + 10;
        //		else if(unit==1)//bar
        //		{
        //			if(speed_cnt>160)
        //				speed_cnt=160;
        //			dat=dat+3+speed_cnt/20;
        //		}
        else if (dat < 1000)
        {
            dat = dat + 1 + speed_cnt / 100;
            if (dat > 1000)
                dat = 1000;
        }
        else
            dat = dat + 10;
        if (dat > max)
            dat = max;
    }
    else
        dat = max;
    return (dat);
}
/***************************************************
 *   鍚嶇О锛�     	us_bKey_Up_Func()
 *   鍔熻兘锛�	Up浜嬩欢
 *   鍑芥暟鍙傛暟锛�    uint8_t *bpdata             浼犻�鐨勫弬鏁�
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
static uint8_t us_bKey_Up_Func(uint8_t *bpdata)
{
    static uint16_t cnt = 0;
    uint16_t        temp, temp1;
    if (bLock == 1)
        return 0;
    if (bButtonDeep_OK == 1)
    {
        cnt++;
        if (cnt >= 2)    // 缈婚〉鎸夐敭璋冩參鐐�
        {
            bMenU_number++;
            cnt = 0;
        }
        if (bMenU_number > 13)
        {
            bMenU_number = 0;
        }
    }
    else if (bButtonDeep_OK == 2)
    {
        switch (bMenU_number)
        {
        case 1:    // 涓嶈皟鏁�
#if UNIT_CHANGE
            if (KEY_MENU_Temp_Data.H01 < 3)
            {
                KEY_MENU_Temp_Data.H01++;
                if ((calibration.pressure_scale_kpa > 999) && (KEY_MENU_Temp_Data.H01 == 2))
                    KEY_MENU_Temp_Data.H01 = 3;
            }
            else
                KEY_MENU_Temp_Data.H01 = 0;
#endif
            break;
        case 2:
            if (KEY_MENU_Temp_Data.H02 < 1)
                KEY_MENU_Temp_Data.H02++;
            else
                KEY_MENU_Temp_Data.H02 = 0;
            break;
        case 3:
            //				if(us_bGetADCINMode()==ADCINSwitch)
            //					KEY_MENU_Temp_Data.H03=2;
            //				else
            //				{
            if (KEY_MENU_Temp_Data.H03 < 2)
                KEY_MENU_Temp_Data.H03++;
            else
                KEY_MENU_Temp_Data.H03 = 0;
            //				}
            break;
        case 4:
            KEY_MENU_Temp_Data.H04 = bKey_up(KEY_MENU_Temp_Data.H04, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
            break;
        case 5:
            KEY_MENU_Temp_Data.H05 = bKey_up(KEY_MENU_Temp_Data.H05, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
            break;
        case 6:
            switch (KEY_MENU_Temp_Data.H06)
            {
            case 0:
                KEY_MENU_Temp_Data.H061 = bKey_up(KEY_MENU_Temp_Data.H061, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 1:
                KEY_MENU_Temp_Data.H062 = bKey_up(KEY_MENU_Temp_Data.H062, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 2:
                KEY_MENU_Temp_Data.H063 = bKey_up(KEY_MENU_Temp_Data.H063, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 3:
                KEY_MENU_Temp_Data.H064 = bKey_up(KEY_MENU_Temp_Data.H064, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
                break;
            }
            break;
        case 7:
            //				if(KEY_MENU_Temp_Data.H01==0)//psi
            //					temp=KEY_MENU_Temp_Data.H08-14;
            //				else
            //					temp=KEY_MENU_Temp_Data.H08-100;
            //				temp1=KEY_MENU_Temp_Data.Max_H08/2;
            //				if(temp>temp1)//鍙栧皬
            //					temp=temp1;
            if (us_bGet_ID3().ID3_Unit == 2)
                temp = KEY_MENU_Temp_Data.H08 - 4;
            else
                temp = KEY_MENU_Temp_Data.H08 - 25;
            //				temp1=KEY_MENU_Temp_Data.Max_H08/2;
            temp1 = KEY_MENU_Temp_Data.H08 / 2;
            if (temp > temp1)    // 鍙栧皬
                temp = temp1;

            if (KEY_MENU_Temp_Data.H07 < temp)
            {
                if (KEY_MENU_Temp_Data.H01 == 3)    // Mpa
                {
                    if (KEY_MENU_Temp_Data.H07 < 0)
                        KEY_MENU_Temp_Data.H07 += 1;
                    else
                        KEY_MENU_Temp_Data.H07 += 10;
                }
                //					else if(KEY_MENU_Temp_Data.H01==1)
                //					{
                //						if(KEY_MENU_Temp_Data.H07<0)
                //							KEY_MENU_Temp_Data.H07+=1;
                //						else
                //						{
                //							if(key_up_cnt>160)
                //								key_up_cnt=160;
                //							KEY_MENU_Temp_Data.H07=KEY_MENU_Temp_Data.H07+3+key_up_cnt/20;
                //						}
                //					}
                else
                    KEY_MENU_Temp_Data.H07 = KEY_MENU_Temp_Data.H07 + 1 + key_up_cnt / 100;
                if (KEY_MENU_Temp_Data.H07 > temp)
                    KEY_MENU_Temp_Data.H07 = temp;
            }
            break;
        case 8:
            if (us_bGet_ID3().ID3_Unit == 2)
                temp = KEY_MENU_Temp_Data.H07 + 4;
            else
                temp = KEY_MENU_Temp_Data.H07 + 25;
            if (temp > KEY_MENU_Temp_Data.Max_H08)
                temp = KEY_MENU_Temp_Data.Max_H08;
            temp1 = KEY_MENU_Temp_Data.Max_H08 / 10;
            if (temp < temp1)    // 鍙栧ぇ
                temp = temp1;
            if (KEY_MENU_Temp_Data.H08 < temp)
                KEY_MENU_Temp_Data.H08 = temp;
            else
                KEY_MENU_Temp_Data.H08 = bKey_up(KEY_MENU_Temp_Data.H08, KEY_MENU_Temp_Data.Max_H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
            //				if(KEY_MENU_Temp_Data.H08 < KEY_MENU_Temp_Data.Max_H08)
            //				{
            ////					KEY_MENU_Temp_Data.H02++;
            //					if(KEY_MENU_Temp_Data.H01==3)
            //						KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08+10;
            //					else if(KEY_MENU_Temp_Data.H01==1)
            //					{
            //						if(key_up_cnt>160)
            //							key_up_cnt=160;
            //						KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08+3+key_up_cnt/20;
            //					}
            //					else
            ////						KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08+1+key_up_cnt/100;
            // 					if(KEY_MENU_Temp_Data.H08<1000)
            //					{
            //						KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08+1+key_up_cnt/100;
            //						if(KEY_MENU_Temp_Data.H08>1000)
            //							KEY_MENU_Temp_Data.H08=1000;
            //					}
            //					else
            //						KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08+10;
            //					if(KEY_MENU_Temp_Data.H08 > KEY_MENU_Temp_Data.Max_H08)
            //						KEY_MENU_Temp_Data.H08 = KEY_MENU_Temp_Data.Max_H08;
            //				}
            //				else
            //					KEY_MENU_Temp_Data.H08 = KEY_MENU_Temp_Data.Max_H08;
            break;
        case 9:
            KEY_MENU_Temp_Data.H09++;
            if (KEY_MENU_Temp_Data.H09 > 50)
            {
                KEY_MENU_Temp_Data.H09 = 5;
            }
            break;
        case 10:
            KEY_MENU_Temp_Data.H10 = bKey_up(KEY_MENU_Temp_Data.H10, KEY_MENU_Temp_Data.H08, key_up_cnt, KEY_MENU_Temp_Data.H01);
            break;
            //			case 11://鍙樉绀�
            //				KEY_MENU_Temp_Data.H11++;
            //				break;
        case 12:
            KEY_MENU_Temp_Data.H12++;
            if (KEY_MENU_Temp_Data.H12 > 1)
                KEY_MENU_Temp_Data.H12 = 0;
            break;
        case 13:
            KEY_MENU_Temp_Data.H13++;
            if (KEY_MENU_Temp_Data.H13 > 1)
                KEY_MENU_Temp_Data.H13 = 0;
            break;
        }
    }
    return 0;
}
/***************************************************
 *   鍚嶇О锛�     	bKey_Down()
 *   鍔熻兘锛�	Down浜嬩欢
 *   鍑芥暟鍙傛暟锛�    uint8_t *bpdata             浼犻�鐨勫弬鏁�
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
uint16_t bKey_Down(uint16_t dat, uint16_t min, uint16_t speed_cnt, uint8_t unit)
{
    if (dat > min)
    {
        if (dat > 10)
        {
            if (unit == 3)
                dat = dat - 10;
            //			else if(unit==1)//bar
            //			{
            //				if(speed_cnt>160)
            //					speed_cnt=160;
            //				dat=dat-3-speed_cnt/20;
            //			}
            //			else
            {
                if (dat >= 1010)
                    dat = dat - 10;
                else
                    dat = dat - 1 - speed_cnt / 100;
            }
        }
        else
            dat--;
    }
    else
        dat = min;
    if (dat < min)
        dat = min;
    return (dat);
}
/***************************************************
 *   鍚嶇О锛�     	us_bKey_Up_Func()
 *   鍔熻兘锛�	Down浜嬩欢
 *   鍑芥暟鍙傛暟锛�    uint8_t *bpdata             浼犻�鐨勫弬鏁�
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
static uint8_t us_bKey_Down_Func(uint8_t *bpdata)
{
    static uint16_t cnt = 0;
    s16             temp, temp1;
    if (bLock == 1)
        return 0;
    if (bButtonDeep_OK == 1)
    {
        if (bMenU_number > 0)
        {
            cnt++;
            if (cnt >= 2)
            {
                bMenU_number--;    // 缈婚〉鎸夐敭璋冩參鐐�
                cnt = 0;
            }
        }
        else
            bMenU_number = 13;
    }
    else if (bButtonDeep_OK == 2)
    {
        switch (bMenU_number)
        {
        case 1:    // 绗竴椤典笉璋冩暣
#if UNIT_CHANGE
            if (KEY_MENU_Temp_Data.H01 > 0)
            {
                KEY_MENU_Temp_Data.H01--;
                if ((calibration.pressure_scale_kpa > 999) && (KEY_MENU_Temp_Data.H01 == 2))
                    KEY_MENU_Temp_Data.H01 = 1;
            }
            else
                KEY_MENU_Temp_Data.H01 = 3;
#endif
            break;
        case 2:
            if (KEY_MENU_Temp_Data.H02 > 0)
                KEY_MENU_Temp_Data.H02--;
            else
                KEY_MENU_Temp_Data.H02 = 1;
            break;
        case 3:
            //				if(us_bGetADCINMode()==ADCINSwitch)
            //					KEY_MENU_Temp_Data.H03=2;
            //				else
            //				{
            if (KEY_MENU_Temp_Data.H03 > 0)
                KEY_MENU_Temp_Data.H03--;
            else
                KEY_MENU_Temp_Data.H03 = 2;
            //				}
            break;
        case 4:
            KEY_MENU_Temp_Data.H04 = bKey_Down(KEY_MENU_Temp_Data.H04, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
            break;
        case 5:
            KEY_MENU_Temp_Data.H05 = bKey_Down(KEY_MENU_Temp_Data.H05, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
            break;
        case 6:
            switch (KEY_MENU_Temp_Data.H06)
            {
            case 0:
                KEY_MENU_Temp_Data.H061 = bKey_Down(KEY_MENU_Temp_Data.H061, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 1:
                KEY_MENU_Temp_Data.H062 = bKey_Down(KEY_MENU_Temp_Data.H062, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 2:
                KEY_MENU_Temp_Data.H063 = bKey_Down(KEY_MENU_Temp_Data.H063, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
                break;
            case 3:
                KEY_MENU_Temp_Data.H064 = bKey_Down(KEY_MENU_Temp_Data.H064, 0, key_down_cnt, KEY_MENU_Temp_Data.H01);
                break;
            }
            break;
        case 7:
            if (KEY_MENU_Temp_Data.H07 > -20)
            {
                if (KEY_MENU_Temp_Data.H07 > 10)
                {
                    if (KEY_MENU_Temp_Data.H01 == 3)    // Mpa
                        KEY_MENU_Temp_Data.H07 = KEY_MENU_Temp_Data.H07 - 10;
                    //						else if(KEY_MENU_Temp_Data.H01==1)//bar
                    //						{
                    //							if(key_down_cnt>160)
                    //								key_down_cnt=160;
                    //							KEY_MENU_Temp_Data.H07=KEY_MENU_Temp_Data.H07-3-key_down_cnt/20;
                    //						}
                    else
                        KEY_MENU_Temp_Data.H07 = KEY_MENU_Temp_Data.H07 - 1 - key_down_cnt / 100;
                }
                else
                    KEY_MENU_Temp_Data.H07--;
            }
            else
                KEY_MENU_Temp_Data.H07 = -20;
            if (KEY_MENU_Temp_Data.H07 > KEY_MENU_Temp_Data.H08 / 2)
                KEY_MENU_Temp_Data.H07 = KEY_MENU_Temp_Data.H08 / 2;
            else if (KEY_MENU_Temp_Data.H07 < -20)    // 涓嶅厑璁稿皬浜�
                KEY_MENU_Temp_Data.H07 = -20;
            break;
        case 8:
            //				temp=KEY_MENU_Temp_Data.H03+4;
            //				temp=KEY_MENU_Temp_Data.Max_H08/2;
            if (us_bGet_ID3().ID3_Unit == 2)
                temp = KEY_MENU_Temp_Data.H07 + 4;
            else
                temp = KEY_MENU_Temp_Data.H07 + 25;
            if (temp > KEY_MENU_Temp_Data.Max_H08)
                temp = KEY_MENU_Temp_Data.Max_H08;
            if (KEY_MENU_Temp_Data.H08 > KEY_MENU_Temp_Data.Max_H08)
                KEY_MENU_Temp_Data.H08 = KEY_MENU_Temp_Data.Max_H08;
            temp1 = KEY_MENU_Temp_Data.Max_H08 / 10;
            //				temp1=KEY_MENU_Temp_Data.H08/10;
            if (temp < temp1)    // 鍙栧ぇ
                temp = temp1;
            KEY_MENU_Temp_Data.H08 = bKey_Down(KEY_MENU_Temp_Data.H08, temp, key_down_cnt, KEY_MENU_Temp_Data.H01);
            //                if(temp<temp1)//鍙栧ぇ
            //                	temp=temp1;
            //				if(KEY_MENU_Temp_Data.H08 > temp)
            //				{
            //					if(KEY_MENU_Temp_Data.H08 > temp+10)
            //					{
            //						if(KEY_MENU_Temp_Data.H01==3)
            //							KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08-10;
            //						else if(KEY_MENU_Temp_Data.H01==1)
            //						{
            //							if(key_down_cnt>160)
            //								key_down_cnt=160;
            //							KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08-3-key_down_cnt/20;
            //						}
            //						else
            ////							KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08-1-key_down_cnt/100;
            //						if(KEY_MENU_Temp_Data.H08>=1010)
            //							KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08-10;
            //						else
            //							KEY_MENU_Temp_Data.H08=KEY_MENU_Temp_Data.H08-1-key_down_cnt/100;
            //					}
            //					else
            //						KEY_MENU_Temp_Data.H08--;
            //				}
            //				else
            //					KEY_MENU_Temp_Data.H08 = temp;
            break;
        case 9:
            if (KEY_MENU_Temp_Data.H09 > 5)
            {
                KEY_MENU_Temp_Data.H09--;
            }
            else
            {
                KEY_MENU_Temp_Data.H09 = 50;
            }
            break;
        case 10:
            if (KEY_MENU_Temp_Data.H07 <= 0)
                temp = 0;
            else
                temp = KEY_MENU_Temp_Data.H07;
            KEY_MENU_Temp_Data.H10 = bKey_Down(KEY_MENU_Temp_Data.H10, temp, key_down_cnt, KEY_MENU_Temp_Data.H01);
            //				if(KEY_MENU_Temp_Data.H10<KEY_MENU_Temp_Data.H07)
            //					KEY_MENU_Temp_Data.H10=KEY_MENU_Temp_Data.H07;
            break;
        case 11:
            KEY_MENU_Temp_Data.H11--;
            break;
        case 12:
            if (KEY_MENU_Temp_Data.H12 > 0)
                KEY_MENU_Temp_Data.H12--;
            else
                KEY_MENU_Temp_Data.H12 = 1;
            break;
        case 13:
            if (KEY_MENU_Temp_Data.H13 > 0)
                KEY_MENU_Temp_Data.H13--;
            else
                KEY_MENU_Temp_Data.H13 = 1;
            break;
        }
    }
    return 0;
}

/***************************************************
 *   鍚嶇О锛�     	us_bKey_Ok_Func()
 *   鍔熻兘锛�	Ok浜嬩欢
 *   鍑芥暟鍙傛暟锛�    uint8_t *bpdata             浼犻�鐨勫弬鏁�
 *   杩斿洖鍊硷細	Ret 鐘舵�
 ***************************************************/
static uint8_t us_bKey_Ok_Func(uint8_t *bpdata)
{
    static uint16_t pre_KEY_MENU_Temp_Data_080 = 0, pre_KEY_MENU_Temp_Data_081 = 0, pre_KEY_MENU_Temp_Data_082 = 0, pre_KEY_MENU_Temp_Data_083 = 0;
    static s16      pre_KEY_MENU_Temp_Data_07 = 0;
    if (bMenU_number == 8)
    {
        switch (KEY_MENU_Temp_Data.H01)
        {
        case 0:    // psi
            if ((KEY_MENU_Temp_Data.H08 > 999) && (pre_KEY_MENU_Temp_Data_080 > 0) && (abs(KEY_MENU_Temp_Data.H08 - pre_KEY_MENU_Temp_Data_080) >= 10))
            {
                KEY_MENU_Temp_Data.H08 = ((KEY_MENU_Temp_Data.H08 + 5) / 10) * 10;
                ubMaxScalePsi10        = KEY_MENU_Data.H08 * 40;
            }
            pre_KEY_MENU_Temp_Data_080 = KEY_MENU_Temp_Data.H08;
            break;
        case 1:    // Bar
            if ((KEY_MENU_Temp_Data.H08 > 999) && (pre_KEY_MENU_Temp_Data_081 > 0) && (abs(KEY_MENU_Temp_Data.H08 - pre_KEY_MENU_Temp_Data_081) >= 10))
            {
                KEY_MENU_Temp_Data.H08 = ((KEY_MENU_Temp_Data.H08 + 5) / 10) * 10;
                ubMaxScalePsi10        = KEY_MENU_Data.H08 * 58;
            }
            pre_KEY_MENU_Temp_Data_081 = KEY_MENU_Temp_Data.H08;
            break;
        case 2:    // Kpa
            if (KEY_MENU_Temp_Data.H08 > 999)
                KEY_MENU_Temp_Data.H08 = 999;
            break;
        case 3:    // Mpa
            if ((pre_KEY_MENU_Temp_Data_083 > 0) && (abs(KEY_MENU_Temp_Data.H08 - pre_KEY_MENU_Temp_Data_083) >= 10))
            {
                KEY_MENU_Temp_Data.H08 = ((KEY_MENU_Temp_Data.H08 + 5) / 10) * 10;
                ubMaxScalePsi10        = KEY_MENU_Data.H08 * 58;
            }
            pre_KEY_MENU_Temp_Data_083 = KEY_MENU_Temp_Data.H08;
            break;
        }
        if (KEY_MENU_Temp_Data.H08 > KEY_MENU_Temp_Data.Max_H08)
            KEY_MENU_Temp_Data.H08 = KEY_MENU_Temp_Data.Max_H08;
    }
    else if ((bMenU_number == 7) && (KEY_MENU_Temp_Data.H01 == 3))
    {
        if ((pre_KEY_MENU_Temp_Data_07 > 0) && (KEY_MENU_Temp_Data.H07 > 0) && (abs(KEY_MENU_Temp_Data.H07 - pre_KEY_MENU_Temp_Data_07) >= 10))
        {
            KEY_MENU_Temp_Data.H07 = ((KEY_MENU_Temp_Data.H07 + 5) / 10) * 10;
            ubMinScalePsi10        = KEY_MENU_Temp_Data.H07 * 58 + 20;
        }
        pre_KEY_MENU_Temp_Data_07 = KEY_MENU_Temp_Data.H07;
    }
    memcpy(&KEY_MENU_Data, &KEY_MENU_Temp_Data, sizeof(KEY_MENU));
    LED_Act();
    bCount_Ok++;
    if (bCount_Ok > 10)        // 闀挎寜瑙﹀彂
    {
        bButtonDeep_OK = 1;    // 娣卞害涓�
        bMenU_number   = 0;    // 杩涘叆绗竴灞傝彍鍗�
    }
    if ((bMenU_number == 6) && (bButtonDeep_OK == 2) && (KEY_MENU_Temp_Data.H06 <= 3))
    {
    }
    else if ((bButtonDeep_OK == 1) && (bCount_Ok == 1))
    {
        bCount_Ok = 0;
        bFlag_OK  = 0;
        switch (bMenU_number)
        {
        case 0:
            bMenU_number = 0;    // 閫�嚭璁剧疆椤甸潰
            //				bButtonDeep_OK = 0;
            bFlash = 1;
            break;
        case 1:
        case 2:    // 绗簩椤电敤
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:    // 用于显示产品类型选择
        case 10:
        case 11:
        case 12:
        case 13:
            bFlash = 1;
            //				bButtonDeep_OK = 2;
            break;
        default:
            break;
        }
    }
    else if ((bButtonDeep_OK == 2) && (bCount_Ok == 1))
    {
        bCount_Ok = 0;
        //		bButtonDeep_OK = 1;
        bFlash = 1;
    }
    return 0;
}
