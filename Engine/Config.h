#pragma once

#include "ChiliWin.h"
#include <string>

class Config
{
public:
	enum class SimulationMode
	{
		Headless,
		Visual,
		VisualDebug,
		Count
	};
public:
	Config( const std::string& filename )
	{
		using namespace std::string_literals;
		char buffer[1024];
		const auto ThrowIfFalse = []( bool pred,const std::string& msg )
		{
			if( !pred )
			{
				throw std::runtime_error( "Config load error.\n" + msg );
			}
		};
		// get full path to ini file
		GetCurrentDirectoryA( sizeof( buffer ),buffer );
		const std::string full_ini_path = buffer + "\\"s + filename;
		// load mode setting
		mode = (SimulationMode)GetPrivateProfileIntA( "simulation","mode",-1,full_ini_path.c_str() );
		ThrowIfFalse( (int)mode >= 0 && (int)mode < (int)SimulationMode::Count,
			"Bad simulation mode: " + std::to_string( (int)mode )
		);
		// load map filename
		GetPrivateProfileStringA( "simulation","map",nullptr,buffer,sizeof( buffer ),full_ini_path.c_str() );
		map_filename = buffer;
		ThrowIfFalse( map_filename != "","Filename not set." );
		// load screen width and height
		screenWidth = GetPrivateProfileIntA( "display","screenwidth",-1,full_ini_path.c_str() );
		screenHeight = GetPrivateProfileIntA( "display","screenheight",-1,full_ini_path.c_str() );
	}
	const std::string& GetMapFilename() const
	{
		return map_filename;
	}
	SimulationMode GetSimulationMode() const
	{
		return mode;
	}
	int GetScreenWidth() const
	{
		return screenWidth;
	}
	int GetScreenHeight() const
	{
		return screenHeight;
	}
private:
	std::string map_filename;
	SimulationMode mode;
	int screenWidth;
	int screenHeight;
};