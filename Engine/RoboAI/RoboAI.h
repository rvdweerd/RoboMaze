#pragma once
#include "..\Robo.h"
#include "..\DebugControls.h"
#include <random>
#include <deque>
#include <stack>
#include <unordered_set>
#include <queue>

enum class RoboDir
{
	NORTH=0,
	EAST,
	SOUTH,
	WEST,
	count
};
RoboDir OppositeDir(RoboDir d)
{
	return RoboDir(((int)d + 2) % 4);
}
struct RoboPosDir // Data structure for tracking the state (position & orientation) on our cache (/exploration) fieldMap
{
	int posIndex = 0; // index on our cache field fieldMap
	RoboDir dir = RoboDir::EAST; // orientation
};
size_t CombineHash(size_t h1, size_t h2)
{
	h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
	return h1;
}
namespace std
{
	template <> struct hash<RoboPosDir>
	{
		size_t operator()(const RoboPosDir& rPosDir) const
		{
			std::hash<int> hasher;
			auto hashx = hasher(rPosDir.posIndex);
			auto hashy = hasher((int)rPosDir.dir);
			return CombineHash(hashx, hashy);
		}
	};
	template <> struct equal_to<RoboPosDir>
	{
		bool operator()(const RoboPosDir& lhs, const RoboPosDir& rhs) const
		{
			return lhs.posIndex == rhs.posIndex && (int)lhs.dir == (int)rhs.dir;
		}
	};
}

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
		vo = -v;
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
		vo = -v;
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
class RoboAI_rvdw
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	RoboAI_rvdw()
	{}
	static constexpr bool implemented = false;
	Action Plan(std::array<TT, 3> view)
	{
		if (view[1] == TT::Wall)
		{
			if (coinflip(rng))
			{
				return Action::TurnRight;
			}
			else
			{
				return Action::TurnLeft;
			}
		}
		//dc.MarkAt(dc.GetRobotPosition(), { Colors::Green,32u });

		return Action::MoveForward;
	}

private:
	//DebugControls& dc;
	std::mt19937 rng = std::mt19937(std::random_device{}());
	std::bernoulli_distribution coinflip;
};




// demo of how to use DebugControls for visualization

class RoboAIDebug_rvdw
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	RoboAIDebug_rvdw(DebugControls& dc)
		:
		dc(dc)
	{
		visited.insert(roboPosDir.posIndex);		// Starting position is visited by definition
		fieldMap[roboPosDir.posIndex] = TT::Floor;	// Starting position is a Floor tile by definition
	}
	static constexpr bool implemented = true;
	Action Plan(std::array<TT, 3> view)
	{
		RecordFieldView(view);
		if (!instructionQueue.empty())
		{
			Action nextAction = instructionQueue.front(); instructionQueue.pop();
			switch (nextAction)
			{
			case Action::TurnLeft:
				FieldMapping_TurnLeft();
				return nextAction;
				break;
			case Action::TurnRight:
				FieldMapping_TurnRight();
				return nextAction;
				break;
			case Action::MoveForward:
				//auto it = fieldMap.find(GetForwardFieldIndex());
				if (fieldMap[GetForwardFieldIndex()] == TT::Floor || fieldMap[GetForwardFieldIndex()] == TT::Goal)
				{
					path.push_back( std::pair<int,RoboDir>{ roboPosDir.posIndex,roboPosDir.dir } );
					FieldMapping_MoveForward();
					dc.MarkAt(dc.GetRobotPosition(), { Colors::Green,32u });
					return nextAction;
				}
				break;
			default:
				assert(false);
			}
		}
		int nNewPositions = AddCandidateNeighborsToStack();
		std::pair<int,RoboDir> targetPos = stack.top(); stack.pop();
		if (nNewPositions > 0)
		{
			BuildInstructionQueue_MoveToNext(targetPos);
			visited.insert(targetPos.first);
		}
		else
		{
			std::pair<int, RoboDir> previousPos;
			do
			{
				//take one step back
				previousPos = { path.back().first,OppositeDir(roboPosDir.dir) };
				path.erase(path.end() - 1);
				BuildInstructionQueue_MoveToNext(previousPos);
			} while (!IsNeighbor(previousPos.first, targetPos.first)); // track back until new targetpos is 
		}
		if (!instructionQueue.empty())
		{
			Action nextAction = instructionQueue.front(); instructionQueue.pop();
			switch (nextAction)
			{
			case Action::TurnLeft:
				FieldMapping_TurnLeft();
				break;
			case Action::TurnRight:
				FieldMapping_TurnRight();
				break;
			case Action::MoveForward:
				FieldMapping_MoveForward();
				dc.MarkAt(dc.GetRobotPosition(), { Colors::Green,32u });
				break;
			default:
				assert(false);
			}
			return nextAction;
		}
		return Action::Done;
	}
private: //Field info
	void FieldMapping_TurnRight()
	{
		switch (roboPosDir.dir)
		{
		case RoboDir::EAST:
			roboPosDir.dir = RoboDir::SOUTH;
			break;
		case RoboDir::WEST:
			roboPosDir.dir = RoboDir::NORTH;
			break;
		case RoboDir::NORTH:
			roboPosDir.dir = RoboDir::EAST;
			break;
		case RoboDir::SOUTH:
			roboPosDir.dir = RoboDir::WEST;
			break;
		default:
			assert(false);
		}
	}
	void FieldMapping_TurnLeft()
	{
		switch (roboPosDir.dir)
		{
		case RoboDir::EAST:
			roboPosDir.dir = RoboDir::NORTH;
			break;
		case RoboDir::WEST:
			roboPosDir.dir = RoboDir::SOUTH;
			break;
		case RoboDir::NORTH:
			roboPosDir.dir = RoboDir::WEST;
			break;
		case RoboDir::SOUTH:
			roboPosDir.dir = RoboDir::EAST;
			break;
		default:
			assert(false);
		}
	}
	void FieldMapping_MoveForward()
	{
		switch (roboPosDir.dir)
		{
		case RoboDir::EAST:
			roboPosDir.posIndex++;
			break;
		case RoboDir::WEST:
			roboPosDir.posIndex--;
			break;
		case RoboDir::NORTH:
			roboPosDir.posIndex -= fieldWidth;
			break;
		case RoboDir::SOUTH:
			roboPosDir.posIndex += fieldWidth;
			break;
		default:
			assert(false);
		}
	}
	int GetForwardFieldIndex()
	{
		switch (roboPosDir.dir)
		{
		case RoboDir::EAST:
			return roboPosDir.posIndex+1;
			break;
		case RoboDir::WEST:
			return roboPosDir.posIndex-1;
			break;
		case RoboDir::NORTH:
			return roboPosDir.posIndex - fieldWidth;
			break;
		case RoboDir::SOUTH:
			return roboPosDir.posIndex + fieldWidth;
			break;
		default:
			assert(false);
		}
	}
	void RecordFieldView(const std::array<TT, 3>& view)
	{
		std::array<int, 3> visibleCellIndices;
		switch (roboPosDir.dir)
		{
		case RoboDir::EAST:
			visibleCellIndices[0] = roboPosDir.posIndex - fieldWidth + 1;
			visibleCellIndices[1] = roboPosDir.posIndex + 1;
			visibleCellIndices[2] = roboPosDir.posIndex + fieldWidth + 1;
			break;
		case RoboDir::WEST:
			visibleCellIndices[0] = roboPosDir.posIndex - fieldWidth - 1;
			visibleCellIndices[1] = roboPosDir.posIndex - 1;
			visibleCellIndices[2] = roboPosDir.posIndex + fieldWidth - 1;
			break;
		case RoboDir::NORTH:
			visibleCellIndices[0] = roboPosDir.posIndex - fieldWidth - 1;
			visibleCellIndices[1] = roboPosDir.posIndex - fieldWidth;
			visibleCellIndices[2] = roboPosDir.posIndex - fieldWidth + 1;
			break;
		case RoboDir::SOUTH:
			visibleCellIndices[0] = roboPosDir.posIndex + fieldWidth + 1;
			visibleCellIndices[1] = roboPosDir.posIndex + fieldWidth;
			visibleCellIndices[2] = roboPosDir.posIndex + fieldWidth - 1;
			break;
		default:
			assert(false);
		}
		for (size_t i = 0; i < 3; i++)
		{
			if (fieldMap.find(visibleCellIndices[i]) == fieldMap.end())
			{
				fieldMap[visibleCellIndices[i]] = view[i];
			}
			else
			{
				assert( fieldMap[visibleCellIndices[i]] == view[i]); // Check if field informatino is consistent with earlier observation
			}
		}
	}
	bool IsNeighbor(int index1, int index2)
	{
		int difference = abs(index1 - index2);
		return (difference == 1 || difference == fieldWidth);
	}
	static constexpr int maxFieldSideLength = 1000;
	static constexpr int fieldWidth = 2 * maxFieldSideLength + 1;
	static constexpr int fieldHeight = fieldWidth;
	static constexpr int nField = fieldWidth * fieldHeight;
private: //Position info
	RoboPosDir roboPosDir = { (fieldWidth / 2) * (1 + fieldWidth) , RoboDir::EAST };
	std::unordered_map<int, TT> fieldMap;
private: //Exploration control
	int AddCandidateNeighborsToStack()
	{
		int count = 0;
		std::array<int, 4> indicesNESW = {	roboPosDir.posIndex - fieldWidth, 
											roboPosDir.posIndex + 1, 
											roboPosDir.posIndex + fieldWidth,
											roboPosDir.posIndex - 1 };
		for (size_t i = 0; i < 4; i++)
		{
			int index = indicesNESW[i];
			auto it = fieldMap.find(index);
			if (it == fieldMap.end())
			{
				stack.push({ index , RoboDir(i)});
				count++;
			}
			else if ((it->second == TT::Floor || it->second == TT::Goal) && // neighbor is accessible
				visited.find(index) == visited.end())						// neighbor hasn't been visited before
			{
				stack.push({ index, RoboDir(i) });
				count++;
			}
		}
		return count;
	}
	void BuildInstructionQueue_MoveToNext(const std::pair<int, RoboDir>& targetPosIndex)
	{
		int option = (((int)targetPosIndex.second - (int)roboPosDir.dir) + 4) % 4; // Modular arithmetic to turn Robot towards target position
		switch (option)
		{
		case 2:
			instructionQueue.push(Action::TurnLeft);
			instructionQueue.push(Action::TurnLeft);
			instructionQueue.push(Action::MoveForward);
			break;
		case 1:
			instructionQueue.push(Action::TurnRight);
			instructionQueue.push(Action::MoveForward);
			break;
		case 3:
			instructionQueue.push(Action::TurnLeft);
			instructionQueue.push(Action::MoveForward);
			break;
		case 0:
			instructionQueue.push(Action::MoveForward);
			break;
		default:
			assert(true);
			break;
		}
	}
	std::stack<std::pair<int,RoboDir>> stack;
	std::unordered_set<int> visited;
	std::vector<std::pair<int,RoboDir>> path;
	std::queue<Action> instructionQueue;
private: //Support data
	DebugControls& dc;
	//std::mt19937 rng = std::mt19937(std::random_device{}());
	//std::bernoulli_distribution coinflip;
};
// if you name your classes different than RoboAI/RoboAIDebug, use typedefs like these
typedef RoboAI_rvdw RoboAI;
typedef RoboAIDebug_rvdw RoboAIDebug;

// TODO: Fix performance for large max map size (block-clean min-max)
// TODO: test perf walk-clean vs. block clean vs. map
// TODO: implement non-slide (turn counting) dijkturn with map