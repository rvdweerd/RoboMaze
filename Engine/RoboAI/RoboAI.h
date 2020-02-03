#pragma once
#include "..\Robo.h"
#include "..\DebugControls.h"
#include <random>
#include <deque>
#include <stack>
#include <unordered_set>
#include <set>
#include <map>
#include <queue>
#include <fstream>

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
		dc(dc),
		out_file("saved_field.txt")
	{
		visited.insert(roboPosDir.posIndex);		// Starting position is visited by definition
		fieldMap[roboPosDir.posIndex] = TT::Floor;	// Starting position is a Floor tile by definition
		path.push_back(roboPosDir);					// Path vector starts with startposition
	}
	static constexpr bool implemented = true;
	Action Plan(std::array<TT, 3> view)
	{
		RecordFieldView(view);
		if (!instructionQueue.empty())
		{
			Robo::Action nextaction = instructionQueue.front(); instructionQueue.pop();
			if (nextaction == Robo::Action::Done) return nextaction;
			if (nextaction == Robo::Action::MoveForward)
			{
				int iForward = GetForwardFieldIndex();
				assert(fieldMap.find(iForward) != fieldMap.end());
				//assert(IsUFG(iForward));
				if (!IsUFG(iForward))
				{
					path.erase(path.end() - 1);
				}
				else
				{
					if (fieldMap[iForward] == TT::Goal)
					{
						while (!instructionQueue.empty()) instructionQueue.pop();
						instructionQueue.push(Robo::Action::Done);
					}
					FieldMapping_MoveForward();
					//path.push_back(roboPosDir); // NEEDS A FIX
					return nextaction;
				}
			}
			else if (nextaction == Robo::Action::TurnLeft)
			{
				FieldMapping_TurnLeft();
				return nextaction;
			}
			else if (nextaction == Robo::Action::TurnRight)
			{
				FieldMapping_TurnRight();
				return nextaction;
			}
		}
		visited.insert(roboPosDir.posIndex);
		int n = AddCandidateNeighborsToStack();
		if (stack.empty()) return Robo::Action::Done;
		int target;
		int returnPos;
		if (n > 0) // Move ahead - compose instructions
		{
			target = stack.top().first; stack.pop();
			if (IsUFG(target) && IsNeighbor(target))
			{
				BuildInstructionQueue_MoveToAdjacentCell(target);
			}
		}
		else // Track Back - compose instructions
		{
			do
			{
				target = stack.top().first;
				returnPos = stack.top().second; stack.pop();
			} while (!IsUFG(target));
			
			RoboDir shadowDir = roboPosDir.dir;
			while (path.back().posIndex != returnPos)
			{
				RoboDir backwardDir = OppositeDir(path.back().dir);
				BuildInstructionQueue_BackTrack(shadowDir, backwardDir);
				path.erase(path.end() - 1);
				if (path.size() == 0)
				{
					return Robo::Action::Done; // We can't track back any further: no target found
				}
			}
			BuildInstructionQueue_BackTrack(shadowDir,GetTargetDirection(target,path.back().posIndex));
		}
		assert(!instructionQueue.empty());
		Robo::Action nextaction = instructionQueue.front(); instructionQueue.pop();
		if (nextaction == Robo::Action::Done) return nextaction;
		if (nextaction == Robo::Action::MoveForward)
		{
			int iForward = GetForwardFieldIndex();
			assert(IsUFG(iForward));
			assert(target == iForward);
			assert(fieldMap.find(iForward) != fieldMap.end());
			if (fieldMap[iForward] == TT::Goal)
			{
				while (!instructionQueue.empty()) instructionQueue.pop();
				instructionQueue.push(Robo::Action::Done);
			}
			FieldMapping_MoveForward();
			//path.push_back(roboPosDir); // NEEDS A FIX
			return nextaction;
		}
		else if (nextaction == Robo::Action::TurnLeft)
		{
			FieldMapping_TurnLeft();
			return nextaction;
		}
		else if (nextaction == Robo::Action::TurnRight)
		{
			FieldMapping_TurnRight();
			return nextaction;
		}
	}
private: //Field info
	void FieldMapping_TurnRight()
	{
		roboPosDir.dir = RoboDir(((int)roboPosDir.dir + 1) % 4);
	}
	void FieldMapping_TurnLeft()
	{
		roboPosDir.dir = RoboDir(((int)roboPosDir.dir + 3) % 4);
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
			visibleCellIndices[0] = roboPosDir.posIndex + fieldWidth - 1;
			visibleCellIndices[1] = roboPosDir.posIndex - 1;
			visibleCellIndices[2] = roboPosDir.posIndex - fieldWidth - 1;
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
				if (fieldMap.size() < 85)
				{
					out_file << visibleCellIndices[i];
					out_file << ",";
					out_file << (int)fieldMap[visibleCellIndices[i]];
					out_file << "\n";
				}
				else
				{
					out_file.close();
				}
			}
			else
			{
				assert( fieldMap[visibleCellIndices[i]] == view[i]); // Check if field informatino is consistent with earlier observation
			}
		}
	}
	bool IsNeighbor(const int& target) const
	{
		int difference = abs(target - roboPosDir.posIndex);
		return (difference == 1 || difference == fieldWidth);
	}
	bool IsUFG(const int& index)
	{
		auto it = fieldMap.find(index);
		if (it == fieldMap.end())
		{
			return true;
		}
		else if (it->second == TT::Floor || it->second == TT::Goal)
		{
			 return true;
		}
		return false;
	}
	RoboDir GetTargetDirection(const int& target, const int& origin) const
	{
		int diff = target - origin;
		switch (diff)
		{
		case 1:
			return RoboDir::EAST;
			break;
		case -1:
			return RoboDir::WEST;
			break;
		case fieldWidth:
			return RoboDir::SOUTH;
			break;
		case -fieldWidth:
			return RoboDir::NORTH;
			break;
		default:
			assert(false);
		}
		return RoboDir::count;
	}
	static constexpr int maxFieldSideLength = 1000;
	static constexpr int fieldWidth = 2 * maxFieldSideLength + 1;
	static constexpr int fieldHeight = fieldWidth;
	static constexpr int nField = fieldWidth * fieldHeight;
private: //Position info
	RoboPosDir roboPosDir = { (fieldWidth / 2) * (1 + fieldWidth) , RoboDir::EAST };
	//std::unordered_map<int, TT> fieldMap;
	std::map<int, TT> fieldMap;
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
			if (it == fieldMap.end()) // unexplored
			{
				stack.push({ index , roboPosDir.posIndex});
				count++;
			}
			else if ((it->second == TT::Floor || it->second == TT::Goal) && // neighbor is accessible
				(visited.find(index) == visited.end()) )					// neighbor hasn't been visited before
			{
				stack.push({ index, roboPosDir.posIndex});
				count++;
			}
		}
		return count;
	}
	void BuildInstructionQueue_MoveToAdjacentCell(const int& target) 
	{
		if (!IsNeighbor(target)) assert(false);
		RoboDir targetDir = GetTargetDirection(target, roboPosDir.posIndex);
		int option = (((int)targetDir - (int)roboPosDir.dir) + 4) % 4; // Modular arithmetic to turn Robot towards target position
		switch (option)
		{
		case 2:
			instructionQueue.push(Action::TurnLeft);
			instructionQueue.push(Action::TurnLeft);
			break;
		case 1:
			instructionQueue.push(Action::TurnRight);
			break;
		case 3:
			instructionQueue.push(Action::TurnLeft);
			break;
		case 0:
			break;
		default:
			assert(false);
			break;
		}
		instructionQueue.push(Action::MoveForward);
		path.push_back({ target,targetDir });
		return;
	}
	void BuildInstructionQueue_BackTrack(RoboDir& shadowDir, const RoboDir& backwardDir) 
	{
		int option = (((int)backwardDir - (int)shadowDir) + 4) % 4; // Modular arithmetic to turn Robot towards target position
		switch (option)
		{
		case 2:
			instructionQueue.push(Action::TurnLeft);
			instructionQueue.push(Action::TurnLeft);
			shadowDir = RoboDir(((int)shadowDir + 2) % 4);
			break;
		case 1:
			instructionQueue.push(Action::TurnRight);
			shadowDir = RoboDir(((int)shadowDir + 1) % 4);
			break;
		case 3:
			instructionQueue.push(Action::TurnLeft);
			shadowDir = RoboDir(((int)shadowDir + 3) % 4);
			break;
		case 0:
			break;
		default:
			assert(false);
			break;
		}
		instructionQueue.push(Action::MoveForward);
		return;
	}
	std::stack<std::pair<int,int>> stack;
	std::unordered_set<int> visited;
	std::vector<RoboPosDir> path;
	std::queue<Action> instructionQueue;
private: //Support data
	DebugControls& dc;
	std::ofstream out_file;
	//std::mt19937 rng = std::mt19937(std::random_device{}());
	//std::bernoulli_distribution coinflip;
};
// if you name your classes different than RoboAI/RoboAIDebug, use typedefs like these
typedef RoboAI_rvdw RoboAI;
typedef RoboAIDebug_rvdw RoboAIDebug;

// TODO: Fix performance for large max map size (block-clean min-max)
// TODO: test perf walk-clean vs. block clean vs. map
// TODO: implement non-slide (turn counting) dijkturn with map
