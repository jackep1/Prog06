//
//  gl_frontEnd.cpp
//  GL threads
//
//  Created by Jean-Yves Hervé on 2017-04-24,
//	revised 2020-04-xx, Chris McCooey
//
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <random>
#include <iostream>
#include "gl_frontEnd.h"
#include "glPlatform.h"


#define F_PI 3.14159265359f
#define F_PI_TIMES_2 6.28318530718f
#define F_PI_DIV_2 1.57079632679f


//---------------------------------------------------------------------------
//  Private functions' prototypes
//---------------------------------------------------------------------------

void myResize(int w, int h);
void displayTextualInfo(const char* infoStr, int x, int y, int isLarge, const ColorRGB *color);
void myMouse(int b, int s, int x, int y);
void myGridPaneMouse(int b, int s, int x, int y);
void myStatePaneMouse(int b, int s, int x, int y);
void myKeyboardDown(unsigned char c, int x, int y);
void myKeyboardUp(unsigned char c, int x, int y);
void myMenuHandler(int choice);
void mySubmenuHandler(int colorIndex);
void myTimer(int val);
void wire_ellipse(GLfloat w, GLfloat h, int tri_count, const ColorRGB &center, const ColorRGB &edges, const ColorRGB* border);
void makeObjectGeometry(int numRows, int numCols);

//---------------------------------------------------------------------------
//  Interface constants
//---------------------------------------------------------------------------

//	I like to set up my menu item indices as enumerated values, but really
//	regular int constants would do the job just fine.

enum MenuItemID {	SEPARATOR = -1,
					//
					QUIT_MENU = 0,
					OTHER_MENU_ITEM
};

const char* MAIN_MENU_ITEM_STR[] = {	"Quit",			//	QUIT_MENU
										"Something"		//	OTHER_MENU_ITEM
};

enum FirstSubmenuItemID {	FIRST_SUBMENU_ITEM = 11,
							SECOND_SUBMENU_ITEM
};

#define SMALL_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_12
#define LARGE_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_18
#define INPUT_KEY_TOTAL 255
constexpr int SMALL_FONT_HEIGHT = 12;
constexpr int TEXT_PADDING = 0;
constexpr float kTextColor[4] = {1.f, 1.f, 1.f, 1.f};
GLuint d_dot = 0u;
GLuint d_ghost = 0u;
GLuint d_powerup = 0u;
bool first_pass = true;
int pacman_frame = 0;
int pacman_animate_dir = 1;
int pacman_triangles = 0;
bool keyboard_state[INPUT_KEY_TOTAL];
bool show_names = false;
GameWorld* gw = nullptr;

//            kTextBackgroundColor[3] = {0.0f, 0.2f, 0.0f};

constexpr int INIT_WIN_X = 100;
constexpr int INIT_WIN_Y = 40;
constexpr int GRID_PANE_WIDTH = 800;
constexpr int GRID_PANE_HEIGHT = 800;
constexpr int STATE_PANE_WIDTH = 400;
constexpr int STATE_PANE_HEIGHT = 800;
constexpr int H_PADDING = 0;
constexpr int WINDOW_WIDTH = 1200;
constexpr int WINDOW_HEIGHT = 800;


//---------------------------------------------------------------------------
//  File-level global variables
//---------------------------------------------------------------------------

void (*gridDisplayFunc)();
void (*stateDisplayFunc)();

//	We use a window split into two panes/sub-windows.  The sub-windows
//	will be accessed by an index.
int	GRID_PANE = 0,
	STATE_PANE = 1;
int	gMainWindow,
	gSubwindow[2];

extern int MAX_LEVEL;
extern int MAX_ADD_INK;
extern int MAX_NUM_TRAVELER_THREADS;


//---------------------------------------------------------------------------
//	Drawing functions
//---------------------------------------------------------------------------
void drawGameWorld(GameWorld *game_world)
{
    if (game_world == nullptr)
    {
        fprintf(stderr, "ERROR: game world is null\n");
        return;
    }
    //std::lock_guard<std::mutex> lock(game_world->grid_mutex);  // Lock the grid prior to access)
    game_world->grid_mutex.lock();
    if (first_pass)
    {
        makeObjectGeometry(game_world->numRows, game_world->numCols);
        first_pass = false;
    }

    const int DH = GRID_PANE_WIDTH / game_world->numCols,
              DV = GRID_PANE_HEIGHT / game_world->numRows;
    float tx, ty; //these are only used as temporary variables to make this code more readable
    constexpr TileType prev_tile = Tile_Total;

    //Display the grid as a series of quad strips
    for (int i = 0; i<game_world->numRows; i++)
    {
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j<game_world->numCols; j++)
        {
            const TileType current = game_world->grid[i][j];

            //the current color does not need to change if the tile we drew is the same as the previous one
            if (current != prev_tile)
            {
                switch(current)
                {
                default:
                case Tile_Floor: glColor3ub(0, 0, 0); break;
                case Tile_Wall: glColor3ub(100, 100, 100); break;
                }
            }

            glVertex2i(j*DH, i*DV);
            glVertex2i(j*DH, (i+1)*DV);
            glVertex2i((j+1)*DH, i*DV);
            glVertex2i((j+1)*DH, (i+1)*DV);
        }
        glEnd();
    }
    // Then draw a grid of lines on top of the squares
    glColor4f(0.5f, 0.5f, 0.5f, 1.f);
    glBegin(GL_LINES);
    // horizontal lines
    for (int i = 0; i <= game_world->numRows; i++)
    {
        glVertex2i(0, i*DV);
        glVertex2i(game_world->numCols*DH, i*DV);
    }
    // vertical lines
    for (int j = 0; j <= game_world->numCols; j++)
    {
        glVertex2i(j*DH, 0);
        glVertex2i(j*DH, game_world->numRows*DV);
    }
    glEnd();

    //draw the objects for other tile types
    for (int i = 0; i<game_world->numRows; i++)
    {
        for (int j = 0; j<game_world->numCols; j++)
        {
            const TileType current = game_world->grid[i][j];
            GLuint lst = 0u;
            switch (current)
            {
            case Tile_Dot: lst = d_dot; break;
            case Tile_PowerUp: lst = d_powerup; break;
            default: break; //nothing to do for others
            }
            if (lst != 0u)
            {
                glPushMatrix();
                glTranslatef((static_cast<GLfloat>(j) + 0.5f)*DH, (static_cast<GLfloat>(i) + 0.5f)*DV, 0.0f);
                glCallList(lst);
                glPopMatrix();
            }
        }
    }

    //draw the ghosts
    for (int i = 0; i < game_world->ghost_count; i++)
    {
        const Ghost& g = game_world->ghost_array[i];
        glPushMatrix();

        game_world->ghost_array_mutex.lock();  // Lock the ghost array prior to access
        tx = (static_cast<GLfloat>(g.col) + 0.5f + (static_cast<GLfloat>(g.col_percentage) * 0.01f)) * DH;
        ty = (static_cast<GLfloat>(g.row) + 0.5f + (static_cast<GLfloat>(g.row_percentage) * 0.01f)) * DV;
        const bool frightened_ghost = g.frightened;
        game_world->ghost_array_mutex.unlock();
        //std::lock_guard<std::mutex> unlock(game_world->ghost_array_mutex);  // Done with ghost reading ghost data

        glTranslatef(tx, ty, 0.0f);
        if (frightened_ghost)
            glColor3ub(28, 18, 92);
        else
            glColor3ub(g.color.r, g.color.g, g.color.b);
        glCallList(d_ghost);
        glPopMatrix();
    }

    //draw pacman
    constexpr int tri_count = 50;
    GLfloat angle_iter = 0.0f;
    if (game_world->pacman->isAnimated)
    {
        pacman_frame++;
        if (pacman_frame >= 8)
        {
            pacman_frame = 0;
            if (pacman_animate_dir == 1)
            {
                pacman_triangles++;
                if (pacman_triangles >= 12)
                    pacman_animate_dir = -1;
            }
            else if (pacman_animate_dir == -1)
            {
                pacman_triangles--;
                if (pacman_triangles <= 0)
                    pacman_animate_dir = 1;
            }
        }
    }
    glPushMatrix();

    tx = (static_cast<GLfloat>(game_world->pacman->col) + 0.5f + (static_cast<GLfloat>(game_world->pacman->col_percentage) * 0.01f)) * DH;
    ty = (static_cast<GLfloat>(game_world->pacman->row) + 0.5f + (static_cast<GLfloat>(game_world->pacman->row_percentage) * 0.01f)) * DV;
    glTranslatef(tx, ty, 0.0f);
    glRotatef(static_cast<float>(game_world->pacman->dir) * 90.f + 90.0f, 0.0f, 0.0f, 1.0f); //rotate along the z-axis

    constexpr GLfloat angle_incr = F_PI_TIMES_2 / static_cast<GLfloat>(tri_count);
    const GLfloat h_offset = static_cast<GLfloat>(pacman_triangles / 2) * angle_incr;
    angle_iter = h_offset;
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(255, 255, 0);
    glVertex2f(0.0f, 0.0f); //center
    while (angle_iter <= F_PI_TIMES_2 + 0.00001f - h_offset)
    {
        const GLfloat x = cosf(angle_iter) * DH * 0.5f;
        const GLfloat y = sinf(angle_iter) * DV * 0.5f;
        glVertex2f(x, y);
        angle_iter += angle_incr;
    }
    glEnd();
    glBegin(GL_POINTS);
    glPointSize(3.0f);
    glColor3ub(0, 0, 0);
    if (game_world->pacman->dir == WEST || game_world->pacman->dir == NORTH)
        glVertex2f(DH * 0.075f, DV * -0.20f);
    else
        glVertex2f(DH * 0.075f, DV * 0.20f);
    glEnd();
    glPopMatrix();
    game_world->grid_mutex.unlock();
    // //draw names above ghosts
    // if (game_world != nullptr && show_names)
    // {
    //     int tx, ty, i, w;
    //     for (i = 0; i < game_world->ghost_count; i++)
    //     {
    //         const Ghost& g = game_world->ghost_array[i];
    //         const int len = (int)g.name.size();
    //         if (len == 0)
    //             continue;
    //         tx = ((GLfloat)g.col + 0.5f + ((GLfloat)g.col_percentage * 0.01f)) * DH;
    //         ty = ((GLfloat)g.row + 1.20f + ((GLfloat)g.row_percentage * 0.01f)) * DV;
    //         w = SMALL_FONT_HEIGHT * len/4;
    //         glBegin(GL_QUADS);
    //         glColor4ub(0, 0, 0, 128);
    //         glVertex2i(tx - w, ty - 2);
    //         glVertex2i(tx + w, ty - 2);
    //         glVertex2i(tx + w, ty + SMALL_FONT_HEIGHT - 2);
    //         glVertex2i(tx - w, ty + SMALL_FONT_HEIGHT - 2);
    //         glEnd();
    //
    //         displayTextualInfo(g.name.c_str(), tx - w + 4, ty, 0, &g.color);
    //     }
    // }
}


void displayTextualInfo(const char* infoStr, int xPos, int yPos, int isLarge, const ColorRGB* color)
{
    //-----------------------------------------------
    //  0.  get current material properties
    //-----------------------------------------------
    float oldAmb[4], oldDif[4], oldSpec[4], oldShiny;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
    glGetMaterialfv(GL_FRONT, GL_SHININESS, &oldShiny);

    glPushMatrix();

    //-----------------------------------------------
    //  1.  Build the string to display <-- parameter
    //-----------------------------------------------
    const int infoLn = static_cast<int>(strlen(infoStr));

    //-----------------------------------------------
    //  2.  Determine the string's length (in pixels)
    //-----------------------------------------------
    int textWidth = 0;
    for (int k=0; k<infoLn; k++)
	{
		if (isLarge)
			textWidth += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		else
			textWidth += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		
    }
	//  add a few pixels of padding
    textWidth += 2 * TEXT_PADDING;
	
    //-----------------------------------------------
    //  4.  Draw the string
    //-----------------------------------------------
    if (color == nullptr)
        glColor4fv(kTextColor); //default text color if none is provided
    else
        glColor3ub(color->r, color->g, color->b);
    int x = xPos;
    for (int k=0; k<infoLn; k++)
    {
        glRasterPos2i(x, yPos);
		if (isLarge)
		{
			glutBitmapCharacter(LARGE_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		}
		else
		{
			glutBitmapCharacter(SMALL_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		}
	}

    //-----------------------------------------------
    //  5.  Restore old material properties
    //-----------------------------------------------
	glMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, oldShiny);  
    
    //-----------------------------------------------
    //  6.  Restore reference frame
    //-----------------------------------------------
    glPopMatrix();
}



void drawState(const int numLiveThreads, const int score, const int dotsCollected, const int pacmanLives)
{
    constexpr int LEVEL_WIDTH = STATE_PANE_WIDTH / 4;
	constexpr int H_PAD = LEVEL_WIDTH / 4;
	constexpr int RED_LEFT = H_PAD;
	constexpr int TOP_LEVEL_TXT_Y = 4*STATE_PANE_HEIGHT / 5;
    constexpr int TEXT_ITER = -4*STATE_PANE_HEIGHT / 100;
    int i = 0;
    ColorRGB rgb;

    char infoStr[256];
	//	display info about number of live threads
	sprintf(infoStr, "Live Threads: %d", numLiveThreads);
    displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y + i, 1, nullptr);
    i += TEXT_ITER;

    //display info about score dots collected and pacman lives
    sprintf(infoStr, "Score: %d", score);
    rgb=ColorRGB(255, 255, 100);
    displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y + i, 1, &rgb);
    i += TEXT_ITER;

    sprintf(infoStr, "Dots Collected: %d", dotsCollected);
    rgb=ColorRGB(255, 255, 100);
    displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y + i, 1, &rgb);
    i += TEXT_ITER;

    sprintf(infoStr, "Lives: %d", pacmanLives);
    rgb=ColorRGB(255, 100, 100);
    displayTextualInfo(infoStr, RED_LEFT, TOP_LEVEL_TXT_Y + i, 1, &rgb);
}

void wire_ellipse(GLfloat w, GLfloat h, const int tri_count, const ColorRGB &center, const ColorRGB &edges, const ColorRGB* border)
{
    GLfloat angle_iter = 0.0f, angle_incr = F_PI_TIMES_2, x, y;

    angle_iter = 0.0f;
    angle_incr = F_PI_TIMES_2 / static_cast<GLfloat>(tri_count);
    glBegin(GL_TRIANGLE_FAN);
    glColor3ub(center.r, center.g, center.b); //white center
    glVertex2f(0.0f, 0.0f); //center
    glColor3ub(edges.r, edges.g, edges.b); //darker yellow edge
    while (angle_iter <= F_PI_TIMES_2 + 0.00001f)
    {
        x = cosf(angle_iter) * w;
        y = sinf(angle_iter) * h;
        glVertex2f(x, y);
        angle_iter += angle_incr;
    }
    glEnd();
    if (border != nullptr) //draw a border if one is specified
    {
        glBegin(GL_LINE_LOOP);
        glColor3ub(border->r, border->g, border->b); //darker yellow edge
        angle_iter = 0.0f;
        while (angle_iter < F_PI_TIMES_2)
        {
            x = cosf(angle_iter) * w;
            y = sinf(angle_iter) * h;
            glVertex2f(x, y);
            angle_iter += angle_incr;
        }
        glEnd();
    }
}

void makeObjectGeometry(int numRows, int numCols)
{
    const int DH = GRID_PANE_WIDTH / numCols,
              DV = GRID_PANE_HEIGHT / numRows;
    constexpr GLfloat dot_scale_factor = 0.15f;
    constexpr GLfloat powerup_scale_factor = 0.40f;
    constexpr GLfloat ghost_scale_factor = 0.50f;
    ColorRGB center;
    ColorRGB edges;

    //create the dot that will be drawn
    center.r = center.g = center.b = 255;
    edges.r = edges.g = 175; edges.b = 0;
    d_dot = glGenLists(1);
    glNewList(d_dot, GL_COMPILE);
    wire_ellipse(DH * dot_scale_factor, DV * dot_scale_factor, 20, center, edges, nullptr);
    glEndList();

    //create the powerup that will be drawn
    center.r = center.g = 255; center.b = 50;
    edges.r = edges.g = 50; edges.b = 0;
    d_powerup = glGenLists(1);
    glNewList(d_powerup, GL_COMPILE);
    wire_ellipse(DH * powerup_scale_factor, DV * powerup_scale_factor, 24, center, edges, nullptr);
    glEndList();

    //create the ghost
    d_ghost = glGenLists(1);
    glNewList(d_ghost, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    GLfloat angle_iter = 0.0f;
    GLfloat angle_incr = F_PI / 16.0f;
    GLfloat rw = DH * ghost_scale_factor;
    GLfloat rh = DV * ghost_scale_factor;
    while (angle_iter <= F_PI + 0.00001f)
    {
        GLfloat x = cosf(angle_iter) * rw;
        GLfloat y = sinf(angle_iter) * rh;
        glVertex2f(x, y);
        angle_iter += angle_incr;
    }
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(rw, -rh);
    glVertex2f(rw, 0.0f);
    glVertex2f(-rw, -rh);
    glVertex2f(-rw, 0.0f);
    glEnd();
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glColor3ub(0, 0, 0);
    glVertex2f(rw * -0.25f, rh * 0.25f);
    glVertex2f(rw * 0.25f, rh * 0.25f);
    glEnd();
    glEndList();

    //print error messages if something goes wrong with any of the call lists
    if (d_dot == 0u) printf("ERROR: Failed to construct geometry for the dot\n");
    if (d_powerup == 0u) printf("ERROR: Failed to construct geometry for the power up\n");
    if (d_ghost == 0u) printf("ERROR: Failed to construct geometry for the ghost\n");
}


//	This callback function is called when the window is resized
//	(generally by the user of the application).
//	It is also called when the window is created, why I placed there the
//	code to set up the virtual camera for this application.
//
void myResize(int w, int h)
{
	if ((w != WINDOW_WIDTH) || (h != WINDOW_HEIGHT))
	{
		glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	}
	else
	{
		glutPostRedisplay();
	}
}


void myDisplay()
{
    glutSetWindow(gMainWindow);

    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);

	gridDisplayFunc();
	stateDisplayFunc();
	
	glutSetWindow(gMainWindow);
}

//	This function is called when a mouse event occurs just in the tiny
//	space between the two sub-windows.
//
void myMouse(int button, int state, int x, int y)
{
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the grid pane
//
void myGridPaneMouse(int button, int state, int x, int y)
{
	// switch (button)
	// {
	// 	case GLUT_LEFT_BUTTON:
	// 		if (state == GLUT_DOWN)
	// 		{
	// 			//	do something
	// 		}
	// 		else if (state == GLUT_UP)
	// 		{
	// 			//	exit(0);
	// 		}
	// 		break;
	//
	// 	default:
	// 		break;
	// }

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the state pane
void myStatePaneMouse(int button, int state, int x, int y)
{
	// switch (button)
	// {
	// 	case GLUT_LEFT_BUTTON:
	// 		if (state == GLUT_DOWN)
	// 		{
	// 			//	do something
	// 		}
	// 		else if (state == GLUT_UP)
	// 		{
	// 			//	exit(0);
	// 		}
	// 		break;
	//
	// 	default:
	// 		break;
	// }

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


//	This callback function is called when a keyboard event occurs
//
void myKeyboardDown(const unsigned char c, const int x, const int y)
{
    (void)x;
    (void)y;

    keyboard_state[c] = true; //key is pressed
    if (gw != nullptr && gw->pacman != nullptr)
    {
        gw->pacman->key_update(c, true);
    }

	int ok = 0;
	
	switch (c)
	{
    //	'esc' to quit
    case 27:
        exit(0);

    case 'n':
    case 'N':
        show_names = !show_names;
        break;

    default:
        ok = 1;
        break;
	}
	if (!ok)
	{
		//	do something?
	}

	
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}
void myKeyboardUp(unsigned char c, int x, int y)
{
    (void)x;
    (void)y;
    keyboard_state[c] = false; //key is released
    if (gw != nullptr && gw->pacman != nullptr)
        gw->pacman->key_update(c, false);
}

void myTimer(int val)
{
    // IF GAME OVER
    // 	Display game over message join all threads
    // 	initialize
    glutTimerFunc(5, myTimer, 0);

	//updatePacAndGhosts();
	
	// glutSetWindow(gMainWindow);
	// myDisplay();

    glutPostRedisplay();
}

void myMenuHandler(const int choice)
{
	switch (choice)
	{
		//	Exit/Quit
		case QUIT_MENU:
			exit(0);

		//	Do something
		case OTHER_MENU_ITEM:
			break;
		
		default:	//	this should not happen
			break;
	
	}

	glutSetWindow(gMainWindow);
    glutPostRedisplay();
}

int initializeGameWorldFile(GameWorld& game_world, const char* filepath)
{
    gw = &game_world;
    game_world.ghost_count = 0;
    game_world.ghost_array = nullptr; //empty array for now

    if (io_loadLevel(game_world, filepath) != 0)
    {
        return -1;
    }

    countTotalDots(game_world);

    return 0;
}

void initializeFrontEnd(int argc, char** argv, void (*gridDisplayCB)(),
						void (*stateDisplayCB)())
{
    //Initialize the keyboard
    for (bool & i : keyboard_state)
    {
        i = false;
    }

	//	Initialize glut and create a new window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);


	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(INIT_WIN_X, INIT_WIN_Y);
    gMainWindow = glutCreateWindow("PacMan -- CSC 412 - Fall 2024");
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	
	//	set up the callbacks for the main window
	glutDisplayFunc(myDisplay);
	glutReshapeFunc(myResize);
	glutMouseFunc(myMouse);
	glutKeyboardFunc(myKeyboardDown);
	glutKeyboardUpFunc(myKeyboardUp);
    glutTimerFunc(15, myTimer, 0);

	gridDisplayFunc = gridDisplayCB;
	stateDisplayFunc = stateDisplayCB;
	
	//	create the two panes as glut sub-windows
	gSubwindow[GRID_PANE] = glutCreateSubWindow(gMainWindow,
												0, 0,							//	origin
												GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glViewport(0, 0, GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, GRID_PANE_WIDTH, 0.0, GRID_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
    glutKeyboardFunc(myKeyboardDown);
    glutKeyboardUpFunc(myKeyboardUp);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(gridDisplayCB);
	
	
	glutSetWindow(gMainWindow);
	gSubwindow[STATE_PANE] = glutCreateSubWindow(gMainWindow,
												GRID_PANE_WIDTH + H_PADDING, 0,	//	origin
												STATE_PANE_WIDTH, STATE_PANE_HEIGHT);
    glViewport(0, 0, STATE_PANE_WIDTH, STATE_PANE_WIDTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, STATE_PANE_WIDTH, 0.0, STATE_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
    glutKeyboardFunc(myKeyboardDown);
    glutKeyboardUpFunc(myKeyboardUp);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(stateDisplayCB);
}
bool gw_dirIsOpposite(Direction dir1, Direction dir2)
{
    return ((dir1 == NORTH && dir2 == SOUTH) || (dir1 == SOUTH && dir2 == NORTH) ||
            (dir1 == EAST && dir2 == WEST) || (dir1 == WEST && dir2 == EAST));
}
bool gw_isInRange(int x, int y)
{
    return (gw != nullptr && x >= 0 && x < gw->numCols && y >= 0 && y < gw->numRows);
}
TileType gw_tileAt(int x, int y)
{
    if (gw_isInRange(x, y))
        return gw->grid[y][x];
    return Tile_Null;
}
bool gw_isFree(int x, int y)
{
    bool free = (gw != nullptr && x >= 0 && x < gw->numCols && y >= 0 && y < gw->numRows);
    if (free)
        free = (gw->grid[y][x] != Tile_Wall);
    return free;
}
bool gw_isFloorTile(int x, int y)
{
    if (gw_isInRange(x, y))
        return (gw->grid[y][x] == Tile_Floor || gw->grid[y][x] == Tile_Dot || gw->grid[y][x] == Tile_PowerUp);
    return false;
}
bool gw_isFreeWithWrap(int x, int y)
{
    bool free = true;
    if (gw_isInRange(x, y))
        free = (gw->grid[y][x] != Tile_Wall);
    return free;
}
bool gw_isIntersection(int x, int y)
{
    bool intersection = false;
    if (gw_isFloorTile(x, y))
    {
        int free_neighbors = 0;
        if (gw_isFree(x - 1, y)) free_neighbors++;
        if (gw_isFree(x + 1, y)) free_neighbors++;
        if (gw_isFree(x, y - 1)) free_neighbors++;
        if (gw_isFree(x, y + 1)) free_neighbors++;
        intersection = (free_neighbors >= 3);
    }
    return intersection;
}
void gw_closetIntersection(int x, int y, int *out_x, int *out_y)
{
    *out_x = *out_y = 0;
    if (gw != nullptr)
    {
        auto lowest_distance = static_cast<float>(~0u);
        const int count = static_cast<int>(gw->grid_intersections.size());
        for (int i = 0; i < count; i++)
        {
            const RowCol &xy = gw->grid_intersections[i];
            const int dx = (xy.col - x);
            const int dy = (xy.row - y);
            if (dx != 0 && dy != 0) //avoid finding the position that it is on
            {
                const auto distance = static_cast<float>(dx * dx + dy * dy);
                if (distance < lowest_distance)
                {
                    lowest_distance = distance;
                    *out_x = xy.col;
                    *out_y = xy.row;
                }
            }
        }
    }
}
void gw_findNeighborsOfTypeHelper(int x, int y, TileType desired_type, std::vector<RowCol>* out_pos)
{
    if (gw_isInRange(x, y) && desired_type == gw->grid[y][x])
    {
        RowCol rc{};
        rc.col = x;
        rc.row = y;
        out_pos->push_back(rc);
    }
}
void gw_findNeighborsOfType(int x, int y, TileType desired_type, std::vector<RowCol>* out_pos)
{
    gw_findNeighborsOfTypeHelper(x - 1, y, desired_type, out_pos);
    gw_findNeighborsOfTypeHelper(x + 1, y, desired_type, out_pos);
    gw_findNeighborsOfTypeHelper(x, y - 1, desired_type, out_pos);
    gw_findNeighborsOfTypeHelper(x, y + 1, desired_type, out_pos);
}
void gw_getValidDirections(int x, int y, TileType desired_type, Direction* dirs, int* dirs_found)
{
    int iter = 0;
    *dirs_found = 0;
    for (int i = 0; i < NUM_DIRECTIONS; i++)
    {
        int gx = 0, gy = 0;
        gw_dirToXY(static_cast<Direction>(i), gx, gy);
        if (desired_type == gw_tileAt(x + gx, y + gy))
        {
            dirs[iter] = static_cast<Direction>(i);
            (*dirs_found)++;
            iter++;
        }
    }
}
void gw_getValidDirectionsMulti(int x, int y, const std::vector<TileType>& desired_type, Direction* dirs, int& dirs_found)
{
    dirs_found = 0;
    const int count = static_cast<int>(desired_type.size());
    int iter = 0;
    for (int i = 0; i < NUM_DIRECTIONS; i++)
    {
        int gx = 0, gy = 0;
        gw_dirToXY(static_cast<Direction>(i), gx, gy);
        for (int j = 0; j < count; j++)
        {
            if (desired_type.at(j) == gw_tileAt(x + gx, y + gy))
            {
                dirs[iter] = static_cast<Direction>(i);
                dirs_found++;
                iter++;
                break;
            }
        }
    }
}
void gw_dirToXY(Direction dir, int& x, int& y)
{
    switch(dir)
    {
    case NORTH: x = 0; y = 1; break;
    case SOUTH: x = 0; y = -1; break;
    case EAST: x = 1; y = 0; break;
    case WEST: x = -1; y = 0; break;
    default: x = 0; y = 0; break;
    }
}
int distanceSquaredI(int x1, int y1, int x2, int y2)
{
    const int delta_x = x2 - x1, delta_y = y2 - y1;
    return ((delta_x * delta_x) + (delta_y * delta_y));
}

void handleEntityMovementValue(int& v, int& vp)
{
    if (vp <= -100 || vp >= 100)
    {
        v += (vp / 100);
        vp -= (vp / 100) * 100;
    }
}
bool keyDown(char key)
{
    return keyboard_state[static_cast<unsigned char>(key)];
}
void handleEntityMovement(int& row, int& row_percentage, int& col, int& col_percentage)
{
    handleEntityMovementValue(row, row_percentage);
    handleEntityMovementValue(col, col_percentage);

    //screen wrap
    if (gw != nullptr)
    {
        if (row < 0)
            row = gw->numRows - 1;
        else if (row >= gw->numRows)
            row = 0;
        if (col < 0)
            col = gw->numCols - 1;
        else if (col >= gw->numCols)
            col = 0;
    }
}

void loadNextLevel(GameWorld& game_world)
{
    static int current_level = 1;
    current_level++;
    std::string next_level_file = "level_0" + std::to_string(current_level) + ".txt";
    if (io_loadLevel(game_world, next_level_file.c_str()) != 0)
    {
        fprintf(stderr, "ERROR: Could not load next level\n");
    }
}

// Function to find a random valid position for Pacman
void findRandomValidPosition(const GameWorld& game_world, int& row, int& col, int& row_percentage, int& col_percentage)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> row_dist(0, game_world.numRows - 1);
    std::uniform_int_distribution<> col_dist(0, game_world.numCols - 1);

    bool valid_position = false;

    while (!valid_position)
    {
        row = row_dist(gen);
        col = col_dist(gen);

        // Check if the position is a valid floor tile and not inside the jail
        if (gw_isFloorTile(col, row))
        {
            valid_position = true;
        }
    }
    row_percentage = col_percentage = 0;
}

void handlePacmanGhostCollision(GameWorld &gw, PacMan &pacman, Ghost &ghost)
{
    if (pacman.row == ghost.row && pacman.col == ghost.col)
    {
        pacman.dir = NO_DIRECTION;
        if (ghost.frightened)
        {
            // Pacman sends the ghost to jail
            ghost.row = gw.numRows / 2;
            ghost.col = gw.numCols / 2;
            ghost.frightened = false;
            gw.score += 200;  // 200 points for each ghost eaten
        }
        else
        {
            // Pacman loses a life
            pacman.lives--;
            if (pacman.lives <= 0)
            {
                // Put the ghosts in the jail and make them motionless
                for (int i = 0; i < gw.ghost_count; i++)
                {
                    gw.ghost_array[i].row = gw.numRows / 2;
                    gw.ghost_array[i].col = gw.numCols / 2;
                    gw.ghost_array[i].dir = NO_DIRECTION;
                }
                std::cout << "Game over!" << std::endl;
            }
            else
            {
                // Reset entity positions
                findRandomValidPosition(gw, pacman.row, pacman.col, pacman.row_percentage, pacman.col_percentage);
                ghost.row = gw.numRows / 2;
                ghost.col = gw.numCols / 2;
            }
        }
    }
}

void countTotalDots(GameWorld& gw)
{
    gw.total_dots = 0;
    for (int row = 0; row < gw.numRows; row++)
    {
        for (int col = 0; col < gw.numCols; col++)
        {
            if (gw.grid[row][col] == Tile_Dot || gw.grid[row][col] == Tile_PowerUp)
            {
                gw.total_dots++;
            }
        }
    }
}