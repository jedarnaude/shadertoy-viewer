#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

/**
 * @return	minimum value between a and b.
 */
double Min(double a, double b) {
	return a < b ? a : b;
}

/**
 * @return	maximum value between a and b.
 */
double Max(double a, double b) {
	return a > b ? a : b;
}

/**
* @return	clamped value of c between min and max.
*/
double Clamp(double c, double min, double max) {
	return Min(Max(min, c), max);
}

/**
 * Audio conversion from linear to decibels.
 */
float LinearToDecibels(float linear) {
	if (!linear)
		return -1000;

	return 20 * log10f(linear);
}
