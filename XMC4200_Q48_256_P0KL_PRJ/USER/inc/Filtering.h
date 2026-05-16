#ifndef __FILTERING_H
#define __FILTERING_H
#include <stdlib.h>
double       KalmanFilter(const float ResrcData, double ProcessNiose_Q, double MeasureNoise_R, int index);
unsigned int IntegralFilter(unsigned int AdData, unsigned int AdTime);

#define KALMAN_Q 0.0001
#define KALMAN_R 5

#endif
