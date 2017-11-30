#pragma once
#include "..\Robo.h"
#include "..\DebugControls.h"
#include <random>
#include <deque>

// simple demo that randomly turns when it runs into a wall
class RoboAI_Demo
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	Action Plan( std::array<TT,3> view )
	{
		if( view[1] == TT::Wall )
		{
			if( coinflip( rng ) )
			{
				return Action::TurnRight;
			}
			else
			{
				return Action::TurnLeft;
			}
		}
		return Action::MoveForward;
	}
private:
	std::mt19937 rng = std::mt19937( std::random_device{}() );
	std::bernoulli_distribution coinflip;
};

// use this if you don't want to implement a debug RoboAI
class RoboAIDebug_Unimplemented
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	RoboAIDebug_Unimplemented( DebugControls& dc )
	{}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = false;
	Action Plan( std::array<TT,3> view )
	{
		assert( "RoboAIDebug is not implemented" && false );
		return Action::TurnLeft;
	}
};

// demo of how to use DebugControls for visualization
class RoboAIDebug_Demo
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	RoboAIDebug_Demo( DebugControls& dc )
		:
		dc( dc )
	{}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = true;
	Action Plan( std::array<TT,3> view )
	{
		if( view[1] == TT::Wall )
		{
			if( coinflip( rng ) )
			{
				return Action::TurnRight;
			}
			else
			{
				return Action::TurnLeft;
			}
		}
		dc.MarkAt( dc.GetRobotPosition(),{ Colors::Green,32u } );
		return Action::MoveForward;
	}
private:
	DebugControls& dc;
	std::mt19937 rng = std::mt19937( std::random_device{}() );
	std::bernoulli_distribution coinflip;
};

// if you name your classes different than RoboAI/RoboAIDebug, use typedefs like these
typedef RoboAI_Demo RoboAI;
typedef RoboAIDebug_Demo RoboAIDebug;