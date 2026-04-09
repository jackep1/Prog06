#ifndef GHOST_H
#define GHOST_H

#include <string>
#include "ColorRGB.h"
#include "Direction.h"

struct GameWorld;

enum GhostAI
{
    AI_None, //no movement nothing
    AI_ChasePacman,
    AI_InterceptPacman,
    AI_ChaseGhost,
    AI_RandomAndDumb,
};

struct Ghost
{
    int row{}; // (row, col) = position on the grid
    int col{};
    int speed{}; // ghost speed percentage per frame (should always be less than 100 but greater than 0)
    int row_percentage{}; // position percentage between two cells (value is between -100 and +100)
    int col_percentage{};
    int exit_counter{};
    Direction dir = NO_DIRECTION; //the direction the ghost is traveling; set to NUM_DIRECTIONS for no traveling at all
    ColorRGB color; // the color of the ghost when in the "normal" state
    bool frightened{}; //false indicates the ghost is after the player true indicates the ghost is running away
    int fright_timer{}; //the number of frames that the ghost will be frightened
    std::string name; //the name of the ghost (blinky, clyde, ect...)
    GhostAI ai = AI_None; //the AI that this ghost uses

    Ghost();
    void ghostAI(const GameWorld& gw);
};

#endif // GHOST_H
