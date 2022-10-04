/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute raylib_compile_execute script
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   This example has been created using raylib 1.0 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2014 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include <iostream>
using namespace std;

#include "raylib.h"
#include <raymath.h>
#include "rlgl.h"
#include <math.h>
#include <float.h>
#include <vector>

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            100
#endif

#define EPSILON 1.e-6f

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

struct Polar {
	float rho;
	float theta;
};

struct Cartesian {
	float x;
	float y;
	float z;
};

struct Cylindrical {
	float rho;
	float theta;
	float y;
};

struct Spherical {
	float rho;
	float theta;
	float phi;
};

//Structure Line définie par 1point et 1 vecteur directeur (Pt, Dir)
struct Line {
	Vector3 pt;
	Vector3 dir;
};

//Structure Segment définie par 2 points (Pt1, Pt2)
struct Segment {
	Vector3 pt1;
	Vector3 pt2;
};

//Structure Triangle définie par 3 points de l'espace
struct Triangle {
	Vector3 pt1;
	Vector3 pt2;
	Vector3 pt3;
};

//Structure Plane défini par un vecteur normal et une distance à l'origine
struct Plane {
	Vector3 n;
	float d;
};

//Structure referenceFrame
struct ReferenceFrame {
	Vector3 origin;
	Vector3 i, j, k;
	Quaternion q;
	ReferenceFrame()
	{
		origin = { 0,0,0 };
		i = { 1,0,0 };
		j = { 0,1,0 };
		k = { 0,0,1 };
		q = QuaternionIdentity();
	}
	ReferenceFrame(Vector3 origin, Quaternion q)
	{
		this->q = q;
		this->origin = origin;
		i = Vector3RotateByQuaternion({ 1,0,0 }, q);
		j = Vector3RotateByQuaternion({ 0,1,0 }, q);
		k = Vector3RotateByQuaternion({ 0,0,1 }, q);
	}
	void Translate(Vector3 vect)
	{
		this->origin = Vector3Add(this->origin, vect);
	}
	void RotateByQuaternion(Quaternion qRot)
	{
		q = QuaternionMultiply(qRot, q);
		i = Vector3RotateByQuaternion({ 1,0,0 }, q);
		j = Vector3RotateByQuaternion({ 0,1,0 }, q);
		k = Vector3RotateByQuaternion({ 0,0,1 }, q);
	}
};

//Structure Disk
struct Disk {
	ReferenceFrame ref;
	float radius;
};

//Structure Box 
struct Box {
	ReferenceFrame ref;
	Vector3 extents;
};

//Structure Sphere 
struct Sphere {
	ReferenceFrame ref;
	float radius;
};

//Structure Quad 
struct Quad {
	ReferenceFrame ref;
	Vector3 extents;

};

//Structure Infinite
struct InfiniteCylider {
	ReferenceFrame ref;
	float radius;
};

//Structure Cylinder
struct Cylinder {
	ReferenceFrame ref;
	float halfHeight;
	float radius;
};

//Méthodes de conversion 
Polar CartesianToPolar(Vector2 cart, bool keepThetaPositive = true) {
	Polar polar = { Vector2Length(cart),atan2f(cart.y,cart.x) };
	if (keepThetaPositive && polar.theta < 0)
		polar.theta += 2 * PI;
	return polar;
}

Vector2 PolarToCartesian(Polar polar) {
	return Vector2Scale({ cosf(polar.theta),sinf(polar.theta) }, polar.rho);
}

Vector3 CylindricalToCartesian(Cylindrical cylindrical) {
	Vector3 cart;

	cart.x = cylindrical.rho * sin(cylindrical.theta);
	cart.y = cylindrical.y;
	cart.z = cylindrical.rho * cos(cylindrical.theta);

	return cart;
}

Cylindrical CartesianToCylindrical(Vector3 cart) {        //(x, y, z) to (rho, theta, y)
	Cylindrical cylindrical;

	cylindrical.y = cart.z;

	cylindrical.rho = sqrtf(powf(cart.x, 2) + powf(cart.y, 2));       //powf = fonction pour mettre au carré

	if (cylindrical.rho < EPSILON) {
		cylindrical.theta = 0;
	}
	else {
		cylindrical.theta = asin(cart.x / cylindrical.rho);
		if (cart.z < 0) {
			cylindrical.theta = PI - cylindrical.theta;
		}
	}
	return cylindrical;
}

Vector3 SphericalToCartesian(Spherical spherical) {
	Vector3 cart;

	cart.x = spherical.rho * sin(spherical.phi) * sin(spherical.theta);
	cart.y = spherical.rho * cos(spherical.phi);
	cart.z = spherical.rho * sin(spherical.phi) * cos(spherical.theta);

	return cart;
}


Spherical CartesianToSpherical(Vector3 cart) {
	Spherical spherical;

	spherical.rho = sqrt(pow(cart.x, 2) + pow(cart.y, 2) + pow(cart.z, 2));

	if (spherical.rho < EPSILON) {
		spherical.theta = 0;
		spherical.phi = 0;
	}
	else {
		spherical.phi = acos(cart.y / spherical.rho);

		if ((spherical.phi < EPSILON) || (spherical.phi > PI - EPSILON)) {
			spherical.theta = 0;
		}
		else {
			spherical.theta = asin(cart.x / (spherical.rho * sin(spherical.phi)));
		}
		if (cart.z < 0) {
			spherical.theta = PI - spherical.theta;
		}
	}

	return spherical;
}
void MyUpdateOrbitalCamera(Camera* camera)
{
	static Spherical sphPos = { 10,PI / 4.f,PI / 4.f }; // ici la position de départ de la caméra est rho = 10 m, theta = 45°, phi = 45°
	Spherical sphSpeed = { 2.0f,0.04f,0.04f }; // 2 m/incrément de molette et 0.04 radians / pixel

	float rhoMin = 4; // 4 m
	float rhoMax = 40; // 40 m

	Vector2 mousePos = GetMousePosition();

	static Vector2 prevMousePos = { 0,0 };

	Vector2 mouseVect;

	Spherical sphDelta;

	mousePos = GetMousePosition();

	mouseVect = Vector2Subtract(mousePos,prevMousePos);

	prevMousePos = mousePos;
	
	sphDelta.rho = GetMouseWheelMove() * sphSpeed.rho; // -10 a 10
	sphDelta.phi = mouseVect.y * sphSpeed.phi;
	sphDelta.theta = mouseVect.x * sphSpeed.theta;

	sphPos.rho += sphDelta.rho;
	sphPos.rho = Clamp(sphPos.rho, rhoMin, rhoMax);

	if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {

		sphPos.phi = Clamp(sphPos.phi + sphDelta.phi , 1 * DEG2RAD, 179 * DEG2RAD) ;
		sphPos.theta += sphDelta.theta;

		cout << "Roulette = " << sphPos.rho << "\n";
		cout << "y = " << sphPos.phi << "\n";
		cout << "x = " << sphPos.theta << "\n";
		cout << "Mouse x = " << mousePos.x << "\n";
		cout << "Mouse y = " << mousePos.y << "\n";
		
		
	}

	camera->position = SphericalToCartesian(sphPos);

}

int main(int argc, char* argv[])
{
	// Initialization
	//--------------------------------------------------------------------------------------
	float screenSizeCoef = .9f;
	const int screenWidth = 1920 * screenSizeCoef;
	const int screenHeight = 1080 * screenSizeCoef;

	InitWindow(screenWidth, screenHeight, "ESIEE - E3FI - 2022 - 2023 -Maths 3D");

	SetTargetFPS(60);

	//CAMERA
	Vector3 cameraPos = { 8.0f, 15.0f, 14.0f }; // position initiale de la caméra
	Camera camera = { 0 };						// la caméra
	camera.position = cameraPos;				// position de la caméra
	camera.target = { 0.0f, 0.0f, 0.0f };		// la caméra regarde l’origine
	camera.up = { 0.0f, 1.0f, 0.0f };			// définit la “verticalité” de la caméra
	camera.fovy = 45.0f;						// field of view (ouverture angulaire verticale)
	camera.type = CAMERA_PERSPECTIVE;			// projection perspective
	SetCameraMode(camera, CAMERA_CUSTOM);		// Set an orbital camera mode // nous positionnons la caméra


	//--------------------------------------------------------------------------------------

	// Main game loop

	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// TODO: Update your variables here
		//----------------------------------------------------------------------------------

		float deltaTime = GetFrameTime();
		float time = (float)GetTime();

		MyUpdateOrbitalCamera(&camera);

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(RAYWHITE);

		BeginMode3D(camera);
		{
			//

			//3D REFERENTIAL
			DrawGrid(20, 1.0f);        // Draw a grid
			DrawLine3D({ 0 }, { 0,10,0 }, DARKGRAY);
			DrawSphere({ 10,0,0 }, .2f, RED);
			DrawSphere({ 0,10,0 }, .2f, GREEN);
			DrawSphere({ 0,0,10 }, .2f, BLUE);
		}
		EndMode3D();

		EndDrawing();
		//----------------------------------------------------------------------------------

	}

	// De-Initialization
	//--------------------------------------------------------------------------------------   
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	//Zone de test terminal
	/*Vector3 cart;
	cart.x = 10;
	cart.y = 40;
	cart.z = 50;

	Cylindrical cylindrical;

	cylindrical = CartesianToCylindrical(cart);

	cout << cylindrical.rho << "\n";
	cout << cylindrical.y << "\n";
	cout << cylindrical.theta << "\n";

	cart = CylindricalToCartesian(cylindrical);

	cout << cart.x << "\n";
	cout << cart.z << "\n";
	cout << cart.y << "\n";

	cout << "\n---------------------------------------------------\n";


	Spherical spherical;
	spherical = CartesianToSpherical(cart);

	cout << spherical.rho << "\n";
	cout << spherical.theta << "\n";
	cout << spherical.phi << "\n";

	cart = SphericalToCartesian(spherical);

	cout << cart.x << "\n";
	cout << cart.z << "\n";
	cout << cart.y << "\n";
	*/
	return 0;
}
