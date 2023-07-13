#define STB_DS_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include "stb_ds.h"
#include "stb_sprintf.h"

#undef sprintf
#define sprintf stbsp_sprintf
#define SQUARE(x) (x*x)

typedef struct Player Player;

struct Player {
	float x;
	float y;
	float width;
	float height;
	float directx;
	float speedx;
	float speedy;
	float accel;
	float mass;
	Color debugcolor;
	bool grounded;
};

const int screenWidth = 1500;
const int screenHeight = 1000;
const float groundLevel = screenHeight - 0.3*screenHeight;
const float friction = 0.15;

#define G (float)(3.0)
#define PLAYER_JUMP (float)(-35.0)
#define PLAYER_BOX_WIDTH 15.0
#define PLAYER_BOX_HEIGHT 30.0
#define PLAYER_INITIAL_Y (groundLevel-PLAYER_BOX_HEIGHT)
#define PLAYER_INITIAL_X 100.0

#define FRAMETIME (float)(0.6666666)

int main() {
	char buf[64];

	Player mario = {
		.x = PLAYER_INITIAL_X, .y = PLAYER_INITIAL_Y,
		.width = PLAYER_BOX_WIDTH, .height = PLAYER_BOX_HEIGHT,
		//.direct = (Vector2){ .x = 0, .y = 0 },
		.directx = 0,
		.speedx = 0.0, // m/s
		.speedy = 0.0, // m/s
		.accel = 3.0, // m/s^2
		.mass = 30.0,
		.debugcolor = GREEN,
		.grounded = true,
	};

	InitWindow(screenWidth, screenHeight, "platform");
	SetTargetFPS(60);

	while(!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);
		float fps = GetFPS();
		float frametime = GetFrameTime();

		sprintf(buf, "FPS: %f\nTime per frame: %f\nPlayer speed: %f\ngrounded = %i\n",
				fps, frametime, mario.speedx, mario.grounded);

		DrawText(buf, 10, 10, 15, RAYWHITE);
		DrawRectangleGradientV(0, groundLevel, screenWidth, screenHeight-groundLevel, RED, BLACK);
		DrawRectangle(mario.x, mario.y, mario.width, mario.height, mario.debugcolor);

		EndDrawing();

		bool movekeydown = false;
		float accel = mario.accel;
		float directx = 0;
		if(IsKeyDown(KEY_UP) && mario.grounded) {
			mario.speedy = PLAYER_JUMP;
			mario.grounded = false;
		}
		if(IsKeyDown(KEY_LEFT)) {
			movekeydown = true;
			directx = -1.0;
		}
		if(IsKeyDown(KEY_RIGHT)) {
			movekeydown = true;
			directx = 1.0;
		}
		if(!movekeydown) {
			accel = 0;
			directx = mario.directx;
		} else {
			mario.directx = directx;
		}

		/*
		 * 0.5*a*t^2 + v*t + p0
		 * a*t + v0
		 * a
		 */

		mario.speedx += accel*FRAMETIME - friction*mario.speedx;
		float offsetx = 0.5*accel * SQUARE(FRAMETIME) + mario.speedx*FRAMETIME;
		float offsety = 0.5*G*SQUARE(FRAMETIME) + mario.speedy*FRAMETIME;
		mario.speedy += G*FRAMETIME;
		float newposx = mario.directx * offsetx;
		float newposy = offsety;
		//Vector2 newpos = Vector2Scale(mario.direct, offset);
		mario.x += newposx;
		if(mario.y+offsety+mario.height > groundLevel) {
			mario.y = groundLevel - mario.height;
			mario.grounded = true;
			mario.speedy = 0.0;
		} else {
			mario.y += newposy;
		}
	}
}
