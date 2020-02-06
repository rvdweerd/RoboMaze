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

// returns angle in units of pi/2 (90deg you pleb)
inline int GetAngleBetween(const Direction& d1, const Direction& d2)
{
	constexpr std::array<int, 4> map = { 0,2,3,1 };
	return map[d2.GetIndex()] - map[d1.GetIndex()];
}
// angle is in units of pi/2 (90deg you pleb)
inline Vei2 GetRotated90(const Vei2& v, int angle)
{
	Vei2 vo;
	switch (angle)
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
		assert("Bad angle in Dir rotation!" && false);
	}
	return vo;
}
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

// test classes
class RoboAI_rvdw
{
	using TT = TileMap::TileType;
	using Action = Robo::Action;
public:
	RoboAI_rvdw()
	{
		visited.insert(roboPosDir.posIndex);		// Starting position is visited by definition
		//fieldMap[roboPosDir.posIndex] = TT::Floor;	// Starting position is a Floor tile by definition
		path.push_back(roboPosDir);					// Path vector starts with startposition
	}
	int SquareDanceToggle = 1;
	std::queue<Robo::Action> ReturnFromSquareDanceQueue;
	//static constexpr bool implemented = false;
	int lastPos = 0;
	Action Plan(std::array<TT, 3> view)
	{
		RecordFieldView(view);
		if (SquareDanceToggle)
		{
			if (!instructionQueue.empty()) return ProcessInstruction();
			switch (SquareDanceToggle)
			{
			case 1:
				if (fieldMap[GetForwardFieldIndex()] == TT::Goal)
				{
					instructionQueue.push(Robo::Action::MoveForward);
					instructionQueue.push(Robo::Action::Done);
					SquareDanceToggle = 3;
				}
				else if (fieldMap[GetForwardFieldIndex()] == TT::Floor)
				{
					instructionQueue.push(Robo::Action::MoveForward);
					instructionQueue.push(Robo::Action::TurnLeft);
					instructionQueue.push(Robo::Action::TurnLeft);
					//ReturnFromSquareDanceQueue.push(Robo::Action::TurnLeft);
					//ReturnFromSquareDanceQueue.push(Robo::Action::TurnLeft);
					SquareDanceToggle = 2;
				}
				else
				{
					instructionQueue.push(Robo::Action::TurnLeft);
					ReturnFromSquareDanceQueue.push(Robo::Action::TurnRight);
				}
				break;
			case 2:
				instructionQueue.push(Robo::Action::MoveForward);
				if (fieldMap[GetForwardFieldIndex()] == TT::Goal)
				{
					instructionQueue.push(Robo::Action::Done);
				}
				else
				{
					while (!ReturnFromSquareDanceQueue.empty())
					{
						instructionQueue.push(ReturnFromSquareDanceQueue.front()); ReturnFromSquareDanceQueue.pop();
					}
					visited.erase(roboPosDir.posIndex);
				}
				SquareDanceToggle = 0;
				break;
			}
		}
		if (!instructionQueue.empty()) return ProcessInstruction();
		int n = AddCandidateNeighborsToStack();
		
		int target, returnPos;
		do
		{
			if (stack.empty())
			{
				return Robo::Action::Done;
			}
			target = stack.top().first;
			returnPos = stack.top().second; stack.pop();
		} while (!IsUFG(target) || visited.find(target) != visited.end());
		visited.insert(target);

		if (IsNeighbor(target)) // Move Ahead
		{
			RoboDir targetDir = BuildInstructionQueue_MoveToAdjacentCell(target);
			path.push_back({ target,targetDir });
		}
		else //backtrack
		{
			RoboDir shadowDir = roboPosDir.dir;
			if (path.size() == 0)
			{
				while (!instructionQueue.empty()) instructionQueue.pop();
				return Robo::Action::Done; // We can't track back any further: no target found
			}
			while (path.back().posIndex != returnPos)
			{
				RoboDir backwardDir = OppositeDir(path.back().dir);
				BuildInstructionQueue_BackTrack(shadowDir, backwardDir);
				path.erase(path.end() - 1);
				if (path.size() == 0)
				{
					while (!instructionQueue.empty()) instructionQueue.pop();
					return Robo::Action::Done; // We can't track back any further: no target found
				}
			}
			RoboDir targetDir = GetTargetDirection(target, path.back().posIndex);
			BuildInstructionQueue_BackTrack(shadowDir, targetDir);
			path.push_back({ target,targetDir });
		}
		assert(!instructionQueue.empty());
		if (!instructionQueue.empty()) return ProcessInstruction();
		assert(false);
		return Robo::Action::Done;
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
			return roboPosDir.posIndex + 1;
			break;
		case RoboDir::WEST:
			return roboPosDir.posIndex - 1;
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
		return -1;
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
			}
			else
			{
				assert(fieldMap[visibleCellIndices[i]] == view[i]); // Check if field informatino is consistent with earlier observation
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
	static constexpr int maxFieldSideLength = 1004;
	static constexpr int fieldWidth = 2 * maxFieldSideLength + 1;
	static constexpr int fieldHeight = fieldWidth;
	static constexpr int nField = fieldWidth * fieldHeight;
private: //Position info
	RoboPosDir roboPosDir = { (fieldWidth / 2) * (1 + fieldWidth) , RoboDir::EAST };
	std::map<int, TT> fieldMap;
private: //Exploration control
	int AddCandidateNeighborsToStack()
	{
		int count = 0;
		std::array<int, 4> indicesNESW = { roboPosDir.posIndex - fieldWidth,
											roboPosDir.posIndex + 1,
											roboPosDir.posIndex + fieldWidth,
											roboPosDir.posIndex - 1 };
		std::pair<int, int> elementToTop;
		bool swap = false;
		for (size_t i = 0; i < 4; i++)
		{
			int index = indicesNESW[i];
			auto it = fieldMap.find(index);
			if (it == fieldMap.end()) // unexplored
			{
				stack.push({ index , roboPosDir.posIndex });
				//visited.insert(index);
				count++;
			}
			else if ((it->second == TT::Floor || it->second == TT::Goal) && // neighbor is accessible
				(visited.find(index) == visited.end()))					// neighbor hasn't been visited before
			{
				if (visited.find(index) == visited.end())					// neighbor hasn't been visited before
				{
					stack.push({ index, roboPosDir.posIndex });
					//visited.insert(index);
					count++;
				}
				else if (index != lastPos)
				{
					elementToTop = { index, roboPosDir.posIndex };
					swap = true;
				}
			}
		}
		if (swap)
		{
			std::stack<std::pair<int, int>> cStack;
			while (!stack.empty())
			{
				if (stack.top() != elementToTop)
				{
					cStack.push(stack.top());
				}
				stack.pop();
			}
			while (!cStack.empty())
			{
				stack.push(cStack.top());
				cStack.pop();
			}
			stack.push(elementToTop);
		}
		return count;
	}
	RoboDir BuildInstructionQueue_MoveToAdjacentCell(const int& target)
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
		return targetDir;
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
	Robo::Action ProcessInstruction()
	{
		lastPos = roboPosDir.posIndex;
		Robo::Action nextaction = instructionQueue.front(); instructionQueue.pop();
		if (nextaction == Robo::Action::Done) return nextaction;
		if (nextaction == Robo::Action::MoveForward)
		{
			int iForward = GetForwardFieldIndex();
			if (!IsUFG(iForward))
			{
				// you can't move forward, turn left instead
				FieldMapping_TurnLeft();
				path.erase(path.end() - 1);
				return Robo::Action::TurnLeft;
			}
			//assert(target == iForward);
			assert(fieldMap.find(iForward) != fieldMap.end());
			if (fieldMap[iForward] == TT::Goal)
			{
				while (!instructionQueue.empty()) instructionQueue.pop();
				instructionQueue.push(Robo::Action::Done);
			}
			FieldMapping_MoveForward();
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
		assert(false);
		return nextaction;
	}
	std::stack<std::pair<int, int>> stack;
	std::unordered_set<int> visited;
	std::vector<RoboPosDir> path;
	std::queue<Action> instructionQueue;
};
class RoboAI_chili
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
		void SetType(TT type_in)
		{
			assert(type == TT::Invalid);
			type = type_in;
		}
		void ClearCost()
		{
			cost = std::numeric_limits<int>::max();
		}
		bool UpdateCost(int new_cost, const Vei2& new_prev = { -1,-1 })
		{
			if (new_cost < cost)
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
	RoboAI_chili()
		:
		nodes(gridWidth* gridHeight),
		pos(gridWidth / 2, gridHeight / 2), // start in middle
		visit_extent(-1, -1, -1, -1)
	{
		// prime the pathing pump
		path.push_back(pos + dir);
		ResetExtents(pos);
	}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = true;
	Action Plan(std::array<TT, 3> view)
	{
		// return DONE if standing on the goal
		if (At(pos).GetType() == TT::Goal)
		{
			return Action::Done;
		}
		// first update map
		UpdateMap(view);
		// check if we have revealed the target unknown square
		if (At(path.back()).GetType() != TT::Invalid)
		{
			// compute new path
			if (!ComputePath())
			{
				// if ComputePath() returns false, impossible to reach goal
				return Action::Done;
			}
		}
		// find our position in path sequence
		const auto i = std::find(path.begin(), path.end(), pos);
		// move to next position in sequence
		return MoveTo(*std::next(i));
	}
private:
	bool ComputePath()
	{
		// create priority queue for 'frontier'
		std::deque<Vei2> frontier;

		// clear costs
		for (Vei2 cp = visit_extent.TopLeft(); cp.y <= visit_extent.bottom; cp.y++)
		{
			for (cp.x = visit_extent.left; cp.x <= visit_extent.right; cp.x++)
			{
				At(cp).ClearCost();
			}
		}

		ResetExtents(pos);

		// clear path sequence
		path.clear();

		// add current tile to frontier to0 start the ball rollin
		At(pos).UpdateCost(0);
		frontier.emplace_back(pos);
		AdjustExtents(pos);

		while (!frontier.empty())
		{
			const auto base = frontier.front();
			frontier.pop_front();

			auto dir = Direction::Up();
			for (int i = 0; i < 4; i++, dir.RotateClockwise())
			{
				const auto nodePos = base + dir;
				auto& node = At(nodePos);
				if (node.GetType() == TT::Invalid || node.GetType() == TT::Goal)
				{
					// then construct path and return
					path.push_back(nodePos);
					auto trace = base;
					while (At(trace).GetPrev() != Vei2{ -1,-1 })
					{
						// add it to the path
						path.push_back(trace);
						// walk to next pos in path
						trace = At(trace).GetPrev();
					}
					// add it to the path
					path.push_back(trace);
					// gotta reverse that shit
					std::reverse(path.begin(), path.end());
					return true;
				}
				else if (node.GetType() == TT::Floor)
				{
					if (node.UpdateCost(At(base).GetCost() + 1, base))
					{
						frontier.push_back(nodePos);
						AdjustExtents(nodePos);
					}
				}
			}
		}
		// cannot reach goal
		return false;
	}
	void UpdateMap(const std::array<TT, 3>& view)
	{
		auto scan_pos = pos + dir + dir.GetRotatedCounterClockwise();
		const auto scan_delta = dir.GetRotatedClockwise();
		for (int i = 0; i < 3; i++, scan_pos += scan_delta)
		{
			auto& node = At(scan_pos);
			const auto type = node.GetType();
			if (type != TT::Invalid)
			{
				assert(type == view[i]);
				continue;
			}
			node.SetType(view[i]);
		}
	}
	Node& At(const Vei2& pos)
	{
		return nodes[pos.x + pos.y * gridWidth];
	}
	Action MoveTo(const Vei2& target)
	{
		const Direction delta = Direction(target - pos);
		if (delta == dir)
		{
			pos += dir;
			return Action::MoveForward;
		}
		else if (dir.GetRotatedClockwise() == delta)
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
	void AdjustExtents(const Vei2& pos)
	{
		visit_extent.left = std::min(pos.x, visit_extent.left);
		visit_extent.right = std::max(pos.x, visit_extent.right);
		visit_extent.top = std::min(pos.y, visit_extent.top);
		visit_extent.bottom = std::max(pos.y, visit_extent.bottom);
	}
	void ResetExtents(const Vei2& pos)
	{
		visit_extent = { pos.x + 1,pos.x - 1,pos.y + 1, pos.y - 1 };
	}
private:
	// grid dimensions 2x the max dimensions
	// (because we don't know where we start!)
	static constexpr int gridWidth = 2000;
	static constexpr int gridHeight = 2000;
	Vei2 pos;
	std::vector<Vei2> path;
	Direction dir = Direction::Up();
	std::vector<Node> nodes;
	// inclusive!
	RectI visit_extent;
};
// Debug  / visualization classes
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
		//fieldMap[roboPosDir.posIndex] = TT::Floor;	// Starting position is a Floor tile by definition
		path.push_back(roboPosDir);					// Path vector starts with startposition
	}
	int SquareDanceToggle = 1;
	std::queue<Robo::Action> ReturnFromSquareDanceQueue;
	static constexpr bool implemented = true;
	int lastPos = 0;
	Action Plan(std::array<TT, 3> view)
	{
		RecordFieldView(view);
		if (SquareDanceToggle)
		{
			if (!instructionQueue.empty()) return ProcessInstruction();
			switch (SquareDanceToggle)
			{
			case 1:
				if (fieldMap[GetForwardFieldIndex()] == TT::Goal)
				{
					instructionQueue.push(Robo::Action::MoveForward);
					instructionQueue.push(Robo::Action::Done);
					SquareDanceToggle = 3;
				}
				else if (fieldMap[GetForwardFieldIndex()] == TT::Floor)
				{
					instructionQueue.push(Robo::Action::MoveForward);
					instructionQueue.push(Robo::Action::TurnLeft);
					instructionQueue.push(Robo::Action::TurnLeft);
					//ReturnFromSquareDanceQueue.push(Robo::Action::TurnLeft);
					//ReturnFromSquareDanceQueue.push(Robo::Action::TurnLeft);
					SquareDanceToggle = 2;
				}
				else
				{
					instructionQueue.push(Robo::Action::TurnLeft);
					ReturnFromSquareDanceQueue.push(Robo::Action::TurnRight);
				}
				break;
			case 2:
				instructionQueue.push(Robo::Action::MoveForward);
				if (fieldMap[GetForwardFieldIndex()] == TT::Goal)
				{
					instructionQueue.push(Robo::Action::Done);
				}
				else
				{
					while (!ReturnFromSquareDanceQueue.empty()) 
					{
						instructionQueue.push(ReturnFromSquareDanceQueue.front()); ReturnFromSquareDanceQueue.pop();
					}
					visited.erase(roboPosDir.posIndex);
				}
				SquareDanceToggle = 0;
				break;
			}
		}
		if (!instructionQueue.empty()) return ProcessInstruction();
		dc.MarkAt(dc.GetRobotPosition(), { Colors::Green,32u });
		int n = AddCandidateNeighborsToStack();
		//if (nSteps > 2000)
		//{
		//	return Action::Done;
		//}
		//else
		{
			int target, returnPos;
			do
			{
				if (stack.empty())
				{
					return Action::Done;
				}
				target = stack.top().first;
				returnPos = stack.top().second; stack.pop();
			} while (!IsUFG(target) || visited.find(target)!=visited.end());
			visited.insert(target);
			if (IsNeighbor(target)) // Move Ahead
			{
				RoboDir targetDir = BuildInstructionQueue_MoveToAdjacentCell(target);
				path.push_back({ target,targetDir });
			}
			else
			{
				//backtrack
				RoboDir shadowDir = roboPosDir.dir;
				if (path.size() == 0)
				{
					while (!instructionQueue.empty()) instructionQueue.pop();
					return Robo::Action::Done; // We can't track back any further: no target found
				}
				while (path.back().posIndex != returnPos)
				{
					RoboDir backwardDir = OppositeDir(path.back().dir);
					BuildInstructionQueue_BackTrack(shadowDir, backwardDir);
					path.erase(path.end() - 1);
					if (path.size() == 0)
					{
						while (!instructionQueue.empty()) instructionQueue.pop();
						return Robo::Action::Done; // We can't track back any further: no target found
					}
				}
				RoboDir targetDir = GetTargetDirection(target, path.back().posIndex);
				BuildInstructionQueue_BackTrack(shadowDir, targetDir);
				path.push_back({ target,targetDir });
			}
		}
		assert(!instructionQueue.empty());
		if (!instructionQueue.empty()) return ProcessInstruction();
		dc.MarkAt(dc.GetRobotPosition(), { Colors::Green,32u });
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
	static constexpr int maxFieldSideLength = 1004;
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
		std::pair<int, int> elementToTop;
		bool swap = false;
		for (size_t i = 0; i < 4; i++)
		{
			int index = indicesNESW[i];
			auto it = fieldMap.find(index);
			if (it == fieldMap.end()) // unexplored
			{
				stack.push({ index , roboPosDir.posIndex});
				//visited.insert(index);
				count++;
			}
			else if (it->second == TT::Floor || it->second == TT::Goal) // neighbor is accessible
			{
				if (visited.find(index) == visited.end())					// neighbor hasn't been visited before
				{	
					stack.push({ index, roboPosDir.posIndex });
					//visited.insert(index);
					count++;
				}
				else if (index != lastPos)
				{
					elementToTop = { index, roboPosDir.posIndex };
					swap = true;
				}
			}
		}
		if (swap)
		{
			std::stack<std::pair<int, int>> cStack;
			while (!stack.empty())
			{
				if (stack.top() != elementToTop)
				{
					cStack.push(stack.top());
				}
				stack.pop();
			}
			while (!cStack.empty())
			{
				stack.push(cStack.top());
				cStack.pop();
			}
			stack.push(elementToTop);
		}
		return count;
	}
	RoboDir BuildInstructionQueue_MoveToAdjacentCell(const int& target)
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
		return targetDir;
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
	Robo::Action ProcessInstruction()
	{
		lastPos = roboPosDir.posIndex;
		Robo::Action nextaction = instructionQueue.front(); instructionQueue.pop();
		if (nextaction == Robo::Action::Done) return nextaction;
		if (nextaction == Robo::Action::MoveForward)
		{
			int iForward = GetForwardFieldIndex();
			if (!IsUFG(iForward))
			{
				// you can't move forward, turn left instead
				FieldMapping_TurnLeft();
				path.erase(path.end() - 1);
				return Robo::Action::TurnLeft;
			}
			//assert(target == iForward);
			assert(fieldMap.find(iForward) != fieldMap.end());
			if (fieldMap[iForward] == TT::Goal)
			{
				while (!instructionQueue.empty()) instructionQueue.pop();
				instructionQueue.push(Robo::Action::Done);
			}
			FieldMapping_MoveForward();
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
	std::stack<std::pair<int,int>> stack;
	std::unordered_set<int> visited;
	std::vector<RoboPosDir> path;
	std::queue<Action> instructionQueue;
private: //Support data
	DebugControls& dc;
	std::ofstream out_file;
};
class RoboAIDebug_chili
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
		void SetType(TT type_in)
		{
			assert(type == TT::Invalid);
			type = type_in;
		}
		void ClearCost()
		{
			cost = std::numeric_limits<int>::max();
		}
		bool UpdateCost(int new_cost, const Vei2& new_prev = { -1,-1 })
		{
			if (new_cost < cost)
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
	RoboAIDebug_chili(DebugControls& dc)
		:
		nodes(gridWidth* gridHeight),
		pos(gridWidth / 2, gridHeight / 2), // start in middle
		dc(dc),
		angle(GetAngleBetween(dir, dc.GetRobotDirection())),
		visit_extent(-1, -1, -1, -1)
	{
		// prime the pathing pump
		path.push_back(pos + dir);
		VisualInit();
		ResetExtents(pos);
	}
	// this signals to the system whether the debug AI can be used
	static constexpr bool implemented = true;
	Action Plan(std::array<TT, 3> view)
	{
		// return DONE if standing on the goal
		if (At(pos).GetType() == TT::Goal)
		{
			const auto lock = dc.AcquireGfxLock();
			ClearPathMarkings();
			return Action::Done;
		}
		// first update map
		UpdateMap(view);
		// check if we have revealed the target unknown square
		if (At(path.back()).GetType() != TT::Invalid)
		{
			// compute new path
			if (!ComputePath())
			{
				// if ComputePath() returns false, impossible to reach goal
				return Action::Done;
			}
		}
		// find our position in path sequence
		const auto i = std::find(path.begin(), path.end(), pos);
		// move to next position in sequence
		return MoveTo(*std::next(i));
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
		for (Vei2 cp = visit_extent.TopLeft(); cp.y <= visit_extent.bottom; cp.y++)
		{
			for (cp.x = visit_extent.left; cp.x <= visit_extent.right; cp.x++)
			{
				At(cp).ClearCost();
			}
		}

		ResetExtents(pos);

		// clear path sequence
		path.clear();

		// add current tile to frontier to0 start the ball rollin
		At(pos).UpdateCost(0);
		frontier.emplace_back(pos);
		AdjustExtents(pos);

		while (!frontier.empty())
		{
			const auto base = frontier.front();
			frontier.pop_front();

			auto dir = Direction::Up();
			for (int i = 0; i < 4; i++, dir.RotateClockwise())
			{
				const auto nodePos = base + dir;
				auto& node = At(nodePos);
				if (node.GetType() == TT::Invalid || node.GetType() == TT::Goal)
				{
					// found our target, first mark it
					dc.MarkAt(ToGameSpace(nodePos), { 96,96,0,0 });
					// then construct path and return
					path.push_back(nodePos);
					auto trace = base;
					while (At(trace).GetPrev() != Vei2{ -1,-1 })
					{
						// mark sequence for visualiztion yellow
						dc.MarkAt(ToGameSpace(trace), { 96,96,96,0 });
						// add it to the path
						path.push_back(trace);
						// walk to next pos in path
						trace = At(trace).GetPrev();
					}
					// mark sequence for visualiztion yellow
					dc.MarkAt(ToGameSpace(trace), { 96,96,96,0 });
					// add it to the path
					path.push_back(trace);
					// gotta reverse that shit
					std::reverse(path.begin(), path.end());
					return true;
				}
				else if (node.GetType() == TT::Floor)
				{
					if (node.UpdateCost(At(base).GetCost() + 1, base))
					{
						frontier.push_back(nodePos);
						AdjustExtents(nodePos);
						// mark each tile visited by my dik
						dc.MarkAt(ToGameSpace(nodePos), { Colors::Green,32 });
						dc.WaitOnTick();
					}
				}
			}
		}
		// cannot reach goal
		return false;
	}
	void UpdateMap(const std::array<TT, 3>& view)
	{
		auto scan_pos = pos + dir + dir.GetRotatedCounterClockwise();
		const auto scan_delta = dir.GetRotatedClockwise();
		for (int i = 0; i < 3; i++, scan_pos += scan_delta)
		{
			auto& node = At(scan_pos);
			const auto type = node.GetType();
			if (type != TT::Invalid)
			{
				assert(type == view[i]);
				continue;
			}
			node.SetType(view[i]);
			dc.ClearAt(ToGameSpace(scan_pos));
		}
	}
	Node& At(const Vei2& pos)
	{
		return nodes[pos.x + pos.y * gridWidth];
	}
	Action MoveTo(const Vei2& target)
	{
		const Direction delta = Direction(target - pos);
		if (delta == dir)
		{
			pos += dir;
			return Action::MoveForward;
		}
		else if (dir.GetRotatedClockwise() == delta)
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
	Vei2 ToGameSpace(const Vei2& pos_in) const
	{
		return GetRotated90(pos_in - pos, angle) + dc.GetRobotPosition();
	}
	Vei2 ToMapSpace(const Vei2& pos_in) const
	{
		return GetRotated90(pos_in - dc.GetRobotPosition(), -angle) + pos;
	}
	void VisualInit()
	{
		if (!visual_init_done)
		{
			auto l = dc.AcquireGfxLock();
			dc.MarkAll({ Colors::Black,96 });
			visual_init_done = true;
		}
	}
	void ClearPathMarkings()
	{
		dc.ForEach([this](const Vei2& p, DebugControls& dc)
			{
				if (At(ToMapSpace(p)).GetType() != TileMap::TileType::Invalid)
				{
					dc.ClearAt(p);
				}
			});
	}
	void AdjustExtents(const Vei2& pos)
	{
		visit_extent.left = std::min(pos.x, visit_extent.left);
		visit_extent.right = std::max(pos.x, visit_extent.right);
		visit_extent.top = std::min(pos.y, visit_extent.top);
		visit_extent.bottom = std::max(pos.y, visit_extent.bottom);
	}
	void ResetExtents(const Vei2& pos)
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
typedef RoboAI_rvdw RoboAI;
typedef RoboAIDebug_rvdw RoboAIDebug;
//typedef RoboAI_chili RoboAI;
//typedef RoboAIDebug_chili RoboAIDebug;
