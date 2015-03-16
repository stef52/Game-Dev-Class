
//*****************************************************************************************//
//                                   Transformations                                       //
//*****************************************************************************************//

//*****************************************************************************************//
//                                      Includes                                           //
//*****************************************************************************************//

#ifndef transformationsModule
#define transformationsModule 

//*****************************************************************************************//
//                           OpenGL Stack Manipulation Extensions                          //
//*****************************************************************************************//

inline void glGetMatrixf (GLenum whichMatrix, Transformation &matrix) {
	glGetFloatv (whichMatrix, (GLfloat *) &matrix);
}

inline void glLoadMatrixf (Transformation &matrix) {
	glLoadMatrixf ((const GLfloat *) &matrix);
}

inline void glMultMatrixf (Transformation &matrix) {
	glMultMatrixf ((const GLfloat *) &matrix);
}

inline void glPushMatrixf (Transformation &matrix) {
	glPushMatrix ();
	glLoadMatrixf (matrix);
}

inline void glPopMatrixf (GLenum whichMatrix, Transformation &matrix) {
	glGetMatrixf (whichMatrix, matrix);
	glPopMatrix ();
}

inline void glPushIdentity () {
	glPushMatrix ();
	glLoadIdentity ();
}

inline GLint currentMatrixStack () {
	GLint mode; glGetIntegerv (GL_MATRIX_MODE, &mode);
	return 
		mode == GL_MODELVIEW ? GL_MODELVIEW_MATRIX :
		mode == GL_PROJECTION ? GL_PROJECTION_MATRIX :
		GL_TEXTURE_MATRIX;
}

inline void glGetMatrixf (Transformation &matrix) {
	glGetFloatv (currentMatrixStack (), (float *) &matrix);
}

inline void glPopMatrixf (Transformation &matrix) {
	glGetMatrixf (matrix);
	glPopMatrix ();
}

//And one useful utility for tabbing...
inline char *indentation (long tabs) {
	static char *string = (char*)"\t\t\t\t\t\t\t\t\t\t"; //Exactly 10 tabs,,,
	return &string [10 - tabs];
}

//*****************************************************************************************//
//                              Transformation Implementation                              //
//*****************************************************************************************//

class Transformation {
public:
	float m11, m12, m13, m14; //avoid arrays and 
	float m21, m22, m23, m24; //subscripting for more
	float m31, m32, m33, m34; //efficient compiler-generated
	float m41, m42, m43, m44; //code.

	inline Transformation () {setToIdentity ();}
	inline Transformation (float a11, float a12, float a13, float a14, 
		float a21, float a22, float a23, float a24, 
		float a31, float a32, float a33, float a34, 
		float a41, float a42, float a43, float a44) {
		m11 = a11; m12 = a12; m13 = a13; m14 = a14;
		m21 = a21; m22 = a22; m23 = a23; m24 = a24;
		m31 = a31; m32 = a32; m33 = a33; m34 = a34;
		m41 = a41; m42 = a42; m43 = a43; m44 = a44;
	}

	inline ~Transformation () {};

	inline static Transformation lookAtForObject (const Point &from, const Vector &direction, const Vector &up, const Vector &alternateUp) {
		//This version returns the inverse of gluLookAt without modifying the stack. Useful when you want an OBJECT to look at something RATHER THAN THE CAMERA.
		//Note: up is an approximate yAxis. If it is erroneously directed into the z-direction, use alternateUp instead.
		Vector zAxis = -direction.normalized ();
		Vector xAxis = (up.cross (zAxis)).normalized ();
		if (xAxis.isZero ()) xAxis = (alternateUp.cross (zAxis)).normalized ();
		Vector yAxis = zAxis.cross (xAxis);
		Transformation transformation; transformation.rotateToAxes (xAxis, yAxis, zAxis, from); return transformation;
	}

	inline void set (float a11, float a12, float a13, float a14, float a21, float a22, float a23, float a24, 
		float a31, float a32, float a33, float a34, float a41, float a42, float a43, float a44) {
		m11 = a11; m12 = a12; m13 = a13; m14 = a14;
		m21 = a21; m22 = a22; m23 = a23; m24 = a24;
		m31 = a31; m32 = a32; m33 = a33; m34 = a34;
		m41 = a41; m42 = a42; m43 = a43; m44 = a44;
	}

	inline void setToIdentity () {
		m11 = 1.0; m12 = 0.0; m13 = 0.0; m14 = 0.0;
		m21 = 0.0; m22 = 1.0; m23 = 0.0; m24 = 0.0;
		m31 = 0.0; m32 = 0.0; m33 = 1.0; m34 = 0.0;
		m41 = 0.0; m42 = 0.0; m43 = 0.0; m44 = 1.0;
	}
	inline void rotateToAxes (const Vector &xAxis, const Vector &yAxis, const Vector &zAxis, const Point &origin) {
		set (xAxis.x, xAxis.y, xAxis.z, 0.0,
			yAxis.x, yAxis.y, yAxis.z, 0.0,
			zAxis.x, zAxis.y, zAxis.z, 0.0,
			origin.x, origin.y, origin.z, 1.0);
	}
	inline void rotateToAxes (const Vector &xAxis, const Vector &yAxis, const Vector &zAxis) {
		set (xAxis.x, xAxis.y, xAxis.z, 0.0,
			yAxis.x, yAxis.y, yAxis.z, 0.0,
			zAxis.x, zAxis.y, zAxis.z, 0.0,
			0.0, 0.0, 0.0, 1.0);
	}
	inline void transpose () {
		float saveM21 = m21; m21 = m12; m12 = saveM21;
		float saveM31 = m31; m31 = m13; m13 = saveM31;
		float saveM32 = m32; m32 = m23; m23 = saveM32;
		float saveM41 = m41; m41 = m14; m14 = saveM41;
		float saveM42 = m42; m42 = m24; m24 = saveM42;
		float saveM43 = m43; m43 = m34; m34 = saveM43;
	}

	//Given a matrix M and a transformation T, we will say that we
	//	pre-multiply M if we compute T * M
	//  post-multiply M if we compute M * T

	inline void preTranslateBy (const Vector &translation) {//Translation * matrix;
		m41 += translation.x*m11 + translation.y*m21 + translation.z*m31;
		m42 += translation.x*m12 + translation.y*m22 + translation.z*m32;
		m43 += translation.x*m13 + translation.y*m23 + translation.z*m33;
		m44 += translation.x*m14 + translation.y*m24 + translation.z*m34;
	}
	inline void postTranslateBy (const Vector &translation) {//matrix * Translation.
		set (
			m11 + m14*translation.x, m12 + m14*translation.y, m13 + m14*translation.z, m14,
			m21 + m24*translation.x, m22 + m24*translation.y, m23 + m24*translation.z, m24,
			m31 + m34*translation.x, m32 + m34*translation.y, m33 + m34*translation.z, m34,
			m41 + m44*translation.x, m42 + m44*translation.y, m43 + m44*translation.z, m44
		); 
	}
	inline void translateBy (Vector &translation) {preTranslateBy (translation);} //translate means pre-translate

	inline void translateTo (Point &position) {postTranslateBy (position - this->position ());}

	static inline Transformation rotationAboutUnitAxis (const float degrees, const Vector &axis) {
		float radians = asRadians (degrees);
		float sine = sin (radians); float cosine = cos (radians);
		float oneMinusCosine = 1.0 - cosine;
		
		Vector vX = axis * (axis.x * oneMinusCosine);
		Vector vY = axis * (axis.y * oneMinusCosine);
		Vector vZ = axis * (axis.z * oneMinusCosine);
		Vector vS = axis * sine;
		
		return Transformation (
			vX.x + cosine, vX.y + vS.z, vX.z - vS.y, 0.0,
			vY.x - vS.z, vY.y + cosine, vY.z + vS.x, 0.0,
			vZ.x + vS.y, vZ.y - vS.x, vZ.z + cosine, 0.0,
			0.0, 0.0, 0.0, 1.0
		);
	}
	inline void preRotateAroundUnitAxisBy (const float degrees, const Vector &axis) {
		this->multiply (rotationAboutUnitAxis (degrees, axis), *this);
	}
	inline void postRotateAroundUnitAxisBy (const float degrees, const Vector &axis) {
		this->multiply (*this, rotationAboutUnitAxis (degrees, axis));
	}

	static inline Transformation rotationAboutAxis (const Vector &rawAxis, const float degrees) {
		return rotationAboutUnitAxis (degrees, rawAxis.normalized ());
	}
	inline void preRotateBy (const float degrees, const Vector &axis) {
		preRotateAroundUnitAxisBy (degrees, axis.normalized ());
	}

	inline void postRotateBy (const float degrees, const Vector &axis) {
		postRotateAroundUnitAxisBy (degrees, axis.normalized ());
	}
	inline void rotateBy (float degrees, Vector &axis) {preRotateBy (degrees, axis);} //rotateBy means pre-rotateBy
	
	inline void preRotateBy (const Vector &rotation) {//rotation * matrix;
		//This rotation denotes [degreesAroundXAxis, degreesAroundYAxis, degreesAroundZAxis].
		//Actually, Rx*Ry*Rz * matrix.
		if (rotation.z != 0.0) preRotateAroundUnitAxisBy (rotation.z, Vector (0.0, 0.0, 1.0));
		if (rotation.y != 0.0) preRotateAroundUnitAxisBy (rotation.y, Vector (0.0, 1.0, 0.0));
		if (rotation.x != 0.0) preRotateAroundUnitAxisBy (rotation.x, Vector (1.0, 0.0, 0.0));
	}
	inline void postRotateBy (const Vector &rotation) {//matrix * rotation
		//This rotation denotes [degreesAroundXAxis, degreesAroundYAxis, degreesAroundZAxis].
		//matrix * Rx*Ry*Rz
		if (rotation.x != 0.0) postRotateAroundUnitAxisBy (rotation.x, Vector (1.0, 0.0, 0.0));
		if (rotation.y != 0.0) postRotateAroundUnitAxisBy (rotation.y, Vector (0.0, 1.0, 0.0));
		if (rotation.z != 0.0) postRotateAroundUnitAxisBy (rotation.z, Vector (0.0, 0.0, 1.0));
	}
	inline void rotateBy (Vector &rotation) {preRotateBy (rotation);} //rotateBy means pre-rotateBy

	inline void preScaleBy (const Vector &scale) {
		m11 *= scale.x; m12 *= scale.x; m13 *= scale.x; m14 *= scale.x;
		m21 *= scale.y; m22 *= scale.y; m23 *= scale.y; m24 *= scale.y;
		m31 *= scale.z; m32 *= scale.z; m33 *= scale.z; m34 *= scale.z;
		//Fourth row is multiplied by 1 throughout.
	}
	inline void postScaleBy (const Vector &scale) {//matrix * scale
		m11 *= scale.x; m12 *= scale.y; m13 *= scale.z; 
		m21 *= scale.x; m22 *= scale.y; m23 *= scale.z;
		m31 *= scale.x; m32 *= scale.y; m33 *= scale.z;	
		m41 *= scale.x; m42 *= scale.y; m43 *= scale.z;
		//Fourth column is multiplied by 1 throughout.
	}
	inline void scaleBy (Vector &scale) {preScaleBy (scale);} //scaleBy means pre-scaleBy

	void multiply (const Transformation &a, const Transformation &b) {
		//this = a * b
		float r11 = a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31 + a.m14 * b.m41;
		float r12 = a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32 + a.m14 * b.m42;
		float r13 = a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33 + a.m14 * b.m43;
		float r14 = a.m11 * b.m14 + a.m12 * b.m24 + a.m13 * b.m34 + a.m14 * b.m44;

		float r21 = a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31 + a.m24 * b.m41;
		float r22 = a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32 + a.m24 * b.m42;
		float r23 = a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33 + a.m24 * b.m43;
		float r24 = a.m21 * b.m14 + a.m22 * b.m24 + a.m23 * b.m34 + a.m24 * b.m44;

		float r31 = a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31 + a.m34 * b.m41;
		float r32 = a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32 + a.m34 * b.m42;
		float r33 = a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33 + a.m34 * b.m43;
		float r34 = a.m31 * b.m14 + a.m32 * b.m24 + a.m33 * b.m34 + a.m34 * b.m44;

		float r41 = a.m41 * b.m11 + a.m42 * b.m21 + a.m43 * b.m31 + a.m44 * b.m41;
		float r42 = a.m41 * b.m12 + a.m42 * b.m22 + a.m43 * b.m32 + a.m44 * b.m42;
		float r43 = a.m41 * b.m13 + a.m42 * b.m23 + a.m43 * b.m33 + a.m44 * b.m43;
		float r44 = a.m41 * b.m14 + a.m42 * b.m24 + a.m43 * b.m34 + a.m44 * b.m44;
		set (r11,r12,r13,r14, r21,r22,r23,r24, r31,r32,r33,r34, r41,r42,r43,r44);
	}

	void multiply (const Transformation &a) {
		//this = this * a
		float r11 = m11 * a.m11 + m12 * a.m21 + m13 * a.m31 + m14 * a.m41;
		float r12 = m11 * a.m12 + m12 * a.m22 + m13 * a.m32 + m14 * a.m42;
		float r13 = m11 * a.m13 + m12 * a.m23 + m13 * a.m33 + m14 * a.m43;
		float r14 = m11 * a.m14 + m12 * a.m24 + m13 * a.m34 + m14 * a.m44;

		float r21 = m21 * a.m11 + m22 * a.m21 + m23 * a.m31 + m24 * a.m41;
		float r22 = m21 * a.m12 + m22 * a.m22 + m23 * a.m32 + m24 * a.m42;
		float r23 = m21 * a.m13 + m22 * a.m23 + m23 * a.m33 + m24 * a.m43;
		float r24 = m21 * a.m14 + m22 * a.m24 + m23 * a.m34 + m24 * a.m44;

		float r31 = m31 * a.m11 + m32 * a.m21 + m33 * a.m31 + m34 * a.m41;
		float r32 = m31 * a.m12 + m32 * a.m22 + m33 * a.m32 + m34 * a.m42;
		float r33 = m31 * a.m13 + m32 * a.m23 + m33 * a.m33 + m34 * a.m43;
		float r34 = m31 * a.m14 + m32 * a.m24 + m33 * a.m34 + m34 * a.m44;

		float r41 = m41 * a.m11 + m42 * a.m21 + m43 * a.m31 + m44 * a.m41;
		float r42 = m41 * a.m12 + m42 * a.m22 + m43 * a.m32 + m44 * a.m42;
		float r43 = m41 * a.m13 + m42 * a.m23 + m43 * a.m33 + m44 * a.m43;
		float r44 = m41 * a.m14 + m42 * a.m24 + m43 * a.m34 + m44 * a.m44;
		set (r11,r12,r13,r14, r21,r22,r23,r24, r31,r32,r33,r34, r41,r42,r43,r44);
	}
	
	inline Transformation operator * (const Transformation &transformation) const {
		Transformation result; result.multiply (*this, transformation);
		return result;
	}

	Transformation scaleFreeInverse () const {
		//This assumes a non-perspective matrix with no scaling. If M=RT, M-1 = T-1*R-1...
		Transformation result;
		//Note that rightmost column of R (also T) is [0,0,0,1] because it's a non-perspective matrix.

		//Part 1: R-1 = transpose of R.
		result.m11 = m11; result.m12 = m21; result.m13 = m31; result.m14 = 0.0;
		result.m21 = m12; result.m22 = m22; result.m23 = m32; result.m24 = 0.0;
		result.m31 = m13; result.m32 = m23; result.m33 = m33; result.m34 = 0.0;
		
		//Part 2: T-1 is -row4 of T, *R-1 is times columns of R-1 or rows of R;
		//So T-1*R-1 is "-row4 of T" * "rows of R". 
		//So  result.m41 = "-row4 of T" dot "row 1 of R". 
		//and result.m42 = "-row4 of T" dot "row 2 of R". 
		//and result.m43 = "-row4 of T" dot "row 3 of R". 
		//and result.m44 = 1.0. 

		result.m41 = - (m41 * m11 + m42 * m12 + m43 * m13);// + m44 * m14);
		result.m42 = - (m41 * m21 + m42 * m22 + m43 * m23);// + m44 * m24);
		result.m43 = - (m41 * m31 + m42 * m32 + m43 * m33);// + m44 * m34);
		result.m44 = 1.0;

		return result;
	}

	Point position () const {return Point (m41, m42, m43);}

	bool isEqual (Transformation &transformation, float epsilon = ::epsilon ()) {
		if (absolute (m11 - transformation.m11) > epsilon) return false;
		if (absolute (m12 - transformation.m12) > epsilon) return false;
		if (absolute (m13 - transformation.m13) > epsilon) return false;
		if (absolute (m14 - transformation.m14) > epsilon) return false;
		
		if (absolute (m21 - transformation.m21) > epsilon) return false;
		if (absolute (m22 - transformation.m22) > epsilon) return false;
		if (absolute (m23 - transformation.m23) > epsilon) return false;
		if (absolute (m24 - transformation.m24) > epsilon) return false;
		
		if (absolute (m31 - transformation.m31) > epsilon) return false;
		if (absolute (m32 - transformation.m32) > epsilon) return false;
		if (absolute (m33 - transformation.m33) > epsilon) return false;
		if (absolute (m34 - transformation.m34) > epsilon) return false;
		
		if (absolute (m41 - transformation.m41) > epsilon) return false;
		if (absolute (m42 - transformation.m42) > epsilon) return false;
		if (absolute (m43 - transformation.m43) > epsilon) return false;
		if (absolute (m44 - transformation.m44) > epsilon) return false;
		return true;
	}
	void log (long tabs = 0) {
		char *tabbing = indentation (tabs);
		::log ("\n%s%3.3f, %3.3f, %3.3f, %3.3f" "\n%s%3.3f, %3.3f, %3.3f, %3.3f" 
			"\n%s%3.3f, %3.3f, %3.3f, %3.3f" "\n%s%3.3f, %3.3f, %3.3f, %3.3f", 
			tabbing, m11, m12, m13, m14, tabbing, m21, m22, m23, m24, 
			tabbing, m31, m32, m33, m34, tabbing, m41, m42, m43, m44);
	}
};

declareCollection (Transformation);

//*****************************************************************************************//
//                      Dual Transformations (Have Their Own Inverses)                     //
//*****************************************************************************************//

class DualTransformation : public Transformation {
public:
	//Has both a normal transformation and an inverse; maintains the identity "this * this->inverse = I".
	Transformation inverse;

	Transformation &normal () {return *((Transformation *) this);}

	//Important fact: if A-1 denotes the inverse of A, then (A*B)-1 = B-1*A-1 (the order is reversed).
	//Proof: If B-1*A-1 is the inverse of A*B, their product will turn out to be the identity.
	//Is it? B-1*A-1 * A*B = B-1 * B (since A-1 * A = I) = I
	//Given a matrix M and a transformation T, we will say that we
	//	pre-multiply M if we compute T * M
	//  post-multiply M if we compute M * T
	//Important fact: when we premultiply M by T, we must post-multiply the inverse by T-1.
	//Proof: (T*M)-1 = M-1*T-1; i.e., the inverse must be post-multiplied by the inverse of T.
	
	inline DualTransformation () {setToIdentity ();}
	inline DualTransformation (float a11, float a12, float a13, float a14, 
		float a21, float a22, float a23, float a24, 
		float a31, float a32, float a33, float a34, 
		float a41, float a42, float a43, float a44) {
		halt ("Illegal since inverse elements too expensive to compute.");
	}
	inline ~DualTransformation () {};

	inline void set (float a11, float a12, float a13, float a14, float a21, float a22, float a23, float a24, 
		float a31, float a32, float a33, float a34, float a41, float a42, float a43, float a44) {
		halt ("Illegal since inverse elements too expensive to compute.");
	}

	inline void setToIdentity () {
		Transformation::setToIdentity (); inverse.setToIdentity ();
	}

	inline void preTranslateBy (const Vector &translation) {
		Transformation::preTranslateBy (translation);
		inverse.postTranslateBy (-translation);
	}
	inline void postTranslateBy (const Vector &translation) {
		Transformation::postTranslateBy (translation);
		inverse.preTranslateBy (-translation);
	}
	inline void translateBy (Vector &translation) {preTranslateBy (translation);}
	inline void translateTo (Point &position) {postTranslateBy (position - this->position ());}


	inline void preRotateBy (const Vector &rotation) {
		Transformation::preRotateBy (rotation);
		inverse.postRotateBy (-rotation);
	}
	inline void postRotateBy (const Vector &rotation) {
		Transformation::postRotateBy (rotation);
		inverse.preRotateBy (-rotation);
	}
	inline void rotateBy (Vector &rotation) {preRotateBy (rotation);}


	inline void preRotateBy (float degrees, Vector &axis) {
		Transformation::preRotateBy (degrees, axis);
		inverse.postRotateBy (-degrees, axis);
	}
	inline void postRotateBy (float degrees, Vector &axis) {
		Transformation::postRotateBy (degrees, axis);
		inverse.preRotateBy (-degrees, axis);
	}
	inline void rotateBy (float degrees, Vector &axis) {preRotateBy (degrees, axis);}

	void multiply (Transformation &a, Transformation &b) {
		halt ("Illegal since inverse elements too expensive to compute.");
	}

	inline void multiply (DualTransformation &a, DualTransformation &b) {
		this->Transformation::multiply (a.normal (), b.normal ());
		this->inverse.Transformation::multiply (b.inverse, a.inverse);
	}

	void multiply (Transformation &a) {
		halt ("Illegal since inverse elements too expensive to compute.");
	}

	inline void multiply (DualTransformation &a) {
		this->Transformation::multiply (a.normal ());
		this->inverse.Transformation::multiply (a.inverse, this->inverse);
	}
	
	Point position () {return this->normal ().position ();}

	void log (long tabs = 0) {
		char *tabbing = indentation (tabs);
		::log ("\n%sNormal:", tabbing); normal ().log (tabs+1); 
		::log ("\n%sInverse:", tabbing); inverse.log (tabs+1);
	}
};

#endif //transformationsModule