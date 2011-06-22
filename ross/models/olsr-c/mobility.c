#include <ross.h>

#include "mobility.h"
/** @file mobility.c
 * @brief Support for mobility in models
 *
 */
#define PI 3.1415926535

/** deg2rad
 * @brief Covert Degrees to Radians
 *
 * 
 */
static inline double deg2rad(double deg) {
	return deg*(PI/180);
}

/** rad2deg
 * @brief Covert Radians to Degrees
 *
 * 
 */
static inline double rad2deg(double rad) {
	return rad*(180/PI);
}

/** calculateGridDistance
 * @brief Calculate Distance on a plain grid
 *
 * We calculate distance on the grid using the Pythagorean theorem
 *
 * \f$d = \sqrt{(x_2 - x_1)^2 + (y_2 - y_1)^2 + (z_2 - z_1)^2}\f$
 */
double calculateGridDistance(tw_grid_pt pt1, tw_grid_pt pt2) {
	return sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2) + pow(pt2.z - pt1.z, 2));
}

