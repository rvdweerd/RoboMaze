#pragma once

#include "TileMap.h"
#include "Robo.h"

class CamController
{
public:
	CamController( Camera& cam,const TileMap& map,const Robo& robo )
		:
		cam( cam ),
		map( map ),
		robo( robo )
	{}
	void Update( float dt )
	{
		const Ved2 robo_pos = (Ved2)map.GetCenterAt( robo.GetPos() );
		const Ved2 cam_center = cam.GetCenter();
		const Ved2 delta = robo_pos - cam_center;
		const double dist_sq = delta.GetLengthSq();
		const double speed = clamp( dist_sq * k,min_speed,max_speed );
		const double pan_dist = std::min( speed * (double)dt,std::sqrt( dist_sq ) );
		cam.PanBy( delta.GetNormalized() * pan_dist );
	}
private:
	static constexpr double k = 0.01;
	static constexpr double max_speed = 2000.0;
	static constexpr double min_speed = 15.0;
	Camera& cam;
	const TileMap& map;
	const Robo& robo;
};