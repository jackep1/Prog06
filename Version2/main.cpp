#include <string>
#include <cstdio>
#include <unistd.h>
#include <pthread.h>
#include "gl_frontEnd.h"
#include "PacMan.h"
#include "Ghost.h"
#include "glPlatform.h"

//==================================================================================
//	Custom data types
//==================================================================================


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane();
void displayStatePane();
void initializeApplication();
void* pacmanLoop(void* v);
void* ghostLoop(void* v);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int	GRID_PANE, STATE_PANE;
extern int	gMainWindow, gSubwindow[2];

//	The state grid and its dimensions
GameWorld gameWorld;

//	pacman game stats
int numLiveThreads = 0;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane()
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
    drawGameWorld(&gameWorld);
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane()
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
    drawState(numLiveThreads, gameWorld.score, gameWorld.dots_collected, gameWorld.pacman->lives);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function
//------------------------------------------------------------------------
int main(const int argc, char** argv)
{
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level
	initializeApplication();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
		
	//	This will never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================


void initializeApplication()
{
    //Allocate the grid and all objects
    initializeGameWorldFile(gameWorld, "Levels/level_01.txt");

    // Set different speed values for each ghost
    for (int i = 0; i < gameWorld.ghost_count; i++)
    {
    	gameWorld.ghost_array[i].speed = 5;
	}

	// Create PacMan's thread
	pthread_t pacmanThread;
	pthread_create(&pacmanThread, nullptr, pacmanLoop, &gameWorld);
	//pthread_detach(pacmanThread);

	// Create Ghosts' threads
	pthread_t ghostThreads[gameWorld.ghost_count];
	for (int i = 0; i < gameWorld.ghost_count; i++)
	{
		pthread_create(&ghostThreads[i], nullptr, ghostLoop, &gameWorld.ghost_array[i]);
		pthread_detach(ghostThreads[i]);
	}
 }

//-------------------------------------------------------------------
//	This is the main work that needs to be done by multiple threads:
//		One thread for Pacman, and one for each ghost
//-------------------------------------------------------------------
// void updatePacAndGhosts()
// {
// 	//update pacman
// 	if (gameWorld.pacman != nullptr)
// 		gameWorld.pacman->update(gameWorld);
//
// 	for (int i = 0; i < gameWorld.ghost_count; i++)
// 	{
// 		gameWorld.ghost_array[i].ghostAI(gameWorld);
// 	}
// }

void* pacmanLoop(void* v)
{
	numLiveThreads++;
	auto* gameWorld = static_cast<GameWorld*>(v);
	while (true)
	{
		gameWorld->pacman->update(*gameWorld);
		usleep(10000);
	}
	return nullptr;
}

void* ghostLoop(void* v)
{
	numLiveThreads++;
	auto* ghost = static_cast<Ghost*>(v);
	while (true)
	{
		ghost->ghostAI(gameWorld);
		usleep(10000);
	}
	return nullptr;
}
