#ifndef SCRABBLE_GAME_H
#define SCRABBLE_GAME_H

#include "settings.h"
#include "shared_mem_util.h"


typedef struct
{
	char gameBoard[BOARD_HEIGHT][BOARD_WIDTH];
	int msg;
	int status;
	char avTiles[25];
	int p1Points;
	int p2Points;
	int didClientDisconnect;
	int didGameEnd;
} game;

void scrabble_game_blank(game*);
void scrabble_game_print_board(char[5][5]);
void scrabble_game_print_title();
void scrabble_game_attach_tiles(char[25]);
void scrabble_game_print_available_tiles(char[], int);
void scrabble_game_print_wait_for_move();
char scrabble_game_get_random_tile(char[25]);
void scrabble_game_print_points(int, int, int);
int scrabble_game_calculate_points(char[5][5], int, int);
int scrabble_game_calculate_vertical(char[5][5], int, int);
int scrabble_game_calculate_horizontal(char[5][5], int, int);

#endif
