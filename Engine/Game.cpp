/****************************************************************************************** 
 *	Chili DirectX Framework Version 16.07.20											  *	
 *	Game.cpp																			  *
 *	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/
#include "MainWindow.h"
#include "Game.h"
#include "Simulator.h"
#include "Evaluator.h"

Game::Game( MainWindow& wnd,const Config& config )
	:
	wnd( wnd ),
	gfx( wnd,config )
{
	InitializeSimulation( config );
}

void Game::InitializeSimulation( const Config& config )
{
	if( config.GetEvalMode() == Config::EvaluationMode::Single )
	{
		switch( config.GetSimulationMode() )
		{
		case Config::SimulationMode::Headless:
			sim = std::make_unique<HeadlessSimulator>( config,config.GetSeed() );
			break;
		case Config::SimulationMode::Visual:
			sim = std::make_unique<NormalSimulator>( config,config.GetSeed() );
			break;
		case Config::SimulationMode::VisualDebug:
			if( RoboAIDebug::implemented )
			{
				sim = std::make_unique<DebugSimulator>( config,config.GetSeed() );
			}
			else
			{
				throw std::runtime_error( "Visual Debug AI not implemented!" );
			}
			break;
		default:
			assert( false && "Bad simulation mode" );
		}
	}
	else if( config.GetEvalMode() == Config::EvaluationMode::Script )
	{
		sim = std::make_unique<Evaluator>( config );
	}
	else
	{
		assert( "Bad evaluation mode" && false );
	}
}

void Game::Go()
{
	gfx.BeginFrame();	
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void Game::UpdateModel()
{
	if( wnd.kbd.KeyIsPressed( VK_BACK ) )
	{
		wnd.Reset();
	}
	sim->Update( wnd,ft.Mark() );
}

void Game::ComposeFrame()
{
	sim->Draw( gfx );
}
