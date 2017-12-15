#pragma once
#include <cmath>
#include <string>
#include <cassert>
#include "Vec2.h"

// utility type to represent direction in the maze
// feel free to use this, but it is not necessary
// unless you are using GetRobotDirection from the
// DebugControls
class Direction
{
public:
	enum class Type
	{
		Up,
		Down,
		Left,
		Right,
		Count
	};
public:
	explicit Direction( const Vei2& dv )
		:
		Direction( dv.x,dv.y )
	{}
	Direction( Type t )
	{
		switch( t )
		{
		case Type::Up:
			*this = Up();
			break;
		case Type::Down:
			*this = Down();
			break;
		case Type::Left:
			*this = Left();
			break;
		case Type::Right:
			*this = Right();
			break;
		default:
			assert( "Bad direction type" && false );
		}
	}
	static Direction Up()
	{
		return{ 0,-1 };
	}
	static Direction Down()
	{
		return{ 0,1 };
	}
	static Direction Left()
	{
		return{ -1,0 };
	}
	static Direction Right()
	{
		return{ 1,0 };
	}
	Direction& RotateClockwise()
	{
		const auto old = dir;
		dir.x = -old.y;
		dir.y = old.x;
		return *this;
	}
	Direction& RotateCounterClockwise()
	{
		const auto old = dir;
		dir.x = old.y;
		dir.y = -old.x;
		return *this;
	}
	Direction GetRotatedClockwise() const
	{
		return Direction( *this ).RotateClockwise();
	}
	Direction GetRotatedCounterClockwise() const
	{
		return Direction( *this ).RotateCounterClockwise();
	}
	operator Vei2() const
	{
		return dir;
	}
	bool operator==( const Direction& rhs ) const
	{
		return dir == rhs.dir;
	}
	bool operator==( const Type& rhs ) const
	{
		return GetType() == rhs;
	}
	Type GetType() const
	{
		if( dir.x == 0 )
		{
			if( dir.y > 0 )
			{
				return Type::Down;
			}
			else
			{
				return Type::Up;
			}
		}
		else
		{
			if( dir.x > 0 )
			{
				return Type::Right;
			}
			else
			{
				return Type::Left;
			}
		}
	}
	int GetIndex() const
	{
		return (int)GetType();
	}
	std::string GetName() const
	{
		if( dir.x == 0 )
		{
			if( dir.y > 0 )
			{
				return "down";
			}
			else
			{
				return "up";
			}
		}
		else
		{
			if( dir.x > 0 )
			{
				return "right";
			}
			else
			{
				return "left";
			}
		}
	}
private:
	Direction( int x,int y )
		:
		dir( x,y )
	{
		assert( IsValid() );
	}
	bool IsValid() const
	{
		return std::abs( dir.x ) + std::abs( dir.y ) == 1;
	}
private:
	Vei2 dir;
};