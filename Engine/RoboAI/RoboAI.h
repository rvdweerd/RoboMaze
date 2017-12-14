#pragma once
#include "..\Robo.h"
#include "..\DebugControls.h"
#include <random>
#include <deque>


// returns angle in units of pi/2 (90deg you pleb)
inline int GetAngleBetween( const Direction& d1,const Direction& d2 )
{
	constexpr std::array<int,4> map = { 0,2,3,1 };
	return map[d2.GetIndex()] - map[d1.GetIndex()];
}

// angle is in units of pi/2 (90deg you pleb)
inline Vei2 GetRotated90( const Vei2& v,int angle )
{
	Vei2 vo;
	switch( angle )
	{
	case -3:
		vo.x = -v.y;
		vo.y = v.x;
		break;
	case -2:
		vo = -vo;
		break;
	case -1:
		vo.x = v.y;
		vo.y = -v.x;
		break;
	case 0:
		vo = v;
		break;
	case 1:
		vo.x = -v.y;
		vo.y = v.x;
		break;
	case 2:
		vo = -vo;
		break;
	case 3:
		vo.x = v.y;
		vo.y = -v.x;
		break;
	default:
		assert( "Bad angle in Dir rotation!" && false );
	}
	return vo;
}

// demo of how to use DebugControls for visualization
class Djikslide
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	class Node
	{
	public:
		TT GetType() const
		{
			return type;
		}
		void SetType( TT type_in )
		{
			assert( type == TT::Invalid );
			type = type_in;
		}
		void ClearCost()
		{
			cost = std::numeric_limits<int>::max();
		}
		bool UpdateCost( int new_cost,const Vei2& new_prev = { -1,-1 } )
		{
			if( new_cost < cost )
			{
				cost = new_cost;
				prev = new_prev;
				return true;
			}
			return false;
		}
		int GetCost() const
		{
			return cost;
		}
		const Vei2& GetPrev() const
		{
			return prev;
		}
	private:
		TT type = TT::Invalid;
		Vei2 prev;
		int cost = std::numeric_limits<int>::max();
	};
public:
	Djikslide()
		:
		nodes( gridWidth * gridHeight ),
		pos( gridWidth / 2,gridHeight / 2 )
	{
		// prime the pathing pump
		path.push_back( pos + dir );
	}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = true;
	Action Plan( std::array<TT,3> view )
	{
		// return DONE if standing on the goal
		if( At( pos ).GetType() == TT::Goal )
		{
			return Action::Done;
		}
		// first update map
		UpdateMap( view );
		// check if we have revealed the target unknown square
		if( At( path.back() ).GetType() != TT::Invalid )
		{
			// compute new path
			ComputePath();
		}
		// find our position in path sequence
		const auto i = std::find( path.begin(),path.end(),pos );
		// move to next position in sequence
		return MoveTo( *std::next( i ) );
	}
private:
	void ComputePath()
	{
		// create priority queue for 'frontier'
		std::deque<Vei2> frontier;

		// clear costs
		for( auto& node : nodes )
		{
			node.ClearCost();
		}

		// clear path sequence
		path.clear();

		// add current tile to frontier to0 start the ball rollin
		At( pos ).UpdateCost( 0 );
		frontier.emplace_back( pos );

		while( !frontier.empty() )
		{
			const auto base = frontier.front();
			frontier.pop_front();

			auto dir = Direction::Up();
			for( int i = 0; i < 4; i++,dir.RotateClockwise() )
			{
				const auto nodePos = base + dir;
				auto& node = At( nodePos );
				if( node.GetType() == TT::Invalid || node.GetType() == TT::Goal )
				{
					// then construct path and return
					path.push_back( nodePos );
					auto trace = base;
					while( At( trace ).GetPrev() != Vei2{ -1,-1 } )
					{
						// add it to the path
						path.push_back( trace );
						// walk to next pos in path
						trace = At( trace ).GetPrev();
					}
					// add it to the path
					path.push_back( trace );
					// gotta reverse that shit
					std::reverse( path.begin(),path.end() );
					return;
				}
				else if( node.GetType() == TT::Floor )
				{
					if( node.UpdateCost( At( base ).GetCost() + 1,base ) )
					{
						frontier.push_back( nodePos );
					}
				}
			}
		}
		assert( "Terrible" && false );
	}
	void UpdateMap( const std::array<TT,3>& view )
	{
		auto scan_pos = pos + dir + dir.GetRotatedCounterClockwise();
		const auto scan_delta = dir.GetRotatedClockwise();
		for( int i = 0; i < 3; i++,scan_pos += scan_delta )
		{
			auto& node = At( scan_pos );
			const auto type = node.GetType();
			if( type != TT::Invalid )
			{
				assert( type == view[i] );
				continue;
			}
			node.SetType( view[i] );
		}
	}
	Node& At( const Vei2& pos )
	{
		return nodes[pos.x + pos.y * gridWidth];
	}
	Action MoveTo( const Vei2& target )
	{
		const Direction delta = Direction( target - pos );
		if( delta == dir )
		{
			pos += dir;
			return Action::MoveForward;
		}
		else if( dir.GetRotatedClockwise() == delta )
		{
			dir.RotateClockwise();
			return Action::TurnRight;
		}
		else
		{
			dir.RotateCounterClockwise();
			return Action::TurnLeft;
		}
	}
private:
	bool visual_init_done = false;
	static constexpr int gridWidth = 100;
	static constexpr int gridHeight = 100;
	Vei2 pos;
	std::vector<Vei2> path;
	Direction dir = Direction::Up();
	std::vector<Node> nodes;
};

// demo of how to use DebugControls for visualization
class Djikslide_Debug
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	class Node
	{
	public:
		TT GetType() const
		{
			return type;
		}
		void SetType( TT type_in )
		{
			assert( type == TT::Invalid );
			type = type_in;
		}
		void ClearCost()
		{
			cost = std::numeric_limits<int>::max();
		}
		bool UpdateCost( int new_cost,const Vei2& new_prev = { -1,-1 } )
		{
			if( new_cost < cost )
			{
				cost = new_cost;
				prev = new_prev;
				return true;
			}
			return false;
		}
		int GetCost() const
		{
			return cost;
		}
		const Vei2& GetPrev() const
		{
			return prev;
		}
	private:
		TT type = TT::Invalid;
		Vei2 prev;
		int cost = std::numeric_limits<int>::max();
	};
public:
	Djikslide_Debug( DebugControls& dc )
		:
		nodes( gridWidth * gridHeight ),
		pos( gridWidth / 2,gridHeight / 2 ), // start in middle
		dc( dc ),
		angle( GetAngleBetween( dir,dc.GetRobotDirection() ) ),
		visit_extent( -1,-1,-1,-1 )
	{
		// prime the pathing pump
		path.push_back( pos + dir );
		VisualInit();
		ResetExtents( pos );
	}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = true;
	Action Plan( std::array<TT,3> view )
	{
		// return DONE if standing on the goal
		if( At( pos ).GetType() == TT::Goal )
		{
			const auto lock = dc.AcquireGfxLock();
			ClearPathMarkings();
			return Action::Done;
		}
		// first update map
		UpdateMap( view );
		// check if we have revealed the target unknown square
		if( At( path.back() ).GetType() != TT::Invalid )
		{
			// compute new path
			if( !ComputePath() )
			{
				// if ComputePath() returns false, impossible to reach goal
				return Action::Done;
			}
		}
		// find our position in path sequence
		const auto i = std::find( path.begin(),path.end(),pos );
		// move to next position in sequence
		return MoveTo( *std::next( i ) );
	}
private:
	bool ComputePath()
	{
		{
			const auto lock = dc.AcquireGfxLock();
			ClearPathMarkings();
		}
		// create priority queue for 'frontier'
		std::deque<Vei2> frontier;

		// clear costs
		for( Vei2 cp = visit_extent.TopLeft(); cp.y <= visit_extent.bottom; cp.y++ )
		{
			for( cp.x = visit_extent.left; cp.x <= visit_extent.right; cp.x++ )
			{
				At( cp ).ClearCost();
			}
		}

		ResetExtents( pos );

		// clear path sequence
		path.clear();

		// add current tile to frontier to0 start the ball rollin
		At( pos ).UpdateCost( 0 );
		frontier.emplace_back( pos );
		AdjustExtents( pos );

		while( !frontier.empty() )
		{
			const auto base = frontier.front();
			frontier.pop_front();

			auto dir = Direction::Up();
			for( int i = 0; i < 4; i++,dir.RotateClockwise() )
			{
				const auto nodePos = base + dir;
				auto& node = At( nodePos );
				if( node.GetType() == TT::Invalid || node.GetType() == TT::Goal )
				{
					// found our target, first mark it
					dc.MarkAt( ToGameSpace( nodePos ),{ 96,96,0,0 } );
					// then construct path and return
					path.push_back( nodePos );
					auto trace = base;
					while( At( trace ).GetPrev() != Vei2{ -1,-1 } )
					{
						// mark sequence for visualiztion yellow
						dc.MarkAt( ToGameSpace( trace ),{ 96,96,96,0 } );
						// add it to the path
						path.push_back( trace );
						// walk to next pos in path
						trace = At( trace ).GetPrev();
					}
					// mark sequence for visualiztion yellow
					dc.MarkAt( ToGameSpace( trace ),{ 96,96,96,0 } );
					// add it to the path
					path.push_back( trace );
					// gotta reverse that shit
					std::reverse( path.begin(),path.end() );
					return true;
				}
				else if( node.GetType() == TT::Floor )
				{
					if( node.UpdateCost( At( base ).GetCost() + 1,base ) )
					{
						frontier.push_back( nodePos );
						AdjustExtents( nodePos );
						// mark each tile visited by my dik
						dc.MarkAt( ToGameSpace( nodePos ),{ Colors::Green,32 } );
						dc.WaitOnTick();
					}
				}
			}
		}
		// cannot reach goal
		return false;
	}
	void UpdateMap( const std::array<TT,3>& view )
	{
		auto scan_pos = pos + dir + dir.GetRotatedCounterClockwise();
		const auto scan_delta = dir.GetRotatedClockwise();
		for( int i = 0; i < 3; i++,scan_pos += scan_delta )
		{
			auto& node = At( scan_pos );
			const auto type = node.GetType();
			if( type != TT::Invalid )
			{
				assert( type == view[i] );
				continue;
			}
			node.SetType( view[i] );
			dc.ClearAt( ToGameSpace( scan_pos ) );
		}
	}
	Node& At( const Vei2& pos )
	{
		return nodes[pos.x + pos.y * gridWidth];
	}
	Action MoveTo( const Vei2& target )
	{
		const Direction delta = Direction( target - pos );
		if( delta == dir )
		{
			pos += dir;
			return Action::MoveForward;
		}
		else if( dir.GetRotatedClockwise() == delta )
		{
			dir.RotateClockwise();
			return Action::TurnRight;
		}
		else
		{
			dir.RotateCounterClockwise();
			return Action::TurnLeft;
		}
	}
	Vei2 ToGameSpace( const Vei2& pos_in ) const
	{
		return GetRotated90( pos_in - pos,angle ) + dc.GetRobotPosition();
	}
	Vei2 ToMapSpace( const Vei2& pos_in ) const
	{
		return GetRotated90( pos_in - dc.GetRobotPosition(),-angle ) + pos;
	}
	void VisualInit()
	{
		if( !visual_init_done )
		{
			auto l = dc.AcquireGfxLock();
			dc.MarkAll( { Colors::Black,96 } );
			visual_init_done = true;
		}
	}
	void ClearPathMarkings()
	{
		dc.ForEach( [this]( const Vei2& p,DebugControls& dc )
		{
			if( At( ToMapSpace( p ) ).GetType() != TileMap::TileType::Invalid )
			{
				dc.ClearAt( p );
			}
		} );
	}
	void AdjustExtents( const Vei2& pos )
	{
		visit_extent.left = std::min( pos.x,visit_extent.left );
		visit_extent.right = std::max( pos.x,visit_extent.right );
		visit_extent.top = std::min( pos.y,visit_extent.top );
		visit_extent.bottom = std::max( pos.y,visit_extent.bottom );
	}
	void ResetExtents( const Vei2& pos )
	{
		visit_extent = { pos.x + 1,pos.x - 1,pos.y + 1, pos.y - 1 };
	}
private:
	DebugControls& dc;
	bool visual_init_done = false;
	// grid dimensions 2x the max dimensions
	// (because we don't know where we start!)
	static constexpr int gridWidth = 2000;
	static constexpr int gridHeight = 2000;
	Vei2 pos;
	std::vector<Vei2> path;
	Direction dir = Direction::Up();
	std::vector<Node> nodes;
	int angle;
	// inclusive!
	RectI visit_extent;
};

// if you name your classes different than RoboAI/RoboAIDebug, use typedefs like these
typedef Djikslide RoboAI;
typedef Djikslide_Debug RoboAIDebug;

// TODO: Fix performance for large max map size (block-clean min-max)
// TODO: test perf walk-clean vs. block clean vs. map
// TODO: implement non-slide (turn counting) dijkturn with map
// TODO: propagate unreachable and rel-dir fixes to non debug ver