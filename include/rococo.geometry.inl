#ifndef ROCOCO_GEOMETRY_INL
#define ROCOCO_GEOMETRY_INL

#ifndef Rococo_TYPES_H
# error "include <rococo.types.h> before including this file"
#endif

#include <intrin.h>
#include <math.h>



namespace Rococo
{
	inline float PI() { return 3.14159265358979323846f; }
	inline Radians ToRadians(Degrees degrees) { return Radians { degrees.quantity * PI() / 180.0f }; }
	inline float Sin(Radians radians) { return sinf(radians.quantity); }
	inline float Cos(Radians radians) { return cosf(radians.quantity); }
	inline float Sin(Degrees degrees) { return Sin(ToRadians(degrees)); }
	inline float Cos(Degrees degrees) { return Cos(ToRadians(degrees)); }
	
}

#endif