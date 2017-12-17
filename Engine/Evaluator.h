#pragma once

#include "Simulator.h"
#include "Config.h"
#include "Graphics.h"
#include "Gameable.h"
#include <vector>
#include <memory>
#include <fstream>

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
		simulations.push_back( std::make_unique<DebugSimulator>( config,seed_gen() ) );
		simulations.push_back( std::make_unique<DebugSimulator>( config,seed_gen() ) );
		simulations.push_back( std::make_unique<DebugSimulator>( config,seed_gen() ) );
	}
	void Update( MainWindow& hwnd,float dt ) override
	{
		while( !IsFinished() && simulations.back()->Finished() )
		{
			const auto& s = *simulations.back();
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
			simulations.back()->Update( hwnd,dt );
		}
		else if( !written )
		{
			WriteResults();
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
		written = true;
	}
private:
	unsigned int seed;
	bool written = false;
	std::vector<std::unique_ptr<Simulator>> simulations;
	std::vector<Result> results;
};