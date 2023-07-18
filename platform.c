#define STB_DS_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include "stb_ds.h"
#include "stb_sprintf.h"

#define ARRLEN(x) (sizeof(x)/sizeof(*x))
#define STRLEN(x) ((sizeof(x)/sizeof(*x))-1)
#undef sprintf
#define sprintf stbsp_sprintf
#define SQUARE(x) (x*x)
#define G (float)(7000.0)
#define PLAYER_JUMP (float)(-2800.0)
#define PLAYER_RUN  (float)(5000.0)
#define PLAYER_BOX_WIDTH 15.0
#define PLAYER_BOX_HEIGHT 30.0
#define HALF_PLAYER_BOX_WIDTH 7.5
#define HALF_PLAYER_BOX_HEIGHT 15
#define PLAYER_INITIAL_Y (groundLevel-PLAYER_BOX_HEIGHT)
#define PLAYER_INITIAL_X 100.0
#define MAX_DEPTH (float)(1200.0)

typedef struct Player Player;
//typedef struct Platform Platform;
//typedef struct Goomba Goomba;

struct Player {
	Vector2 pos;
	Vector2 vel;
	Vector2 accel;
	float width;
	float height;
	float mass;
	float invmass;
	Color debugcolor;
	bool grounded;
};

Rectangle platforms[100];
int platformCount = 4;
bool gameOver = false;

const int screenWidth = 1500;
const int screenHeight = 1000;
const float groundLevel = screenHeight - 0.3*screenHeight;
const float friction = 0.15;
const float drag = 0.08;

void playerUpdate(Player *player, float timestep) {
	bool movekeydown = false;
	if(IsKeyDown(KEY_UP) && player->grounded) {
		player->vel.y = PLAYER_JUMP;
		player->grounded = false;
	}
	if(IsKeyDown(KEY_LEFT)) {
		movekeydown = true;
		player->accel.x = -PLAYER_RUN;
	}
	if(IsKeyDown(KEY_RIGHT)) {
		movekeydown = true;
		player->accel.x = PLAYER_RUN;
	}
	player->accel.x *= movekeydown;

	/*
	 * 0.5*a*t^2 + v*t + p0
	 * a*t + v0
	 * a
	 */

	Vector2 accel = Vector2Scale(player->accel, timestep);
	player->vel.x += accel.x - friction*player->vel.x;
	player->vel.y += accel.y - drag*player->vel.y;
	accel = Vector2Scale(accel, timestep*0.5);
	Vector2 offset = Vector2Add(accel, Vector2Scale(player->vel, timestep));
	Vector2 newpos = Vector2Add(player->pos, offset);

	/* inelastic collisions */
	for(int i = 0; i < platformCount; ++i) {
		Rectangle *platform = platforms+i;

		if(newpos.x + PLAYER_BOX_WIDTH*0.5 >= platform->x &&
		   newpos.x + PLAYER_BOX_WIDTH*0.5 <= platform->x + platform->width &&
		   newpos.y + PLAYER_BOX_HEIGHT >= platform->y && player->pos.y <= platform->y &&
		   player->vel.y >= 0)
		{
			player->vel.y = 0;
			newpos.y -= newpos.y + PLAYER_BOX_HEIGHT - platform->y;
			player->grounded = true;
			break;
		}
	}

	if(newpos.y >= MAX_DEPTH) {
		newpos.x = PLAYER_INITIAL_X;
		newpos.y = PLAYER_INITIAL_Y;
	}
	player->pos = newpos;
}

int main() {
	char buf[64];

	Player mario = {
		.width = PLAYER_BOX_WIDTH, .height = PLAYER_BOX_HEIGHT,
		.vel = {0},
		.pos = { .x = PLAYER_INITIAL_X, .y = PLAYER_INITIAL_Y },
		.accel = { .x = 0 , .y = G },
		.mass = 30.0,
		.invmass = 1/30.0,
		.debugcolor = GREEN,
		.grounded = true,
	};

	platforms[0] = (Rectangle){
		.x = 0, .y = groundLevel,
		.width = screenWidth, .height = screenHeight-groundLevel,
	};

	platforms[1] = (Rectangle){
		.x = 300, .y = groundLevel-120,
		.width = 400, .height = 40,
	};

	platforms[2] = (Rectangle){
		.x = 1000, .y = groundLevel-120,
		.width = 400, .height = 40,
	};

	platforms[3] = (Rectangle){
		.x = 460, .y = groundLevel-300,
		.width = 200, .height = 30,
	};

	Camera2D camera = {0};
	camera.rotation = 0.0f;
	camera.zoom = 1.5f;


	const float fps = 60;

	InitWindow(screenWidth, screenHeight, "platform");
	SetTargetFPS(fps);

	while(!WindowShouldClose()) {

		float timestep = GetFrameTime();
		playerUpdate(&mario, timestep);
		camera.offset = (Vector2){ screenWidth*0.5f, screenHeight*0.5f };
		camera.target = (Vector2){ mario.pos.x + HALF_PLAYER_BOX_WIDTH, mario.pos.y+10};

		BeginDrawing();

		ClearBackground(BLACK);

		BeginMode2D(camera);
		{
			for(Rectangle *platform = platforms; platform-platforms < platformCount; ++platform)
				DrawRectangleRec(*platform, RED);
			Rectangle playerRect = (Rectangle){ mario.pos.x, mario.pos.y, mario.width, mario.height};
			DrawRectangleRec(playerRect, mario.debugcolor);
		}
		EndMode2D();

		float fps = GetFPS();
		char *fmt = "FPS: %f\nTIME: %f\nGROUNDED: %i\nVX: %f\nVY: %f\n";
		sprintf(buf, fmt,
		fps, timestep, mario.grounded, mario.vel.x, mario.vel.y);

		DrawText(buf, 10, 10, 15, RAYWHITE);

		EndDrawing();
	}
}
