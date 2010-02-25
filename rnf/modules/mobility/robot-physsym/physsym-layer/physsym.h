#ifndef _PHYSSIM_H_
#define _PHYSSIM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
	The main interface of 4-wheeled robot physical simulation
*/

// Can switch between single or double precision
#define physReal double

/************************************************************************/
/* Control interface                                                    */
/************************************************************************/
struct _physControlInput
{
	// goal location
	physReal	goalX;
	physReal	goalY;	// goalZ should be inferred from the map

	physReal	currentX; // optional
	physReal	currentY; // 

	physReal	maxTime; //
};

struct _physControlOutput
{
	int			status; //

	physReal	endX;
	physReal	endY;
	physReal	endZ;
};

/************************************************************************/
/* Query interface                                                      */
/************************************************************************/
struct _physQueryInput
{
	int type;
};

struct _physQueryOutput
{
	physReal	posX;
	physReal	posY;
	physReal	posZ;
};

/************************************************************************/
/*                                                                      */
/************************************************************************/

struct _robotDesc 
{
	// Robot body
	physReal	cogX,cogY,cogZ; // center of gravity
	physReal	inertiaX,inertiaY,inertiaZ ; // principle moment of inertia
	physReal	mass;

	// Wheels
	physReal	w1x,w1y,w1z; // wheels location w.r.t to the cog
	physReal	w2x,w2y,w2z;
	physReal	w3x,w3y,w3z;
	physReal	w4x,w4y,w4z;

	physReal	w1r;		// radius
	physReal	w2r;
	physReal	w3r;
	physReal	w4r;
};

struct _robotState
{
	// Body
	physReal	x,y,z; // position of cog in simulation	
	physReal	rx,ry,rz; // rotation
	physReal	drive; // rotation of front wheels w.r.t to body
};

struct _robot
{
	struct _robotDesc	d;
	struct _robotState	s;
};


/************************************************************************/
/* Interface functions                                                 */
/************************************************************************/

int physSimControl(struct _physControlInput* inp, struct _physControlOutput* outp,struct _robot* r);

int physSimQuery(struct _physQueryInput* inp, struct _physQueryOutput* outp,struct _robot* r);

#ifdef __cplusplus
}
#endif

#endif
