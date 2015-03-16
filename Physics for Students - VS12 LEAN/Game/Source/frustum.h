//*****************************************************************************************//
//                                        Frustum                                           //
//*****************************************************************************************//
#ifndef frustumObjectModule
#define frustumObjectModule 

#include "boundingBox.h"

class Frustum {
public:
	Frustum ();
	~Frustum ();

	void tick ();
	
	bool isOutside (Point &point);
	bool isOutside (BoundingBox &box);


private:
	Plane planes [4];
	long mostPositivePointIndex [4];

	long computeIndexOfMostPositivePoint (BoundingBox &anyBox, Plane &plane);
};


#endif // frustumObjectModule