#pragma once

#include "Robo.h"
#include "TileMap.h"
#include <thread>

class DebugControls
{
public:
	DebugControls( TileMap& map,const Robo& robo )
		:
		map( map ),
		robo( robo )
	{}
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
	auto AcquireGfxLock() const
	{
		return std::unique_lock<std::mutex>( mutex_gfx );
	}
	void SignalTick() const
	{
		cv.notify_all();
	}
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