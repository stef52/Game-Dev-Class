//*****************************************************************************************//
//                                       BoundingBox                                            //
//*****************************************************************************************//

#ifndef boundingBoxModule
#define boundingBoxModule 

#include "points.h"

#define BOTTOM_LEFT_BACK 0
#define BOTTOM_LEFT_FRONT 1
#define BOTTOM_RIGHT_BACK 2
#define BOTTOM_RIGHT_FRONT 3
#define TOP_LEFT_BACK 4
#define TOP_LEFT_FRONT 5
#define TOP_RIGHT_BACK 6
#define TOP_RIGHT_FRONT 7

class GamePoint;

class BoundingBox {
public:
	BoundingBox ();
	BoundingBox (double minX, double minY, double minZ, double maxX, double maxY, double maxZ);
	BoundingBox (Point &min, Point &max);
	~BoundingBox ();

	void add (GamePoint &point);
	Point minimum () const;
	Point maximum () const;	
	Point extent () const;
	Point getCorner (long whichCorner) const;
	Point center () const;
	Point bottomCenter () const;
	
	
	void offsetBy (Point &offset);
	void center (Point &center);

private:
	void initialize ();

	Point min;
	Point max;
	Point privateExtent;
	Point privateCenter;
	Point privateBottomCenter;
	
};

#endif // end boundingBoxModule
