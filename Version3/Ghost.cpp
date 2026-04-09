#include <random>
#include "Ghost.h"
#include "gl_frontEnd.h"

const std::vector<TileType> desired_types({Tile_Floor, Tile_Dot, Tile_PowerUp});

std::random_device randDev;
std::default_random_engine randEngine(randDev());

Ghost::Ghost() = default;

void determineTarget(const Ghost& ghost, const GameWorld& gw, int& x, int& y)
{
    x = y = 0;

    //AI of Blinky (go to pacman)
    if (ghost.ai == AI_ChasePacman && gw.pacman != nullptr)
    {
        x = gw.pacman->col;
        y = gw.pacman->row;
    }
    else if (ghost.ai == AI_InterceptPacman && gw.pacman != nullptr)
    {
        int tx = 0, ty = 0;
        gw_dirToXY(gw.pacman->dir, tx, ty);
        x = gw.pacman->col + (tx * 4);
        y = gw.pacman->row + (ty * 4);
    }
    else if (ghost.ai == AI_RandomAndDumb)
    {
        Direction free_dirs[NUM_DIRECTIONS];
        int dirCount = 0, tx = 0, ty = 0;;

        gw_getValidDirectionsMulti(ghost.col, ghost.row, desired_types, free_dirs, dirCount);
        if (dirCount > 0) {
			std::uniform_int_distribution<int> randomDir(0, dirCount-1);

			gw_dirToXY(free_dirs[randomDir(randEngine)], tx, ty);
        }
        x = ghost.col + tx;
        y = ghost.row + ty;
    }
}

bool aiActive(const GameWorld& gw, const GhostAI& ai)
{
    bool a = false;
    if (gw.pacman != nullptr)
    {
        switch(ai)
        {
        case AI_ChasePacman: a = true; break;
        case AI_InterceptPacman: a = (gw.pacman->squares_traveled >= 32); break;
        case AI_ChaseGhost: a = (gw.pacman->squares_traveled >= 64); break;
        case AI_RandomAndDumb: a = (gw.pacman->squares_traveled >= 90); break;
        default: break;
        }
    }
    return a;
}

Direction getDirection(const Ghost& ghost, const GameWorld& gw)
{
    Direction requested_dir = NUM_DIRECTIONS;
    const TileType current_tile = gw.grid[ghost.row][ghost.col];

    if (!aiActive(gw, ghost.ai))
        return requested_dir;

    //determine the cell we are standing on
    if (current_tile == Tile_Jail)
    {
        if (gw_tileAt(ghost.col, ghost.row + 1) == Tile_GhostGate) requested_dir = NORTH;
        else if (gw_tileAt(ghost.col + 1, ghost.row) == Tile_Jail) requested_dir = EAST;
        else if (gw_tileAt(ghost.col - 1, ghost.row) == Tile_Jail) requested_dir = WEST;
    }
    else if (current_tile == Tile_GhostGate)
        requested_dir = NORTH;
    else if (current_tile != Tile_Wall)
    {
        Direction free_dirs[NUM_DIRECTIONS];
        Direction good_dirs[NUM_DIRECTIONS];
        int dirCount = 0, goodDir_count = 0, iter = 0;

        //choose a direction that is non-opposite of the current direction
        gw_getValidDirectionsMulti(ghost.col, ghost.row, desired_types, free_dirs, dirCount);
        for (int i = 0; i < dirCount; i++)
        {
            if (ghost.dir == NUM_DIRECTIONS || !gw_dirIsOpposite(ghost.dir, free_dirs[i]))
            {
                good_dirs[iter] = free_dirs[i];
                iter++;
                goodDir_count++;
            }
        }

        //now determine which direction is best
        if (goodDir_count <= 0)
            requested_dir = NUM_DIRECTIONS;
        else if (goodDir_count == 1)
            requested_dir = good_dirs[0];
        else
        {
            int target_x = 0, target_y = 0;
            int tx, ty;

            int closest_distance = ((gw.numCols * gw.numCols) + (gw.numRows * gw.numRows));
            determineTarget(ghost, gw, target_x, target_y);

            //Find the closest direction to the desired target
            for (int i = 0; i < goodDir_count; i++)
            {
                gw_dirToXY(good_dirs[i], tx, ty); //determine the change in X and Y based on the direction
                const int distance = distanceSquaredI(ghost.col + tx, ghost.row + ty, target_x, target_y);
                if (distance <= closest_distance)
                {
                    closest_distance = distance;
                    requested_dir = good_dirs[i];
                }
            }

        }
    }

    return requested_dir;
}

void Ghost::ghostAI(GameWorld& gw)
{
    // Lock the ghosts prior to access
    gw.ghost_array_mutex.lock();
    // Read relevant ghost information
    const bool fright = this->frightened;
    const int f_time = this->fright_timer;
    const Direction direction = this->dir;
    const int prev_row = row, prev_col = col;
    gw.ghost_array_mutex.unlock();

    if (fright)
    {
        if (f_time > 0)
        {
            // Acquire lock on ghost info, then update ghost's fright timer
            gw.ghost_array_mutex.lock();
            fright_timer--;
            gw.ghost_array_mutex.unlock();
        }
        else
        {
            // Acquire lock on ghost info, then update ghost fright state
            gw.ghost_array_mutex.lock();
            frightened = false;
            gw.ghost_array_mutex.unlock();
        }
    }

    //move the ghost if we found a direction
    if (direction != NUM_DIRECTIONS)
    {
        int x = 0, y = 0;
        gw_dirToXY(direction, x, y); //determine the change in X and Y based on the direction

        //determine if the desired position is a free space if not find a new direction to go
        if (gw_isFreeWithWrap(prev_col + x, prev_row + y) || gw_tileAt(prev_col, prev_row + 1) == Tile_GhostGate)
        {
            //move column or X position
            gw.ghost_array_mutex.lock();
            col_percentage += x * speed;
            row_percentage += y * speed;
            gw.ghost_array_mutex.unlock();
		}
        else
        {
            gw.ghost_array_mutex.lock();
            dir = getDirection(*this, gw);
            gw.ghost_array_mutex.unlock();
        }
    }
    else
    {
        gw.ghost_array_mutex.lock();
        dir = getDirection(*this, gw);
        gw.ghost_array_mutex.unlock();
    }

    gw.ghost_array_mutex.lock();
    //keep moving the ghost if he is stuck between two grid cells and handles grid wrapping
    handleEntityMovement(row, row_percentage, col, col_percentage);
    gw.ghost_array_mutex.unlock();

    gw.ghost_array_mutex.lock();
    //if the position has changed checked to see if we reached an intersection and change directions if this is the case
    if (row != prev_row || col != prev_col)
    {
        const TileType tt = gw_tileAt(col, row);
        if (gw_isIntersection(col, row) || tt == Tile_Jail || tt == Tile_GhostGate)
            dir = getDirection(*this, gw);
    }
    gw.ghost_array_mutex.unlock();
}
