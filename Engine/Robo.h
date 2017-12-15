#pragma once

#include "TileMap.h"
#include "Direction.h"
#include <random>
#include <array>
#include <cassert>

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