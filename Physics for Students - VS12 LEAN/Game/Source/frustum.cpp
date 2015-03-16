//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Frustum                                           //
//*****************************************************************************************//

Frustum::Frustum () {}

Frustum::~Frustum () {}

void Frustum::tick () {
	Transformation cameraInverse;
	glGetMatrixf (cameraInverse);

	Transformation projectionMatrix;
	glGetMatrixf (GL_PROJECTION_MATRIX, projectionMatrix);

	Transformation mul; mul.multiply (cameraInverse, projectionMatrix);
	
	// Left
	Point leftP = Point (mul.m14 + mul.m11, mul.m24 + mul.m21, mul.m34 + mul.m31);
	double leftOffset = (mul.m44 + mul.m41) / leftP.length ();
	leftP /= leftP.length ();
	planes [0] = Plane (leftP, leftOffset);

	// Right
	Point rightP = Point (mul.m14 - mul.m11, mul.m24 - mul.m21, mul.m34 - mul.m31);
	double rightOffset = (mul.m44 - mul.m41) / rightP.length ();
	rightP /= rightP.length ();
	planes [1] = Plane (rightP, rightOffset);

	// Up
	Point upP = Point (mul.m14 - mul.m12, mul.m24 - mul.m22, mul.m34 - mul.m32);
	double upOffset = (mul.m44 - mul.m42) / upP.length ();
	upP /= upP.length ();
	planes [2] = Plane (upP, upOffset);

	// Down
	Point downP = Point (mul.m14 + mul.m12, mul.m24 + mul.m22, mul.m34 + mul.m32);
	double downOffset = (mul.m44 + mul.m42) / downP.length ();
	downP /= downP.length ();
	planes [3] = Plane (downP, downOffset);

	// Calculate the closest corner for each plane
	BoundingBox unitBox (-0.5, -0.5, -0.5, 0.5, 0.5, 0.5); // not sure about the Zs
	for (long i = 0; i < 4; i++) {
		mostPositivePointIndex [i] = computeIndexOfMostPositivePoint (unitBox, planes [i]);
	}
}

long Frustum::computeIndexOfMostPositivePoint  (BoundingBox &anyBox, Plane &plane) {
	double distance = plane.distanceToPoint (anyBox.getCorner (0));

	long indexOfMostPositive = 0; //So far

	for (long index = 1; index < 8; index++) {		
		double d = plane.distanceToPoint (anyBox.getCorner (index));
		if (d > distance) {
			distance = d; 
			indexOfMostPositive = index;
		}
	}
	return indexOfMostPositive ; 
}

bool Frustum::isOutside (Point &point) {
	for (long index = 0; index < 4; index++) {
		if (planes [index].isOutside (point))
			return true; //i.e., distance < 0.0
	}
	return false;
}
bool Frustum::isOutside (BoundingBox &box) {
	for (long index = 0; index < 4; index++) {	    
	    if (planes [index].isOutside (box.getCorner (mostPositivePointIndex [index])))
			return true;
	}
	return false;
}