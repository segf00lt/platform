#define STB_DS_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <assert.h>
#include "stb_ds.h"
#include "stb_sprintf.h"

#define inline __attribute__((always_inline))
#define ARRLEN(x) (sizeof(x)/sizeof(*x))
#define STRLEN(x) ((sizeof(x)/sizeof(*x))-1)
#undef sprintf
#define sprintf stbsp_sprintf
#define SQUARE(x) (x*x)

const int screenWidth = 1200;
const int screenHeight = 900;
const float friction = 9.20;
const float drag = 1.20;
const float GROUNDLEVEL = screenHeight - 0.3*screenHeight;
const float G = 9500.0;
const float PLAYER_INITIAL_X = 100.0;
const float PLAYER_MOVE_FORCE = 1.5e5;
const float PLAYER_JUMP_FORCE = -1e6;
const float PLAYER_MASS = 27;
const float PLAYER_INVMASS = 1/PLAYER_MASS;
const int PLAYER_THRUST_TIME = 5;
const float MAX_DEPTH = 1200.0;
const int PLAYER_FRAME_SPEED = 30;
const float FPS = 60;


typedef struct Player Player;
//typedef struct Platform Platform;
//typedef struct Goomba Goomba;

struct Player {
	Vector2 pos;
	Vector2 vel;
	Vector2 accel;
	Vector2 force;
	Texture2D tex[3];
	Rectangle frame;
	int curFrame;
	int frameCounter;
	int frameSpeed;
	float width;
	float height;
	float mass;
	float invmass;
	int moveState;
	bool airborne;
	int thrust;
};

Rectangle platforms[100];
int platformCount = 5;
bool gameOver = false;

inline bool lineSegIntersect(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, Vector2 *p) {
	float t, u;
	float numerator, denominator;

	float p12x = p1.x - p2.x;
	float p12y = p1.y - p2.y;
	float p34x = p3.x - p4.x;
	float p34y = p3.y - p4.y;
	float p13x = p1.x - p3.x;
	float p13y = p1.y - p3.y;

	denominator = 1/(p12x * p34y - p12y*p34x);

	numerator = p13x * p34y - p13y * p34x;

	t = numerator * denominator;

	numerator = p13x * p12y - p13y * p12x;

	u = numerator * denominator;

	*p = (Vector2){ p1.x + t*(p2.x - p1.x), p1.y + t*(p2.y - p1.y) };

	return (0.0 <= t && t <= 1.0) && (0.0 <= u && u <= 1.0);
}

void playerUpdate(Player *player, float timestep) {
	player->force.y = G * player->mass;

	if(IsKeyDown(KEY_UP) && player->thrust > 0) {
		player->force.y += PLAYER_JUMP_FORCE;
		--player->thrust;
	} else {
		player->thrust = 0;
	}

	if(IsKeyDown(KEY_LEFT)) {
		player->force.x = -PLAYER_MOVE_FORCE;
		player->moveState = 1;
	} else if(IsKeyDown(KEY_RIGHT)) {
		player->force.x = PLAYER_MOVE_FORCE;
		player->moveState = 1;
	} else {
		player->force.x = 0;
		player->moveState = 0;
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

	if(player->airborne)
		player->moveState = 2;

	if(player->moveState == 1) {
		++player->frameCounter;

		if(player->frameCounter >= (FPS/player->frameSpeed)) {
			player->frameCounter = 0;
			++player->curFrame;
			if(player->curFrame > 3)
				player->curFrame = 0;
			player->frame.width = (float)player->tex[1].width/3;
			player->frame.x = (float)player->curFrame*player->frame.width;
			if(*(int*)&player->vel.x < 0)
				player->frame.width *= -1;
		}
	} else {
		player->curFrame = 0;
		player->frame.width = (float)player->tex[player->moveState].width;
		player->frame.x = 0;
		if(*(int*)&player->vel.x < 0)
			player->frame.width *= -1;
	}

	player->airborne = true;
	/* inelastic collisions */
	for(int i = 0; i < platformCount; ++i) {
		bool intersect = false;
		Vector2 a, b, c, d, p;
		Rectangle *platform = platforms+i;

		p = (Vector2){0};
		a = (Vector2){ player->pos.x, player->pos.y + player->height };
		b = (Vector2){ newpos.x, newpos.y + player->height};
		c = (Vector2){ platform->x, platform->y };
		d = (Vector2){ platform->x + platform->width, platform->y };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.y >= 0) {
			player->vel.y = 0.0;
			// TODO newpos needs to adjusted differently
			newpos.y = p.y - player->height;
			player->thrust = PLAYER_THRUST_TIME;
			player->airborne = false;
		}
	}

	if(player->airborne && player->thrust == PLAYER_THRUST_TIME)
		player->thrust = 0;

	if(newpos.y >= MAX_DEPTH) {
		newpos.x = PLAYER_INITIAL_X;
		newpos.y = GROUNDLEVEL-player->height;
	}

	player->pos = newpos;
}

int main(void) {
	char buf[64];

	InitWindow(screenWidth, screenHeight, "platform");
	SetTargetFPS(FPS);

	Player mario = {
		//.width = PLAYER_BOX_WIDTH, .height = PLAYER_BOX_HEIGHT,
		.vel = {0},
		.pos = { .x = PLAYER_INITIAL_X },
		.force = { .x = 0, .y = 30 * G },
		.accel = { .x = 0 , .y = G },
		.frameCounter = 0,
		.curFrame = 0,
		.frameSpeed = PLAYER_FRAME_SPEED,
		.mass = PLAYER_MASS,
		.invmass = PLAYER_INVMASS,
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

	platforms[4] = (Rectangle){
		.x = 1600, .y = GROUNDLEVEL- 100,
		.width = 50, .height = 100,
	};


	mario.tex[0] = LoadTexture("mario_still_scaled.png");
	mario.tex[1] = LoadTexture("mario_anim_scaled.png");
	mario.tex[2] = LoadTexture("mario_jump_scaled.png");
	mario.frame.x = mario.frame.y = 0;
	mario.frame.width = mario.width = mario.tex[0].width;
	mario.frame.height = mario.height = mario.tex[0].height;
	mario.pos.y = GROUNDLEVEL-mario.height;
	Camera2D camera = {0};
	camera.rotation = 0.0f;
	camera.zoom = 1.5f;

	while(!WindowShouldClose()) {

		BeginDrawing();

		ClearBackground(BLACK);

		float timestep = GetFrameTime();
		playerUpdate(&mario, timestep);
		camera.offset = (Vector2){ screenWidth*0.5f, screenHeight*0.5f };
		camera.target = (Vector2){ mario.pos.x + mario.width*0.5, mario.pos.y + mario.height*0.5 };


		BeginMode2D(camera);
		{
			for(Rectangle *platform = platforms; platform-platforms < platformCount; ++platform)
				DrawRectangleRec(*platform, RED);
			Rectangle playerRect = (Rectangle){ mario.pos.x, mario.pos.y, mario.width, mario.height};
			DrawRectangleLinesEx(playerRect, 2.0, GREEN);
			Vector2 framepos;
			if(*(int*)&mario.vel.x < 0 && mario.moveState > 0) {
				framepos = (Vector2){ mario.pos.x - (float)mario.tex[2].width + mario.width, mario.pos.y };
			} else {
				framepos = mario.pos;
			}
			DrawTextureRec(mario.tex[mario.moveState], mario.frame, framepos, WHITE);
		}
		EndMode2D();

		float FPS = GetFPS();
		char *fmt =
		"FPS: %f\nTIME: %f\nTHRUST: %i\nAIRBORNE: %i\nFX: %f\nFY: %f\nAX: %f\nAY: %f\nVX: %f\nVY: %f\n";
		sprintf(buf, fmt,
		FPS, timestep, mario.thrust, mario.airborne,
		mario.force.x, mario.force.y,
		mario.accel.x, mario.accel.y,
		mario.vel.x, mario.vel.y);

		DrawText(buf, 10, 10, 15, RAYWHITE);

		EndDrawing();
	}

	UnloadTexture(mario.tex[0]);
	UnloadTexture(mario.tex[1]);
	UnloadTexture(mario.tex[2]);

	CloseWindow();

	return 0;
}
