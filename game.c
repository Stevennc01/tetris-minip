#include <stdio.h>
#include <ncurses.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define WELL_HEIGHT 22
#define WELL_WIDTH 32
#define NO_LEVELS 10

/* Points are stored in three parts : points, lines, levels
 *
 * DOT is the structure defining the blocks
 */
typedef struct points {
	unsigned int points;
	unsigned int lines;
	unsigned int level;
} POINTS;

typedef struct dot {
	unsigned char y;
	unsigned char x;
} DOT;



/* well_data defines the occupancy of well  in terms of a character array.
 * delay[] is the time delay in microseconds depending on the level no.
 */
char *well_data;
int delay[NO_LEVELS] = {1000000, 770000, 593000, 457000, 352000, 271000, 208000, 160000, 124000, 95000};
WINDOW *w1;

//block_data[types][orientations][dots]
//defining the blocks
const DOT block_data[7][4][4] =
{
	{
		{{2,0},{2,1},{2,2},{2,3}},	/*      */
		{{0,1},{1,1},{2,1},{3,1}},	/*      */
		{{2,0},{2,1},{2,2},{2,3}},	/* XXXX */
		{{0,1},{1,1},{2,1},{3,1}}	/*      */
	},
	{	
		{{1,1},{2,1},{1,2},{2,2}},	/*      */
		{{1,1},{2,1},{1,2},{2,2}},	/*  XX  */
		{{1,1},{2,1},{1,2},{2,2}},	/*  XX  */
		{{1,1},{2,1},{1,2},{2,2}}	/*      */
	},
	{
		{{1,0},{1,1},{1,2},{2,2}},	/*      */
		{{2,0},{0,1},{1,1},{2,1}},	/* XXX  */
		{{0,0},{1,0},{1,1},{1,2}},	/*   X  */
		{{0,1},{1,1},{2,1},{0,2}}	/*      */
	},
	{
		{{1,0},{2,0},{1,1},{1,2}},	/*      */
		{{0,0},{0,1},{1,1},{2,1}},	/* XXX  */
		{{1,0},{1,1},{0,2},{1,2}},	/* X    */
		{{0,1},{1,1},{2,1},{2,2}}	/*      */
	},
	{
		{{1,0},{1,1},{2,1},{2,2}},	/*      */
		{{1,0},{2,0},{0,1},{1,1}},	/* XX   */
		{{1,0},{1,1},{2,1},{2,2}},	/*  XX  */
		{{1,0},{2,0},{0,1},{1,1}}	/*      */
	},
	{	
		{{2,0},{1,1},{2,1},{1,2}},	/*      */
		{{0,0},{1,0},{1,1},{2,1}},	/*  XX  */
		{{2,0},{1,1},{2,1},{1,2}},	/* XX   */
		{{0,0},{1,0},{1,1},{2,1}}	/*      */
	},
	{
		{{1,0},{1,1},{2,1},{1,2}},	/*  X   */
		{{1,0},{0,1},{1,1},{2,1}},	/* XXX  */
		{{1,0},{0,1},{1,1},{1,2}},	/*      */
		{{0,1},{1,1},{2,1},{1,2}}	/*      */
	}
};		

// Converts (y, x) cordinates into pointer to particular location in well_data
char *yx2pointer(int y, int x)
{
	return well_data + (y * WELL_WIDTH) + x;
}

//Redraws well with the new fixed block
void update_well(int start, int lines)
{
	int y, x;
	
	for (y = start; y < (start + lines); y++)	{
		wmove(w1, y, 0);
		for (x = 0; x < WELL_WIDTH; x++) {
			wattrset(w1, COLOR_PAIR(1));
			mvwprintw(w1, y, 2 * x, "  ");
		}
	}
	wrefresh(w1);
}


// Fixes the block in a particular location in well
void fix_block(int y, int x, int type,int orient) {
	int i;
	DOT dot;
	for (i = 0; i < 4; i++) {
		dot = block_data[type][orient][i];
		*yx2pointer(y + dot.y, x + dot.x) = 1;
	}
//	update_well(y, 4);
}

//checks if the block can be moved to the next location
int check_pos(int y, int x, int type, int orient) {
	int i;
	DOT dot;
	for(i = 0; i < 4; i++) {
		dot = block_data[type][orient][i];
		if ((y + dot.y > WELL_HEIGHT - 1)		||
		    (x + dot.x < 0) 				||
		    (x + dot.x > WELL_WIDTH/2 - 1)		||
		    ((*yx2pointer(y + dot.y, x + dot.x) > 0))
		   )
		        	return 0;
	}
	return 1;
	
}


// Draws or erases the block depending on the delete value
void draw_block(WINDOW *win, int y, int x, int type, int orient, char delete)
{	
	int i;
	DOT dot; 
		
	for (i = 0; i < 4; i++) {
		dot = block_data[type][orient][i];
		wattron(win, COLOR_PAIR(delete ? 2 : 1));
		mvwprintw(win, y + dot.y, 2*(x + dot.x), "  ");
	}
	if (delete == 0)
		wrefresh(win);
}


/* Drops block till it reaches either well floor or another line
 * Uses time variables
 * Controls the block while it falls
 */
int drop_block(int type, int level) {
	int mid = WELL_WIDTH / 2 - 2;
	int y = 0;
	int x = mid/2;
	int orient = 0;
	char ch;
	fd_set t1, t2;
	struct timeval timeout;
	int sel_ret;
	if (0 == check_pos(y, x, type, orient))			//check if game over
		return -1;

	timeout.tv_sec = 0;
	timeout.tv_usec = delay[level];

	FD_ZERO(&t1);			//initialise
	FD_SET(0, &t1);
	
	draw_block(w1, y, x, type, orient, 0);
	
	while(1) {
		t2 = t1;
		sel_ret = select(FD_SETSIZE, &t2, (fd_set *) 0, (fd_set *) 0, &timeout);
	
		ch = getch();
	
		switch (ch) {
			case 'j':							//-------------------------------move left
				if(check_pos(y, x - 1, type, orient)) {
					draw_block(w1, y, x, type, orient, 1);
					draw_block(w1, y, --x, type, orient, 0);
				}
				break;
			case 'l':							//-------------------------------move right
				if(check_pos(y, x + 1, type, orient)) {
					draw_block(w1, y, x, type, orient, 1);
					draw_block(w1, y, ++x, type, orient, 0);
				}
				break;
			case 'i':							//-----------------------------------rotate
				if (check_pos(y, x, type,	orient + 1 == 4 ? 0 : orient + 1)) {
					draw_block(w1, y, x, type, orient, 1);
					++orient == 4 ? orient = 0 : 0;
					draw_block(w1, y, x, type, orient, 0);
				}
				break;
			case 'k':							//---------------------------------move down
				sel_ret = 0;
				break;
		}
		
		if(sel_ret == 0) {
			if(check_pos(y + 1, x, type, orient)) {
				draw_block(w1, y, x, type, orient, 1);
				draw_block(w1, ++y, x, type, orient, 0);
			}
			else {							//------------------------------- fix block in place
				fix_block(y, x, type, orient);
				return y;
			}		
			timeout.tv_sec = 0;
			timeout.tv_usec = delay[level];
		}
	}
}

/* Actual loop of the game.
 * initialises well_data, calls drop_block, sets score
 *
 */

POINTS play_game(int level) {

	POINTS points, temp;
	int i, curr, y;

	well_data = (unsigned char *)malloc(WELL_HEIGHT * WELL_WIDTH);
	for(i = 0; i < (WELL_HEIGHT * WELL_WIDTH); i++)
		well_data[i] = 0;
//	memset(well_data, 0, WELL_HEIGHT * WELL_WIDTH);	
//	update_screen();
	//wclear(w1);

	points.points = 0;
	points.lines = 0;
	points.level = 0;
	
	while(1) {
		curr = rand() % 7;
		y = drop_block(curr, points.level);
		if(y > 0) {
			temp = check_lines(y);
		}


	if(y == -1) {delwin( w1 );
	endwin();}
			
	}
}


int main() {
//	WINDOW *w1;
	int ch, level, play = 1;
	keypad(w1, TRUE);
	initscr();
	if(menu() == 0) {delwin( w1 );
		endwin();
		return 0;}
	w1 = newwin( WELL_HEIGHT + 2, WELL_WIDTH + 2, 0, 0);
	box(w1, ACS_VLINE, ACS_HLINE);
	mvwprintw(w1, 0, 10, "GAME WINDOW");
	wrefresh( w1 );
start_color();
init_pair(1, COLOR_BLACK, COLOR_RED);
init_pair(2, COLOR_BLACK, COLOR_BLACK);

//	int y = 2, x = 2, type = 2, orient = 0;

	nodelay(stdscr, TRUE);
	while (play) {
		level = 0;
		play_game(level);
		//play = show_score(points, use_highscore);
	}
/*
	wrefresh( w1 );
	draw_block(w1, y, x, type, orient, 0);
	wrefresh( w1 );
	while(( ch = wgetch(w1)) != 'q') {
	//	draw_block(w1, y+4, x+4, type, orient, 0);
		switch (ch) {
		case 'j':
			draw_block(w1, y, x, type, orient, 1);
			draw_block(w1, y, --x, type, orient, 0);
			break;
		case 'l':
			draw_block(w1, y, x, type, orient, 1);
			draw_block(w1, y, ++x, type, orient, 0);
			break;
		case 'k':
			draw_block(w1, y, x, type, orient, 1);
			draw_block(w1, ++y, x, type, orient, 0);
			wrefresh( w1 );
			break;
		case 'i':
			draw_block(w1, y, x, type, orient, 1);
			draw_block(w1, --y, x, type, orient, 0);
			break;
		}
	wrefresh( w1 );
	}
*/	delwin( w1 );
	endwin();
}
