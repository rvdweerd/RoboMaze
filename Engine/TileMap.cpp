#include "TileMap.h"
#include "Config.h"

TileMap::TileMap( const Config & config )
	:
	tiles( config.GetMapWidth(),config.GetMapHeight() ),
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

	// first place goal room if room mode active
	if( config.GetGoalMode() == Config::GoalMode::RoomCenter )
	{
		// goal room is 9x9 (could make this vary in the future)
		const int width = 11;
		const int height = 11;
		std::uniform_int_distribution<int> pos_dist_x( 0,tiles.GetWidth() - width - 1 );
		std::uniform_int_distribution<int> pos_dist_y( 0,tiles.GetHeight() - height - 1 );
		
		// add to compartment map
		compartments.emplace( cur_id,std::vector<Vei2>{} );
		// generate top left corner pos
		const int xLeft = pos_dist_x( rng );
		const int yTop = pos_dist_y( rng );
		// fill with floor
		for( Vei2 pos = { 0,yTop + 1 }; pos.y < yTop + height - 1; pos.y++ )
		{
			for( pos.x = xLeft + 1; pos.x < xLeft + width - 1; pos.x++ )
			{
				tiles.At( pos ).type = TileType::Floor;
				compartmentIds.At( pos ) = cur_id;
				compartments[cur_id].push_back( pos );
			}
		}
		// place goal
		tiles.At( Vei2{ xLeft,yTop } + Vei2{ 5,5 } ).type = TileType::Goal;
		// update id
		cur_id++;
	}

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
		// add to compartment map
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
	for( int count = 0; count < config.GetMapRoomTries(); )
	{
		if( !TryPlaceRoom() )
		{
			count++;
		}
	}
	// generate surrounding walls
	for( int x = 0; x < tiles.GetWidth(); x++ )
	{
		tiles.At( { x,0 } ).type = TileType::Wall;
		tiles.At( { x,tiles.GetHeight() - 1 } ).type = TileType::Wall;
	}
	for( int y = 0; y < tiles.GetHeight(); y++ )
	{
		tiles.At( { 0,y } ).type = TileType::Wall;
		tiles.At( { tiles.GetWidth() - 1,y } ).type = TileType::Wall;
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
			for( Vei2 pos = { 0,0 }; pos.y < tiles.GetHeight(); pos.y++ )
			{
				for( pos.x = 0; pos.x < tiles.GetWidth(); pos.x++ )
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
	// generate linking doors here
	while( compartments.size() > 1 )
	{
		std::vector<Vei2> wall_cands;
		const int off = std::uniform_int_distribution<int>{ 0,(int)compartments.size() - 1 }(rng);
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
	// extra doors
	{
		std::uniform_int_distribution<int> dist_x( 1,tiles.GetWidth() - 2 );
		std::uniform_int_distribution<int> dist_y( 1,tiles.GetHeight() - 2 );
		for( int n = 0; n < config.GetExtraDoors(); )
		{
			const Vei2 pos = { dist_x( rng ),dist_y( rng ) };
			if( tiles.At( pos ).type == TileType::Wall )
			{
				tiles.At( pos ).type = TileType::Floor;
				n++;
			}
		}
	}
	// (maybe generate flair here)
	// TODO: add goal
	// TODO: file might need to determine accessibility? or something? (why is crashing when not found?)
	// scan to find start pos
	for( Vei2 pos = { 0,0 }; pos.y < tiles.GetHeight(); pos.y++ )
	{
		for( pos.x = 0; pos.x < tiles.GetWidth(); pos.x++ )
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
