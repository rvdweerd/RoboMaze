#pragma once

#include "Sound.h"
#include "TileMap.h"
#include "Robo.h"
#include "RoboAI\RoboAI.h"
#include "DebugControls.h"
#include <future>
#include "Font.h"
#include "MainWindow.h"
#include "Window.h"
#include "Config.h"
#include <atomic>
#include <deque>
#include <unordered_set>

class Simulator
{
public:
	enum class State
	{
		Working,
		Success,
		Failure,
		Count
	};
public:
	Simulator( const std::string& map_filename,const Direction& dir )
		:
		map( map_filename ),
		rob( map.GetStartPos(),dir ),
		goalReachable( ComputeGoalReachability() )
	{
		stateTexts.resize( (int)State::Count );
		stateTexts[(int)State::Success] = { { "Done" },Colors::White };
		stateTexts[(int)State::Failure] = { { "Fail" },Colors::Red };
		stateTexts[(int)State::Working] = { { "Work" },Colors::Green };
	}
	virtual ~Simulator() = default;
	int GetMoveCount() const
	{
		return move_count;
	}
	virtual void Draw( Graphics& gfx ) const
	{
		// move count bottom right
		{
			const auto mct = std::to_string( move_count );
			auto tr = font.CalculateTextRect( mct );
			const auto offset = Graphics::GetScreenRect().BottomRight() - tr.BottomRight() - Vei2{ 5,0 };
			tr.DisplaceBy( offset );
			font.DrawText( std::to_string( move_count ),tr.TopLeft(),Colors::White,gfx );
		}
		// simulation state drawn after where the ctrls go
		font.DrawText(
			stateTexts[(int)state].first,
			{ 430,Graphics::ScreenHeight - 39 },
			stateTexts[(int)state].second,gfx
		);
	}
	virtual void Update( MainWindow& wnd,float dt )
	{}
	bool GoalReached() const
	{
		return map.At( rob.GetPos() ) == TileMap::TileType::Goal;
	}
	bool ComputeGoalReachability() const
	{
		// create queue for 'frontier'
		std::deque<Vei2> frontier;

		// hash function for positions
		struct Vei2Hash
		{
			size_t operator()( const Vei2& pos ) const
			{
				const std::hash<int> hasher;
				const size_t seed = hasher( pos.x );
				return seed ^ (hasher( pos.y ) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
			}
		};

		// set of closed nodes (reserve a decent amount of space)
		std::unordered_set<Vei2,Vei2Hash> closed;
		closed.reserve( map.GetGridHeight() * map.GetGridWidth() );

		// add current tile to frontier to0 start the ball rollin
		frontier.push_back( rob.GetPos() );
		closed.insert( rob.GetPos() );

		while( !frontier.empty() )
		{
			const auto base = frontier.front();
			frontier.pop_front();

			auto dir = Direction::Up();
			for( int i = 0; i < 4; i++,dir.RotateClockwise() )
			{
				const auto nodePos = base + dir;
				const auto node = map.At( nodePos );
				if( node == TileMap::TileType::Goal )
				{
					// goal is reachable
					return true;
				}
				else if( node == TileMap::TileType::Floor && closed.find( nodePos ) == closed.cend() )
				{
					frontier.push_back( nodePos );
					closed.insert( nodePos );
				}
			}
		}
		// cannot reach goal
		return false;
	}
	bool Finished() const
	{
		return state != State::Working;
	}
	State GetState() const
	{
		return state;
	}
	void UpdateState( Robo::Action action )
	{
		if( action == Robo::Action::Done )
		{
			if( GoalReached() || !goalReachable )
			{
				state = State::Success;
			}
			else
			{
				state = State::Failure;
			}
		}
	}
protected:
	void IncrementMoveCount()
	{
		move_count++;
	}
	TileMap map;
	Robo rob;
	Font font = Font( "Images\\Fixedsys16x28.bmp" );
private:
	bool goalReachable;
	int move_count = 0;
	State state = State::Working;
	std::vector<std::pair<std::string,Color>> stateTexts;
};

class HeadlessSimulator : public Simulator
{
public:
	HeadlessSimulator( const Config& config )
		:
		Simulator( config.GetMapFilename(),config.GetStartDirection() )
	{
		worker = std::thread( [this]()
		{
			RoboAI ai;
			FrameTimer ft;

			while( !Finished() && !dying )
			{
				const auto view = rob.GetView( map );
				ft.Mark();
				const auto action = ai.Plan( view );
				workingTime += ft.Mark();
				rob.TakeAction( action,map );
				UpdateState( action );
				IncrementMoveCount();
			}
		} );
	}
	void Draw( Graphics& gfx ) const override
	{
		Simulator::Draw( gfx );
		font.DrawText(
			std::to_string( workingTime ),
			{ Graphics::GetScreenRect().left + 5,Graphics::GetScreenRect().bottom - 30 },
			Colors::White,gfx
		);
	}
	~HeadlessSimulator() override
	{
		dying = true;
		worker.join();
	}
private:
	float workingTime = 0.0f;
	std::thread worker;
	std::atomic<bool> dying = false;
};

class VisualSimulator : public Simulator,public Window::SimstepControllable
{
public:
	VisualSimulator( const Config& config )
		:
		Simulator( config.GetMapFilename(),config.GetStartDirection() ),
		ctrls( Graphics::GetScreenRect() )
	{
		auto pCamlockTemp = std::make_unique<Window::CamLockToggle>(
			RectI{
			350,420,
			Graphics::GetScreenRect().bottom - 50,
			Graphics::GetScreenRect().bottom
		},
			font
			);
		{
			auto region = ctrls.GetRegion();
			region.bottom -= 50;
			ctrls.AddWindow( std::make_unique<Window::Map>(
				region,map,rob,*pCamlockTemp
			) );
		}
		ctrls.AddWindow( std::move( pCamlockTemp ) );
		ctrls.AddWindow( std::make_unique<Window::PlayPause>(
			Vei2( 0,Graphics::GetScreenRect().bottom - 50 ),50,10,*this
		) );
		ctrls.AddWindow( std::make_unique<Window::SingleStep>(
			Vei2( 50,Graphics::GetScreenRect().bottom - 50 ),50,10,*this
		) );
		ctrls.AddWindow( std::make_unique<Window::SpeedSlider>(
			RectI{ 150,350,Graphics::GetScreenRect().bottom - 50,Graphics::GetScreenRect().bottom },
			35,10,0.002f,0.6f,0.1f,*this
		) );
	}
	void Update( MainWindow& wnd,float dt ) override
	{
		// process discrete mouse events
		while( !wnd.mouse.IsEmpty() )
		{
			auto p = ctrls.OnMouseEvent( wnd.mouse.Read() );
			if( pHover && pHover != p )
			{
				pHover->OnMouseLeave();
			}
			pHover = p;
		}
		// update simulation timestep if not yet finished
		if( !Finished() )
		{
			Window::SimstepControllable::Update( dt );
		}
		// update windows that need user-time updates
		ctrls.Update( dt );
	}
	void Draw( Graphics& gfx ) const override
	{
		ctrls.Draw( gfx );
		Simulator::Draw( gfx );
	}
private:
	Window::Window* pHover = nullptr;
	Window::Container ctrls;
};

class DebugSimulator : public VisualSimulator
{
public:
	DebugSimulator( const Config& config )
		:
		VisualSimulator( config ),
		dc( map,rob ),
		ai( dc )
	{
		LaunchAI();
	}
	void Draw( Graphics& gfx ) const override
	{
		const auto lock = dc.AcquireGfxLock();
		VisualSimulator::Draw( gfx );
	}
	void OnTick() override
	{
		if( !Finished() )
		{
			using namespace std::chrono_literals;
			// check the future
			if( future.wait_for( 0ms ) == std::future_status::ready )
			{
				const auto action = future.get();
				rob.TakeAction( action,map );
				UpdateState( action );
				IncrementMoveCount();
				if( !Finished() )
				{
					LaunchAI();
				}
			}
			// signal the future
			dc.SignalTick();
		}
	}
	~DebugSimulator() override
	{
		using namespace std::chrono_literals;
		if( future.valid() )
		{
			// continuously signal go-ahead while future not ready
			while( future.wait_for( 500us ) != std::future_status::ready )
			{
				dc.SignalTick();
			}
			// complete the future
			future.get();
		}
	}
private:
	void LaunchAI()
	{
		future = std::async( std::launch::async,
			&RoboAIDebug::Plan,&ai,rob.GetView( map )
		);
	}
private:
	DebugControls dc;
	RoboAIDebug ai;
	std::future<Robo::Action> future;
};

class NormalSimulator : public VisualSimulator
{
public:
	using VisualSimulator::VisualSimulator;
	void OnTick() override
	{
		if( !Finished() )
		{
			const auto action = ai.Plan( rob.GetView( map ) );
			rob.TakeAction( action,map );
			UpdateState( action );
			IncrementMoveCount();
		}
	}
private:
	RoboAI ai;
};