//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                     BoundingBox                                         //
//*****************************************************************************************//
BoundingBox::BoundingBox ()
{
	min = Point (MAXINT,MAXINT,MAXINT);
	max = Point (MININT,MININT,MININT);
	initialize ();
}

BoundingBox::BoundingBox (Point &min, Point &max)
{
	this->min = min;
	this->max = max;
	initialize ();
}

BoundingBox::BoundingBox (double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
{
	this->min = Point (minX, minY, minZ);
	this->max = Point (maxX, maxY, maxZ);
	initialize ();
}

BoundingBox::~BoundingBox ()
{
}

void BoundingBox::add (GamePoint &point) {
	min.x = min.x < point.x ? min.x : point.x; 
	min.y = min.y < point.y ? min.y : point.y; 
	min.z = min.z < point.z ? min.z : point.z; 
	max.x = max.x > point.x ? max.x : point.x; 
	max.y = max.y > point.y ? max.y : point.y; 
	max.z = max.z > point.z ? max.z : point.z; 

	// Make addjustments
	initialize ();
}

void BoundingBox::initialize () {
	// initialize relevant point data from current min and max
	privateCenter = (min + max) / 2;

	privateBottomCenter = privateCenter;
	privateBottomCenter.y = min.y;

	privateExtent = max - min;
}

Point BoundingBox::maximum () const {
	return max;
}

Point BoundingBox::minimum () const {
	return min;
}

Point BoundingBox::getCorner (long whichCorner) const {
	// get a point on the box
	switch (whichCorner) {
		case BOTTOM_LEFT_BACK:
			return min;
		case BOTTOM_LEFT_FRONT:
			return Point (min.x, min.y, max.z);
		case BOTTOM_RIGHT_BACK:
			return Point (max.x, min.y, min.z);
		case BOTTOM_RIGHT_FRONT:
			return Point (max.x, min.y, max.z);
		case TOP_LEFT_BACK:
			return Point (min.x, max.y, min.z);
		case TOP_LEFT_FRONT:
			return Point (min.x, max.y, max.z);
		case TOP_RIGHT_BACK:
			return Point (max.x, max.y, min.z);
		case TOP_RIGHT_FRONT:
			return max;
		default:
			return Point (-INT_MAX,-INT_MAX,-INT_MAX);
	}
}

Point BoundingBox::center () const {
	return privateCenter;
}

Point BoundingBox::bottomCenter () const {
	return privateBottomCenter;
}

Point BoundingBox::extent () const {
	return privateExtent;
}

void BoundingBox::offsetBy (Point &offset) {
	min += offset;
	max += offset;
}

void BoundingBox::center (Point &center) {
	max = privateCenter + privateExtent * 0.5;
	min = privateCenter - privateExtent * 0.5;
	initialize ();
}