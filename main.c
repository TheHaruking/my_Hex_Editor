#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <linux/time.h>
#include <unistd.h>
#include <ncurses.h>

#define MARGIN_L 9
#define MARGIN_U 2

struct status {
	int max_x, max_y;
	int cur_x, cur_y;
	bool hi;	// true = left of byte
	int num;
	int scr, scr2, spd, spd2;
	int c, c2;
	int mode;
};

void bufInit(uint8_t buf[], int max){
	for(int i = 0; i < max; i++)
		buf[i] = i & 0xff;
}

void incnum(struct status* stt){
	stt->num++;
	stt->cur_x = stt->num & 0x0F;
	stt->cur_y = stt->num >>   4;
}

////////////////////

void inpLogic(struct status* stt){
	stt->c2 = stt->c;
	stt->c  = wgetch(stdscr);
}

void preLogic(struct status* stt){
	switch(stt->c){
		case KEY_RIGHT: stt->cur_x++; break;
		case KEY_LEFT:  stt->cur_x--; break;
		case KEY_DOWN:  stt->cur_y++; break;
		case KEY_UP:    stt->cur_y--; break;
	}
	stt->cur_x &= 0xF;  stt->cur_y &= 0xF;
	stt->num = stt->cur_x + stt->cur_y * 16;
}

/////////////////////
char editbyte1(char v, char buf, bool hi){
	buf &= hi ? 0x0F : 0xF0;
	buf |= hi ? v<<6 : v<<2;
	return buf;
}

char editbyte2(char v, char buf, bool hi){
	buf += hi ? v<<4 : v<<0;
	return buf;
}

char editbyte(char v, char buf, bool hi){
	buf &= hi ? 0x0F : 0xF0;
	buf |= hi ? v<<4 : v<<0;
	return buf;
}

void mainLogic(struct status* stt, uint8_t buf[4096]){
	int v = -1;
	switch(stt->c){
		case 'x':	buf[stt->num]++;	break;
		case 'z':	buf[stt->num]--;	break;
		case 'j':	v = 0;				break;
		case 'k':	v = 1;			break;
		case 'l':	v = 2;			break;
		case ';':	v = 3;			break;
		case 'u':	v = 4;			break;
		case 'i':	v = 5;			break;
		case 'o':	v = 6;			break;
		case 'p':	v = 7;			break;
		case 'f':	v = 8;				break;
		case 'd':	v = 9;			break;
		case 's':	v = 10;			break;
		case 'a':	v = 11;			break;
		case 'r':	v = 12;			break;
		case 'e':	v = 13;			break;
		case 'w':	v = 14;			break;
		case 'q':	v = 15;			break;
	}
	if(v >= 0) {
		buf[stt->num] = editbyte(v, buf[stt->num], stt->hi);
		stt->hi ^= 1;
		if(stt->hi) incnum(stt);
	}
}

void pstLogic(struct status stt, const uint8_t buf[4096]){
	// top(background)
	move(0, 0);	
	attron(COLOR_PAIR(3));
	for(int i = 0; i < stt.max_x; i++)
		printw(" ");
	attroff(COLOR_PAIR(3));

	// top(moji)
	move(0, 0);
	attron(COLOR_PAIR(3));
	printw("max_x : %d\tmax_y : %d", stt.max_x, stt.max_y);
	attroff(COLOR_PAIR(3));

	//00 01 02 03 ...	
	move(1, MARGIN_L-1);
	attron(COLOR_PAIR(2));
	for(int i = 0; i < 16; i++)
		printw(" %02X", i);
	attroff(COLOR_PAIR(2));
	
	//00000004 ...
	move(1, 0);
	attron(COLOR_PAIR(1));
	for(int i = 0; i < stt.max_y - 1; i++)
		mvprintw(i + 1, 0, "%08X ", i * 0x10);
	attroff(COLOR_PAIR(1));

	//main data
	move(MARGIN_U, MARGIN_L);
	for(int j = 0; j < stt.max_y - MARGIN_U; j++){
		move(j + MARGIN_U, MARGIN_L);
		for(int i = 0; i < 16; i++)
			printw("%02X ", buf[i+j*16]);
	}

	//cursol
	int pos_x;
	pos_x = stt.cur_x * 3;
	move(MARGIN_U + stt.cur_y, MARGIN_L + pos_x);
	attron(COLOR_PAIR(3));
	printw("%02X", buf[stt.num]);
	attron(COLOR_PAIR(3));
}

int main(){
	uint8_t buf[4096];
	struct status stt;

	initscr();
	clear();
	noecho();
	cbreak();
	start_color();
	init_color(0, 1000, 1000, 1000);
	init_color(7,  333,  333,  333);
	init_color(1,  666,  666,  666);
	init_color(2, 1000,  600,    0);
	init_pair(1, 0, 7);
	init_pair(2, 0, 1);
	init_pair(3, 0, 2);
	getmaxyx(stdscr, stt.max_y, stt.max_x);
	keypad(stdscr, TRUE);

	bufInit(buf, sizeof(buf));

	while(1){
		inpLogic(&stt);
		preLogic(&stt);
		mainLogic(&stt, buf);
		pstLogic(stt, buf);
		//usleep(1600);
		napms(16);
		refresh();
	}

	getch();
	endwin();	
	return 0;
}


