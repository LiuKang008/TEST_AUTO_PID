#include "Filtering.h"
#define USE_TICKCLOCK  (0)
#define INTERGRAL_TIME (1)                  // �����˲��õ��ı�����Ϊ�����ڴ�ռ�úܴ󣬲��õ�ʱ����Ϊ1���õ�ʱ���޸Ļ��ִ���
unsigned int AdData_Buf[INTERGRAL_TIME];    //

/*
 Q:����������Q���󣬶�̬��Ӧ��죬�����ȶ��Ա仵
 R:����������R���󣬶�̬��Ӧ�����������ȶ��Ա��
 ��������������������ѧ������ƫ��̶ȣ����������̶ȡ�
 ���ݲ���ԭʼADֵ��
 ƽ��ֵ	��׼ƫ��		��Ʒ����
 0V		3871 3876 3877 3874 3876 3873 3870 3873 3874 3871 3875 3872 3875  	3873.6	  2.18		4.75
 5V		2858 2857 2854 2855 2853 2852 2857 2854 2859 2857 2856 2858 2859 	2856.07	  2.289		5.24
 10V	1839 1835 1837 1841 1838 1835 1836 1841 1836 1837 1834 1837 1835	1837	  2.236		5

 ��������RֵѡȡΪ��5
 ��������QֵѡȡֵΪ��0.0001
 */
double KalmanFilter(const float ResrcData, double ProcessNiose_Q, double MeasureNoise_R, int index)
{
    static double x_last[8];
    static double p_last[8];
    double        R = MeasureNoise_R;
    double        Q = ProcessNiose_Q;
    double        x_mid;
    double        x_now;
    double        p_mid;
    double        p_now;
    double        kg;
#if 1
    if (abs(x_last[index] - ResrcData) >= 20)
    {
        x_mid = ResrcData * 0.382f + x_last[index] * 0.618f;
        // x_mid = ResrcData;
        // p_last[index]=0;
    }
    else
    {
        x_mid = x_last[index];
    }
#endif
#if 0
	x_mid = x_last[index]; //x_last=x(k-1|k-1),x_mid=x(k|k-1)
#endif
    p_mid         = p_last[index] + Q;                   // p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=����
    kg            = p_mid / (p_mid + R);                 // kgΪkalman filter��RΪ����
    x_now         = x_mid + kg * (ResrcData - x_mid);    // ���Ƴ�������ֵ
    p_now         = (1 - kg) * p_mid;                    // ����ֵ��Ӧ��covariance
    p_last[index] = p_now;                               // ����covarianceֵ
    x_last[index] = x_now;                               // ����ϵͳ״ֵ̬
    return x_now;
}

/*------------------------------------------------------------------*/

unsigned int IntegralFilter(unsigned int AdData, unsigned int AdTime)
{
    static unsigned int  Count_AdTime      = 0;
    static unsigned int  AdData_Accumulate = 0;
    static unsigned int  AdData_Result     = 0;
    static unsigned char Flag_FirstLoop    = 0;

    static unsigned int AdTime_last = 1;

    if (AdTime_last != AdTime)
    {
        AdTime_last       = AdTime;
        Count_AdTime      = 0;
        Flag_FirstLoop    = 0;
        AdData_Accumulate = 0;
        Flag_FirstLoop    = 0;
    }
    if (Flag_FirstLoop == 1)
    {
        AdData_Accumulate -= AdData_Buf[Count_AdTime];    // ���ݻ��� ��ȥǰ�������
    }

    AdData_Buf[Count_AdTime] = AdData;                // ���ݻ��� ��������������
    AdData_Accumulate += AdData_Buf[Count_AdTime];    // ���ݻ���  �����������ݵ���ֵ
    Count_AdTime++;                                   // ���ݻ��ۼ���

                                                      /////////////////////////////////////////////////////////////////////////////
    if (Flag_FirstLoop == 0)
    {
        AdData_Result = AdData_Accumulate / Count_AdTime;    // ���ݻ��۵�һ��ѭ�� ������Ӧ���ۻ�����
    }
    else if (Flag_FirstLoop == 1)
    {
        AdData_Result = AdData_Accumulate / AdTime_last;    // ���ݻ��۵�һ��ѭ��֮��  �����ܴ���
    }
    /////////////////////////////////////////////////////////////////////////////
    if (Count_AdTime == AdTime_last)
    {
        Count_AdTime   = 0;    // ��ͷ�滻����  �������ݻ��������λ
        Flag_FirstLoop = 1;    // ��һ��ѭ�������ı�־
    }

    return AdData_Result;
}
/*------------------------------------------------------------------*/
