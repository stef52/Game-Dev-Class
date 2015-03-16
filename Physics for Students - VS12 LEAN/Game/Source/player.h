//*****************************************************************************************//
//                                       Player                                            //
//*****************************************************************************************//

#ifndef playerModule
#define playerModule 

class Player {
public:		

	Player ();
	~Player ();

	DualTransformation playerMatrix; Point rotation; DualTransformation delta;

	//WILF PhysX...
	PxRigidDynamic *playerSphere;

	static void setup ();
	static void wrapup ();

	Point asFarAsPossibleGo (Point &from, Point &to, double EXTRA) const;
	void tick ();
	void draw ();

	void beginCamera ();
	void endCamera ();
	
	void moveBy (Point &translation);
	void rotateBy (Point &rotation);

	void reset (Point &worldPosition);

	Point position () {return playerMatrix.position ();}
	void position (Point &point) {playerMatrix.translateBy (point - position ());}

	Vector heading () const {return Vector (0.0, 0.0, -1.0).vectorTransformBy (((DualTransformation &) playerMatrix).normal ());}
	Vector raisedHeading () const {return (Vector (0.0, 0.5, -1.0).vectorTransformBy (((DualTransformation &) playerMatrix).normal ())).normalized ();} //Half way between ahead and 45 degrees...;
};

class Player;

extern Player *player;

#endif //playerModule