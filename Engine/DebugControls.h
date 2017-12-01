#pragma once

#include "Robo.h"
#include "TileMap.h"
#include <thread>
#include <condition_variable>
#include <mutex>

// used by debug visualization version of AI to visualize the algorithm as it runs
// also can be used in debug to get information about the actual state of the maze
// for whatever reason
class DebugControls
{
public:
	DebugControls( TileMap& map,const Robo& robo )
		:
		map( map ),
		robo( robo )
	{}
	DebugControls( const DebugControls& ) = delete;
	DebugControls& operator=( const DebugControls& ) = delete;
	void MarkAt( const Vei2& pos,Color color )
	{
		map.SetColorAt( pos,color );
	}
	void ClearAt( const Vei2& pos )
	{
		map.SetColorAt( pos,{ 0,0,0,0 } );
	}
	void MarkAll( Color c )
	{
		ForEach( [this,c]( const auto& p,auto& dc ) { MarkAt( p,c ); } );
	}
	void ClearAll()
	{
		MarkAll( { 0,0,0,0 } );
	}
	// pass a functor that takes ( Vei2,DebugControls& )
	// will run that functor for every tile in map
	template<typename F>
	void ForEach( F func )
	{
		for( Vei2 p = { 0,0 }; p.y < map.GetGridHeight(); p.y++ )
		{
			for( p.x = 0; p.x < map.GetGridWidth(); p.x++ )
			{
				func( p,*this );
			}
		}
	}
	Color GetMarkAt( const Vei2& pos ) const
	{
		return map.GetColorAt( pos );
	}
	TileMap::TileType GetTypeAt( const Vei2& pos ) const
	{
		return map.At( pos );
	}
	Vei2 GetRobotPosition() const
	{
		return robo.GetPos();
	}
	Direction GetRobotDirection() const
	{
		return robo.GetDirection();
	}
	// returns lock object
	// prevents graphics from rendering while lock is held
	auto AcquireGfxLock() const
	{
		return std::unique_lock<std::mutex>( mutex_gfx );
	}
	// ignore: called by engine
	void SignalTick() const
	{
		cv.notify_all();
	}
	// wait for a tick to be signaled by engine
	// tick speed is related to UI slider
	// use this to control the speed of your algorithm's visualization animation
	void WaitOnTick() const
	{
		std::unique_lock<std::mutex> lock( mutex_tick );
		cv.wait( lock );
	}
private:
	TileMap& map;
	const Robo& robo;
	mutable std::condition_variable cv;
	mutable std::mutex mutex_tick;
	mutable std::mutex mutex_gfx;
	mutable std::unique_lock<std::mutex> lock;
};