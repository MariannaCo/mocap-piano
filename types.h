/*

Common types

*/

#pragma once

typedef struct Point2f{
	Point2f(){ x = 0.0f; y = 0.0f; }
	Point2f( float _x, float _y ){ x = _x; y = _y; }
	float x;
	float y;
} Point2f;

#define DEG2RAD 3.14159/180.0