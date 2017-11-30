#pragma once

#include <cmath>

static constexpr double PI_D = 3.14159265358979323846;
static constexpr float PI = (float)PI_D;

// integer division with round up if remainer exists
inline int div_int_ceil( int lhs,int rhs )
{
	return (lhs + rhs - 1) / rhs;
}

template<typename T>
inline T clamp( T x,T min,T max )
{
	return std::min( max,std::max( min,x ) );
}