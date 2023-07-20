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

const int screenWidth = 1500;
const int screenHeight = 1000;
const float friction = 10.20;
const float drag = 1.00;
const float GROUNDLEVEL = screenHeight - 0.3*screenHeight;
const float G = 1000.0;
const float PLAYER_JUMP = -1e10;
const float PLAYER_RUN = 10.0;
const float PLAYER_BOX_WIDTH = 15.0;
const float PLAYER_BOX_HEIGHT = 30.0;
const float HALF_PLAYER_BOX_WIDTH = 7.5;
const float HALF_PLAYER_BOX_HEIGHT = 15;
const float PLAYER_INITIAL_Y = GROUNDLEVEL-PLAYER_BOX_HEIGHT;
const float PLAYER_INITIAL_X = 100.0;
const float PLAYER_MOVE_FORCE = 2e5;
const float PLAYER_JUMP_FORCE = -3e5;
const int PLAYER_THRUST_TIME = 5;
const float MAX_DEPTH = 1200.0;

typedef struct Player Player;
//typedef struct Platform Platform;
//typedef struct Goomba Goomba;

struct Player {
	Vector2 pos;
	Vector2 vel;
	Vector2 accel;
	Vector2 force;
	float width;
	float height;
	float mass;
	float invmass;
	Color debugcolor;
	int thrust;
};

Rectangle platforms[100];
int platformCount = 4;
bool gameOver = false;

void playerUpdate(Player *player, float timestep) {
	player->force.y = G * player->mass;

	if(IsKeyDown(KEY_UP) && player->thrust > 0) {
		player->force.y += PLAYER_JUMP_FORCE;
		--player->thrust;
	}
	if(IsKeyDown(KEY_LEFT)) {
		player->force.x = -PLAYER_MOVE_FORCE;
	} else if(IsKeyDown(KEY_RIGHT)) {
		player->force.x = PLAYER_MOVE_FORCE;
	} else {
		player->force.x = 0;
	}

	player->accel = Vector2Scale(player->force, player->invmass);

	/*
	 * 0.5*a*t^2 + v*t + p0
	 * a*t + v0
	 * a
	 */

	Vector2 accel = Vector2Scale(player->accel, timestep);
	player->vel.x += accel.x - timestep*friction*player->vel.x;
	player->vel.y += accel.y - timestep*drag*player->vel.y;
	accel = Vector2Scale(accel, timestep*0.5);
	Vector2 offset = Vector2Add(accel, Vector2Scale(player->vel, timestep));
	Vector2 newpos = Vector2Add(player->pos, offset);

	bool colliding = false;
	/* inelastic collisions */
	for(int i = 0; i < platformCount; ++i) {
		Rectangle *platform = platforms+i;

		if(newpos.x + PLAYER_BOX_WIDTH*0.5 >= platform->x &&
		   newpos.x + PLAYER_BOX_WIDTH*0.5 <= platform->x + platform->width &&
		   newpos.y + PLAYER_BOX_HEIGHT >= platform->y &&
		   player->pos.y+player->height <= platform->y &&
		   player->vel.y >= 0)
		{
			player->force.y = G * player->mass;
			player->vel.y = 0;
			newpos.y -= newpos.y + PLAYER_BOX_HEIGHT - platform->y;
			player->thrust = PLAYER_THRUST_TIME;
			colliding = true;
			break;
		}
	}

	if(!colliding && player->thrust == PLAYER_THRUST_TIME) {
		player->thrust = 0;
	}

	//if(newpos.y >= MAX_DEPTH) {
	//	newpos.x = PLAYER_INITIAL_X;
	//	newpos.y = PLAYER_INITIAL_Y;
	//}
	player->pos = newpos;
}

int main() {
	char buf[64];

	Player mario = {
		.width = PLAYER_BOX_WIDTH, .height = PLAYER_BOX_HEIGHT,
		.vel = {0},
		.pos = { .x = PLAYER_INITIAL_X, .y = PLAYER_INITIAL_Y },
		.force = { .x = 0, .y = 30 * G },
		.accel = { .x = 0 , .y = G },
		.mass = 30.0,
		.invmass = 1/30.0,
		.debugcolor = GREEN,
		.thrust = PLAYER_THRUST_TIME,
	};

	platforms[0] = (Rectangle){
		.x = 0, .y = GROUNDLEVEL,
		.width = screenWidth*7000, .height = screenHeight-GROUNDLEVEL,
	};

	platforms[1] = (Rectangle){
		.x = 300, .y = GROUNDLEVEL-120,
		.width = 400, .height = 40,
	};

	platforms[2] = (Rectangle){
		.x = 1000, .y = GROUNDLEVEL-120,
		.width = 400, .height = 40,
	};

	platforms[3] = (Rectangle){
		.x = 460, .y = GROUNDLEVEL-300,
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
		char *fmt = "FPS: %f\nTIME: %f\nTHRUST: %i\nFX: %f\nFY: %f\nAX: %f\nAY: %f\nVX: %f\nVY: %f\n";
		sprintf(buf, fmt,
		fps, timestep, mario.thrust,
		mario.force.x, mario.force.y,
		mario.accel.x, mario.accel.y,
		mario.vel.x, mario.vel.y);

		DrawText(buf, 10, 10, 15, RAYWHITE);

		EndDrawing();
	}
}
