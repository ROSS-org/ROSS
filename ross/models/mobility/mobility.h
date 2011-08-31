#ifndef _MOBILITY_H_
#define _MOBILITY_H_

typedef struct {
	double time;
	double lat;
	double lng;
	double alt;
} tw_geo_pt;

typedef struct {
	double time;
	double x;
	double y;
	double z;
} tw_grid_pt;

typedef struct {
	int x;
	int y;
	int z;
} tw_integer_grid_pt;

double calculateGridDistance(tw_grid_pt pt1, tw_grid_pt pt2);


#endif /* _MOBILITY_H_ */



