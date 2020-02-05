#pragma once

#include "Simulator.h"
#include "Config.h"
#include "Graphics.h"
#include "Gameable.h"
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm>
#include <numeric>

class Evaluator : public Gameable
{
private:
	struct Result
	{
		size_t seed;
		float time;
		int nMoves;
		Simulator::State result;
	};
public:
	Evaluator( const Config& config )
		:
		seed( config.GetSeed() )
	{
		std::mt19937 seed_gen( seed );
		for( int n = 0; n < config.GetNumberRuns() - 1; n++ )
		{
			simulations.push_back( GenerateSimulation( config,seed_gen() ) );
		}
		// stress
		auto& calias = config;
		{
			Config config = calias;
			config.goalMode = Config::GoalMode::Random;
			config.mapWidth = 1000;
			config.mapHeight = 1000;
			config.roomTries = (config.mapWidth + config.mapHeight) / 2;
			config.extraDoors = (config.mapWidth + config.mapHeight) / 2;
			config.maxMoves = config.mapWidth * config.mapHeight * 4;
			simulations.push_back( std::make_unique<HeadlessSimulator>( config,seed_gen() ) );
		}
	}
	void Update( MainWindow& wnd,float dt ) override
	{
		while( !IsFinished() && simulations.back()->Finished() )
		{
			const auto& s = *simulations.back();
			if (s.GetState() == Simulator::State::Failure)
			{
				s.map.Save("failmaze.txt");
			}
			
			results.push_back( {
				s.GetSeed(),
				s.GetWorkingTime(),
				s.GetMoveCount(),
				s.GetState()
			} );
			simulations.pop_back();
		}

		if( !IsFinished() )
		{
			simulations.back()->Update( wnd,dt );
		}
		else if( !written )
		{
			WriteResults();
			wnd.ShowMessageBox( L"Finished",L"Done!" );
			wnd.Kill();
		}
	}
	void Draw( Graphics& gfx ) const override
	{
		if( !IsFinished() )
		{
			simulations.back()->Draw( gfx );
		}
	}
	bool IsFinished() const
	{
		return simulations.empty();
	}
	void WriteResults()
	{
		std::ofstream file( "results.txt" );
		file << "  Master seed: [" << seed << "]\n" <<
			    "=========================================" << std::endl;
		for( const auto& r : results )
		{
			file << std::endl << " [" << r.seed << "]\n";
			file << "Result: " << ((r.result == Simulator::State::Success) ? "Success\n" : "Failure\n");
			file << "Moves taken:" << r.nMoves << std::endl;
			file << "Time taken:" << r.time << std::endl;
		}

		// write totals
		const auto total_time = std::accumulate( results.begin(),results.end(),0.0f,
			[]( float s,const Result& r )
			{
				return s + r.time;
			}
		);
		const auto total_moves = std::accumulate( results.begin(),results.end(),0,
			[]( int s,const Result& r )
			{
				return s + r.nMoves;
			}
		);
		const auto nSuccess = std::count_if( results.begin(),results.end(),
			[]( const Result& r )
			{
				return r.result == Simulator::State::Success;
			}
		);

		file << std::endl << std::endl
			<< "========================================\n"
			<< "Success Rate: " << nSuccess << "/" << results.size() << std::endl
			<< "Total Moves: " << total_moves << std::endl
			<< "Total Time: " << total_time;

		written = true;
	}
private:
	std::unique_ptr<Simulator> GenerateSimulation( Config config,unsigned int seed )
	{
		std::mt19937 param_gen( seed );
		std::discrete_distribution<> spawn_d = { 10,45,35,5,5 };
		std::uniform_int_distribution<int> size_d( 20,300 );
		config.goalMode = (Config::GoalMode)spawn_d( param_gen );
		config.mapWidth = size_d( param_gen );
		config.mapHeight = size_d( param_gen );
		config.roomTries = (config.mapWidth * config.mapHeight) / 6000;
		config.extraDoors = (config.mapWidth + config.mapHeight) / 2;
		config.maxMoves = config.mapWidth * config.mapHeight * 4;
		return std::make_unique<HeadlessSimulator>( config,seed );
	}
private:
	unsigned int seed;
	bool written = false;
	std::vector<std::unique_ptr<Simulator>> simulations;
	std::vector<Result> results;
};