// ADAPTED FROM JIM MCCANN'S BASE1 CODE FOR 15-466 COMPUTER GAME PROGRAMMING

#include "load_save_png.hpp"
#include "GL.hpp"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <fstream>
using namespace std;

const float PI = 3.1415f;
static GLuint compile_shader(GLenum type, std::string const &source);
static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);

float dot(glm::vec2 a, glm::vec2 b) {
	return (a.x * b.x + a.y * b.y);
}

static const char *BG_MUSIC_PATH = 
"../sounds/Light_And_Shadow_Soundtrack.wav";

static const char *ALERT_MUSIC_PATH =
"../sounds/alert_quiet.wav";

static const char *DOOR_MUSIC_PATH =
"../sounds/door_grab.wav";

static const char *LADDER_MUSIC_PATH =
"../sounds/ladder_grab.wav";

static const char *ORNAMENT_PATH =
"../sounds/ornament_smash_3.wav";

static const char *STEP_MUSIC_PATH =
"../sounds/step.wav";

struct AudioData {
	Uint8 *pos;
	Uint32 length;
	Uint8 *init_pos;
	Uint32 init_length;
};

//----------------- Structs ----------------------------------------------
struct CameraInfo{
	glm::vec2 pos = glm::vec2(6.0f, 2.5f);
	glm::vec2 size = glm::vec2(12.0f, 8.0f);
};

struct SpriteInfo {
	glm::vec2 min_uv;
	glm::vec2 max_uv;
	glm::vec2 origin;
};

struct MouseInfo{
	glm::vec2 pos = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(6.0f);

	SpriteInfo sprite_throw = {
		glm::vec2(4900.0f/7000.0f, (5500.0f - 1900.0f) / 5500.0f),
		glm::vec2(5300.0f/7000.0f, (5500.0f - 1500.0f) / 5500.0f),
	};

	float remaining_time = 0.0f;
};

struct Background{
	SpriteInfo background = {
		glm::vec2(   0.0f/7000.0f, (5500.0f - 2000.0f) / 5500.0f),
		glm::vec2( 200.0f/7000.0f, (5500.0f - 2200.0f) / 5500.0f),
	};
};

struct PlayerInfo{
	glm::vec2 pos = glm::vec2(0.25f, 1.0f);
	glm::vec2 size = glm::vec2(0.5f, 1.0f);
	glm::vec2 vel = glm::vec2(0.0f);

	bool walking;

	int animation_delay;
	int animation_count;

	SpriteInfo sprite_animations[9] = {
		{
			glm::vec2(1100.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(1500.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(1500.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(1900.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(1900.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(2300.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(2300.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(2700.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(2700.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(3100.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(3100.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(3500.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(3500.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(3900.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(3900.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(4300.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
		{
			glm::vec2(700.0f / 7000.0f, (5500.0f - 2000.0f) / 5500.0f),
			glm::vec2(1100.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
		},
	};

	bool face_right = false;
	bool jumping = false;
	bool shifting = false;
	bool behind_door = false;
	bool aiming = false;
	bool visible = false;  

	float walk_sound = 2.0f;
	float run_sound = 6.0f;
	float throw_sound = 6.0f;
	float sound_time = 0.5f;

	glm::vec2 aimed_pos;
	std::vector<glm::vec2> projectiles_pos;
	int num_projectiles = 5;
};

struct Light {
	glm::vec2 pos = glm::vec2(0.0f, 0.0f);
	glm::vec2 size = glm::vec2(0.0f, 0.0f);
	float dir = PI * 1.5f;
	bool light_on = true;
	glm::u8vec4 color = glm::u8vec4(0xff, 0xff, 0xff, 0xff);

	SpriteInfo sprite = {
		glm::vec2(1945.0f/3503.0f, 1289.0f/1689.0f),
		glm::vec2(2585.0f/3503.0f, 1.0f),
		glm::vec2(0.0f, 0.0f),
	};

	glm::vec2 vectors [3] = { glm::vec2(pos.x, 
			pos.y + (0.5f * size.y)),
		glm::vec2(pos.x + (0.5f * size.x), 
				pos.y + (0.5f * -size.y)),
		glm::vec2(pos.x + (0.5f * -size.x),
				pos.y + (0.5f * -size.y)) };

	void rotate() {
		if (dir == 0.0f) {
			vectors[0] = glm::vec2(pos.x + (0.5f + -size.y), 
					pos.y);
			vectors[1] = glm::vec2(pos.x + (0.5f * size.y), 
					pos.y + (0.5f * size.x));
			vectors[2] = glm::vec2(pos.x + (0.5f * size.y),
					pos.y + (0.5f * -size.x));
		}
		else if (dir == (PI * 0.5f)) {
			vectors[0] = glm::vec2(pos.x, 
					pos.y + (0.5f * -size.y));
			vectors[1] = glm::vec2(pos.x + (0.5f * -size.x), 
					pos.y + (0.5f * size.y));
			vectors[2] = glm::vec2(pos.x + (0.5f * size.x),
					pos.y + (0.5f * size.y));
		}
		else if (dir == PI) {
			vectors[0] = glm::vec2(pos.x + (0.5f + size.y), 
					pos.y);
			vectors[1] = glm::vec2(pos.x + (0.5f * -size.y), 
					pos.y + (0.5f * size.x));
			vectors[2] = glm::vec2(pos.x + (0.5f * -size.y),
					pos.y + (0.5f * -size.x));
		}
		else {
			vectors[0] = glm::vec2(pos.x, 
					pos.y + (0.5f * size.y));
			vectors[1] = glm::vec2(pos.x + (0.5f * size.x), 
					pos.y + (0.5f * -size.y));
			vectors[2] = glm::vec2(pos.x + (0.5f * -size.x),
					pos.y + (0.5f * -size.y));
		}
	}
};

struct Enemy {
	glm::vec2 pos = glm::vec2(10.0f, 1.0f);
	glm::vec2 vel = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(0.5, 1.0f);
	glm::vec2 alert_size = glm::vec2(0.2, 0.4);
	glm::vec2 waypoints [2] = { glm::vec2(10.0f, 1.0f), glm::vec2(4.0f, 1.0f) };
	glm::vec2 target = glm::vec2(0.0f, 0.0f);
	glm::vec2 right_flashlight_offset = glm::vec2(2.7f, 0.0f);
	glm::vec2 left_flashlight_offset = glm::vec2(-4.0f, 0.0f);

	bool face_right = true;
	bool alerted = false;
	bool walking = false;

	float wait_timers [2] = { 5.0f, 5.0f };
	float remaining_wait = 5.0f;
	float sight_range = 5.0f;
	float catch_range = 0.5f;
	int curr_index  = 0;

	Light flashlight;

	int animation_count;
	int animation_delay;

	void update_pos() {
		if (face_right) {
			flashlight.dir = 0.0f;
			flashlight.pos = pos + right_flashlight_offset;
		}
		else {
			flashlight.dir = PI;
			flashlight.pos = pos + left_flashlight_offset;
		}

	}

	SpriteInfo sprite_animations[5] = {
		{
			glm::vec2(   0.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2( 400.0f / 7000.0f, (5500.0f -   0.0f) / 5500.0f),
		},
		{
			glm::vec2( 400.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2( 800.0f / 7000.0f, (5500.0f -    0.0f) / 5500.0f),
		},
		{
			glm::vec2( 800.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(1200.0f / 7000.0f, (5500.0f -    0.0f) / 5500.0f),
		},
		{
			glm::vec2(1200.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(1600.0f / 7000.0f, (5500.0f -    0.0f) / 5500.0f),
		},
		{
			glm::vec2(1600.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(2000.0f / 7000.0f, (5500.0f -    0.0f) / 5500.0f),
		},
	};

	SpriteInfo alert = {
		glm::vec2(2600.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		glm::vec2(2800.0f / 7000.0f, (5500.0f - 900.0f) / 5500.0f),
	};
};

struct Door {
	glm::vec2 pos = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(1.0f, 1.5f);
	bool in_use = false;

	SpriteInfo sprite_empty = {
		glm::vec2(2400.0f / 7000.0f, (5500.0f - 800.0f) / 5500.0f),
		glm::vec2(2900.0f / 7000.0f, (5500.0f - 0.0f) / 5500.0f),
	};
	SpriteInfo sprite_used = {
		glm::vec2(0.0f, 481.0f/1689.0f),
		glm::vec2(740.0f/3503.0f, 1.0f),
	};
};

struct Ladder {
	glm::vec2 pos = glm::vec2(0.0f);
	glm::vec2 size = glm::vec2(1.0f, 1.5f);
	bool in_use = false;
	bool player_collision = false;

	SpriteInfo sprite_empty = {
		glm::vec2(200.0f / 7000.0f, (5500.0f - 2600.0f) / 5500.0f),
		glm::vec2(600.0f / 7000.0f, (5500.0f - 1400.0f) / 5500.0f),
	};
	void detect_collision(glm::vec2 player_pos, glm::vec2 player_size) {
		if (((player_pos.y + player_size.y / 2.0f) <= (pos.y + size.y/2.0f)) &&
				((player_pos.y - player_size.y / 2.0f) >= (pos.y - size.y/2.0f))) {
			if (((player_pos.x + player_size.x / 2.0f) <= (pos.x + size.x/2.0f)) &&
					((player_pos.x - player_size.x / 2.0f) >= (pos.x - size.x/2.0f))) {
				player_collision = true;
			}
			else
				player_collision = false;
		}
		else {
			player_collision = false;
		}
	}
};

struct Platform {
	glm::vec2 pos = glm::vec2(10.0f, 0.25f);
	glm::vec2 size = glm::vec2(20.0f, 0.5f);
	bool player_collision = false;

	SpriteInfo sprite = {
		glm::vec2(0.0f / 7000.0f, (5500.0f - 1300.0f) / 5500.0f),
		glm::vec2(2400.0f / 7000.0f, (5500.0f - 800.0f) / 5500.0f),
	};
	void detect_collision(glm::vec2 player_pos, glm::vec2 player_size) {
		if (((player_pos.y + player_size.y / 2.0f) >= (pos.y + size.y/2.0f)) &&
				((player_pos.y - player_size.y / 2.0f) <= (pos.y + size.y/2.0f))) {
			if (((player_pos.x + player_size.x / 2.0f) <= (pos.x + size.x/2.0f)) &&
					((player_pos.x - player_size.x / 2.0f) >= (pos.x - size.x/2.0f))) {
				player_collision = true;
			}
			else
				player_collision = false;
		}
		else {
			player_collision = false;
		}
	}
};

struct Air_Platform {
	glm::vec2 pos = glm::vec2(10.0f, 1.4f);
	glm::vec2 size = glm::vec2(5.0f, 0.5f);

	SpriteInfo sprite = {
		glm::vec2(0.0f / 7000.0f, (5500.0f - 1300.0f) / 5500.0f),
		glm::vec2(2400.0f / 7000.0f, (5500.0f - 800.0f) / 5500.0f),
	};
};

struct MenuesInfo {

	int selected_menu;
	int selected_level;

	SpriteInfo levels_highlighted[5] = {
		{
			glm::vec2(3100.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(3700.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		},
		{
			glm::vec2(3700.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(4300.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		},
		{
			glm::vec2(4300.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(4900.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		},
		{
			glm::vec2(4900.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(5500.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		},
		{
			glm::vec2(5500.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
			glm::vec2(6100.0f / 7000.0f, (5500.0f - 1200.0f) / 5500.0f),
		},
	};

	SpriteInfo levels[5] = {
		{
			glm::vec2(3100.0f / 7000.0f, (5500.0f -  200.0f) / 5500.0f),
			glm::vec2(3700.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
		},
		{
			glm::vec2(3700.0f / 7000.0f, (5500.0f -  200.0f) / 5500.0f),
			glm::vec2(4300.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
		},
		{
			glm::vec2(4300.0f / 7000.0f, (5500.0f -  200.0f) / 5500.0f),
			glm::vec2(4900.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
		},
		{
			glm::vec2(4900.0f / 7000.0f, (5500.0f -  200.0f) / 5500.0f),
			glm::vec2(5500.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
		},
		{
			glm::vec2(5500.0f / 7000.0f, (5500.0f -  200.0f) / 5500.0f),
			glm::vec2(6100.0f / 7000.0f, (5500.0f -  700.0f) / 5500.0f),
		},
	};

	SpriteInfo menus[2] = {
		{
			glm::vec2(3900.0f / 7000.0f, (5500.0f - 2200.0f) / 5500.0f),
			glm::vec2(4900.0f / 7000.0f, (5500.0f - 2600.0f) / 5500.0f),
		},
		{
			glm::vec2(3900.0f / 7000.0f, (5500.0f - 3700.0f) / 5500.0f),
			glm::vec2(4900.0f / 7000.0f, (5500.0f - 4200.0f) / 5500.0f),
		},
	};

	SpriteInfo menus_selected[2] = {
		{
			glm::vec2(5000.0f / 7000.0f, (5500.0f - 2200.0f) / 5500.0f),
			glm::vec2(6000.0f / 7000.0f, (5500.0f - 2600.0f) / 5500.0f),
		},
		{
			glm::vec2(5000.0f / 7000.0f, (5500.0f - 3700.0f) / 5500.0f),
			glm::vec2(6000.0f / 7000.0f, (5500.0f - 4200.0f) / 5500.0f),
		},
	};
};

static void playTone(void *userdata, Uint8 *stream, int streamlength);

void readSizes(int level,
		void *sizes_p){

	int *sizes = reinterpret_cast<int*>(sizes_p);

	const int num_variables = 5;
	int x;
	ifstream inFile;

	std::string prefix = "level";
	std::string level_str = std::to_string(level);
	std::string suffix = "/num_objects.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_variables; i++){
		inFile >> x;
		sizes[i] = x;
	}

	inFile.close();
}

void readLevel(int level,
		void *sizes_p,
		void* plat_pos_x_p, void* plat_pos_y_p, void* plat_size_x_p, void* plat_size_y_p,
		void* enem_pos_x_p, void* enem_pos_y_p, void* enem_w1_x_p, void* enem_w1_y_p, void* enem_w2_x_p, void* enem_w2_y_p, void* enem_fs_x_p, void* enem_fs_y_p,
		void* lights_pos_x_p, void* lights_pos_y_p, void* lights_size_x_p, void* lights_size_y_p, void* lights_dir_p,
		void* doors_pos_x_p, void* doors_pos_y_p,
		void* ladders_pos_x_p, void* ladders_pos_y_p, void* ladders_height_p){

	int *sizes = reinterpret_cast<int*>(sizes_p);

	//platforms
	float *plat_pos_x = reinterpret_cast<float*>(plat_pos_x_p);
	float *plat_pos_y = reinterpret_cast<float*>(plat_pos_y_p);
	float *plat_size_x = reinterpret_cast<float*>(plat_size_x_p);
	float *plat_size_y = reinterpret_cast<float*>(plat_size_y_p);

	//enemies
	float *enem_pos_x = reinterpret_cast<float*>(enem_pos_x_p);
	float *enem_pos_y = reinterpret_cast<float*>(enem_pos_y_p);
	float *enem_w1_x = reinterpret_cast<float*>(enem_w1_x_p);
	float *enem_w1_y = reinterpret_cast<float*>(enem_w1_y_p);
	float *enem_w2_x = reinterpret_cast<float*>(enem_w2_x_p);
	float *enem_w2_y = reinterpret_cast<float*>(enem_w2_y_p);
	float *enem_fs_x = reinterpret_cast<float*>(enem_fs_x_p);
	float *enem_fs_y = reinterpret_cast<float*>(enem_fs_y_p);

	//lights
	float *lights_pos_x = reinterpret_cast<float*>(lights_pos_x_p);
	float *lights_pos_y = reinterpret_cast<float*>(lights_pos_y_p);
	float *lights_size_x = reinterpret_cast<float*>(lights_size_x_p);
	float *lights_size_y = reinterpret_cast<float*>(lights_size_y_p);
	float *lights_dir = reinterpret_cast<float*>(lights_dir_p);

	//doors
	float *doors_pos_x = reinterpret_cast<float*>(doors_pos_x_p);
	float *doors_pos_y = reinterpret_cast<float*>(doors_pos_y_p);

	//ladders
	float *ladders_pos_x = reinterpret_cast<float*>(ladders_pos_x_p);
	float *ladders_pos_y = reinterpret_cast<float*>(ladders_pos_y_p);
	float *ladders_height = reinterpret_cast<float*>(ladders_height_p);

	float y;
	ifstream inFile;

	//initialize the number of objects per things in level
	int num_lights = sizes[0];
	int num_plats = sizes[1];
	int num_enemies = sizes[2];
	int num_doors = sizes[3];
	int num_ladders = sizes[4];

	//grab platform info
	std::string prefix = "level";
	std::string level_str = std::to_string(level);
	std::string suffix = "/plats.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_plats; i++){
		inFile >> y;
		plat_pos_x[i] = y;
		inFile >> y;
		plat_pos_y[i] = y;
		inFile >> y;
		plat_size_x[i] = y;
		inFile >> y;
		plat_size_y[i] = y;
	}

	inFile.close();

	//grab enemy info
	suffix = "/enemies.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_enemies; i++){
		inFile >> y;
		enem_pos_x[i] = y;
		inFile >> y;
		enem_pos_y[i] = y;
		inFile >> y;
		enem_w1_x[i] = y;
		inFile >> y;
		enem_w1_y[i] = y;
		inFile >> y;
		enem_w2_x[i] = y;
		inFile >> y;
		enem_w2_y[i] = y;
		inFile >> y;
		enem_fs_x[i] = y;
		inFile >> y;
		enem_fs_y[i] = y;
	}

	inFile.close();

	//grab lights info
	suffix = "/lights.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_lights; i++){
		inFile >> y;
		lights_pos_x[i] = y;
		inFile >> y;
		lights_pos_y[i] = y;
		inFile >> y;
		lights_size_x[i] = y;
		inFile >> y;
		lights_size_y[i] = y;
		inFile >> y;
		lights_dir[i] = y;
	}

	inFile.close();

	//grab doors info
	suffix = "/doors.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_doors; i++){
		inFile >> y;
		doors_pos_x[i] = y;
		inFile >> y;
		doors_pos_y[i] = y;
	}

	inFile.close();

	//grab ladder info
	suffix = "/ladders.txt";

	inFile.open(prefix + level_str + suffix);
	if (!inFile) {
		cout << "Unable to open file";
		exit(1); // terminate with error
	}

	for (int i = 0; i < num_ladders; i++){
		inFile >> y;
		ladders_pos_x[i] = y;
		inFile >> y;
		ladders_pos_y[i] = y;
		inFile >> y;
		ladders_height[i] = y;
	}

	inFile.close();
}

void loadLevel(int level,
		void* Vector_Platforms_p,
		void* Vector_Doors_p,
		void* Vector_Lights_p,
		void* Vector_Enemies_p,
		void* Vector_Ladders_p){

	std::vector< Platform >* Vector_Platforms_point = reinterpret_cast< std::vector< Platform > *>(Vector_Platforms_p);
	std::vector< Door >* Vector_Doors_point = reinterpret_cast< std::vector< Door > *>(Vector_Doors_p);
	std::vector< Light >* Vector_Lights_point = reinterpret_cast< std::vector< Light > *>(Vector_Lights_p);
	std::vector< Enemy >* Vector_Enemies_point = reinterpret_cast< std::vector< Enemy > *>(Vector_Enemies_p);
	std::vector< Ladder >* Vector_Ladders_point = reinterpret_cast< std::vector< Ladder > *>(Vector_Ladders_p);

	//---- Level Loading ----
	/* Levels will be imported from outside code / txt file. Levels will define
	 * object initialization, as well as the variables attached to each object.
	 */

	const int num_variables = 5;

	int sizes[num_variables];

	readSizes(level, reinterpret_cast<void*>(sizes));

	// printf("read sizes successful\n");

	//initialize the number of objects per things in level
	int num_lights = sizes[0];
	int num_plats = sizes[1];
	int num_enemies = sizes[2];
	int num_doors = sizes[3];
	int num_ladders = sizes[4];

	float* plat_pos_x;
	float* plat_pos_y;
	float* plat_size_x;
	float* plat_size_y;
	plat_pos_x = new float[num_plats];
	plat_pos_y = new float[num_plats];
	plat_size_x = new float[num_plats];
	plat_size_y = new float[num_plats];

	float* enem_pos_x;
	float* enem_pos_y;
	float* enem_w1_x;
	float* enem_w1_y;
	float* enem_w2_x;
	float* enem_w2_y;
	float* enem_fs_x;
	float* enem_fs_y;
	enem_pos_x = new float[num_enemies];
	enem_pos_y = new float[num_enemies];
	enem_w1_x = new float[num_enemies];
	enem_w1_y = new float[num_enemies];
	enem_w2_x = new float[num_enemies];
	enem_w2_y = new float[num_enemies];
	enem_fs_x = new float[num_enemies];
	enem_fs_y = new float[num_enemies];

	float* lights_pos_x;
	float* lights_pos_y;
	float* lights_size_x;
	float* lights_size_y;
	float* lights_dir;
	lights_pos_x = new float[num_lights];
	lights_pos_y = new float[num_lights];
	lights_size_x = new float[num_lights];
	lights_size_y = new float[num_lights];
	lights_dir = new float[num_lights];

	float* doors_pos_x;
	float* doors_pos_y;
	doors_pos_x = new float[num_doors];
	doors_pos_y = new float[num_doors];

	float* ladders_pos_x;
	float* ladders_pos_y;
	float* ladders_height;
	ladders_pos_x = new float[num_ladders];
	ladders_pos_y = new float[num_ladders];
	ladders_height = new float[num_ladders];

	readLevel(level,
			reinterpret_cast<void*>(sizes),
			reinterpret_cast<void*>(plat_pos_x), reinterpret_cast<void*>(plat_pos_y), reinterpret_cast<void*>(plat_size_x), reinterpret_cast<void*>(plat_size_y),
			reinterpret_cast<void*>(enem_pos_x), reinterpret_cast<void*>(enem_pos_y), reinterpret_cast<void*>(enem_w1_x), reinterpret_cast<void*>(enem_w1_y), 
			reinterpret_cast<void*>(enem_w2_x), reinterpret_cast<void*>(enem_w2_y), reinterpret_cast<void*>(enem_fs_x), reinterpret_cast<void*>(enem_fs_y),
			reinterpret_cast<void*>(lights_pos_x), reinterpret_cast<void*>(lights_pos_y), reinterpret_cast<void*>(lights_size_x), reinterpret_cast<void*>(lights_size_y), reinterpret_cast<void*>(lights_dir),
			reinterpret_cast<void*>(doors_pos_x), reinterpret_cast<void*>(doors_pos_y),
			reinterpret_cast<void*>(ladders_pos_x), reinterpret_cast<void*>(ladders_pos_y), reinterpret_cast<void*>(ladders_height));

	// printf("read level successful\n");


	/***** done importing level, start initializing *****/

	Platform* platforms;
	Light* lights;
	Enemy* enemies;
	Door* door;
	Ladder* ladders;

	platforms = new Platform[num_plats];
	lights = new Light[num_lights];
	enemies = new Enemy[num_enemies];
	door = new Door[num_doors];
	ladders = new Ladder[num_ladders];

	for (int i = 0; i < num_plats; i++) {
		(*Vector_Platforms_point).emplace_back(platforms[i]);
	}
	for (int i = 0; i < num_lights; i++) {
		(*Vector_Lights_point).emplace_back(lights[i]);
	}
	for (int i = 0; i < num_enemies; i++) {
		(*Vector_Enemies_point).emplace_back(enemies[i]);
	}
	for (int i = 0; i < num_doors; i++) {
		(*Vector_Doors_point).emplace_back(door[i]);
	}
	for (int i = 0; i < num_ladders; i++) {
		(*Vector_Ladders_point).emplace_back(ladders[i]);
	}


	//---- Set Object Variables ---

	//Platforms
	for (int i = 0; i < num_plats; i++) {
		(*Vector_Platforms_point)[i].pos = glm::vec2(plat_pos_x[i], plat_pos_y[i]);
		(*Vector_Platforms_point)[i].size = glm::vec2(plat_size_x[i], plat_size_y[i]);
	}

	//Enemies
	for (int i = 0; i < num_enemies; i++) {
		(*Vector_Enemies_point)[i].pos = glm::vec2(enem_pos_x[i], enem_pos_y[i]);
		(*Vector_Enemies_point)[i].waypoints[0] = glm::vec2(enem_w1_x[i], enem_w1_y[i]);
		(*Vector_Enemies_point)[i].waypoints[1] = glm::vec2(enem_w2_x[i], enem_w2_y[i]);
		(*Vector_Enemies_point)[i].flashlight.size = glm::vec2(enem_fs_x[i], enem_fs_y[i]);
		(*Vector_Enemies_point)[i].update_pos();
		(*Vector_Enemies_point)[i].flashlight.rotate();
	}

	//Lights
	for (int i = 0; i < num_lights; i++) {
		(*Vector_Lights_point)[i].pos = glm::vec2(lights_pos_x[i], lights_pos_y[i]);
		(*Vector_Lights_point)[i].size = glm::vec2(lights_size_x[i], lights_size_y[i]);
		(*Vector_Lights_point)[i].dir = PI * lights_dir[i];
	}

	//Doors
	for (int i = 0; i < num_doors; i++) {
		(*Vector_Doors_point)[i].pos = glm::vec2(doors_pos_x[i], doors_pos_y[i]);
	}

	//Ladders
	for (int i = 0; i < num_ladders; i++) {
		(*Vector_Ladders_point)[i].pos = glm::vec2(ladders_pos_x[i], ladders_pos_y[i]);
		(*Vector_Ladders_point)[i].size = glm::vec2(1.0f, ladders_height[i]);
	}

	// printf("finished initialization\n");


	//free memory (initialization is done, we don't need these variable sized arrays)

	delete [] platforms;  // Free memory allocated for the a array.
	platforms = NULL;     // Be sure the deallocated memory isn't used.
	delete [] lights;  // Free memory allocated for the a array.
	lights = NULL;     // Be sure the deallocated memory isn't used.
	delete [] enemies;  // Free memory allocated for the a array.
	enemies = NULL;     // Be sure the deallocated memory isn't used.
	delete [] door;  // Free memory allocated for the a array.
	door = NULL;     // Be sure the deallocated memory isn't used.
	delete [] ladders;  // Free memory allocated for the a array.
	ladders = NULL;     // Be sure the deallocated memory isn't used.

	delete [] plat_pos_x;
	delete [] plat_pos_y;
	delete [] plat_size_x;
	delete [] plat_size_y;

	plat_pos_x = NULL;
	plat_pos_y = NULL;
	plat_size_x = NULL;
	plat_size_y = NULL;

	delete [] enem_pos_x;
	delete [] enem_pos_y;
	delete [] enem_w1_x;
	delete [] enem_w1_y;
	delete [] enem_w2_x;
	delete [] enem_w2_y;
	delete [] enem_fs_x;
	delete [] enem_fs_y;

	enem_pos_x = NULL;
	enem_pos_y = NULL;
	enem_w1_x = NULL;
	enem_w1_y = NULL;
	enem_w2_x = NULL;
	enem_w2_y = NULL;
	enem_fs_x = NULL;
	enem_fs_y = NULL;

	delete [] lights_pos_x;
	delete [] lights_pos_y;
	delete [] lights_size_x;
	delete [] lights_size_y;
	delete [] lights_dir;

	lights_pos_x = NULL;
	lights_pos_y = NULL;
	lights_size_x = NULL;
	lights_size_y = NULL;
	lights_dir = NULL;

	delete [] doors_pos_x;
	delete [] doors_pos_y;

	doors_pos_x = NULL;
	doors_pos_y = NULL;

	delete [] ladders_pos_x;
	delete [] ladders_pos_y;
	delete [] ladders_height;

	ladders_pos_x = NULL;
	ladders_pos_y = NULL;
	ladders_height = NULL;

	// printf("finished freeing\n");
}

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game1: Text/Tiles";
		glm::uvec2 size = glm::uvec2(1200, 800);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_AUDIO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
			config.title.c_str(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			config.size.x, config.size.y,
			SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
			);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	//SDL_ShowCursor(SDL_DISABLE);

	//SDL Background Audio
	SDL_AudioSpec wavSpec;
	Uint8 *wavStart;
	Uint32 wavLength;
	//alert audio
	SDL_AudioSpec alertSpec;
	Uint8 *alertStart;
	Uint32 alertLength;
	//door audio
	SDL_AudioSpec doorSpec;
	Uint8 *doorStart;
	Uint32 doorLength;
	//ladder audio
	SDL_AudioSpec ladderSpec;
	Uint8 *ladderStart;
	Uint32 ladderLength;
	//ornament audio
	SDL_AudioSpec ornSpec;
	Uint8 *ornStart;
	Uint32 ornLength;
	//step audio
	SDL_AudioSpec stepSpec;
	Uint8 *stepStart;
	Uint32 stepLength;

	if (SDL_LoadWAV(BG_MUSIC_PATH, &wavSpec, &wavStart, &wavLength) == NULL) {
		std::cerr << "Failed to load back ground music" << std::endl;
		exit(1);
	}
	if (SDL_LoadWAV(ALERT_MUSIC_PATH, &alertSpec, &alertStart, &alertLength) == NULL) {
		std::cerr << "Failed to load alert music" << std::endl;
		exit(1);
	}
	if (SDL_LoadWAV(DOOR_MUSIC_PATH, &doorSpec, &doorStart, &doorLength) == NULL) {
		std::cerr << "Failed to load door music" << std::endl;
		exit(1);
	}
	if (SDL_LoadWAV(LADDER_MUSIC_PATH, &ladderSpec, &ladderStart, &ladderLength) == NULL) {
		std::cerr << "Failed to load ladder music" << std::endl;
		exit(1);
	}
	if (SDL_LoadWAV(ORNAMENT_PATH, &ornSpec, &ornStart, &ornLength) == NULL) {
		std::cerr << "Failed to load ornament sound" << std::endl;
		exit(1);
	}
	if (SDL_LoadWAV(STEP_MUSIC_PATH, &stepSpec, &stepStart, &stepLength) == NULL) {
		std::cerr << "Failed to load step sound" << std::endl;
		exit(1);
	}

	AudioData audioData;
	audioData.pos = wavStart;
	audioData.length = wavLength;

	audioData.init_pos = wavStart;
	audioData.init_length = wavLength;

	wavSpec.callback = playTone;
	wavSpec.userdata = &audioData;

	AudioData doorData;
	doorData.pos = doorStart;
	doorData.length = doorLength;

	doorData.init_pos = doorStart;
	doorData.init_length = doorLength;

	doorSpec.callback = playTone;
	doorSpec.userdata = &doorData;

	AudioData ladderData;
	ladderData.pos = ladderStart;
	ladderData.length = ladderLength;

	ladderData.init_pos = ladderStart;
	ladderData.init_length = ladderLength;

	ladderSpec.callback = playTone;
	ladderSpec.userdata = &ladderData;

	AudioData ornData;
	ornData.pos = ornStart;
	ornData.length = ornLength;

	ornData.init_pos = ornStart;
	ornData.init_length = ornLength;

	ornSpec.callback = playTone;
	ornSpec.userdata = &ornData;

	AudioData alertData;
	alertData.pos = alertStart;
	alertData.length = alertLength;

	alertData.init_pos = alertStart;
	alertData.init_length = alertLength;

	alertSpec.callback = playTone;
	alertSpec.userdata = &alertData;

	AudioData stepData;
	stepData.pos = stepStart;
	stepData.length = stepLength;

	stepData.init_pos = stepStart;
	stepData.init_length = stepLength;

	stepSpec.callback = playTone;
	stepSpec.userdata = &stepData;


	SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	SDL_AudioDeviceID doorDevice = SDL_OpenAudioDevice(NULL, 0, &doorSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	SDL_AudioDeviceID ladderDevice = SDL_OpenAudioDevice(NULL, 0, &ladderSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	SDL_AudioDeviceID ornDevice = SDL_OpenAudioDevice(NULL, 0, &ornSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	SDL_AudioDeviceID alertAudioDevice = SDL_OpenAudioDevice(NULL, 0, &alertSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	SDL_AudioDeviceID stepDevice = SDL_OpenAudioDevice(NULL, 0, &stepSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);

	if (audioDevice == 0) {
		std::cerr << "Failed to grab a device" << std::endl;
		exit(1);
	}
	if (doorDevice == 0) {
		std::cerr << "Failed to grab a device (door)" << std::endl;
		exit(1);
	}
	if (ladderDevice == 0) {
		std::cerr << "Failed to grab a device (ladder) " << std::endl;
		exit(1);
	}
	if (ornDevice == 0) {
		std::cerr << "Failed to grab a device (ornament) " << std::endl;
		exit(1);
	}
	if (alertAudioDevice == 0) {
		std::cerr << "Failed to grab a device (alert)" << std::endl;
		exit(1);
	}


	//------------ opengl objects / game assets ------------

	//texture:
	GLuint tex = 0;
	glm::uvec2 tex_size = glm::uvec2(0,0);

	{ //load texture 'tex':
		std::vector< uint32_t > data;
		if (!load_png("atlas.png", &tex_size.x, &tex_size.y, &data, LowerLeftOrigin)) {
			std::cerr << "Failed to load texture." << std::endl;
			exit(1);
		}
		//create a texture object:
		glGenTextures(1, &tex);
		//bind texture object to GL_TEXTURE_2D:
		glBindTexture(GL_TEXTURE_2D, tex);
		//upload texture data from data:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_size.x, tex_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		//set texture sampling parameters:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}


	// separate texture specifically for lights
	GLuint tex2 = 0;
	glm::uvec2 tex2_size = glm::uvec2(0,0);

	{ //load texture 'tex2':
		std::vector< uint32_t > data;
		if (!load_png("light.png", &tex2_size.x, &tex2_size.y, &data, LowerLeftOrigin)) {
			std::cerr << "Failed to load texture." << std::endl;
			exit(1);
		}
		//create a texture object:
		glGenTextures(1, &tex2);
		//bind texture object to GL_TEXTURE_2D:
		glBindTexture(GL_TEXTURE_2D, tex2);
		//upload texture data from data:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex2_size.x, tex2_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		//set texture sampling parameters:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	//shader program:
	GLuint program = 0;
	GLuint program_Position = 0;
	GLuint program_TexCoord = 0;
	GLuint program_Color = 0;
	GLuint program_mvp = 0;
	GLuint program_tex = 0;
	{ //compile shader program:
		GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER,
				"#version 330\n"
				"uniform mat4 mvp;\n"
				"in vec4 Position;\n"
				"in vec2 TexCoord;\n"
				"in vec4 Color;\n"
				"out vec2 texCoord;\n"
				"out vec4 color;\n"
				"void main() {\n"
				"	gl_Position = mvp * Position;\n"
				"	color = Color;\n"
				"	texCoord = TexCoord;\n"
				"}\n"
				);

		GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER,
				"#version 330\n"
				"uniform sampler2D tex;\n"
				"in vec4 color;\n"
				"in vec2 texCoord;\n"
				"out vec4 fragColor;\n"
				"void main() {\n"
				"	fragColor = texture(tex, texCoord) * color;\n"
				"}\n"
				);

		program = link_program(fragment_shader, vertex_shader);

		//look up attribute locations:
		program_Position = glGetAttribLocation(program, "Position");
		if (program_Position == -1U) throw std::runtime_error("no attribute named Position");
		program_TexCoord = glGetAttribLocation(program, "TexCoord");
		if (program_TexCoord == -1U) throw std::runtime_error("no attribute named TexCoord");
		program_Color = glGetAttribLocation(program, "Color");
		if (program_Color == -1U) throw std::runtime_error("no attribute named Color");

		//look up uniform locations:
		program_mvp = glGetUniformLocation(program, "mvp");
		if (program_mvp == -1U) throw std::runtime_error("no uniform named mvp");
		program_tex = glGetUniformLocation(program, "tex");
		if (program_tex == -1U) throw std::runtime_error("no uniform named tex");
	}

	//vertex buffer:
	GLuint buffer = 0;
	{ //create vertex buffer
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
	}

	struct Vertex {
		Vertex(glm::vec2 const &Position_, glm::vec2 const &TexCoord_, glm::u8vec4 const &Color_) :
			Position(Position_), TexCoord(TexCoord_), Color(Color_) { }
		glm::vec2 Position;
		glm::vec2 TexCoord;
		glm::u8vec4 Color;
	};
	static_assert(sizeof(Vertex) == 20, "Vertex is nicely packed.");

	//vertex array object:
	GLuint vao = 0;
	{ //create vao and set up binding:
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glVertexAttribPointer(program_Position, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0);
		glVertexAttribPointer(program_TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLbyte *)0 + sizeof(glm::vec2));
		glVertexAttribPointer(program_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLbyte *)0 + sizeof(glm::vec2) + sizeof(glm::vec2));
		glEnableVertexAttribArray(program_Position);
		glEnableVertexAttribArray(program_TexCoord);
		glEnableVertexAttribArray(program_Color);
	}

	//------------ structs and variables ------------

	//----------------- Variables --------------------------------------------

	CameraInfo camera;
	MouseInfo mouse;
	PlayerInfo player;
	MenuesInfo menus;
	Background bg;

	//adjust for aspect ratio
	camera.size.x = camera.size.y * (float(config.size.x) / float(config.size.y));


	//------------ Initialization ---------------------------------------------

	int completed_levels = 0;

	int level = 0;

	std::vector< Platform > Vector_Platforms = {};
	std::vector< Door > Vector_Doors = {};
	std::vector< Light > Vector_Lights = {};
	std::vector< Enemy > Vector_Enemies = {};
	std::vector< Ladder > Vector_Ladders = {};

	//to start the game, we don't load the first level, we instead load the main menu page
	bool in_menu = true;
	bool in_level_select = false;

	//likewise, we must set the player to be invisible if behind a door (beccomes visible when loading level)
	player.behind_door = true;
	player.walking = false;

	player.animation_count = 9;
	player.animation_delay = 0;

	//create variables associated with these 2 different screens
	//for menu
	bool play_highlighted = true;

	//for level select
	bool back_button_highlighted = false;
	bool unlocked[5] = {true, false, false, false, false};
	int num_unlocked = 0;
	int level_highlighted = 0;

	glm::vec2 default_player_pos = glm::vec2(0.25f, 1.0f);
	glm::vec2 default_player_vel = glm::vec2(0.0f);

	//const float ceiling_height = 10.0f;
	const float floor_height = 0.25f;
	const float level_end = 40.0f;
	//for tutorial level, fix later but it looks like it works idk u tell me
	//const float air_plat_height = 2.0f;

	bool on_platform = false;
	bool on_ladder = false;
	bool check_on_ladder = false;


	//debugging
	bool caught = false;

	//------------ game loop ------------

	//Start audio playback
	SDL_PauseAudioDevice(audioDevice, 0);

	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				mouse.pos.x = ((evt.motion.x + 0.5f) / float(config.size.x) * 2.0f - 1.0f) * 0.5f * camera.size.x + camera.pos.x;
				mouse.pos.y = ((evt.motion.y + 0.5f) / float(config.size.y) *-2.0f + 1.0f) * 0.5f * camera.size.y + camera.pos.y;
			} 
			else if (evt.type == SDL_MOUSEBUTTONDOWN) {
				if (evt.button.button == SDL_BUTTON_LEFT) {
					if (player.aiming && player.num_projectiles > 0) {
						player.num_projectiles--;
						SDL_PauseAudioDevice(ornDevice, 0);
						player.projectiles_pos.push_back(player.aimed_pos);
					}
				} else if (evt.button.button == SDL_BUTTON_RIGHT) {
					if (mouse.remaining_time <= 0.0f) {
						player.aiming = !player.aiming;
						if (!player.aiming) {
							mouse.remaining_time = 1.0f;
						}
					}
				}
			} 
			else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} 
			else if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
				if (evt.key.keysym.sym == SDLK_w) {
					if (in_menu && evt.key.state == SDL_PRESSED){
						if (!in_level_select){
							play_highlighted = !play_highlighted;
						}
						else{
							back_button_highlighted = !back_button_highlighted;
						}
					}

					if (!in_menu && !on_ladder && !player.aiming && evt.key.state == SDL_PRESSED) {
						//climb onto the ladder
						for (Ladder& ladder : Vector_Ladders){
							ladder.detect_collision(player.pos, player.size);
							on_ladder = on_ladder || ladder.player_collision;
						}

						if (on_ladder){
							/****** add "player climbing" sprite *******/
						}
					}

					if (!in_menu && on_ladder && evt.key.state == SDL_PRESSED) {
						check_on_ladder = false;

						//check if player will remain on ladder
						for (Ladder& ladder : Vector_Ladders){
							ladder.detect_collision(glm::vec2(player.pos.x, player.pos.y + 0.02f), player.size);
							check_on_ladder = check_on_ladder || ladder.player_collision;
						}

						if (check_on_ladder){
							//climb the actual ladder
							SDL_PauseAudioDevice(ladderDevice, 0);
							player.pos.y += 0.1f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_UP) {
					if (in_menu && evt.key.state == SDL_PRESSED){
						if (!in_level_select){
							play_highlighted = !play_highlighted;
						}
						else{
							back_button_highlighted = !back_button_highlighted;
						}
					}
				} 
				//selection using the return key
				else if (evt.key.keysym.sym == SDLK_RETURN) {
					if (in_menu && evt.key.state == SDL_PRESSED){
						if (!in_level_select){
							if (play_highlighted){
								in_level_select = true;
								back_button_highlighted = false;
							} else{
								should_quit = true;
							}
						}else{
							if (back_button_highlighted){
								in_level_select = false;
							}else{
								//load the level in this case

								//exit main menu and level select
								in_menu = false;
								in_level_select = false;

								//reset player statuses
								player.pos = default_player_pos;
								player.vel = default_player_vel;

								on_platform = false;
								on_ladder = false;
								check_on_ladder = false;

								player.face_right = false;
								player.jumping = false;
								player.shifting = false;
								player.behind_door = false;
								player.aiming = false;
								player.visible = false; 

								player.num_projectiles = 5;

								level = level_highlighted;

								Vector_Platforms = {};
								Vector_Doors = {};
								Vector_Lights = {};
								Vector_Enemies = {};
								Vector_Ladders = {};

								loadLevel(level,
										reinterpret_cast<void*>(&Vector_Platforms),
										reinterpret_cast<void*>(&Vector_Doors),
										reinterpret_cast<void*>(&Vector_Lights),
										reinterpret_cast<void*>(&Vector_Enemies),
										reinterpret_cast<void*>(&Vector_Ladders));
							}
						}
					}
				} 
				//testing ability to return to main menu
				else if (evt.key.keysym.sym == SDLK_m) {
					//go to menu
					in_menu = true;
					in_level_select = false;
					play_highlighted = true;

					//reset player statuses
					player.pos = default_player_pos;
					player.vel = default_player_vel;

					on_platform = false;
					on_ladder = false;
					check_on_ladder = false;

					player.face_right = false;
					player.jumping = false;
					player.shifting = false;

					//we set player behind door as a hack to "remove" player while we're in the main menu
					player.behind_door = true;

					player.aiming = false;
					player.visible = false; 

					player.num_projectiles = 5;

					//reset to level 0 just in case (shouldn't matter though)
					level = 0;

					Vector_Platforms = {};
					Vector_Doors = {};
					Vector_Lights = {};
					Vector_Enemies = {};
					Vector_Ladders = {};
				} 
				else if (evt.key.keysym.sym == SDLK_a) {
					if (evt.key.state == SDL_PRESSED) {
						if (in_menu){
							if (in_level_select){
								if (!back_button_highlighted){
									if (level_highlighted == 0){
										level_highlighted = num_unlocked;
									}
									else{
										level_highlighted -= 1;
									}
								}
							}
						}
						else{
							if (player.shifting) {
								player.vel.x = -2.0f;
								player.face_right = false;
							} else {
								player.vel.x = -1.0f;
								player.face_right = false;
							}
							if (on_ladder){
								on_ladder = false;
							}
						}
					} else {
						if (player.vel.x == -1.0f || player.vel.x == -2.0f) {
							player.vel.x = 0.0f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_LEFT) {
					if (evt.key.state == SDL_PRESSED) {
						if (in_menu){
							if (in_level_select){
								if (!back_button_highlighted){
									if (level_highlighted == 0){
										level_highlighted = num_unlocked;
									}
									else{
										level_highlighted -= 1;
									}
								}
							}
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_d) {
					if (evt.key.state == SDL_PRESSED) {
						if (in_menu){
							if (in_level_select){
								if (!back_button_highlighted){
									if (level_highlighted == num_unlocked){
										level_highlighted = 0;
									}
									else{
										level_highlighted += 1;
									}
								}
							}
						}
						else{
							if (player.shifting) {
								player.vel.x = 2.0f;
								player.face_right = true;
							} else {
								player.vel.x = 1.0f;
								player.face_right = true;
							}
							if (on_ladder){
								on_ladder = false;
							}
						}

					} else {
						if (player.vel.x == 1.0f || player.vel.x == 2.0f) {
							player.vel.x = 0.0f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_RIGHT) {
					if (evt.key.state == SDL_PRESSED) {
						if (in_menu){
							if (in_level_select){
								if (!back_button_highlighted){
									if (level_highlighted == num_unlocked){
										level_highlighted = 0;
									}
									else{
										level_highlighted += 1;
									}
								}
							}
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_s) {
					if (in_menu && evt.key.state == SDL_PRESSED){
						if (!in_level_select){
							play_highlighted = !play_highlighted;
						}else{
							back_button_highlighted = !back_button_highlighted;
						}
					}

					if (!in_menu && !on_ladder && !player.aiming && evt.key.state == SDL_PRESSED) {
						//climb onto the ladder
						for (Ladder& ladder : Vector_Ladders){
							ladder.detect_collision(player.pos, player.size);
							on_ladder = on_ladder || ladder.player_collision;
						}

						if (on_ladder){
							/****** add "player climbing" sprite *******/
						}
					}

					if (!in_menu && evt.key.state == SDL_PRESSED) {
						if (on_ladder){
							check_on_ladder = false;

							//check if player will remain on ladder
							for (Ladder& ladder : Vector_Ladders){
								ladder.detect_collision(glm::vec2(player.pos.x, player.pos.y - 0.02f), player.size);
								check_on_ladder = check_on_ladder || ladder.player_collision;
							}

							if (check_on_ladder){
								//climb the actual ladder
								player.pos.y -= 0.1f;
							}
						}
					} else {
						if (player.vel.x == 1.0f || player.vel.x == 2.0f) {
							player.vel.x = 0.0f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_DOWN) {
					if (in_menu && evt.key.state == SDL_PRESSED){
						if (!in_level_select){
							play_highlighted = !play_highlighted;
						}else{
							back_button_highlighted = !back_button_highlighted;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_LSHIFT) {
					if (evt.key.state == SDL_PRESSED) {
						if (player.vel.x == 1.0f) {
							player.face_right = true;
							player.vel.x = 2.0f;
						} else if (player.vel.x == -1.0f) {
							player.face_right = false;
							player.vel.x = -2.0f;
						}
						player.shifting = true;
					} else {
						if (player.vel.x == 2.0f) {
							player.face_right = true;
							player.vel.x = 1.0f;
						} else if (player.vel.x == -2.0f) {
							player.face_right = false;
							player.vel.x = -1.0f;
						}
						player.shifting = false;
					}
				} 
				else if (evt.key.keysym.sym == SDLK_SPACE) {
					// handle space press
					// check interractable state
					if (evt.type == SDL_KEYDOWN && evt.key.repeat == 0) {
						for (Door& door : Vector_Doors){
							if (player.pos.x + player.size.x / 2 < door.pos.x + door.size.x / 2
									&& player.pos.x - player.size.x / 2 > door.pos.x - door.size.x / 2
									&& player.pos.y + player.size.y / 2 < door.pos.y + door.size.y / 2) {
								SDL_PauseAudioDevice(doorDevice, 0);
								player.behind_door = !player.behind_door;
							}
						}
					}	
				}
			} 

			else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}

		if (player.vel.x != 0.0f || player.vel.y != 0.0f) {
			player.walking = true;
		} else {
			player.walking = false;
		}

		//Audio stuff

		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		static auto previous_time = current_time;
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		auto check_visibility = [&player](Light &light) {
			//will have this loop through all lights
			if (!light.light_on)
				return false;

			glm::vec2 A = light.vectors[0];
			glm::vec2 B = light.vectors[1];
			glm::vec2 C = light.vectors[2];

			glm::vec2 v0 = C - A;
			glm::vec2 v1 = B - A;
			glm::vec2 v2 = player.pos - A;

			// Compute dot products
			float dot00 = dot(v0, v0);
			float dot01 = dot(v0, v1);
			float dot02 = dot(v0, v2);
			float dot11 = dot(v1, v1);
			float dot12 = dot(v1, v2);

			// Compute barycentric coordinates
			float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
			float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			// Check if point is in triangle
			//return (u >= 0) && (v >= 0) && (u + v < 1)
			if ((u >= 0.0f) && (v >= 0.0f) && (u + v < 1.0f))
				return true;
			return false;
		};

		if (!player.aiming) { //update game state:
			bool isVisible = false;
			//check if player is in light
			for (Light& light : Vector_Lights) {
				light.rotate();
				isVisible = isVisible || check_visibility(light);
			}
			for (Enemy& enemy : Vector_Enemies) {
				enemy.flashlight.rotate();
				isVisible = isVisible || check_visibility(enemy.flashlight);
			}
			if ((!player.behind_door) && (isVisible)) {
				player.visible = true;
			}

			// player update -----------------------------------------------------------------
			if (player.behind_door == false) {
				if (player.jumping && !on_ladder) {
					player.vel.y -= elapsed * 9.0f;
				}

				player.pos += player.vel * elapsed;

				if (player.pos.x < floor_height) {
					player.pos.x = floor_height;
				} else if (player.pos.x > level_end) {
					player.pos.x = level_end;
				}
			}

			on_platform = false;

			//set up every other platform
			for (Platform& platform : Vector_Platforms){
				platform.detect_collision(player.pos, player.size);
				on_platform = on_platform || platform.player_collision;
				if (platform.player_collision && !on_ladder){
					player.pos.y = platform.pos.y + platform.size.y/2.0f + player.size.y/2.0f;
				}
			}
			if (on_platform && (player.vel.y <= 0.0f)) {
				player.jumping = false;
				player.vel.y = 0.0f;
				//player_pos.y = pos.y + size.y/2.0f + player_size.y/2.0f;
			}

			if (on_ladder){
				player.vel.y = 0.0f;
			}

			if (!on_platform){
				player.jumping = true;
			}


			//camera update ---------------------------------------------------------------
			camera.pos.x += player.vel.x * elapsed;
			if (player.pos.x < 6.0f) {
				camera.pos.x = 6.0f;
			} else if (player.pos.x > level_end - 6.0f) {
				camera.pos.x = level_end - 6.0f;
			}

			//have the camera vertically follow the player (for level 1)
			//camera.pos.y = 2.5f + (player.pos.y - 1.0f);

			//enemy update --------------------------------------------------------------
			int counter = 0;
			for (Enemy& enemies : Vector_Enemies) {
				counter += 1;
				if (!enemies.alerted) {
					if (!enemies.walking) {
						enemies.remaining_wait -= elapsed;
						if (enemies.remaining_wait <= 0.0f) {
							enemies.walking = true;
							enemies.face_right = !enemies.face_right;
							enemies.curr_index = (enemies.curr_index + 1) % 2;
							if (enemies.face_right) {
								enemies.vel.x = 1.0f;
							} else {
								enemies.vel.x = -1.0f;
							}
						}
					} else {
						enemies.pos += enemies.vel * elapsed;
						if ((enemies.face_right && enemies.pos.x > enemies.waypoints[enemies.curr_index].x) ||
								(!enemies.face_right && enemies.pos.x < enemies.waypoints[enemies.curr_index].x)) {
							enemies.face_right = enemies.waypoints[enemies.curr_index].x > 
								enemies.waypoints[(enemies.curr_index + 1) % 2].x;
							enemies.pos = enemies.waypoints[enemies.curr_index];
							enemies.remaining_wait = enemies.wait_timers[enemies.curr_index];
							enemies.walking = false;
							enemies.vel.x = 0.0f;
							enemies.update_pos();
						}
					}
				} else {
					if (!enemies.walking) {
						enemies.remaining_wait -= elapsed;
						if (enemies.remaining_wait <= 0.0f) {
							enemies.alerted = false;
							enemies.walking = true;
							enemies.face_right = (enemies.waypoints[enemies.curr_index].x > enemies.pos.x);
							if (enemies.face_right) {
								enemies.vel.x = 1.0f;
							} else {
								enemies.vel.x = -1.0f;
							}
						}
					} else {
						enemies.pos += enemies.vel * elapsed;
						if ((enemies.face_right && enemies.pos.x > enemies.target.x) ||
								(!enemies.face_right && enemies.pos.x < enemies.target.x)) {
							enemies.pos.x = enemies.target.x;
							enemies.remaining_wait = 10.0f;
							enemies.walking = false;
							enemies.vel.x = 0.0f;
						}
					}
				}

				if (player.visible && !player.behind_door) {
					if (enemies.face_right) {
						if (enemies.pos.x <= player.pos.x && enemies.pos.x + enemies.sight_range >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
							SDL_PauseAudioDevice(alertAudioDevice, 0);
							enemies.target = player.pos;
							enemies.vel.x = 2.5f;
							enemies.alerted = true;
							enemies.walking = true;
						}
					} else {
						if (enemies.pos.x - enemies.sight_range <= player.pos.x && enemies.pos.x >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
							SDL_PauseAudioDevice(alertAudioDevice, 0);
							enemies.target = player.pos;
							enemies.vel.x = -2.5f;
							enemies.alerted = true;
							enemies.walking = true;
						}
					}
				}

				if (!player.behind_door) {

					if (enemies.face_right) {
						if (enemies.pos.x <= player.pos.x && enemies.pos.x + enemies.catch_range >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
							// should_quit = true;

							//player was caught restart the level
							player.pos = default_player_pos;
							player.vel = default_player_vel;

							on_platform = false;
							on_ladder = false;
							check_on_ladder = false;

							player.face_right = false;
							player.jumping = false;
							player.shifting = false;
							player.behind_door = false;
							player.aiming = false;
							player.visible = false; 

							player.num_projectiles = 5;

							Vector_Platforms = {};
							Vector_Doors = {};
							Vector_Lights = {};
							Vector_Enemies = {};
							Vector_Ladders = {};

							loadLevel(level,
									reinterpret_cast<void*>(&Vector_Platforms),
									reinterpret_cast<void*>(&Vector_Doors),
									reinterpret_cast<void*>(&Vector_Lights),
									reinterpret_cast<void*>(&Vector_Enemies),
									reinterpret_cast<void*>(&Vector_Ladders));
						}
					} else {
						if (enemies.pos.x - enemies.catch_range <= player.pos.x && enemies.pos.x >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
							// should_quit = true;

							printf("made it to checkpoint 1\n");
							caught = true;

							//player was caught restart the level
							player.pos = default_player_pos;
							player.vel = default_player_vel;

							on_platform = false;
							on_ladder = false;
							check_on_ladder = false;

							player.face_right = false;
							player.jumping = false;
							player.shifting = false;
							player.behind_door = false;
							player.aiming = false;
							player.visible = false; 

							player.num_projectiles = 5;

							Vector_Platforms = {};
							Vector_Doors = {};
							Vector_Lights = {};
							Vector_Enemies = {};
							Vector_Ladders = {};

							printf("made it to checkpoint 2\n");

							loadLevel(level,
									reinterpret_cast<void*>(&Vector_Platforms),
									reinterpret_cast<void*>(&Vector_Doors),
									reinterpret_cast<void*>(&Vector_Lights),
									reinterpret_cast<void*>(&Vector_Enemies),
									reinterpret_cast<void*>(&Vector_Ladders));

							printf("made it to checkpoint 3\n");
						}
					}
				}
			}

			//detect footsteps
			for (Enemy& enemy : Vector_Enemies) {
				float h_diff = enemy.pos.x - player.pos.x;
				float v_diff = (enemy.pos.y + 0.35f * enemy.size.y) - (player.pos.y - 0.5f * player.size.y);
				float sound = 0.0f;
				if ((player.vel.x == 1.0f || player.vel.x == -1.0f) && !player.jumping && !player.behind_door) {
					sound = 0.5f * player.walk_sound;
					SDL_PauseAudioDevice(stepDevice, 0);
				} else if ((player.vel.x == 2.0f || player.vel.x == -2.0f) && !player.jumping && !player.behind_door) {
					sound = 0.5f * player.run_sound;
					SDL_PauseAudioDevice(stepDevice, 0);
				}

				if (sqrt(h_diff * h_diff + v_diff * v_diff) <= sound) {
					if (player.pos.x > enemy.pos.x) {
						enemy.target = (player.pos + enemy.pos)/2.0f;
						enemy.vel.x = 2.5f;
						enemy.alerted = true;
						enemy.walking = true;
						enemy.face_right = true;
					} else {
						enemy.target = (player.pos + enemy.pos)/2.0f;
						enemy.vel.x = -2.5f;
						enemy.alerted = true;
						enemy.walking = true;
						enemy.face_right = false;
					}
				}
			}

			// projectile update ----------------------------------------------------------------
			if (mouse.remaining_time == 1.0f) {
				for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end() ; ++i) {

					for (Enemy& enemy : Vector_Enemies) {
						//enemies
						float h_diff = enemy.pos.x - i->x;
						float v_diff = (enemy.pos.y + 0.35f * enemy.size.y) - i->y;
						if (sqrt(h_diff*h_diff + v_diff*v_diff) <= 0.5f * player.throw_sound) {
							if (i->x > enemy.pos.x) {
								enemy.target = *i;
								enemy.vel.x = 2.5f;
								enemy.alerted = true;
								enemy.walking = true;
								enemy.face_right = true;
							} else {
								enemy.target = *i;
								enemy.vel.x = -2.5f;
								enemy.alerted = true;
								enemy.walking = true;
								enemy.face_right = false;
							}
						}
					}

					//lights
					for (Light& light : Vector_Lights) {
						float h_diff = light.pos.x - i->x;
						float v_diff = light.pos.y + 0.5f * light.size.y - i->y;
						if (sqrt(h_diff*h_diff + v_diff*v_diff) <= 1.5f) {
							light.light_on = false;
						}
					}
				}       
			}

			for (Enemy& enemy : Vector_Enemies) {
				if (enemy.vel.x > 0.0f) {
					enemy.flashlight.dir = 0.0f;
					enemy.update_pos();
					//rotate_light(enemy.flashlight);
				} else if (enemy.vel.x < 0.0f) {
					enemy.flashlight.dir = PI;
					enemy.update_pos();
					//rotate_light(enemy.flashlight);
				}
			}

			//level win -----------------------------------------------------------
			if (player.pos.x >= level_end) {
				/* go on to the next level (currently goes to level 1) */



				//record the completed level
				if (completed_levels < level) {
					completed_levels = level;
				}

				if (level < 4){
					unlocked[level + 1] = true;
					if (num_unlocked < 4){
						num_unlocked += 1;
					}
				}

				//reset player statuses
				player.pos = default_player_pos;
				player.vel = default_player_vel;

				on_platform = false;
				on_ladder = false;
				check_on_ladder = false;

				player.face_right = false;
				player.jumping = false;
				player.shifting = false;
				player.behind_door = false;
				player.aiming = false;
				player.visible = false; 

				player.num_projectiles = 5;

				level += 1;

				//if the player beats the fifth level, cycle around to the starting level
				level = level % 5;

				Vector_Platforms = {};
				Vector_Doors = {};
				Vector_Lights = {};
				Vector_Enemies = {};
				Vector_Ladders = {};

				loadLevel(level,
						reinterpret_cast<void*>(&Vector_Platforms),
						reinterpret_cast<void*>(&Vector_Doors),
						reinterpret_cast<void*>(&Vector_Lights),
						reinterpret_cast<void*>(&Vector_Enemies),
						reinterpret_cast<void*>(&Vector_Ladders));
				//should_quit = true;
			}

		}

		//Audio Stuff
		if (alertData.length == 0) {
			SDL_PauseAudioDevice(alertAudioDevice, 1);
			alertData.pos = alertData.init_pos;
			alertData.length = alertData.init_length;
		}
		if (doorData.length == 0) {
			SDL_PauseAudioDevice(doorDevice, 1);
			doorData.pos = doorData.init_pos;
			doorData.length = doorData.init_length;
		}
		if (ladderData.length == 0) {
			SDL_PauseAudioDevice(ladderDevice, 1);
			ladderData.pos = ladderData.init_pos;
			ladderData.length = ladderData.init_length;
		}
		if (ornData.length == 0) {
			SDL_PauseAudioDevice(ornDevice, 1);
			ornData.pos = ornData.init_pos;
			ornData.length = ornData.init_length;
		}
		if (stepData.length == 0) {
			SDL_PauseAudioDevice(stepDevice, 1);
			stepData.pos = stepData.init_pos;
			stepData.length = stepData.init_length;
		}

		if (caught == true){
			printf("made it to checkpoint 4\n");
		}


		//draw output:
		glClearColor(231.0f / 255.0f, 125.0f / 255.0f, 65.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		{ //draw game state:
			std::vector< Vertex > verts;
			std::vector< Vertex > tri_verts;


			//---- Functions ----
			auto draw_sprite = [&verts](SpriteInfo const &sprite, glm::vec2 const &at, glm::vec2 size, glm::u8vec4 tint = glm::u8vec4(0x34, 0x4c, 0x73, 0xff), float angle = 0.0f) {
				glm::vec2 min_uv = sprite.min_uv;
				glm::vec2 max_uv = sprite.max_uv;
				glm::vec2 right = glm::vec2(std::cos(angle), std::sin(angle));
				glm::vec2 up = glm::vec2(-right.y, right.x);

				verts.emplace_back(at + right * -size.x/2.0f + up * -size.y/2.0f, glm::vec2(min_uv.x, min_uv.y), tint);
				verts.emplace_back(verts.back());
				verts.emplace_back(at + right * -size.x/2.0f + up * size.y/2.0f, glm::vec2(min_uv.x, max_uv.y), tint);
				verts.emplace_back(at + right *  size.x/2.0f + up * -size.y/2.0f, glm::vec2(max_uv.x, min_uv.y), tint);
				verts.emplace_back(at + right *  size.x/2.0f + up *  size.y/2.0f, glm::vec2(max_uv.x, max_uv.y), tint);
				verts.emplace_back(verts.back());
			};

			//helper: add character to game
			auto draw_triangle = [&tri_verts](glm::vec2 vec1, glm::vec2 vec2, glm::vec2 vec3, glm::vec2 const &rad, glm::u8vec4 const &tint) {
				tri_verts.emplace_back(vec1, glm::vec2(0.0f, 0.0f), tint);
				tri_verts.emplace_back(tri_verts.back());
				tri_verts.emplace_back(vec2, glm::vec2(0.0f, 1.0f), tint);
				tri_verts.emplace_back(vec3, glm::vec2(1.0f, 0.0f), tint);
				// tri_verts.emplace_back(at + glm::vec2( rad.x, rad.y), glm::vec2(1.0f, 1.0f), tint);
				tri_verts.emplace_back(tri_verts.back());
			};

			//------------- Draw Objects -------------

			//draw doors ------------------------------------------------------------------
			for (Door& door : Vector_Doors){
				draw_sprite(door.sprite_empty, door.pos, door.size);
			}

			//draw ladders ------------------------------------------------------------------
			for (Ladder& ladder : Vector_Ladders){
				draw_sprite(ladder.sprite_empty, ladder.pos, ladder.size);
			}

			if (caught == true){
				printf("made it to checkpoint 5\n");
			}

			//draw player -----------------------------------------------------------
			glm::vec2 player_size = player.size;
			if (!player.face_right) {
				player_size.x *= -1.0f;
			}
			if (player.behind_door == false) {
				draw_sprite(player.sprite_animations[player.animation_count], player.pos, player_size);
				if (player.walking) {
					player.animation_count = (player.animation_count + player.animation_delay / 9) % 8;
					player.animation_delay = (player.animation_delay + 1) % 11;
				} 
				else {
					player.animation_count = 8;
				}
			}

			if (caught == true){
				printf("made it to checkpoint 6\n");
			}

			//draw enemies -----------------------------------------------------------
			for (Enemy& enemy : Vector_Enemies){
				glm::vec2 enemy_size = enemy.size;
				if (enemy.face_right) {
					enemy_size.x *= -1.0f;
				}

				if (caught == true){
					printf("made it to checkpoint 7\n");
				}

				if (enemy.walking) {
					draw_sprite(enemy.sprite_animations[enemy.animation_count], enemy.pos, enemy_size);
					enemy.animation_count = (enemy.animation_count + enemy.animation_delay / 10) % 4;
					enemy.animation_delay = (enemy.animation_delay + 1) % 11;
				} else {
					// standing
					// make sure any other animation lines are not left dangling if they are
					if (enemy.animation_count < 4) {
						enemy.animation_count = (enemy.animation_count + enemy.animation_delay / 10);
						enemy.animation_delay = (enemy.animation_delay + 1) % 11;
					} else {
						enemy.animation_count = 4;
					}
					draw_sprite(enemy.sprite_animations[enemy.animation_count], enemy.pos, enemy_size);
				}

				if (caught == true){
					printf("made it to checkpoint 8\n");
				}

				if (enemy.alerted) {
					glm::vec2 alert_pos = glm::vec2(enemy.pos.x, 
							enemy.pos.y + 0.51f*enemy.size.y + 0.51f*enemy.alert_size.y );
					draw_sprite(enemy.alert, alert_pos, enemy.alert_size);
				}
				//}

				if (caught == true){
					printf("made it to checkpoint 9\n");
				}

				//draw flashlights --------------------------------------------------------------
				if (enemy.flashlight.light_on) {
					draw_triangle(enemy.flashlight.vectors[0], enemy.flashlight.vectors[1], enemy.flashlight.vectors[2], 
							glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				}

				if (caught == true){
					printf("made it to checkpoint 10\n");
				}
			}

		//draw stage lights
		for (Light& light : Vector_Lights) {
			if (light.light_on) {
				//printf("drawing triangles: (%f,%f), (%f,%f)\n", light.pos.x, light.pos.y, light.size.x, light.size.y);
				draw_triangle(light.vectors[0], light.vectors[1], light.vectors[2], 
						glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
			}
		}

		if (caught == true){
			printf("made it to checkpoint 11\n");
		}

		//draw menu options --------------------------------------------------------
		//at the moment, we use triangles for buttons
		if (in_menu){
			//determine whether to be on the main select screen or the level select screen
			if (!in_level_select){
				//do an if statement to "highlight" the hovering option
				if (play_highlighted){
					//draw_triangle(glm::vec2(3.0f, 3.0f), glm::vec2(5.0f, 3.0f), glm::vec2(5.0f, 5.0f), 
					//		glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
					//draw_triangle(glm::vec2(3.0f, 2.0f), glm::vec2(4.0f, 2.0f), glm::vec2(3.0f, 1.0f), 
					//		glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
					draw_sprite(menus.menus_selected[0], glm::vec2(2.0f, 6.0f), glm::vec2(1.5f,2.5f));
					draw_sprite(menus.menus[1], glm::vec2(4.0f, 6.0f), glm::vec2(1.5f,2.5f));
				}
				else{
					//draw_triangle(glm::vec2(3.0f, 3.0f), glm::vec2(4.0f, 3.0f), glm::vec2(4.0f, 4.0f), 
					//		glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
					//draw_triangle(glm::vec2(3.0f, 2.0f), glm::vec2(5.0f, 2.0f), glm::vec2(3.0f, 0.0f), 
					//		glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				}
			}
			else{
				//display the levels (represented as triangles)
				for (int i = 0; i < 5; i++){
					float offset = (float)i;

					int alpha;

					if (unlocked[i]){
						alpha = 0x88;
					}else{
						alpha = 0x22;
					}

					float y_scale;

					if (i == level_highlighted && !back_button_highlighted){
						y_scale = 0.5f;
					}else{
						y_scale = 0.0f;
					}

					draw_triangle(glm::vec2(0.5f + 2.5f * offset, 3.0f), glm::vec2(1.5f + 2.5f * offset, 3.0f), glm::vec2(1.5f + 2.5f * offset, 4.0f + y_scale), 
							glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, alpha));
				}

				if (back_button_highlighted){
					draw_triangle(glm::vec2(5.5f, 1.0f), glm::vec2(6.5f, 1.0f), glm::vec2(6.5f, 2.0f + 0.5f), 
							glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				}else{
					draw_triangle(glm::vec2(5.5f, 1.0f), glm::vec2(6.5f, 1.0f), glm::vec2(6.5f, 2.0f), 
							glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				}
			}
		}

		//draw platforms -----------------------------------------------------------
		for (Platform& platform : Vector_Platforms){
			draw_sprite(platform.sprite, platform.pos, platform.size);
		}

		//draw sounds ---------------------------------------------------------------
		if (!player.aiming && mouse.remaining_time > 0.0f) {
			mouse.remaining_time -= elapsed;
			for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end(); ++i) {
				draw_sprite(mouse.sprite_throw, *i, glm::vec2(player.throw_sound * (1.0f - mouse.remaining_time)));
			}

			if (mouse.remaining_time <= 0.0f) {
				mouse.remaining_time = 0.0f;
				player.projectiles_pos.clear();
			}
		}

		if (player.aiming) {
			for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end(); ++i) {
				draw_sprite(mouse.sprite_throw, *i, glm::vec2(player.throw_sound));
			}

			bool light_aimed = false;
			for (Light& light : Vector_Lights) {
				float h_diff = light.pos.x - mouse.pos.x;
				float v_diff = light.pos.y + 0.5f * light.size.y - mouse.pos.y;
				if (sqrt(h_diff*h_diff + v_diff*v_diff) <= 1.5f) {
					light_aimed = true;
					player.aimed_pos = glm::vec2(light.pos.x, light.pos.y + 0.5f * light.size.y);
					float slope = (player.aimed_pos.y - player.pos.y) / (player.aimed_pos.x - player.pos.x);
					for (float x = player.pos.x; x > player.aimed_pos.x; x -= 0.3f) {
						draw_sprite(mouse.sprite_throw, glm::vec2(x, (x - player.pos.x) * slope + player.pos.y), 
								glm::vec2(0.03f * player.throw_sound));
					}
					for (float x = player.pos.x; x < player.aimed_pos.x; x += 0.3f) {
						draw_sprite(mouse.sprite_throw, glm::vec2(x, (x - player.pos.x) * slope + player.pos.y), 
								glm::vec2(0.03f * player.throw_sound));
					}
					draw_sprite(mouse.sprite_throw, glm::vec2(player.aimed_pos.x, player.aimed_pos.y), glm::vec2(player.throw_sound));
					break;
				}
			}

			if (!light_aimed) {

				float max_y = 0.5f;
				for (Platform& platform : Vector_Platforms) {
					if (platform.pos.y + 0.5f * platform.size.y > max_y &&
							platform.pos.x - 0.5f * platform.size.x <= mouse.pos.x &&
							platform.pos.x + 0.5f * platform.size.x >= mouse.pos.x &&
							platform.pos.y + 0.5f * platform.size.y <= mouse.pos.y) { 
						max_y = platform.pos.y + 0.5f * platform.size.y;
					}
				}
				player.aimed_pos = glm::vec2(mouse.pos.x, max_y);

				float y1 = player.pos.y;
				float y2 = player.aimed_pos.y + 2.0f;
				float y3 = player.aimed_pos.y;
				float x1 = player.pos.x;
				float x2 = 0.5f*(player.aimed_pos.x + player.pos.x);
				float x3 = player.aimed_pos.x;
				//from https://stackoverflow.com/questions/16896577/using-points-to-generate-quadratic-equation-to-interpolate-data
				float a = y1/((x1-x2)*(x1-x3)) 
					+ y2/((x2-x1)*(x2-x3)) 
					+ y3/((x3-x1)*(x3-x2));
				float b = -y1*(x2+x3)/((x1-x2)*(x1-x3))
					- y2*(x1+x3)/((x2-x1)*(x2-x3))
					- y3*(x1+x2)/((x3-x1)*(x3-x2));
				float c = y1*x2*x3/((x1-x2)*(x1-x3))
					+ y2*x1*x3/((x2-x1)*(x2-x3))
					+ y3*x1*x2/((x3-x1)*(x3-x2));
				for (float x = player.pos.x; x > player.aimed_pos.x; x -= 0.3f) {
					draw_sprite(mouse.sprite_throw, glm::vec2(x, a*x*x + b*x + c), glm::vec2(0.03f * player.throw_sound));
				}
				for (float x = player.pos.x; x < player.aimed_pos.x; x += 0.3f) {
					draw_sprite(mouse.sprite_throw, glm::vec2(x, a*x*x + b*x + c), glm::vec2(0.03f * player.throw_sound));
				}
				draw_sprite(mouse.sprite_throw, glm::vec2(player.aimed_pos.x, player.aimed_pos.y), glm::vec2(player.throw_sound));
			}
		}

		player.sound_time -= elapsed;
		if (player.sound_time < 0.0f) {
			player.sound_time = 1.0f;
		} else if (player.sound_time < 1.0f) {
			float sound = 0.0f;
			if ((player.vel.x == 1.0f || player.vel.x == -1.0f) && !player.jumping && !player.behind_door) {
				sound = player.walk_sound;
			} else if ((player.vel.x == 2.0f || player.vel.x == -2.0f) && !player.jumping&& !player.behind_door) {
				sound = player.run_sound;
			}
			draw_sprite(mouse.sprite_throw, glm::vec2(player.pos.x, player.pos.y - 0.5 * player.size.y), glm::vec2(sound * (1.0f - player.sound_time)));
		}

		//-----------------------------------------------------------------------

		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * verts.size(), &verts[0], GL_STREAM_DRAW);

		glUseProgram(program);
		glUniform1i(program_tex, 0);
		glm::vec2 scale = 2.0f / camera.size;
		glm::vec2 offset = scale * -camera.pos;
		glm::mat4 mvp = glm::mat4(
				glm::vec4(scale.x, 0.0f, 0.0f, 0.0f),
				glm::vec4(0.0f, scale.y, 0.0f, 0.0f),
				glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
				glm::vec4(offset.x, offset.y, 0.0f, 1.0f)
				);
		glUniformMatrix4fv(program_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(vao);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, verts.size());
		//glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// glDisable(GL_TEXTURE_2D);

		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * tri_verts.size(), &tri_verts[0], GL_STREAM_DRAW);

		glBindTexture(GL_TEXTURE_2D, tex2);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, tri_verts.size());
	}

	SDL_GL_SwapWindow(window);
}


//------------  teardown ------------

//Close the audio devices
SDL_CloseAudioDevice(audioDevice);
SDL_CloseAudioDevice(alertAudioDevice);
SDL_FreeWAV(wavStart);

SDL_GL_DeleteContext(context);
context = 0;

SDL_DestroyWindow(window);
window = NULL;

SDL_Quit();

return 0;
}


static GLuint compile_shader(GLenum type, std::string const &source) {
	GLuint shader = glCreateShader(type);
	GLchar const *str = source.c_str();
	GLint length = source.size();
	glShaderSource(shader, 1, &str, &length);
	glCompileShader(shader);
	GLint compile_status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		std::cerr << "Failed to compile shader." << std::endl;
		GLint info_log_length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector< GLchar > info_log(info_log_length, 0);
		GLsizei length = 0;
		glGetShaderInfoLog(shader, info_log.size(), &length, &info_log[0]);
		std::cerr << "Info log: " << std::string(info_log.begin(), info_log.begin() + length);
		glDeleteShader(shader);
		throw std::runtime_error("Failed to compile shader.");
	}
	return shader;
}

static GLuint link_program(GLuint fragment_shader, GLuint vertex_shader) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	GLint link_status = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		std::cerr << "Failed to link shader program." << std::endl;
		GLint info_log_length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
		std::vector< GLchar > info_log(info_log_length, 0);
		GLsizei length = 0;
		glGetProgramInfoLog(program, info_log.size(), &length, &info_log[0]);
		std::cerr << "Info log: " << std::string(info_log.begin(), info_log.begin() + length);
		throw std::runtime_error("Failed to link program");
	}
	return program;
}

static void playTone(void *userData, Uint8 *stream, int streamLength) {

	// change the user data passed by SDL into our User defined AudioData format
	AudioData *audioData = (AudioData *)userData;

	if (audioData ->length == 0) {
		//Set the init length and pos
		audioData->pos = audioData->init_pos;
		audioData->length = audioData->init_length;
	}

	Uint32 length = (Uint32) streamLength;
	length = length > audioData->length ? audioData->length : length;

	SDL_memcpy(stream, audioData->pos, length);

	audioData->pos += length;
	audioData->length -= length;
}
