#include <cmath>
#include "PacMan.h"
#include "glPlatform.h"
#include "gl_frontEnd.h"

PacMan::PacMan()
{
    for (bool & key : keys) { key = false; }
    speed = 8;
    row_percentage = col_percentage = 0;
    squares_traveled = 0;
    dir = EAST;
    requested_dir = NO_DIRECTION;
}

void PacMan::key_update(const unsigned char key, const bool keydown)
{
    if (lives <= 0) { return; }
    if (keydown) {
        switch (key) //this code handles capital letters and lower case letters in case caps lock is unknowingly on
        {
            case 'A':
            case 'a': requested_dir = WEST; break;
            case 'S':
            case 's': requested_dir = SOUTH; break;
            case 'D':
            case 'd': requested_dir = EAST; break;
            case 'W':
            case 'w': requested_dir = NORTH; break;
            default: break;
        }
    }
}

void PacMan::checkUserInput()
{
    if (dir != requested_dir && requested_dir != NO_DIRECTION)
    {
        int x = 0, y = 0;

        gw_dirToXY(requested_dir, x, y);
        if (gw_isFreeWithWrap(col + x, row + y))
        {
            constexpr int dir_leeway = 10;
            if ((requested_dir == NORTH || requested_dir == SOUTH) && abs(col_percentage) <= dir_leeway)
            {
                 col_percentage = 0;
                 dir = requested_dir;
            }
            if ((requested_dir == EAST || requested_dir == WEST) && abs(row_percentage) <= dir_leeway)
            {
                 row_percentage = 0;
                 dir = requested_dir;
            }
        }
    }
}
void PacMan::update(GameWorld& gw)
{
    const int prev_x = col, prev_y = row;
    int x = 0, y = 0;

    checkUserInput();

    //move pacman
    gw_dirToXY(dir, x, y); //determine the change in X and Y based on the direction

    //determine if the desired position is a free space
    if (gw_isFreeWithWrap(col + x, row + y))
    {
        isAnimated = true;

        //move column or X position
        col_percentage += x * speed;
        row_percentage += y * speed;
    }
    else
    {
        isAnimated = false;
    }

    //keep moving pacman if he is stuck between two grid cells and handles grid wrapping
    handleEntityMovement(row, row_percentage, col, col_percentage);

    //update the squares traveled counter if the position has changed
    if (prev_x != col || prev_y != row)
    {
        squares_traveled++;
    }

    // Check if pacman is on a dot
    if (gw.grid[row][col] == Tile_Dot)
    {
        gw.grid[row][col] = Tile_Floor;
        gw.score += 10; // 10 points for eating a typical dot
        gw.dots_collected++;
    }
    else if (gw.grid[row][col] == Tile_PowerUp)
    {
        gw.grid[row][col] = Tile_Floor;
        gw.score += 50; // 50 points for eating a power-up
        gw.dots_collected++;

        for (int i = 0; i < gw.ghost_count; i++)
        {
            gw.ghost_array[i].frightened = true;
            gw.ghost_array[i].fright_timer = 300; // 300 frames of fright
        }
    }

    // Check for collisions with ghosts
    for (int i = 0; i < gw.ghost_count; i++)
    {
        handlePacmanGhostCollision(gw, *this, gw.ghost_array[i]);
    }

    // Check if all dots are collected
    if (gw.dots_collected == gw.total_dots)
    {
        loadNextLevel(gw);
    }
}
