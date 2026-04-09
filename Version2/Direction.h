#ifndef DIRECTION_H
#define DIRECTION_H

//	Travel direction data type
//	Note that if you define a variable
//	TravelDirection dir = whatever;
//	you get the opposite directions from dir as (NUM_DIRECTIONS - dir)
//	you get left turn from dir as (dir + 1) % NUM_DIRECTIONS
enum Direction
{
	NO_DIRECTION = -1,
    NORTH = 0,
    WEST,
    SOUTH,
    EAST,
    NUM_DIRECTIONS
};

#endif // DIRECTION_H
