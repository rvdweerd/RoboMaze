#pragma once

#include "ChiliWin.h"
#include "Direction.h"
#include <string>
#include <cassert>

class Config
{
	friend class Evaluator;
public:
	enum class SimulationMode
	{
		Headless,
		Visual,
		VisualDebug,
		Script,
		Count
	};
	enum class MapMode
	{
		File,
		Procedural,
		Count
	};
	enum class GoalMode
	{
		NoGoal,
		Random,
		RoomCenter,
		StartPosition,
		InView,
		Count
	};
public:
	explicit Config( const std::string& filename )
	{
		using namespace std::string_literals;
		char buffer[1024];
		const auto ThrowIfFalse = []( bool pred,const std::string& msg )
		{
			if( !pred )
			{
				//throw std::runtime_error( "Config load error.\n" + msg );
			}
		};
		// get full path to ini file
		GetCurrentDirectoryA( sizeof( buffer ),buffer );
		const std::string full_ini_path = buffer + "\\"s + filename;
		// load sim mode setting
		sim_mode = (SimulationMode)GetPrivateProfileIntA( "simulation","sim_mode",-1,full_ini_path.c_str() );
		ThrowIfFalse( (int)sim_mode >= 0 && (int)sim_mode < (int)SimulationMode::Count,
			"Bad simulation mode: " + std::to_string( (int)sim_mode )
		);
		// load map filename
		GetPrivateProfileStringA( "simulation","map",nullptr,buffer,sizeof( buffer ),full_ini_path.c_str() );
		map_filename = "Maps\\"s + buffer;
		ThrowIfFalse( map_filename != "","Filename not set." );
		// load screen width and height
		screenWidth = GetPrivateProfileIntA( "display","screenwidth",-1,full_ini_path.c_str() );
		screenHeight = GetPrivateProfileIntA( "display","screenheight",-1,full_ini_path.c_str() );
		// load map mode setting
		map_mode = (MapMode)GetPrivateProfileIntA( "simulation","map_mode",-1,full_ini_path.c_str() );
		ThrowIfFalse( (int)map_mode >= 0 && (int)map_mode < (int)MapMode::Count,
			"Bad map mode: " + std::to_string( (int)map_mode )
		);
		// load direction (Count signifies random)
		dir = (Direction::Type)GetPrivateProfileIntA( "simulation","direction",-1,full_ini_path.c_str() );
		ThrowIfFalse( (int)dir >= 0 && (int)dir <= (int)Direction::Type::Count,
			"Bad start direction code: " + std::to_string( (int)dir )
		);
		// load goal spawn mode
		goalMode = (GoalMode)GetPrivateProfileIntA( "simulation","goal_spawn",-1,full_ini_path.c_str() );
		ThrowIfFalse( (int)goalMode >= 0 && (int)goalMode < (int)GoalMode::Count,
			"Bad goal spawn mode code: " + std::to_string( (int)goalMode )
		);
		// load map width and height
		mapWidth = GetPrivateProfileIntA( "simulation","map_width",-1,full_ini_path.c_str() );
		mapHeight = GetPrivateProfileIntA( "simulation","map_height",-1,full_ini_path.c_str() );
		// load map proc extra info
		roomTries = GetPrivateProfileIntA( "simulation","map_room",-1,full_ini_path.c_str() );
		extraDoors = GetPrivateProfileIntA( "simulation","extra_doors",-1,full_ini_path.c_str() );
		// load seed
		seed = GetPrivateProfileIntA( "simulation","seed",-1,full_ini_path.c_str() );
		// max moves
		maxMoves = GetPrivateProfileIntA( "simulation","max_moves",-1,full_ini_path.c_str() );
		// n runs
		nRuns = GetPrivateProfileIntA( "simulation","runs",-1,full_ini_path.c_str() );
	}
	std::string GetMapFilename() const
	{
		assert( GetMapMode() != MapMode::Procedural );
		return map_filename;
	}
	int GetMaxMoves() const
	{
		return maxMoves;
	}
	SimulationMode GetSimulationMode() const
	{
		return sim_mode;
	}
	MapMode GetMapMode() const
	{
		return map_mode;
	}
	GoalMode GetGoalMode() const
	{
		assert( GetMapMode() == MapMode::Procedural );
		return goalMode;
	}
	int GetScreenWidth() const
	{
		return screenWidth;
	}
	int GetScreenHeight() const
	{
		return screenHeight;
	}
	int GetMapWidth() const
	{
		assert( map_mode == MapMode::Procedural );
		return mapWidth;
	}
	int GetExtraDoors() const
	{
		assert( GetMapMode() == MapMode::Procedural );
		return extraDoors;
	}
	int GetMapHeight() const
	{
		assert( map_mode == MapMode::Procedural );
		return mapHeight;
	}
	int GetMapRoomTries() const
	{
		assert( map_mode == MapMode::Procedural );
		return roomTries;
	}
	bool IsRandomStartDirection() const
	{
		return dir == Direction::Type::Count;
	}
	Direction GetStartDirection() const
	{
		assert( !IsRandomStartDirection() );
		switch( dir )
		{
		case Direction::Type::Up:
			return Direction::Up();
		case Direction::Type::Down:
			return Direction::Down();
		case Direction::Type::Left:
			return Direction::Left();
		case Direction::Type::Right:
			return Direction::Right();
		default:
			assert( "Bad start direction!" && false );
			return Direction::Up();
		}
	}
	int GetNumberRuns() const
	{
		return nRuns;
	}
	unsigned int GetSeed() const
	{
		return (unsigned int)seed;
	}
private:
	std::string map_filename;
	SimulationMode sim_mode;
	MapMode map_mode;
	GoalMode goalMode;
	int mapWidth;
	int mapHeight;
	int roomTries;
	int extraDoors;
	int screenWidth;
	int screenHeight;
	int maxMoves;
	int nRuns;
	int seed;
	Direction::Type dir;
};