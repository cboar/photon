#ifndef PHOTON_H
#define PHOTON_H

#define _WIN32_WINNT    0x0501
#define WINVER 0x0500
#include <Windows.h>
#include <winable.h>

/* Structs */
	enum { LEFT, RIGHT };
	typedef enum { false, true } bool;
	typedef unsigned char COLOR8;
	typedef unsigned int COLOR24;

	typedef struct Bitmap {
		int x, y, w, h;
		HBITMAP dib;
		COLOR24* pixels;
	} Bitmap;

	typedef struct Point {
		int x, y;
	} Point;
	typedef struct Pixel {
		COLOR24 color;
		int x, y;
	} Pixel;
	typedef struct Cloud {
		Pixel* pixels;
		size_t size;
	} Cloud;

	typedef struct ColorHSL {
		double hue, sat, lum;
	} ColorHSL;

	typedef struct MouseSequence {
		size_t size;
		MouseEvent* events;
	} MouseSequence;
	typedef struct MouseEvent {
		char x, y, delta;
	} MouseEvent;

/* Init */
	void photon_init(int, int, int, int);
	void load_mousedata(char*);
	Cloud cloud(char*);
	int time();
	int elapsed(int);

/* Utility */
	int rng(int, int);
	int deviate(int, double);
	bool chance(double);
	void usleep_ex(int);
	void usleep(int, int);
	void sleep_ex(int);
	void sleep(int, int);

/* Vision */
	void screenshot();
	bool find(Cloud*);
	bool find_at(Cloud*, int, int);
	bool find_in(Cloud*, int, int, int, int);
	bool find_xy(Cloud*, int*, int*);
	bool find_xya(Cloud*, int*, int*, int, int, int, int);
	
/* Color */
	COLOR24 color_at(int, int);
	ColorHSL colorHSL(COLOR24);

/* Mouse */
	bool mouse_isdown(int);
	void mouse_pos(int*, int*);
	void mouse_jump(int, int);
	void mouse_down(int);
	void mouse_up(int);
	void mouse_click(int);

/* Keyboard */
	void key_down(unsigned int);
	void key_up(unsigned int);
	void key_press(unsigned int);

#endif
