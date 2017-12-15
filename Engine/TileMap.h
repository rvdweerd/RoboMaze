#pragma once

#include "Surface.h"
#include "Graphics.h"
#include "SpriteEffect.h"
#include "Direction.h"
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <random>
#include <unordered_map>

class Viewport
{
public:
	Viewport( const RectI& rect )
		:
		rect( rect )
	{}
	const RectI& GetClipRect() const
	{
		return rect;
	}
private:
	RectI rect;
};

class Camera
{
public:
	Camera( const Viewport& port,const RectI& bounds )
		:
		port( port ),
		bounds( bounds ),
		pos( { 0,0 } )
	{}
	RectD GetViewingRect() const
	{
		return{ pos,pos + (Ved2)port.GetClipRect().GetDimensions() };
	}
	Ved2 GetCenter() const
	{
		return GetViewingRect().GetCenter();
	}
	void PanBy( const Vei2& delta )
	{
		PanBy( (Ved2)delta );
	}
	void PanBy( const Ved2& delta )
	{
		// pan camera
		pos += delta;
		// adjust to camera boundaries
		const auto vr = GetViewingRect();
		if( vr.left < bounds.left )
		{
			pos.x = bounds.left;
		}
		else if( vr.right > bounds.right )
		{
			pos.x = bounds.right - (double)port.GetClipRect().GetWidth();
		}
		if( vr.top < bounds.top )
		{
			pos.y = bounds.top;
		}
		else if( vr.bottom > bounds.bottom )
		{
			pos.y = bounds.bottom - (double)port.GetClipRect().GetHeight();
		}
	}
	void CenterAt( const Ved2& pos )
	{
		PanBy( GetCenter() - pos );
	}
	void CenterAt( const Vei2& pos )
	{
		CenterAt( (Ved2)pos );
	}
	Ved2 GetTotalTranslation() const
	{
		return (Ved2)port.GetClipRect().TopLeft() - pos;
	}
	Ved2 GetTranslatedPoint( const Vei2& in ) const
	{
		return (Ved2)in + GetTotalTranslation();
	}
private:
	Ved2 pos;
	RectD bounds;
	const Viewport& port;
};

class ScrollRegion
{
public:
	ScrollRegion( Camera& cam )
		:
		cam( cam )
	{}
	void Grab( const Vei2& base )
	{
		isGrabbed = true;
		grab_base = base;
	}
	void Release()
	{
		isGrabbed = false;
	}
	void MoveTo( const Vei2& pos )
	{
		if( IsGrabbed() )
		{
			cam.PanBy( grab_base - pos );
			grab_base = pos;
		}
	}
	bool IsGrabbed() const
	{
		return isGrabbed;
	}
private:
	Vei2 grab_base;
	bool isGrabbed = false;
	Camera& cam;
};

template<typename T>
class Grid : public std::vector<T>
{
public:
	Grid() = default;
	Grid( int width,int height )
		:
		vector( width*height ),
		width( width ),
		height( height )
	{}
	Grid( int width,int height,const T& val )
		:
		vector( width*height,val ),
		width( width ),
		height( height )
	{}
	int GetWidth() const
	{
		return width;
	}
	int GetHeight() const
	{
		return height;
	}
	const T& At( const Vei2& pos ) const
	{
		return At( pos.x,pos.y );
	}
	T& At( const Vei2& pos )
	{
		return At( pos.x,pos.y );
	}
	const T& At( int x,int y ) const
	{
		return const_cast<Grid*>(this)->At( x,y );
	}
	T& At( int x,int y )
	{
		assert( Contains( { x,y } ) );
		return (*this)[x + y * width];
	}
	bool Contains( const Vei2& pos ) const
	{
		return Contains( pos.x,pos.y );
	}
	bool Contains( int x,int y ) const
	{
		return
			x >= 0 &&
			x < width &&
			y >= 0 &&
			y < height;
	}
	// F: void(cVei2&)
	template<typename F>
	void VisitNeighbors( const Vei2& pos,F f )
	{
		auto dir = Direction::Up();
		for( int i = 0; i < 4; i++,dir.RotateClockwise() )
		{
			const auto p = pos + dir;
			if( Contains( p ) )
			{
				f( p );
			}
		}
	}
	// C: bool(cT&)
	template<typename C>
	int CountNeighbors( const Vei2& pos,C comp ) const
	{
		int count = 0;
		VisitNeighbors( pos,
			[this,comp]( const Vei2& pos )
			{
				if( comp( At( pos ) ) )
				{
					count++;
				}
			}
		);
		return count;
	}
private:
	int width = -1;
	int height = -1;
};

class TileMap
{
public:
	enum class TileType
	{
		Wall,
		Floor,
		Goal,
		Invalid
	};
private:
	struct Tile
	{
		Tile() = default;
		Tile( TileType type )
			:
			type( type ),
			c( 0,0,0,0 )
		{}
		TileType type = TileType::Invalid;
		Color c;
	};
public:
	TileMap( const std::string& filename,const class Direction& sd );
	// procedurally generated map
	TileMap( const class Config& config );
	const TileType& At( const Vei2& pos ) const
	{
		return tiles.At( pos ).type;
	}
	TileType& At( const Vei2& pos )
	{
		return tiles.At( pos ).type;
	}
	void Draw( Graphics& gfx,const Camera& cam,const Viewport& port ) const
	{
		// establish grid looping extents
		const RectI viewRect = (RectI)cam.GetViewingRect();
		const int x_start = viewRect.left / tileWidth;
		const int y_start = viewRect.top / tileHeight;
		const int x_end = div_int_ceil( viewRect.right,tileWidth );
		const int y_end = div_int_ceil( viewRect.bottom,tileHeight );

		// establish translation and clip rect
		const RectI clipRect = port.GetClipRect();
		const Vei2 translation = clipRect.TopLeft() - viewRect.TopLeft();

		// draw tiles
		for( int y = y_start; y < y_end; y++ )
		{
			for( int x = x_start; x < x_end; x++ )
			{
				int screen_x = x * tileWidth + translation.x;
				int screen_y = y * tileHeight + translation.y;
				auto& tile = tiles.At( { x,y } );
				const auto tileType = tile.type;
				if( tileType == TileType::Floor )
				{
					gfx.DrawSprite( screen_x,screen_y,pFloorSurf->GetRect(),
						port.GetClipRect(),*pFloorSurf,SpriteEffect::Copy{}
					);
				}
				else if( tileType == TileType::Wall )
				{
					gfx.DrawSprite( screen_x,screen_y,pWallSurf->GetRect(),
						port.GetClipRect(),*pWallSurf,SpriteEffect::Copy{}
					);
				}
				else if( tileType == TileType::Goal )
				{
					gfx.DrawSprite( screen_x,screen_y,pGoalSurf->GetRect(),
						port.GetClipRect(),*pGoalSurf,SpriteEffect::Copy{}
					);
				}
				else
				{
					assert( "Bad tile type" && false );
				}
				// check if color is visible
				if( tile.c.GetA() != 0u )
				{
					gfx.DrawRect( 
						{ { screen_x,screen_y },tileWidth,tileHeight },
						tile.c,
						clipRect
					);					
				}
			}
		}
	}
	void Save( const std::string& filename ) const
	{
		std::ofstream file( filename );
		file << tiles.GetWidth() << " " << tiles.GetHeight() << std::endl;
		file << start_pos.x << " " << start_pos.y << std::endl;

		for( Vei2 pos = { 0,0 }; pos.y < tiles.GetHeight(); pos.y++ )
		{
			for( pos.x = 0; pos.x < tiles.GetWidth(); pos.x++ )
			{
				switch( tiles.At( pos ).type )
				{
				case TileType::Floor:
					file << ".";
					break;
				case TileType::Wall:
					file << "#";
					break;
				case TileType::Goal:
					file << "%";
					break;
				case TileType::Invalid:
					file << "?";
					break;
				}
			}
			if( pos.y != tiles.GetHeight() - 1 )
			{
				file << std::endl;
			}
		}
	}
	Direction GetStartDirection() const
	{
		return start_dir;
	}
	RectI GetMapBounds() const
	{
		return{ 0,tiles.GetWidth()*tileWidth,0,tiles.GetHeight()*tileHeight };
	}
	Vei2 GetCenterAt( const Vei2& pos ) const
	{
		assert( Contains( pos ) );
		return{ pos.x * tileWidth + tileWidth / 2,pos.y * tileHeight + tileHeight / 2 };
	}
	bool Contains( const Vei2& pos ) const
	{
		return tiles.Contains( pos );
	}
	Color GetColorAt( const Vei2& pos ) const
	{
		return tiles.At( pos ).c;
	}
	void SetColorAt( const Vei2& pos,Color color )
	{
		tiles.At( pos ).c = color;
	}
	int GetGridWidth() const
	{
		return tiles.GetWidth();
	}
	int GetGridHeight() const
	{
		return tiles.GetHeight();
	}
	const Vei2& GetStartPos() const
	{
		return start_pos;
	}
	template<typename F>
	void VisitNeighbors( const Vei2& pos,F f )
	{
		auto dir = Direction::Up();
		for( int i = 0; i < 4; i++,dir.RotateClockwise() )
		{
			const auto p = pos + dir;
			if( Contains( p ) )
			{
				f( p );
			}
		}
	}
private:
	std::unique_ptr<Surface> pFloorSurf;
	std::unique_ptr<Surface> pWallSurf;
	std::unique_ptr<Surface> pGoalSurf;
	int tileWidth;
	int tileHeight;
	Grid<Tile> tiles;
	Vei2 start_pos;
	Direction start_dir;
};

// TODO: start pos in map / multiple start pos