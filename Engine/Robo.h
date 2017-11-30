#pragma once

#include "TileMap.h"
#include <random>
#include <array>

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

class Robo
{
public:
	enum class Action
	{
		MoveForward,
		TurnRight,
		TurnLeft,
		Done
	};
public:
	Robo( const Vei2& pos = { 0,0 },const Direction& dir = Direction::Up() )
		:
		pos( pos ),
		dir( dir )
	{
		sprites.reserve( (int)Direction::Type::Count );
		for( int i = 0; i < (int)Direction::Type::Count; i++ )
		{
			sprites.emplace_back( "Images\\robo_" + 
				Direction( (Direction::Type)i ).GetName() + ".bmp" );
		}
		offset_to_center = { sprites.front().GetWidth() / 2,sprites.front().GetHeight() / 2 };
	}
	void MoveForward( const TileMap& map )
	{
		const auto target = pos + dir;
		assert( map.Contains( target ) );
		if( map.At( target ) != TileMap::TileType::Wall )
		{
			pos = target;
		}
	}
	void TurnRight()
	{
		dir.RotateClockwise();
	}
	void TurnLeft()
	{
		dir.RotateCounterClockwise();
	}
	void TakeAction( Action action,const TileMap& map )
	{
		switch( action )
		{
		case Action::MoveForward:
			MoveForward( map );
			break;
		case Action::TurnRight:
			TurnRight();
			break;
		case Action::TurnLeft:
			TurnLeft();
			break;
		case Action::Done:
			break;
		default:
			assert( "Bad action type in take action" && false );
		}
	}
	void Draw( Graphics& gfx,const Camera& cam,const Viewport& port,const TileMap& map ) const
	{
		const auto center_in_world = map.GetCenterAt( pos );
		const auto draw_pos = (Vei2)cam.GetTranslatedPoint( center_in_world ) - offset_to_center;
		const auto& surf = sprites[dir.GetIndex()];
		gfx.DrawSprite( draw_pos.x,draw_pos.y,surf.GetRect(),port.GetClipRect(),surf,
			SpriteEffect::Chroma{ Colors::Black }
		);
	}
	std::array<TileMap::TileType,3> GetView( const TileMap& map ) const
	{
		auto scan_pos = pos + dir + dir.GetRotatedCounterClockwise();
		const auto scan_delta = dir.GetRotatedClockwise();
		
		std::array<TileMap::TileType,3> view;
		for( int i = 0; i < 3; i++,scan_pos += scan_delta )
		{
			view[i] = map.At( scan_pos );
		}
		return view;
	}
	Vei2 GetPos() const
	{
		return pos;
	}
	Direction GetDirection() const
	{
		return dir;
	}
private:
	Vei2 pos;
	Direction dir;
	// graphical offset from top left of sprite to center
	Vei2 offset_to_center;
	// up down left right
	std::vector<Surface> sprites;
};