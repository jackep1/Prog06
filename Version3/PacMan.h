#ifndef PACMAN_H
#define PACMAN_H

struct GameWorld;

#include "Direction.h"

//Object data structures
class PacMan
{
	private:
		bool keys[NUM_DIRECTIONS];

	public:
		PacMan();

		int row; //(row, col) = position on the grid
		int col; //note that if the row is set to be 3.5 it means that pacman is between row 3 and row 4
		int speed; //pacman speed percentage per frame (should always be less than 100 but greater than 0)
		int squares_traveled; //the number of squares that pacman has traveled

		int row_percentage;
		int col_percentage;

		Direction dir; //direction pacman faces
		Direction requested_dir; //the direction requested by the player
		bool isAnimated; //controls whether PacMan is animating or now
		int isLive; //determines if the thread is running or not
		int lives = 3; // default starting number of lives

		void key_update(unsigned char key, bool keydown); //used to change the direction of pacman with keyboard input
		void checkUserInput();

		void update(GameWorld& gw); //this function is called every update cycle
};

#endif // PACMAN_H
