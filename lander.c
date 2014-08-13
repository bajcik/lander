/*
        file: lander.c
 description:
     license: GPL
      author: Krzysztof Garus <kgarus@bigfoot.com>
              http://www.bigfoot.com/~kgarus
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>

#define TLO_X 6
#define TLO_Y 6
#define TLO_W 566
#define TLO_H 442

#define FUEL_W 30
#define FUEL_H 150
#define FUEL_Y1 3
#define FUEL_Y2 134
#define FUEL_X1 6
#define FUEL_X2 23
#define FUEL_IW (FUEL_X2-FUEL_X1+1)
#define FUEL_IH (FUEL_Y2-FUEL_Y1+1)

#define DXFUEL 0.001
#define DYFUEL 0.003

#define L_DKVX 0.2
#define L_DKVY 0.3
#define GRAVITY 0.1
#define INTERVAL 40 /* ticks */
#define LAND_VY 1.4
#define LAND_VX 0.3

#define NEXT_BLOCK_TICKS 50
#define FILL_TICKS 50

#define LIVES 5

SDL_Surface *tlo, *ekran, *lander[9], *led_on, *led_off, *fuel0, *fuel1;
SDL_Rect *kw;
int Nkw = 0, kwland = 0;
double fuel = 0.0;

SDL_Rect
R(int x, int y, int w, int h)
{
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}

int
rect_collision(SDL_Rect r1, SDL_Rect r2)
{
	int xcol = (r1.x < r2.x+r2.w) && (r2.x < r1.x+r1.w);
	int ycol = (r1.y < r2.y+r2.h) && (r2.y < r1.y+r1.h);

	return xcol && ycol;
}

void
na_ekran(SDL_Surface *co, int x0, int y0, int w0, int h0, int x1, int y1)
{
	SDL_Rect srec, drec;
	srec.x = x0;
	srec.y = y0;
	srec.w = w0;
	srec.h = h0;
	drec.x = x1;
	drec.y = y1;
	SDL_BlitSurface(co, &srec, ekran, &drec);
}

#define na_plansze(co,x0,y0,w0,h0,x1,y1) na_ekran((co),(x0),(y0),(w0),(h0),TLO_X+(x1),TLO_Y+(y1))

/* Lander */
enum lander_state
{
	l_normal = 0,
	l_crash,
	l_up,
	l_right,
	l_left,
	l_win,
	l_fill,
	l_up_right,
	l_up_left
};

int lives = 0;
int immortal = 0;
enum lander_state state = l_normal;
double  l_x = 0.0,  l_y = 0.0,  /* po³o¿enie (wzglêdem TLO_X/TLO_Y) */
       l_vx = 0.0, l_vy = 0.0,  /* prêdko¶ci; vy - prêdko¶æ w dó³ (ku wy¿szym wspó³rzêdnym) */
	   l_oldx = 0.0, l_oldy = 0.0;


void
lander_update()
{
	int l_ooldx = l_oldx;
	int l_ooldy = l_oldy;
	na_plansze(tlo, l_oldx,l_oldy,30,40, l_oldx,l_oldy);
	
	l_oldx = l_x; l_oldy = l_y;
	na_plansze(lander[state], 0, 0, 30, 40, l_x, l_y);

	SDL_UpdateRect(ekran, TLO_X+l_ooldx, TLO_Y+l_ooldy, 30, 40);
	SDL_UpdateRect(ekran, TLO_X+l_x, TLO_Y+l_y, 30, 40);
}

/* -------------------- LEDs */
SDL_Rect led_yok = {575, 10, 25, 25};
SDL_Rect led_xok = {575, 40, 25, 25};

void
set_led(SDL_Rect rec, int on)
{
	SDL_Rect rec2 = rec;
	boxRGBA(ekran, rec.x, rec.y, rec.x+rec.w-1, rec.y+rec.h-1, 0, 0, 0, 255);
	SDL_BlitSurface(on ? led_on:led_off, NULL, ekran, &rec);	
	SDL_UpdateRect(ekran, rec2.x, rec2.y, rec2.w, rec2.h);
	//printf("x=%d y=%d w=%d h=%d\n", rec2.x, rec2.y, rec2.w, rec2.h);
}

/* num = 0 .. (LIVES-1) */
void
set_live(int num, int alive)
{
	SDL_Rect rec;
	rec.x = 610;
	rec.y = 10 + num * 43;
	rec.w = 30;
	rec.h = 40;
	boxRGBA(ekran, rec.x, rec.y, rec.x+rec.w-1, rec.y+rec.h-1, 0, 0, 0, 255);
	SDL_BlitSurface(lander[alive ? l_normal:l_crash], NULL, ekran, &rec);	
	SDL_UpdateRect(ekran, rec.x, rec.y, rec.w, rec.h);
}

void
set_fuel(double l)
{
	SDL_Rect semptyrec = R(0, 0, FUEL_W, FUEL_H);
	SDL_Rect sfullrec = R(0, 0, FUEL_W, FUEL_H);
	SDL_Rect demptyrec  = R(575, 70, FUEL_W, FUEL_H);
	SDL_Rect dfullrec  = R(575, 70, FUEL_W, FUEL_H);
	SDL_Rect updrec = demptyrec;
	int part = (1.0-l)*(FUEL_Y2-FUEL_Y1); /* 0 (l==1.0) <-> FUEL_Y2-FUEL_y1 (l==0.0) */
	
	semptyrec.h = FUEL_Y1+part;
	sfullrec.y  = FUEL_Y1+part;
	sfullrec.h  = FUEL_H-sfullrec.y;
	
	dfullrec.y += sfullrec.y;
	
	boxRGBA(ekran, updrec.x, updrec.y, updrec.x+updrec.w-1, updrec.y+updrec.h-1, 0, 0, 0, 255);
	SDL_BlitSurface(fuel0, &semptyrec, ekran, &demptyrec);	
	SDL_BlitSurface(fuel1, &sfullrec, ekran, &dfullrec);	
	SDL_UpdateRect(ekran, updrec.x, updrec.y, updrec.w, updrec.h);
	
}

/* is_target:
 *   1.0 - target (fioletowy)
 *   0.0 - standard (czerwony)
 *   po¶rednie - po¶rednie
 * alpha==0 ==> remove block */
void
alpha_block(SDL_Rect rec, double is_target, int alpha)
{
	int r,g,b;
	
	r = is_target*80 + (1.0-is_target)*200;
	g = is_target*70  + (1.0-is_target)*130;
	b = is_target*130 + (1.0-is_target)*30;

	na_ekran(tlo, rec.x, rec.y, rec.w, rec.h, TLO_X+rec.x, TLO_Y+rec.y);
	boxRGBA(ekran, TLO_X+rec.x, TLO_Y+rec.y, TLO_X+rec.x+rec.w-1, TLO_Y+rec.y+rec.h-1, r,g,b, alpha*4/5);
	rectangleRGBA(ekran, TLO_X+rec.x, TLO_Y+rec.y, TLO_X+rec.x+rec.w-1, TLO_Y+rec.y+rec.h-1, r/3, g/3, b/3, alpha);
	SDL_UpdateRect(ekran, TLO_X+rec.x, TLO_Y+rec.y, rec.w, rec.h);
}

int
w_planszy()
{
	int r = 1;
	if (l_x < 0) {l_x = 0; l_vx = 0; r=0;}
	if (l_y < 0) {l_y = 0; l_vy = 0; r=0;}
	if (l_x+30 > TLO_W) {l_x = TLO_W-30-1; l_vx = 0; r=0;}
	if (l_y+40 > TLO_H) {l_y = TLO_H-40-1; l_vy = 0; r=0;}
	
	return r;
}

int
inicjuj()
{
	FILE *f = fopen("./rec", "r");
	int i;
	
	if (!f)
		return -1;

	fscanf(f, "%d", &Nkw);
	kw = (SDL_Rect *)malloc(Nkw*sizeof(SDL_Rect));
	if (kwland >= Nkw) kwland = Nkw-1;                /* je¶li opcja -b z³a */
	for (i=0; i<Nkw; i++)
	{
		fscanf(f, "%d", &kw[i].x);
		fscanf(f, "%d", &kw[i].y);
		fscanf(f, "%d", &kw[i].w);
		fscanf(f, "%d", &kw[i].h);
	}

	SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER );
	
	tlo = IMG_Load("./tlo.png");
	lander[0] = IMG_Load("./lander0.png");
	lander[1] = IMG_Load("./lander1.png");
	lander[2] = IMG_Load("./lander2.png");
	lander[3] = IMG_Load("./lander3.png");
	lander[4] = IMG_Load("./lander4.png");
	lander[5] = IMG_Load("./lander5.png");
	lander[6] = IMG_Load("./lander6.png");
	lander[7] = IMG_Load("./lander7.png");
	lander[8] = IMG_Load("./lander8.png");

	led_off = IMG_Load("./led-off.png");
	led_on = IMG_Load("./led-on.png");
	
	fuel0 = IMG_Load("./fuel0.png");
	fuel1 = IMG_Load("./fuel1.png");

	ekran = SDL_SetVideoMode( 640, 480, 16, SDL_ANYFORMAT);

	lives = LIVES;
	for (i=0; i<LIVES; i++)
		set_live(i, i<lives);
	
	rectangleRGBA(ekran, TLO_X-1, TLO_Y-1, TLO_X+TLO_W, TLO_Y+TLO_H, 120,180,30, 255);
	na_ekran(tlo, 0,0,TLO_W,TLO_H, TLO_X,TLO_Y);
	
	for (i=kwland; i<Nkw; i++)
		alpha_block(kw[i], i==kwland ? 1.0:0.0, 255);

	set_fuel(fuel);
	set_led(led_xok, 0);
	set_led(led_yok, 0);
	set_fuel(fuel);
	lander_update();
	
	SDL_Flip(ekran);
	return 0;
}


#define WAITING     1
#define MOVING      2
#define NEXT_BLOCK  3
#define FILL        4
/*#define MOVING  1*/

void
mainloop()
{
	Uint8 *klawisze;    /* Uint8 to liczba jednobajtowa (char) */
	int esc=0;
	int next_time = SDL_GetTicks() + INTERVAL;
	int delay;
	int i;
	int game_state = WAITING;
	int to_00 = 0;
	int waiting_for_state = FILL;
	double progress;
	
	while(1)                                                 
	{
		/* SDL_PumpEvents() przetwarza ostatnie zdarzenia ( np. nacisniecie klawisza lub jednoczesnie kilku klawiszy( co jest wazne w grach ) ) */
		SDL_PumpEvents();
		/* SDL_GetKeyState() zwroci nam adres tablicy danych jednobajtowych, w ktorej sa zapisane ostatnie zdarzenia */
		klawisze = SDL_GetKeyState( NULL );
		
		if( klawisze[ SDLK_ESCAPE ] == SDL_PRESSED )
			break;

		if (game_state == MOVING)
		{
			if( klawisze[ SDLK_DOWN ] == SDL_PRESSED &&
			    klawisze[ SDLK_RIGHT ] == SDL_PRESSED  && fuel >0.0)
			{
				l_vy -= L_DKVY;
				l_vx += L_DKVX;
				state = l_up_right;
				fuel -= DYFUEL+DXFUEL;
			}
			else if( klawisze[ SDLK_DOWN ] == SDL_PRESSED &&
			         klawisze[ SDLK_LEFT ] == SDL_PRESSED  && fuel >0.0)
			{
				l_vy -= L_DKVY;
				l_vx -= L_DKVX;
				state = l_up_left;
				fuel -= DYFUEL+DXFUEL;
			}
			else if( klawisze[ SDLK_DOWN ] == SDL_PRESSED && fuel >0.0)
			{
				l_vy -= L_DKVY;
				state = l_up;
				fuel -= DYFUEL;
			}
			else if( klawisze[ SDLK_LEFT ] == SDL_PRESSED  && fuel >0.0)
			{
				l_vx -= L_DKVX;
				state = l_left;
				fuel -= DXFUEL;
			}
			else if( klawisze[ SDLK_RIGHT] == SDL_PRESSED  && fuel >0.0)
			{
				l_vx += L_DKVX;
				state = l_right;
				fuel -= DXFUEL;
			}
			else
				state = l_normal;
			
			if (fuel < 0.0) fuel = 0.0;
			set_fuel(fuel);
		
			/* grawitacja */
			l_vy += GRAVITY;
			
			/* czy osiadli¶my na kw[kwland] */
			set_led(led_xok, l_vx > -LAND_VX && l_vx < LAND_VX);
			set_led(led_yok, l_vy >=0 && l_vy < LAND_VY);
		
			/* po³o¿enie */
			l_x += l_vx;
			l_y += l_vy;
			
			/* kolizja z brzegiem */
			if (!w_planszy())
				state = l_crash;
			
			/* kolizja z blokiem */
			for (i=kwland; i<Nkw; i++)
				if (rect_collision(kw[i], R(l_x, l_y, 30, 40)))
				{
					state = l_crash;
					break;
				}
			
			/* czy nie osiadli¶my */
			{
				int d_x, d_y;
				d_x = l_x + 30/2;
				d_y = l_y + 40;
				
				if (   d_x > kw[kwland].x+2 && d_x <  kw[kwland].x+kw[kwland].w-1-2
				    && d_y < kw[kwland].y   && d_y >= kw[kwland].y-2
					&& l_vy >=0 && l_vy < LAND_VY
					&& l_vx > -LAND_VX && l_vx < LAND_VX)
				{
					l_y = kw[kwland].y-1-40;
					l_x = kw[kwland].x + kw[kwland].w/2 - 30/2;
					state = l_win;
					game_state = NEXT_BLOCK;
					progress = 0.0;
					l_vx = l_vy = 0;
				}
			}
			
			if (state == l_crash)
			{
				l_vx = l_vy = 0;
				l_x = l_oldx;
				l_y = l_oldy;
				if (!immortal)
					set_live(--lives, 0);
				game_state = WAITING;
				waiting_for_state = lives ? FILL:0;
				to_00 = 1;
				progress = 0.0;
				fuel = 0.0;
			}
			
			lander_update(state);
		}
		else if (game_state == NEXT_BLOCK)
		{
			progress += 1.0/NEXT_BLOCK_TICKS;
			if (progress >= 1.0)
			{
				alpha_block(kw[kwland++], 0.0, 0);
				if (kwland < Nkw)
				{
					alpha_block(kw[kwland], 1.0, 255);
					game_state = MOVING;
					//l_x = l_y = l_vy = l_vy = 0.0;
				}
				else
				{
					SDL_Rect srec=R(l_x,l_y,30,40),
					         drec=R(TLO_X+l_x, TLO_Y+l_y, 30,40);
					SDL_BlitSurface(tlo, &srec, ekran, &drec);	
					SDL_UpdateRect(ekran, drec.x, drec.y, 30,40);
					game_state = WAITING;
					waiting_for_state = 0;
				}
				
			}
			else
			{
				alpha_block(kw[kwland], 1.0, (1-progress)*255);
				if (kwland < Nkw-1)
					alpha_block(kw[kwland+1], progress, 255);
			}
			
		}
		else if (game_state == FILL)
		{
			fuel += 1.0/FILL_TICKS;
			if (fuel >= 1.0)
			{
				fuel = 1.0;
				game_state = MOVING;
			}
			set_fuel(fuel);
		}
		else if (game_state == WAITING)
		{
			if( klawisze[ SDLK_SPACE ] == SDL_PRESSED )
			{
				game_state = waiting_for_state;
				if (game_state == FILL)
				{
					state = l_fill;
					lander_update();
				}
				set_fuel(fuel);
			}
			if (to_00)
			{
				l_x = l_y = l_vy = l_vy = 0.0;
				to_00 = 0;
			}
		}
		else
			break;
		
		/* delay */
		delay = next_time - SDL_GetTicks();
		if (delay < 0)
			delay = 0;
		SDL_Delay(delay);
		next_time += INTERVAL;
	}
	
}

int main(int argc, char *argv[])
{
	int c;

	while ((c=getopt(argc, argv, "vib:")) != -1)
		switch (c)
		{
			case 'v': printf("lander 1.0\n"); return 0;
			case 'i': immortal = 1; break;
			case 'b': kwland = strtol(optarg,NULL,10); break;
			default: return 1;
		}
	
	if (kwland < 0) kwland = 0;
	
	if (inicjuj() != 0)
	{
		fprintf(stderr, "B³±d przy wczytaniu trójk±tów\n");
		exit(1);
	}
	
	mainloop();
	
	return 0;
} /* main */


