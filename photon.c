#include "twister.h"
#include "photon.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/* Init */
static HDC screen;
static HDC compatibleDC;
static Bitmap bmp;

void photon_init(int x, int y, int w, int h){
	mt_seed(time(NULL));
	screen = GetDC(NULL);
	compatibleDC = CreateCompatibleDC(screen);

	COLOR24* buffer = malloc(sizeof(*buffer) * w * h);
	BITMAPINFO info;
	memset(&info, 0, sizeof(info));
	info.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biBitCount	= 32;
	info.bmiHeader.biPlanes		= 1;
	info.bmiHeader.biWidth		= w;
	info.bmiHeader.biHeight		= -h;

	HBITMAP hbmp = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void**) &buffer, NULL, 0);
	bmp = (Bitmap){ x, y, w, h, hbmp, buffer };
	SelectObject(compatibleDC, bmp.dib);
}

MouseSequence read_sequence(FILE* file){
	size_t capacity = 32;
	MouseEvent* events = malloc(sizeof(MouseEvent) * capacity);

	size_t index = 0;
	char delta, x, y;
	while(fscanf(file, "%d:%d:%d ", &delta, &x, &y)){
		events[index++] = (MouseEvent){ x, y, delta };
		if(index == capacity){
			capacity += 32;
			events = realloc(sizeof(MouseEvent) * capacity);
		}
	}
	return (MouseSequence){ index, events };
}

void load_mousedata(char* fn){
	FILE* file = fopen(fn, "r");

	read_sequence(file);

	fclose(file);
}

Cloud cloud(char* str){
	Pixel* pixels = malloc(sizeof(*pixels) * 32);

	size_t i = 0, count = 0;
	while(str[i]){
		COLOR24 col = 0;
		for (int j = 0; j < 4; j++)
			col += ((str[i++] - 35) << (6 * j));

		while(str[i]){
			int x = str[i++] - 80;
			if (x == -47)
				break;
			int y = str[i++] - 80;
			pixels[count++] = (Pixel){ col, x, y };
			//printf("%d, %d: %06x\n", x, y, col);
		}
	}
	pixels = realloc(pixels, sizeof(*pixels) * count);
	return (Cloud){ pixels, count };
}

int time(){
	return GetTickCount();
}

int elapsed(int since){
	return (int)(GetTickCount() - since);
}

/* Utility */
int rng(int min, int max){
	return min + (mt_rand() % (1 + max - min));
}

int deviate(int base, double factor){
	return rng((int)(base*(1.0-factor)), (int)(base*(1.0+factor)));
}

bool chance(double percent){
	return ((double)(mt_rand()) / UINT_MAX) <= percent;
}

void usleep_ex(int us){
	LARGE_INTEGER a;

	QueryPerformanceFrequency(&a);
	const double t_freq = ((double) a.QuadPart) / 1000000.0;
	QueryPerformanceCounter(&a);
	const long long t_start = a.QuadPart;

	int t_spent = 0;
	while(t_spent < us) {
		QueryPerformanceCounter(&a);
		t_spent = (a.QuadPart - t_start) / t_freq;
	}
}

void usleep(int min, int max){
	usleep_ex(rng(min, max));
}

void sleep_ex(int ms){
	usleep_ex(ms * 1000);
}

void sleep(int min, int max){
	usleep(min * 1000, max * 1000);
}

/* Vision */
void screenshot(){
	BitBlt(compatibleDC, 0, 0, bmp.w, bmp.h, screen, bmp.x, bmp.y, SRCCOPY);
}

bool find_sub(Cloud* cloud, int ax, int ay, int x1, int y1, int x2, int y2){
	for(size_t i = 1; i < cloud->size; i++){
		Pixel pixel = cloud->pixels[i];
		int cx = pixel.x + ax;
		int cy = pixel.y + ay;
		if(cx < x1 || cy < y1 || cx >= x2 || cy >= y2 || color_at(cx, cy) != pixel.color)
			return false;
	}
	return true;
}

bool find_sub_a(Cloud* cloud, int ax, int ay){
	int x2 = bmp.x + bmp.w, y2 = bmp.y + bmp.h;
	return find_sub(cloud, ax, ay, bmp.x, bmp.y, x2, y2);
}

bool find(Cloud* cloud){
	int x, y;
	return find_xy(cloud, &x, &y);
}

bool find_at(Cloud* cloud, int x, int y){
	COLOR24 target = cloud->pixels[0].color;
	return (color_at(x, y) == target && find_sub_a(cloud, x, y));
}

bool find_in(Cloud* cloud, int x, int y, int w, int h){
	int dx, dy;
	return find_xya(cloud, &dx, &dy, x, y, w, h);
}

bool find_xy(Cloud* cloud, int* ox, int* oy){
	return find_xya(cloud, ox, oy, bmp.x, bmp.y, bmp.w, bmp.h);
}

bool find_xya(Cloud* cloud, int* ox, int* oy, int x, int y, int w, int h){
	COLOR24 target = cloud->pixels[0].color;
	int x2 = x + w, y2 = y + h;

	for(int cx = x; cx < x2; cx++){ for(int cy = y; cy < y2; cy++){
		if(color_at(cx, cy) == target && find_sub(cloud, cx, cy, x, y, x2, y2)){
			*ox = x;
			*oy = y;
			return true;
		}
	}}
	return false;
}

inline COLOR24 color_at(int x, int y){
	return bmp.pixels[(y * bmp.w) + x] ^ 0xff000000;
}

/* Mouse */
void mouse_send(int x, int y, int flags){
	INPUT in = {0};
	in.type = INPUT_MOUSE;
	in.mi.dx = x * (65536.0 / 1919);
	in.mi.dy = y * (65536.0 / 1079);
	in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | flags;
	SendInput(1, &in, sizeof(in));
}

void mouse_send_here(int flags){
	int cx, cy;
	mouse_pos(&cx, &cy);
	mouse_send(cx, cy, flags);
}

bool mouse_isdown(int btn){
	return (GetKeyState(btn ? VK_RBUTTON : VK_LBUTTON) & 0x100) != 0;
}

void mouse_pos(int *x, int *y) {
	POINT p;
	GetCursorPos(&p);
	*x = p.x;
	*y = p.y;
}

void mouse_jump(int x, int y){
	mouse_send(x, y, MOUSEEVENTF_MOVE);
}

void mouse_down(int btn){
	mouse_send_here((btn ? 4 : 1) * MOUSEEVENTF_LEFTDOWN);
}

void mouse_up(int btn){
	mouse_send_here((btn ? 4 : 1) * MOUSEEVENTF_LEFTUP);
}

void mouse_click(int btn){
	mouse_down(btn);
	mouse_up(btn);
}

/* Keyboard */
void key_send(int vkey, int flag){
	INPUT in = {0};
	in.type = INPUT_KEYBOARD;
	in.ki.wVk = vkey;
	in.ki.dwFlags = flag;
	SendInput(1, &in, sizeof(in));
}

void key_down(unsigned int vkey) {
	key_send(vkey, 0);
}

void key_up(unsigned int vkey) {
	key_send(vkey, KEYEVENTF_KEYUP);
}

void key_press(unsigned int vkey) {
	key_down(vkey);
	key_up(vkey);
}
