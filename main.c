#define _XOPEN_SOURCE_EXTENDED
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>
#include <locale.h>
#ifdef	__unix__
	#include <ncursesw/ncurses.h>
#elif	__APPLE__
	#include <ncurses.h>
#endif

#define WAIT_TIME		96
#define MARGIN_L		9
#define MARGIN_U		2
#define MARGIN_ENDX		58
#define ACCEL			0x400
#define BRAKE			0x3

#define MODE_NOBRAKE	0x80000000

struct status {
	int max_x, max_y;	// 最大画面サイズ
	int cur_x, cur_y;	// カーソル位置
	int now;			// 選択中byte
	bool hi;			// true なら 左側の4bit(X0)
	int num;			// 選択中byte
	int scr, scr2, spd; // スクロール関連
	int c, c2;			// 入力関連
	unsigned int mode;	// モード
};

void bufInit(uint8_t buf[], int max){
	for(int i = 0; i < max; i++)
		buf[i] = i & 0xff;
}

void incnum(struct status* stt){
	if(stt->cur_x++ == 0x10)
		stt->cur_y++;
	stt->cur_x &= 0x0F;
}

////////////////////

void inpLogic(struct status* stt){
	stt->c2 = stt->c;
	stt->c  = wgetch(stdscr);
}

void preLogic(struct status* stt){
	// 入力
	switch(stt->c){
		case KEY_RIGHT: stt->cur_x++; break;
		case KEY_LEFT:  stt->cur_x--; break;
		case KEY_DOWN:  stt->cur_y++; break;
		case KEY_UP:    stt->cur_y--; break;
		
		case KEY_SRIGHT:    stt->spd += ACCEL; break;
		case KEY_SLEFT :    stt->spd -= ACCEL; break;
	}
	
	// カーソルロジック
	stt->cur_x &= 0xF;  stt->cur_y &= 0xF;
	stt->num = stt->scr2 + stt->cur_x + stt->cur_y * 16;
}

/////////////////////
char editbyte(char v, char buf, bool hi){
	// and して OR
	buf &= hi ? 0x0F : 0xF0;
	buf |= hi ? v<<4 : v<<0;
	return buf;
}

void mainLogic(struct status* stt, uint8_t buf[4096]){
	// 入力
	int v = -1;
	switch(stt->c){
		case 'x':	stt->scr += 0x100;	break;
		case 'z':	stt->scr -= 0x100;	break;
		case 'j':	v = 0;	break;
		case 'k':	v = 1;	break;
		case 'l':	v = 2;	break;
		case ';':	v = 3;	break;
		case 'u':	v = 4;	break;
		case 'i':	v = 5;	break;
		case 'o':	v = 6;	break;
		case 'p':	v = 7;	break;
		case 'f':	v = 8;	break;
		case 'd':	v = 9;	break;
		case 's':	v = 10;	break;
		case 'a':	v = 11;	break;
		case 'r':	v = 12;	break;
		case 'e':	v = 13;	break;
		case 'w':	v = 14;	break;
		case 'q':	v = 15;	break;
	}
	if(v >= 0) { // バイト編集
		buf[stt->num] = editbyte(v, buf[stt->num], stt->hi);
		stt->hi ^= 1;
		if(stt->hi) incnum(stt);
	}
	
	// スクロールロジック
	stt->scr += stt->spd;
	if(stt->scr < 0){
		stt->scr = 0;
		stt->spd = 0;
	}
	
	// スピードロジック
	int spd_sub = stt->spd >> BRAKE;
	if(stt->spd > 0) spd_sub++;  // ??????
	stt->spd -= spd_sub;
	
	// スクロール値 -> 表示スクロール値
	stt->scr2  = stt->scr >> 4;
	stt->scr2 &= 0xFFFFFFF0;
	
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

	// 00 01 02 03 ...	
	move(1, MARGIN_L-1);
	attron(COLOR_PAIR(2));
	for(int i = 0; i < 16; i++)
		printw(" %02X", i);
	attroff(COLOR_PAIR(2));
	
	// 00000004 ...
	move(1, 0);
	attron(COLOR_PAIR(1));
	for(int i = 0; i < stt.max_y - 1; i++)
		mvprintw(i + 1, 0, "%08X ", i * 0x10 + stt.scr2);
	attroff(COLOR_PAIR(1));

	// main data
	const unsigned char *tmp = buf + stt.scr2;
	move(MARGIN_U, MARGIN_L);
	for(int j = 0; j < stt.max_y - MARGIN_U; j++){
		//tmp = stt.scr2 + j*16;
		move(j + MARGIN_U, MARGIN_L);
		for(int i = 0; i < 16; i++)
			printw("%02X ", *tmp++);
	}

	// cursol
	int pos_x;
	pos_x = stt.cur_x * 3;
	move(MARGIN_U + stt.cur_y, MARGIN_L + pos_x);
	attron(COLOR_PAIR(3));
	printw("%02X", buf[stt.now]);
	attron(COLOR_PAIR(3));
	
	// debug
	mvprintw(2, MARGIN_ENDX, "spd = %08X", stt.spd);
	mvprintw(3, MARGIN_ENDX, "日本語あいう");
	mvprintw(4, MARGIN_ENDX, u"/u2580");
}

int main(int argv, char** args){
	// Init
	uint8_t buf[4096];
	FILE *fp;
	struct status stt = { 0 } ;
	setlocale(LC_ALL,"");
	
	// error check
	if(argv==2){
		fp = fopen(*(args+1),"w");
		if(fp == NULL)
			return 1;			
	}	
	
	// 初期化 : ncurses
	initscr(); clear(); noecho(); cbreak(); 
	nodelay(stdscr,TRUE); timeout(WAIT_TIME);
	getmaxyx(stdscr, stt.max_y, stt.max_x);
	keypad(stdscr, TRUE);
	start_color();
	init_color(0, 1000, 1000, 1000); // 白
	init_color(7,  333,  333,  333); // 濃灰
	init_color(1,  666,  666,  666); // 淡灰
	init_color(2, 1000,  600,    0); // 橙
	init_pair(1, 0, 7); // 1.見出し
	init_pair(2, 0, 1); // 2.サブ見出し
	init_pair(3, 0, 2); // 3.アクセント
	/* 色変えMAC対応していない・・・ */

	// 初期化 : myHexEd
	bufInit(buf, sizeof(buf));

	while(1){
		inpLogic(&stt);			// 入力
		preLogic(&stt);			// モードロジック
		mainLogic(&stt, buf);	// メインロジック
		pstLogic(stt, buf);		// 描画ロジック
		refresh();				// 描画
	}
	
	// おわり
	endwin();	
	return 0;
}
