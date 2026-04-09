#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include "gl_frontEnd.h"
#include "PacMan.h"

struct SpecialLine
{
    char type;
    int x;
    int y;
    std::string name;
    int color_index;
    int ai;
};

TileType io_charToTile(char c)
{
    TileType tt = Tile_Total;
    switch(c)
    {
    case '_': tt = Tile_Floor; break;
    case 'w': tt = Tile_Wall; break;
    case 'o': tt = Tile_Dot; break;
    case 'P': tt = Tile_PowerUp; break;
    case 'G': tt = Tile_GhostGate; break;
    case 'J': tt = Tile_Jail; break;
    default: break;
    }
    return tt;
}
char io_tileToChar(TileType tt)
{
    char c = 0;
    switch(tt)
    {
    case Tile_Floor: c = '_'; break;
    case Tile_Wall: c = 'w'; break;
    case Tile_Dot: c = 'o'; break;
    case Tile_PowerUp: c = 'P'; break;
    case Tile_GhostGate: c = 'G'; break;
    case Tile_Jail: c = 'J'; break;
    default: break;
    }
    return c;
}
int io_loadLevel(GameWorld& game_world, const char* filepath)
{
    std::ifstream reader;
    int rc = 0;

    reader.open(filepath);
    if (reader.is_open())
    {
        std::vector<std::string> grid_lines;
        std::vector<SpecialLine> special_lines;
        int width = 1;

        //read the file line by line
        while (!reader.eof())
        {
            std::string line;
            reader >> line;
            if (!line.empty())
            {
                if (line[0] == '$')
                {
                    SpecialLine sp;
                    reader >> sp.type;
                    reader >> sp.x;
                    reader >> sp.y;
                    reader >> sp.name;
                    reader >> sp.color_index;
                    reader >> sp.ai;
                    special_lines.push_back(sp);
                }
                else
                {
                    width = static_cast<int>(line.length());
                    grid_lines.push_back(line);
                }
            }
        }
        reader.close();

        //load the grid
        game_world.numRows = static_cast<int>(grid_lines.size());
        game_world.numCols = width;
        game_world.grid = new TileType*[game_world.numRows];
        for (int i = 0; i < game_world.numRows; i++)
        {
            const std::string &str = grid_lines[game_world.numRows - i - 1];
            game_world.grid[i] = new TileType[game_world.numCols];
            for (int j = 0; j < game_world.numCols; j++)
                game_world.grid[i][j] = io_charToTile(str[j]);
        }
        auto ghost_vector = std::vector<Ghost>(); //empty vector for now
        game_world.pacman = nullptr;
        game_world.ghost_array = nullptr;
        game_world.ghost_count = 0;

        //find the intersections
        game_world.grid_intersections = std::vector<RowCol>();
        for (int i = 0; i < game_world.numRows; i++)
        {
            for (int j = 0; j < game_world.numCols; j++)
            {
                if (gw_isIntersection(j, i))
                {
                    RowCol row_col{};
                    row_col.row = i;
                    row_col.col = j;
                    game_world.grid_intersections.push_back(row_col);
                }
            }
        }

        //create the ghosts
        if (!special_lines.empty())
        {
            ColorRGB colors[4];
            int count = static_cast<int>(special_lines.size());

            colors[0].r = 255; colors[0].g = 50; colors[0].b = 50; //red ghost
            colors[1].r = 255; colors[1].g = 100; colors[1].b = 255; //pink ghost
            colors[2].r = 0; colors[2].g = 255; colors[2].b = 255; //cyan ghost
            colors[3].r = 255; colors[3].g = 150; colors[3].b = 0; //orange ghost
            for (int i = 0; i < count; i++)
            {
                const SpecialLine &sp = special_lines.at(i);
                if (sp.type == 'G')
                {
                    Ghost ghost;
                    if (sp.color_index >= 0 && sp.color_index < 4)
                        ghost.color = colors[sp.color_index];
                    else
                        ghost.color = ColorRGB(255, 255, 255); //white if color is not valid
                    ghost.col = sp.x;
                    ghost.row = sp.y;
                    ghost.frightened = false;
                    ghost.speed = 20;
                    ghost.col_percentage = 0;
                    ghost.row_percentage = 0;
                    ghost.dir = NUM_DIRECTIONS;
                    ghost.name = sp.name;
                    ghost.ai = static_cast<GhostAI>(sp.ai);
                    ghost_vector.push_back(ghost);
                }
                else if (sp.type == 'P')
                {
                    //create pacman
                    if (game_world.pacman == nullptr)
                    {
                        game_world.pacman = new PacMan();
                        game_world.pacman->dir = EAST; //set pacman initial direction
                        game_world.pacman->isAnimated = true; //set to true to animate pacman
                    }
                    game_world.pacman->row = sp.y; //set pacman initial position
                    game_world.pacman->col = sp.x;
                }
                else 
                    std::cout << "ERROR: Found bad type in file. Invalid character is " << sp.type << std::endl;
            }

            if (!ghost_vector.empty())
            {
                game_world.ghost_array = new Ghost[ghost_vector.size()];
                game_world.ghost_count = static_cast<int>(ghost_vector.size());
                for (unsigned int i = 0; i < ghost_vector.size(); i++)
                    game_world.ghost_array[i] = ghost_vector[i];
            }
        }
    }
    else
    {
        std::cout << "ERROR: Failed to load level \"" << filepath << "\"" << std::endl;
        rc = -1;
    }
    return rc;
}