#pragma once

#include "Surface.h"
#include "Graphics.h"
#include "SpriteEffect.h"
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
	TileMap( const std::string& filename )
		:
		pFloorSurf( std::make_unique<Surface>( "Images\\floor.bmp" ) ),
		pWallSurf( std::make_unique<Surface>( "Images\\wall.bmp" ) ),
		pGoalSurf( std::make_unique<Surface>( "Images\\goal.bmp" ) ),
		tileWidth( pFloorSurf->GetWidth() ),
		tileHeight( pFloorSurf->GetHeight() )
	{
		const auto ThrowIfFalse = []( bool pred,const std::string& msg )
		{
			if( !pred )
			{
				throw std::runtime_error( "Tilemap load error.\n" + msg );
			}
		};

		std::stringstream iss;
		// load file into string stream buffer
		{
			std::ifstream file( filename );
			ThrowIfFalse( file.good(),"File: '" + filename + "' could not be opened." );
			iss << file.rdbuf();
		}
		// tile width init
		assert( tileWidth == pWallSurf->GetWidth() );
		assert( tileHeight == pWallSurf->GetHeight() );
		assert( tileWidth == pGoalSurf->GetWidth() );
		assert( tileHeight == pGoalSurf->GetHeight() );
		// read in grid dimensions
		int gridWidth;
		int gridHeight;
		{
			iss >> gridWidth;
			ThrowIfFalse( iss.good(),"Bad input reading grid width." );
			ThrowIfFalse( gridWidth > 0 && gridWidth <= 1000,"Bad width: " + std::to_string( gridWidth ) );
			iss >> gridHeight;
			ThrowIfFalse( iss.good(),"Bad input reading grid height." );
			ThrowIfFalse( gridHeight > 0 && gridHeight <= 1000,"Bad height: " + std::to_string( gridHeight ) );
		}
		// create working grid
		Grid<Tile> tiles( gridWidth,gridHeight );
		// read in start_pos
		{
			iss >> start_pos.x;
			ThrowIfFalse( iss.good(),"Bad input reading start pos x." );
			ThrowIfFalse( start_pos.x >= 0 && start_pos.x < gridWidth,"Bad start x: " + std::to_string( start_pos.x ) );
			iss >> start_pos.y;
			ThrowIfFalse( iss.good(),"Bad input reading start pos x." );
			ThrowIfFalse( start_pos.y >= 0 && start_pos.y < gridHeight,"Bad start y: " + std::to_string( start_pos.y ) );
		}
		// read in tiles
		{
			using namespace std::string_literals;
			tiles.reserve( gridWidth * gridHeight );
			auto i = tiles.begin();
			for( int y = 0; y < gridHeight; y++,i += tiles.GetWidth() )
			{
				std::string line;
				iss >> line;
				ThrowIfFalse( line.length() == gridWidth,"Bad row width in tilemap at line " + std::to_string( y ) + "." );
				std::transform(
					line.begin(),line.end(),
					i,
					[=]( char c ) -> Tile
				{
					switch( c )
					{
					case '#':
						return{ TileType::Wall };
					case '.':
						return{ TileType::Floor };
					case '%':
						return{ TileType::Goal };
					default:
						ThrowIfFalse( false,"Bad tile: '"s + c + "' in line "s + std::to_string( y ) + "."s );
						return TileType::Wall;
					}
				} );
			}
			ThrowIfFalse( iss.get() == -1 && iss.eof(),"Unexpected token at end of tilemap / wrong map height." );
		}
		// move working grid into map grid
		this->tiles = std::move( tiles );
	}
	// procedurally generated map
	TileMap( int width,int height,int max_tries )
		:
		tiles( width,height ),
		pFloorSurf( std::make_unique<Surface>( "Images\\floor.bmp" ) ),
		pWallSurf( std::make_unique<Surface>( "Images\\wall.bmp" ) ),
		pGoalSurf( std::make_unique<Surface>( "Images\\goal.bmp" ) ),
		tileWidth( pFloorSurf->GetWidth() ),
		tileHeight( pFloorSurf->GetHeight() )
	{
		const int min_room_size = 4;
		const int max_room_size = 20;
		std::uniform_int_distribution<int> room_dist( min_room_size,max_room_size );
		std::mt19937 rng( std::random_device{}() );
		Grid<int> compartmentIds( tiles.GetWidth(),tiles.GetHeight(),-1 );
		std::unordered_map<int,std::vector<Vei2>> compartments;
		int cur_id = 0;

		const auto TryPlaceRoom = [this,&id = cur_id,&rng,&room_dist,&compartmentIds,&compartments]()
		{
			const int width = room_dist( rng );
			const int height = room_dist( rng );
			std::uniform_int_distribution<int> pos_dist_x( 0,tiles.GetWidth() - width - 1 );
			std::uniform_int_distribution<int> pos_dist_y( 0,tiles.GetHeight() - height - 1 );
			
			const int xLeft = pos_dist_x( rng );
			const int yTop = pos_dist_y( rng );
			// check for overlap
			for( Vei2 pos = { 0,yTop }; pos.y < yTop + height; pos.y++ )
			{
				for( pos.x = xLeft; pos.x < xLeft + width; pos.x++ )
				{
					if( tiles.At( pos ).type != TileType::Invalid )
					{
						return false;
					}
				}
			}
			// add to compartment vector
			compartments.emplace( id,std::vector<Vei2>{} );
			// fill with floor
			for( Vei2 pos = { 0,yTop + 1 }; pos.y < yTop + height - 1; pos.y++ )
			{
				for( pos.x = xLeft + 1; pos.x < xLeft + width - 1; pos.x++ )
				{
					tiles.At( pos ).type = TileType::Floor;
					compartmentIds.At( pos ) = id;
					compartments[id].push_back( pos );
				}
			}
			// update id
			id++;
			return true;
		};
		// place rooms
		for( int count = 0; count < max_tries; )
		{
			if( !TryPlaceRoom() )
			{
				count++;
			}
		}
		// generate surrounding walls
		for( int x = 0; x < width; x++ )
		{
			tiles.At( { x,0 } ).type = TileType::Wall;
			tiles.At( { x,height - 1 } ).type = TileType::Wall;
		}
		for( int y = 0; y < height; y++ )
		{
			tiles.At( { 0,y } ).type = TileType::Wall;
			tiles.At( { width - 1,y } ).type = TileType::Wall;
		}
		// generate corridors here
		{
			const auto CountNeighboring = [this]( const Vei2& pos,TileType type )
			{
				// make sure tile has only 1 floor neighbor
				int floorCount = 0;
				VisitNeighbors( pos,
					[this,&floorCount,type]( const Vei2& pos )
					{
						if( tiles.At( pos ).type == type )
						{
							floorCount++;
						}
					}
				);
				return floorCount;
			};
			bool finished = false;
			while( !finished )
			{
				finished = true;
				for( Vei2 pos = { 0,0 }; pos.y < height; pos.y++ )
				{
					for( pos.x = 0; pos.x < width; pos.x++ )
					{
						// must have no neighbor floors
						if( tiles.At( pos ).type == TileType::Invalid &&
							CountNeighboring( pos,TileType::Floor ) == 0 )
						{
							finished = false;
							std::vector<Vei2> cands;
							tiles.At( pos ).type = TileType::Floor;
							compartmentIds.At( pos ) = cur_id;
							const auto AddCands = [this,&cands,CountNeighboring,&rng]( const Vei2& pos )
							{
								cands.clear();
								VisitNeighbors( pos,
									[this,&cands,CountNeighboring]( const Vei2& pos )
									{
										if( tiles.At( pos ).type == TileType::Invalid &&
											CountNeighboring( pos,TileType::Floor ) == 1 )
										{
											cands.emplace_back( pos );
										}
									}
								);
							};
							AddCands( pos );
							compartments[cur_id].push_back( pos );
							while( cands.size() > 0u )
							{
								std::uniform_int_distribution<int> cdist( 0,(int)cands.size() - 1 );
								pos = cands[cdist( rng )];
								tiles.At( pos ).type = TileType::Floor;
								compartments[cur_id].push_back( pos );
								compartmentIds.At( pos ) = cur_id;
								AddCands( pos );
							}
							cur_id++;
						}
					}
				}
			}
		}
		// last compartment is empty
		compartments.erase( cur_id-- );
		// TODO: assert no empty compartments
		// generate doors here
		while( compartments.size() > 1 )
		{
			std::vector<Vei2> wall_cands;
			const int off = std::uniform_int_distribution<int>{ 0,(int)compartments.size() - 1 }( rng );
			const int iMerge = std::next( compartments.begin(),off )->first;
			auto& merging = compartments[iMerge];
			std::random_shuffle( merging.begin(),merging.end() );
			// DoorTile: tile in comp that connects to door
			int iDoorTile = 0;
			for( ; iDoorTile < merging.size(); iDoorTile++ )
			{
				compartmentIds.VisitNeighbors( merging[iDoorTile],
					[&wall_cands,this]( const Vei2& pos )
					{
						if( tiles.At( pos ).type == TileType::Invalid )
						{
							wall_cands.push_back( pos );
						}
					}
				);
			}
			// find first candidate that borders a different compartment
			std::vector<Vei2> mergable_neighbors;
			while( !wall_cands.empty() )
			{
				//Save( "debug.txt" );
				// maybe just get a list of non-merge neighbors
				const auto cur_wall_cand = wall_cands.back();
				compartmentIds.VisitNeighbors( cur_wall_cand,
					[&mergable_neighbors,iMerge,&compartmentIds]( const Vei2& p )
					{
						const int t = compartmentIds.At( p );
						if( t != -1 && t != iMerge )
						{
							if( std::find_if( mergable_neighbors.begin(),mergable_neighbors.end(),
								[t,&compartmentIds]( const Vei2& pos )
								{
									return t == compartmentIds.At( pos );
								}
							) == mergable_neighbors.end() )
							{
								mergable_neighbors.push_back( p );
							}
						}
					}
				);
				if( mergable_neighbors.size() != 0 )
				{
					// merge with all neighbors (take iMerge)
					for( const auto& vm : mergable_neighbors )
					{
						const int i = compartmentIds.At( vm );
						for( auto& p : compartments[i] )
						{
							compartmentIds.At( p ) = iMerge;
						}
						merging.insert( merging.end(),
							compartments[i].begin(),compartments[i].end()
						);
						compartments.erase( i );
					}
					// remove wall and add floor (iMerge)
					compartmentIds.At( wall_cands.back() ) = iMerge;
					tiles.At( wall_cands.back() ).type = TileType::Floor;
					// empty wall_cands
					wall_cands.clear();
				}
				else
				{
					mergable_neighbors.clear();
					wall_cands.pop_back();
				}
			}
		}
		// generate walls here (replace ?s)
		for( auto& t : tiles )
		{
			if( t.type == TileType::Invalid )
			{
				t.type = TileType::Wall;
			}
		}
		// (maybe generate flair here)
		// TODO: add goal
		// TODO: file might need to determine accessibility? or something? (why is crashing when not found?)
		// scan to find start pos
		for( Vei2 pos = { 0,0 }; pos.y < height; pos.y++ )
		{
			for( pos.x = 0; pos.x < width; pos.x++ )
			{
				if( tiles.At( pos ).type == TileType::Floor )
				{
					start_pos = pos;
					return;
				}
			}
		}
		throw std::runtime_error( "Could not find a start pos!" );
	}
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
};

// TODO: start pos in map / multiple start pos