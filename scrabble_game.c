#include "scrabble_game.h"

void scrabble_game_blank(game* add)
{
	int i, j;
	for(i = 0; i < BOARD_HEIGHT; i++)
	{
		for(j = 0; j < BOARD_WIDTH; j++)
		{
			add->gameBoard[i][j] = 'x';
		}
	}
}

void scrabble_game_print_board(char gameBoard[5][5])
{
	int i, j;
	for(i = 0; i < BOARD_HEIGHT; i++)
	{
		printf("              ");
		for(j = 0; j < BOARD_WIDTH; j++)
		{
			printf("%c ", gameBoard[i][j]);
		}
		printf("\n");
	}
}

void scrabble_game_print_title()
{
	printf("##########################################\n");
	printf("##             GAME BOARD               ##\n");
	printf("##########################################\n");
}

void scrabble_game_attach_tiles(char tiles[25])
{
	char avTiles[25] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
					  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
					  'Q','R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y'};
	int z;
	for(z = 0; z < 25; z++)
		tiles[z] = avTiles[z];
}

void scrabble_game_print_available_tiles(char tiles[], int n)
{
	int i;
	for(i = 0; i < n; i++)
	{
		if(tiles[i] != UNAVAILABLE)
			printf("%c ", tiles[i]);
	}
	printf("\n");
}

void scrabble_game_print_wait_for_move()
{
	printf("------------------------------------------\n");
	printf("## Waiting for another player's move... ##\n");
	printf("------------------------------------------\n");
}

char scrabble_game_get_random_tile(char tiles[25])
{
	srand(time(NULL));
	char c;
	int r,i,isAnyTileAvaliable;
	isAnyTileAvaliable = 0;
	for( i=0;i<25;i++){
		if(tiles[i] != UNAVAILABLE){
			isAnyTileAvaliable = 1;
		}
	}
	if(isAnyTileAvaliable == 1) {
		while (1) {
			r = rand() % 25;
			if (tiles[r] == UNAVAILABLE)
				continue;
			else {
				c = tiles[r];
				tiles[r] = UNAVAILABLE;
				return c;
			}
		}
	}
	else{
		return 'x';
	}
}

void scrabble_game_print_points(int p1, int p2, int type)
{
	printf("------------------------------------------\n");
	switch(type)
	{
		case FIRST:
			printf("#  You:      %d\n# Opponent: %d\n", p1, p2);
			break;
		case SECOND:
			printf("#  You:      %d\n# Opponent: %d\n", p2, p1);
			break;
	}
	printf("------------------------------------------\n");
}

int scrabble_game_calculate_points(char gb[5][5], int x, int y)
{
	int v_p, h_p,  max;
	
	v_p = scrabble_game_calculate_vertical(gb, x ,y);
	max = v_p;
	
	h_p = scrabble_game_calculate_horizontal(gb, x ,y);
	max = (max > h_p) ? max : h_p;

	/* +1 for tile in (x,y). */
	return max + 1;	 
}

int scrabble_game_calculate_vertical(char gb[5][5], int x, int y)
{
	int i, points;
	
	points = 0;
	
	/* Up */
	for(i = (x-1); i>=0; i--)
	{
		if(gb[i][y] == 'x') break;
		points++;
	}
	/* Down */
	for(i = x + 1; i < BOARD_HEIGHT; i++)
	{
		if(gb[i][y] == 'x') break;
		points++;
	}
	
	return points;
}

int scrabble_game_calculate_horizontal(char gb[5][5], int x, int y)
{
		int i, points;
	
	points = 0;
	
	/* Left */
	for(i = (y-1); i>=0; i--)
	{
		if(gb[x][i] == 'x') break;
		points++;
	}
	/* Right */
	for(i = y + 1; i < BOARD_WIDTH; i++)
	{
		if(gb[x][i] == 'x') break;
		points++;
	}
	
	return points;
}



