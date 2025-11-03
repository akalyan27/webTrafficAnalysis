/*
PlayerCommand.h
Defines the task object for the SafeQueue, representing a single player action.
*/

#pragma once
#include <chrono>

enum class ActionType {
    FLAP,
    NONE
};

struct PlayerCommand {
    int player_id = 0;
    ActionType type = ActionType::NONE;
    std::chrono::high_resolution_clock::time_point timestamp;
    
    // Default constructor
    PlayerCommand() = default;
    
    // Move constructor
    PlayerCommand(PlayerCommand&&) = default;
    
    // Move assignment
    PlayerCommand& operator=(PlayerCommand&&) = default;
    
    // Copy constructor (needed for queue operations)
    PlayerCommand(const PlayerCommand&) = default;
    
    // Copy assignment (needed for queue operations)
    PlayerCommand& operator=(const PlayerCommand&) = default;
};
