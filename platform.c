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
#define SQR(x) (x*x)
#define DOUBLE(x) (x+x)
#define HALF(x) (x*0.5f)
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

const int screenWidth = 1200;
const int screenHeight = 900;
const float frictionX = 5.20;
const float frictionY = 80.0;
const float drag = 4.20;
const float GROUNDLEVEL = screenHeight - 210;
const float G = 9500.0;
const float PLAYER_INITIAL_X = 100.0;
const float PLAYER_RUN_FORCE = 1.5e5;
const float PLAYER_JUMP_FORCE = 1e6;
const float PLAYER_AIR_RUN_FORCE = 0.8e5;
const float PLAYER_WALL_RUN_FORCE = 1.8e6;
const float PLAYER_WALL_JUMP_FORCE = 0.5e6;
const float PLAYER_MASS = 27;
const float PLAYER_INVMASS = 1/PLAYER_MASS;
const int PLAYER_THRUST_TIME = 5;
const float MAX_DEPTH = 1200.0;
const int PLAYER_FRAME_SPEED = 30;
const float FPS = 60;
const int TILE_SIZE = 20; // tiles will be 20x20 pixels
const float PLANCK_LENGTH = 0.0001f;

const char *WORLD =
"\
................................................................\
................................................................\
.....................................##########.................\
............########............................................\
................................................................\
.............................######.............................\
................................................................\
................................................................\
.........############..............#############................\
................................................................\
................................................................\
################################################################\
................................................................\
................................................................\
................................................................\
................................................................\
................................................................\
";

// TODO add tiles
	/*
		if(*tp != '#')
			continue;
		size_t coord = tp - WORLD;
		float x = (float)(coord & 0x3f);
		float y = (float)(coord >> 6);
		Rectangle tile = (Rectangle){ x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
		bool intersect = false;
		Vector2 a, b, c, d, p;
		//Rectangle *platform = platforms+i;

		p = (Vector2){0};
		a = (Vector2){ player->pos.x, player->pos.y };
		b = (Vector2){ newpos.x     , newpos.y      };

		// top face
		c = (Vector2){ tile.x - player->widthH, tile.y - player->heightH };
		d = (Vector2){ tile.x + tile.width + player->widthH, tile.y - player->heightH };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.y >= 0) {
			player->vel.y = 0.0;
			newpos.y = p.y;
			player->thrust = PLAYER_THRUST_TIME;
			player->airborne = false;
			printf("TOP FACE\n");
		}

		// right face
		c = d;
		d = (Vector2){ tile.x + tile.width + player->widthH, tile.y + tile.height + player->widthH };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.x <= 0) {
			*(unsigned int*)&player->vel.x = 0x80000000;
			newpos.x = p.x;
			printf("RIGHT FACE\n");
		}

		// bottom face
		c = (Vector2){ tile.x - player->widthH, tile.y + tile.height + player->widthH };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.y <= 0) {
			player->vel.y = 0.0;
			newpos.y = p.y;
			printf("BOTTOM FACE\n");
		}

		// left face
		d = c;
		c = (Vector2){ tile.x - player->widthH, tile.y - player->heightH };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.x >= 0) {
			player->vel.x = 0.0;
			newpos.x = p.x;
			printf("LEFT FACE\n");
		}

	*/
typedef struct PlayerState PlayerState;
//typedef struct Platform Platform;
//typedef struct Goomba Goomba;

struct PlayerState {
	Vector2 pos;
	Vector2 vel;
	Vector2 accel;
	Vector2 force;
	Vector2 normal;
	Texture2D tex[3];
	Rectangle frame;
	int curFrame;
	int frameCounter;
	int frameSpeed;
	float widthH; // half of the width
	float heightH; // half of the height
	float mass;
	float invmass;
	float runForce; // smaller when airborne
	float jumpForce; // smaller when sliding down a wall
	int moveState;
	bool airborne;
	bool wallclimb;
	int thrust;
};

#ifdef DEBUG
Vector2 debugDirection;
int debugclosestrectindex[2];
#endif

Rectangle platforms[6];
const int platformCount = ARRLEN(platforms);
//const int platformCount = 0;
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

	denominator = p12x*p34y - p12y*p34x;
	denominator = 1/denominator;

	numerator = p13x * p34y - p13y * p34x;

	t = numerator * denominator;

	numerator = p13x * p12y - p13y * p12x;

	u = numerator * denominator;

	*p = (Vector2){ p1.x + t*(p2.x - p1.x), p1.y + t*(p2.y - p1.y) };

	return (0.0 <= t && t <= 1.0) && (0.0 <= u && u <= 1.0);
}

void playerUpdate(PlayerState *player, float timestep) {
	player->force.y = G * player->mass;

	if(IsKeyDown(KEY_LEFT)) {
		player->force.x = -player->runForce;
		player->moveState = 1;
	} else if(IsKeyDown(KEY_RIGHT)) {
		player->force.x = player->runForce;
		player->moveState = 1;
	} else {
		player->force.x = 0;
		player->moveState = 0;
	}

	bool issliding = false;
	if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && player->normal.x != 0.0f && player->airborne) {
		player->normal.y = -1;
		player->thrust = PLAYER_THRUST_TIME;
		player->jumpForce = PLAYER_WALL_JUMP_FORCE;
		player->runForce = PLAYER_WALL_RUN_FORCE;
		issliding = true;
	}

	if(IsKeyDown(KEY_UP) && player->thrust > 0 && player->normal.y <= 0) {
		if(issliding)
			player->force.x += player->normal.x * player->runForce;
		player->force.y += -player->jumpForce;
		--player->thrust;
	} else {
		player->thrust = 0;
	}

	player->accel = Vector2Scale(player->force, player->invmass);

	/*
	 * 0.5*a*t^2 + v*t + p0
	 * a*t + v0
	 * a
	 */

	Vector2 accel = Vector2Scale(player->accel, timestep);
	float velx = player->vel.x + accel.x - timestep*drag*player->vel.x;
	velx -= fabsf(player->normal.y)*timestep*frictionX*player->vel.x;
	if(velx != 0.0f)
		player->vel.x = velx;
	if(player->vel.y > 0)
		player->vel.y -= fabsf(player->normal.y)*timestep*frictionY*player->vel.y;
	player->vel.y += accel.y - timestep*drag*player->vel.y;
	accel = Vector2Scale(accel, HALF(timestep));
	Vector2 offset = Vector2Add(accel, Vector2Scale(player->vel, timestep));
	Vector2 newpos = Vector2Add(player->pos, offset);
#ifdef DEBUG
	debugDirection = offset;
#endif

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
		}
	} else {
		player->curFrame = 0;
		player->frame.width = (float)player->tex[player->moveState].width;
		player->frame.x = 0;
	}

	/* inelastic collisions */

	player->airborne = true;
	player->normal = (Vector2){0};

	int indexes[platformCount];
	float distances[platformCount];

	// order platforms by distance to center
	// NOTE this is a heuristic for getting platforms in an order
	// that avoids having the player get caught on corners of adjacent
	// platforms
	for(int i = 0; i < platformCount; ++i) {
		Rectangle *platform = platforms+i;
		int index = i;
		Vector2 v = (Vector2){ platform->x + HALF(platform->width), platform->y + HALF(platform->height) };
		float dist = Vector2DistanceSqr(player->pos, v);
		indexes[i] = i;
		distances[i] = dist;
		for(int j = 1, k = 0; j <= i; j++) {
			index = indexes[j];
			dist = distances[j];
			k = j - 1;
			while (k >= 0 && distances[k] > dist) {
				indexes[k+1] = indexes[k];
				distances[k+1] = distances[k];
				--k;
			}
			indexes[k+1] = index;
			distances[k+1] = dist;
		}
	}

#ifdef DEBUG
	debugclosestrectindex[0] = indexes[0];
	debugclosestrectindex[1] = indexes[1];
#endif

	for(int i = 0; i < platformCount; ++i) {
		bool intersect;
		Vector2 a, b, c, d, p;
		Rectangle *platform = platforms+indexes[i];

		a = (Vector2){ player->pos.x, player->pos.y };
		b = (Vector2){ newpos.x     , newpos.y      };

		// top face
		p = (Vector2){0};
		c = (Vector2){ platform->x - player->widthH, platform->y - player->heightH };
		d = (Vector2){ platform->x + platform->width + player->widthH, platform->y - player->heightH };

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.y >= 0) {
			player->vel.y = 0.0;
			newpos.y = p.y - PLANCK_LENGTH;
			player->thrust = PLAYER_THRUST_TIME;
			player->airborne = false;
			player->normal.y = -1;
			player->runForce = PLAYER_RUN_FORCE;
			player->jumpForce = PLAYER_JUMP_FORCE;
			continue;
		}

		// bottom face
		p = (Vector2){0};
		c.y = platform->y + platform->height + player->heightH;
		d.y = platform->y + platform->height + player->heightH;

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.y < 0) {
			player->vel.y = 0.0;
			newpos.y = p.y + PLANCK_LENGTH;
			player->normal.y = 1;
			continue;
		}

		// right face
		p = (Vector2){0};
		c = (Vector2){ platform->x + platform->width + player->widthH, platform->y - player->heightH };
		d.x = platform->x + platform->width + player->widthH;

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.x <= 0) {
			*(unsigned int*)&player->vel.x = 0x80000000;
			newpos.x = p.x + PLANCK_LENGTH;
			player->normal.x = 1;
			continue;
		}

		// left face
		p = (Vector2){0};
		c.x = platform->x - player->widthH;
		d.x = platform->x - player->widthH;

		intersect = lineSegIntersect(a, b, c, d, &p);

		if(intersect && player->vel.x >= 0) {
			player->vel.x = 0.0;
			newpos.x = p.x - PLANCK_LENGTH;
			player->normal.x = -1;
			continue;
		}
	}

	if(player->normal.x == 0 && player->normal.y == 0)
		player->runForce = PLAYER_AIR_RUN_FORCE;
	else
		player->runForce = PLAYER_RUN_FORCE;

	if(player->airborne && player->force.y >= 0 && player->normal.x == 0 && player->normal.y == 0)
		player->thrust = 0;

	if(newpos.y >= GROUNDLEVEL-player->heightH) {
		newpos.y = GROUNDLEVEL-player->heightH;
		player->vel.y = 0.0;
		player->thrust = PLAYER_THRUST_TIME;
		player->airborne = false;
		player->normal.y = -1;
		player->runForce = PLAYER_RUN_FORCE;
		player->jumpForce = PLAYER_JUMP_FORCE;
	}

	if(newpos.x <= player->widthH)
		newpos.x = player->widthH;

	player->pos = newpos;
}

int main(void) {
	char buf[64];

	InitWindow(screenWidth, screenHeight, "platform");
	SetTargetFPS(FPS);

	PlayerState player = {
		//.width = PLAYER_BOX_WIDTH, .height = PLAYER_BOX_HEIGHT,
		.vel = {0},
		.pos = { .x = PLAYER_INITIAL_X, },
		.force = { .x = 0, .y = 30 * G },
		.accel = { .x = 0 , .y = G },
		.normal = { .x = 0, .y = -1 },
		.frameCounter = 0,
		.curFrame = 0,
		.frameSpeed = PLAYER_FRAME_SPEED,
		.mass = PLAYER_MASS,
		.invmass = PLAYER_INVMASS,
		.runForce = PLAYER_RUN_FORCE,
		.jumpForce = PLAYER_JUMP_FORCE,
		.thrust = PLAYER_THRUST_TIME,
	};

	//platforms[0] = (Rectangle){
	//	.x = 0, .y = GROUNDLEVEL,
	//	.width = screenWidth*7000, .height = screenHeight-GROUNDLEVEL,
	//};

	platforms[0] = (Rectangle){
		.x = 300, .y = GROUNDLEVEL-190,
		.width = 400, .height = 40,
	};

	platforms[1] = (Rectangle){
		.x = 1000, .y = GROUNDLEVEL-120,
		.width = 400, .height = 40,
	};

	platforms[2] = (Rectangle){
		.x = 460, .y = GROUNDLEVEL-300,
		.width = 200, .height = 30,
	};

	platforms[3] = (Rectangle){
		.x = 660, .y = GROUNDLEVEL-300,
		.width = 200, .height = 30,
	};

	platforms[4] = (Rectangle){
		.x = 600, .y = GROUNDLEVEL- 100,
		.width = 50, .height = 100,
	};

	//platforms[5] = (Rectangle){
	//	.x = 1800, .y = GROUNDLEVEL-200,
	//	.width = 80, .height = 200,
	//};

	platforms[5] = (Rectangle){
		.x = 1800, .y = GROUNDLEVEL-400,
		.width = 80, .height = 200,
	};

	player.tex[0] = LoadTexture("assets/mario_still_scaled.png");
	player.tex[1] = LoadTexture("assets/mario_anim_scaled.png");
	player.tex[2] = LoadTexture("assets/mario_jump_scaled.png");
	Texture2D groundTex = LoadTexture("assets/groundtile48x48.png");
	player.frame.x = player.frame.y = 0;
	player.frame.width = player.tex[0].width - 10;
	player.frame.height = player.tex[0].height;
	player.widthH = player.frame.width * 0.5;
	player.heightH = player.frame.height * 0.5;
	assert(player.heightH == 24);
	player.pos.y = GROUNDLEVEL-player.heightH;
	Camera2D camera = {0};
	camera.target.y = GROUNDLEVEL-player.heightH-100;
	camera.rotation = 0.0f;
	camera.zoom = 1.3f;
	Rectangle groundRect = (Rectangle){
		.width = DOUBLE(screenWidth), .height = 250,
	};
	Vector2 groundPos = (Vector2){
		.x = 0, .y = GROUNDLEVEL,
	};

	while(!WindowShouldClose()) {

		BeginDrawing();

		ClearBackground(SKYBLUE);

		float timestep = GetFrameTime();
		float dx = -player.pos.x;
		playerUpdate(&player, timestep);
		dx += player.pos.x;
		camera.offset = (Vector2){ HALF(screenWidth), HALF(screenHeight) };
		if(player.pos.x < HALF(screenWidth)-148) {
			camera.target.x = HALF(screenWidth)-148;
		} else
			camera.target.x = player.pos.x;
		groundRect.x = camera.target.x - HALF(screenWidth);

		BeginMode2D(camera);
		{
			int c = 0;
			for(Rectangle *platform = platforms; platform-platforms < platformCount; ++platform) {
				Color color = RED;
				if(c & 0x1) color = ORANGE;
				DrawRectangleRec(*platform, color);
				++c;
			}
#ifdef DEBUG
			Rectangle playerRect = (Rectangle){ player.pos.x - player.widthH, player.pos.y - player.heightH, DOUBLE(player.widthH), DOUBLE(player.heightH)};
			DrawRectangleLinesEx(playerRect, 2.0, GREEN);
#endif
			Vector2 framepos = (Vector2){
				player.pos.x - player.frame.width*0.5,
				player.pos.y - player.frame.height*0.5,
			};
			Rectangle frame = player.frame;
			if(*(int*)&player.vel.x < 0)
				frame.width *= -1;
			DrawTextureRec(player.tex[player.moveState], frame, framepos, WHITE);
			groundPos.x = camera.target.x - HALF(screenWidth);
			DrawTextureRec(groundTex, groundRect, groundPos, WHITE);
#ifdef DEBUG
			Vector2 a, b;
			a = player.pos;
			b = Vector2Add(player.pos, debugDirection);
			DrawLineEx(a, b, 2.0, YELLOW);
			b = Vector2Add(player.pos, Vector2Scale(player.normal, 30));
			DrawLineEx(a, b, 2.0, PINK);
			DrawLineEx((Vector2){0,0},(Vector2){0,3000}, 2.0, GREEN);
			//Vector2 closesest = (Vector2){
			//	platforms[debugclosestrectindex[0]].x + HALF(platforms[debugclosestrectindex[0]].width),
			//	platforms[debugclosestrectindex[0]].y + HALF(platforms[debugclosestrectindex[0]].height),
			//};
			//DrawLineEx(player.pos, closesest, 2.0, ORANGE);
			//closesest = (Vector2){
			//	platforms[debugclosestrectindex[1]].x + HALF(platforms[debugclosestrectindex[1]].width),
			//	platforms[debugclosestrectindex[1]].y + HALF(platforms[debugclosestrectindex[1]].height),
			//};
			//DrawLineEx(player.pos, closesest, 2.0, PURPLE);
#endif
		}
		EndMode2D();

#ifdef DEBUG
		//DrawLineEx((Vector2){HALF(screenWidth),0},(Vector2){HALF(screenWidth),screenHeight},2.0,YELLOW);
		//DrawLineEx((Vector2){0,HALF(screenHeight)},(Vector2){screenWidth,HALF(screenHeight)},2.0,YELLOW);
#endif

		float FPS = GetFPS();
		char *fmt =
		"FPS: %f\nTIME: %f\nTHRUST: %i\nAIRBORNE: %i\nPX: %f\nPY: %f\nFX: %f\nFY: %f\nAX: %f\nAY: %f\nVX: %f\nVY: %f\nRUNFORCE: %f\n";
		sprintf(buf, fmt,
		FPS, timestep, player.thrust, player.airborne,
		player.pos.x, player.pos.y,
		player.force.x, player.force.y,
		player.accel.x, player.accel.y,
		player.vel.x, player.vel.y,
		player.runForce);

		DrawText(buf, 10, 10, 15, RAYWHITE);

		EndDrawing();
	}

	UnloadTexture(player.tex[0]);
	UnloadTexture(player.tex[1]);
	UnloadTexture(player.tex[2]);
	UnloadTexture(groundTex);

	CloseWindow();

	return 0;
}
