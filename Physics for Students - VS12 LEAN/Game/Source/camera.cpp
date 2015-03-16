
//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#include "includes.all"

//*****************************************************************************************//
//                                       Camera                                            //
//*****************************************************************************************//

Camera *camera = NULL;

void Camera::setup () {
	camera = new Camera;
}

void Camera::wrapup () {
	delete camera; camera = NULL;
} 

Camera::Camera () {
	xRotation = 0.0; target = player; offset = Point (0.0, 1.0, 1.0); //Above and behind...
	//xRotation = 0.0; target = player; offset = Point (0.0, 1.0, 2.0); //Above and behind...
	frustum = new Frustum ();
};

Camera::~Camera () {
	delete frustum;
}

void Camera::tick () {
	//Not currently doing anything but could if lagging was supported...
	frustum->tick ();
}

void Camera::draw () {
	//Normally, the camera is invisible but you might consider drawing a cursor in the center of the screen.
}

void Camera::beginCamera () {
	//If R is the camera's rotation, T is the camera's translation, and W is the player's 
	//transformation, then moving the camera LOCALLY is a pre-transformation. So the movement
	//result needed is RTW... However, we are being asked to push the transformation
	//needed to play the role of a camera...; i.e., an inverse. This means we need to push (RTW)-1  (where -1 means
	//inverse) which by the theorem dealing with inverses becomes W-1T-1R-1... Since the operations 
	//on the stack are done right to left, we need to deal with R-1, then T-1, and then W-1.

	glPushMatrix ();
		glRotated (-xRotation, 1.0, 0.0, 0.0); //Rotate around x-axis...
		glTranslated (-offset.x, -offset.y, -offset.z);
		target->beginCamera ();
			glGetMatrixf (inverseCameraMatrix); cameraMatrix = inverseCameraMatrix.scaleFreeInverse ();

}

void Camera::endCamera () {
		target->endCamera (); //Match indentation in beginCamera.
	glPopMatrix (); //Match indentation in beginCamera.
}
