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

const float PI = 3.1415f;
static GLuint compile_shader(GLenum type, std::string const &source);
static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);

float dot(glm::vec2 a, glm::vec2 b) {
	return (a.x * b.x + a.y * b.y);
}

static const char *BG_MUSIC_PATH = 
"../sounds/Light_And_Shadow_Soundtrack.wav";

struct AudioData {
  Uint8 *pos;
  Uint32 length;
  Uint8 *init_pos;
  Uint32 init_length;
};

static void playTone(void *userdata, Uint8 *stream, int streamlength);

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game1: Text/Tiles";
		glm::uvec2 size = glm::uvec2(1200, 700);
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
  
  //SDL Audio
  SDL_AudioSpec wavSpec;
  Uint8 *wavStart;
  Uint32 wavLength;

  if (SDL_LoadWAV(BG_MUSIC_PATH, &wavSpec, &wavStart, &wavLength) == NULL) {
    std::cerr << "Failed to load back ground music" << std::endl;
    exit(1);
  }

  AudioData audioData;
  audioData.pos = wavStart;
  audioData.length = wavLength;

  audioData.init_pos = wavStart;
  audioData.init_length = wavLength;

  wavSpec.callback = playTone;
  wavSpec.userdata = &audioData;

  SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);

  if (audioDevice == 0) {
    std::cerr << "Failed to grab a device" << std::endl;
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	//----------------- Structs ----------------------------------------------
	struct {
		glm::vec2 pos = glm::vec2(6.0f, 2.5f);
		glm::vec2 size = glm::vec2(12.0f, 7.0f);
	} camera;
	//adjust for aspect ratio
	camera.size.x = camera.size.y * (float(config.size.x) / float(config.size.y));

	struct SpriteInfo {
		glm::vec2 min_uv;
		glm::vec2 max_uv;
	};

	struct {
		glm::vec2 pos = glm::vec2(0.0f);
		glm::vec2 size = glm::vec2(6.0f);
		
		SpriteInfo sprite_throw = {
			glm::vec2(1199.0f/3503.0f, 1625.0f/1689.0f),
			glm::vec2(1263.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_shoot = {
			glm::vec2(1199.0f/3503.0f, 1625.0f/1689.0f),
			glm::vec2(1263.0f/3503.0f, 1.0f),
		};

		float remaining_time = 0.0f;
	} mouse;
	
	struct {
		glm::vec2 pos = glm::vec2(0.25f, 1.0f);
		glm::vec2 size = glm::vec2(0.5f, 1.0f);
		glm::vec2 vel = glm::vec2(0.0f);
		
		SpriteInfo sprite_stand = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_walk = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_run = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_jump = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_throw = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_shoot = {
			glm::vec2(740.0f/3503.0f, 746.0f/1689.0f),
			glm::vec2(1199.0f/3503.0f, 1.0f),
		};
		
		// 0: throwing
		// 1: shooting
		int ability_mode = 0;

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
		
		std::vector<glm::vec2> projectiles_pos;
		int num_projectiles = 5;
	} player;

	struct Light {
		glm::vec2 pos = glm::vec2(0.0f, 0.0f);
		glm::vec2 size = glm::vec2(0.0f, 0.0f);
		float dir = PI * 1.5f;
		bool light_on = true;
		glm::u8vec4 color = glm::u8vec4(0xff, 0xff, 0xff, 0xff);

		SpriteInfo sprite = {
			glm::vec2(1945.0f/3503.0f, 1289.0f/1689.0f),
			glm::vec2(2585.0f/3503.0f, 1.0f),
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
		glm::vec2 right_flashlight_offset = glm::vec2(0.05f, 0.0f);
		glm::vec2 left_flashlight_offset = glm::vec2(-1.4f, 0.0f);

		bool face_right = true;
		bool alerted = false;
		bool walking = false;

		float wait_timers [2] = { 5.0f, 5.0f };
		float remaining_wait = 5.0f;
		float sight_range = 4.0f;
		float catch_range = 0.5f;
		int curr_index  = 0;

		Light flashlight;

		// Light() {
		// 	flashlight.pos = glm::vec2(0.0f, 0.0f);
		// }

		void update_pos() {
			if (face_right) {
				flashlight.pos = pos + glm::vec2(1.0f, 0.0f);
			}
			else {
				flashlight.pos = pos - glm::vec2(1.4f, 0.0f);
			}

		}
		SpriteInfo sprite_stand = {
			glm::vec2(1263.0f/3503.0f, 741.0f/1689.0f),
			glm::vec2(1776.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_walk = {
			glm::vec2(1263.0f/3503.0f, 741.0f/1689.0f),
			glm::vec2(1776.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_alert = {
			glm::vec2(1776.0f/3503.0f, 1381.0f/1689.0f),
			glm::vec2(1945.0f/3503.0f, 1.0f),
		};

	};

	struct Door {
		glm::vec2 pos = glm::vec2(0.0f);
		glm::vec2 size = glm::vec2(1.0f, 1.5f);
		bool in_use = false;

		SpriteInfo sprite_empty = {
			glm::vec2(0.0f, 481.0f/1689.0f),
			glm::vec2(740.0f/3503.0f, 1.0f),
		};
		SpriteInfo sprite_used = {
			glm::vec2(0.0f, 481.0f/1689.0f),
			glm::vec2(740.0f/3503.0f, 1.0f),
		};
	};

	struct Platform {
		glm::vec2 pos = glm::vec2(10.0f, 0.25f);
		glm::vec2 size = glm::vec2(20.0f, 0.5f);

		SpriteInfo sprite = {
			glm::vec2(0.0f),
			glm::vec2(1.0f, 481.0f/1689.0f),
		};
	};

	struct Air_Platform {
		glm::vec2 pos = glm::vec2(10.0f, 1.4f);
		glm::vec2 size = glm::vec2(5.0f, 0.5f);

		SpriteInfo sprite = {
			glm::vec2(0.0f),
			glm::vec2(1.0f, 481.0f/1689.0f),
		};
	};


	//------------ Initialization ---------------------------------------------


	//---- Level Loading ----
	/* Levels will be imported from outside code / txt file. Levels will define
	 * object initialization, as well as the variables attached to each object.
	 */

	//---- Instantiate Obects ----
	//delete this once import code is written
	Light ceilingLight;
	Light l0;
	Light l1;
	Light l2;
	Light l3;

	Platform floor_plat_1;
	Platform floor_plat_2;
	Platform floor_plat_3;
	Platform floor_plat_4;

	Enemy enemy_1;
	Enemy enemy_2;
	Enemy enemy_3;



	//uncomment for original level
	//Platform platform;
	//Enemy enemy;
	//Door door;
	std::vector< Platform > Vector_Platforms = {floor_plat_1, floor_plat_2, floor_plat_3, floor_plat_4};
	// std::vector< Door > Vector_Doors;
	std::vector< Light > Vector_Lights = {ceilingLight, l0, l1, l2, l3 };
	std::vector< Enemy > Vector_Enemies = {enemy_1, enemy_2, enemy_3};

	//must declare const since variable sized arrays are not allowed in c++

	const int num_platforms = 4;
	const int num_enemies = 2;
	const int num_doors = 1;
	const float ceiling_height = 10.0f;
	const float floor_height = 0.25f;	

	bool on_platform = false;

	//for level 1:
	Platform platforms[num_platforms];
	Enemy enemies[num_enemies];
	Door door[num_doors];

	//---- Set Object Variables ---

	//Platforms
	//first floor
	Vector_Platforms[0].pos = glm::vec2(10.0f, floor_height);
	Vector_Platforms[1].pos = glm::vec2(11.0f, 2.0f);
	Vector_Platforms[2].pos = glm::vec2(15.0f, 2.0f);
	Vector_Platforms[3].pos = glm::vec2(18.5f, 2.0f);

	Vector_Platforms[0].size = glm::vec2(20.0f, 0.5f);
	Vector_Platforms[1].size = glm::vec2(3.0f, 0.25f);
	Vector_Platforms[2].size = glm::vec2(3.0f, 0.25f);
	Vector_Platforms[3].size = glm::vec2(1.0f, 0.25f);

	//Enemies

	/**************** comment for easier debugging ********************/
	Vector_Enemies[0].pos = glm::vec2(10.0f, 1.0f);
	Vector_Enemies[1].pos = glm::vec2(19.0f, 1.0f);

	Vector_Enemies[0].waypoints[0] = glm::vec2(10.0f, 1.0f);
	Vector_Enemies[0].waypoints[1] = glm::vec2(4.0f, 1.0f);
	Vector_Enemies[1].waypoints[0] = glm::vec2(17.0f, 1.0f);
	Vector_Enemies[1].waypoints[1] = glm::vec2(15.0f, 1.0f);
	/**************** comment for easier debugging ********************/

	/**************** for debugging **********************************/
	
	//commented code is to debug
	/*for (int i = 0; i < num_enemies; i++){
		enemies[i].pos = glm::vec2(10.0f, 1.0f);
		enemies[i].waypoints[0] = glm::vec2(10.0f, 1.0f);
		enemies[i].waypoints[1] = glm::vec2(4.0f, 1.0f);
	}*/

	/**************** for debugging **********************************/

	//Light
	// ceilingLight.pos = glm::vec2(10.0f, 3.0f);
	// ceilingLight.size = glm::vec2(4.0f, 10.0f);
	// ceilingLight.dir = PI * 1.5f;

	enemies[0].flashlight.size = glm::vec2(2.0f, 4.0f);
	enemies[0].flashlight.pos = enemies[0].pos + glm::vec2(enemies[0].flashlight.size.y - 0.35f, 0.0f);
	enemies[0].flashlight.dir = 0.0f;
	//rotate_light(enemies[0].flashlight);
	
  	enemies[1].flashlight.size = glm::vec2(2.0f, 4.0f);
	enemies[1].flashlight.pos = enemies[1].pos + glm::vec2(enemies[1].flashlight.size.y - 0.35f, 0.0f);
	enemies[1].flashlight.dir = 0.0f;
	//rotate_light(enemies[1].flashlight);

	Vector_Lights[0].pos = glm::vec2(2.0f, 3.0f);
	Vector_Lights[0].size = glm::vec2(1.5f, ceiling_height);
	Vector_Lights[0].dir = PI * 1.5f;

	Vector_Lights[1].pos = glm::vec2(5.0f, 3.0f);
	Vector_Lights[1].size = glm::vec2(3.0f, ceiling_height);
	Vector_Lights[1].dir = PI * 1.5f;

	Vector_Lights[2].pos = glm::vec2(8.5f, 3.0f);
	Vector_Lights[2].size = glm::vec2(1.5f, ceiling_height);
	Vector_Lights[2].dir = PI * 1.5f;

	Vector_Lights[3].pos = glm::vec2(15.0f, 5.125f);
	Vector_Lights[3].size = glm::vec2(2.0f, 6.0f);
	Vector_Lights[3].dir = PI * 1.5f;

	for (Light& light : Vector_Lights) {
		light.rotate();
	}

	//Door
	door[0].pos = glm::vec2(5.0f, 1.25f);

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
						if (player.ability_mode == 0) {
							player.projectiles_pos.push_back(glm::vec2(mouse.pos.x, 0.5f));
						} else if (player.ability_mode == 1) {
							player.projectiles_pos.push_back(glm::vec2(mouse.pos.x, 7.0f));
						}
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
					if (!player.jumping && !player.behind_door && !player.aiming && evt.key.state == SDL_PRESSED) {
						player.jumping = true;
						player.vel.y = 6.0f;
					}
				} 
				else if (evt.key.keysym.sym == SDLK_a) {
					if (evt.key.state == SDL_PRESSED) {
						if (player.shifting) {
							player.vel.x = -2.0f;
							player.face_right = false;
						} else {
							player.vel.x = -1.0f;
							player.face_right = false;
						}
					} else {
						if (player.vel.x == -1.0f || player.vel.x == -2.0f) {
							player.vel.x = 0.0f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_d) {
					if (evt.key.state == SDL_PRESSED) {
						if (player.shifting) {
							player.vel.x = 2.0f;
							player.face_right = true;
						} else {
							player.vel.x = 1.0f;
							player.face_right = true;
						}
					} else {
						if (player.vel.x == 1.0f || player.vel.x == 2.0f) {
							player.vel.x = 0.0f;
						}
					}
				} 
				else if (evt.key.keysym.sym == SDLK_q) {
					if (evt.key.state == SDL_PRESSED) {
						player.ability_mode = 0;
					}
				} 
				else if (evt.key.keysym.sym == SDLK_e) {
					if (evt.key.state == SDL_PRESSED) {
						player.ability_mode = 1;
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
						for (int i = 0; i < num_doors; i++){
							if (player.pos.x + player.size.x / 2 < door[i].pos.x + door[i].size.x / 2
									&& player.pos.x - player.size.x / 2 > door[i].pos.x - door[i].size.x / 2
									&& player.pos.y + player.size.y / 2 < door[i].pos.y + door[i].size.y / 2) {
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
			
			//check if player is in light

			if ((!player.behind_door) && (check_visibility(enemies[0].flashlight) || check_visibility(enemies[1].flashlight) || check_visibility(l0) || check_visibility(l1) || check_visibility(l2) || check_visibility(l3))) {
				player.visible = true;
			}

			// player update -----------------------------------------------------------------
			if (player.behind_door == false) {
				if (player.jumping) {
					player.vel.y -= elapsed * 9.0f;
				}

				player.pos += player.vel * elapsed;

				if (player.pos.x < 0.25f) {
					player.pos.x = 0.25f;
				} else if (player.pos.x > 29.75f) {
					player.pos.x = 29.75f;
				}
			}

			//uncomment for original level

			// if (player.pos.y < 1.0f) {
			// 	player.jumping = false;
			// 	player.pos.y = 1.0f;
			// 	player.vel.y = 0.0f;
			// }

			//set up ground floor first
			if (player.pos.y < platforms[0].pos.y + platforms[0].size.y/2.0f + player.size.y/2.0f) {
				player.jumping = false;
				player.pos.y = platforms[0].pos.y + platforms[0].size.y/2.0f + player.size.y/2.0f;
				player.vel.y = 0.0f;
			}

			on_platform = false;

			//set up every other platform
			for (int i = 1; i < num_platforms; i++){
				if ((player.pos.y >= platforms[i].pos.y + platforms[i].size.y/2.0f + player.size.y/2.0f - 0.1f) &&
					(player.pos.y <= platforms[i].pos.y + platforms[i].size.y/2.0f + player.size.y/2.0f + 0.05f) &&
					(player.pos.x <= platforms[i].pos.x + platforms[i].size.x/2.0f + player.size.x/2.0f) &&
					(player.pos.x >= platforms[i].pos.x - platforms[i].size.x/2.0f - player.size.x/2.0f) &&
					(player.vel.y < 0.0f)) {
					player.jumping = false;
					player.pos.y = platforms[i].pos.y + platforms[i].size.y/2.0f + player.size.y/2.0f;
					player.vel.y = 0.0f;

					on_platform = true;
				}
			}

			if (!on_platform && player.pos.y != 1.0f){
				player.jumping = true;
			}


			//camera update ---------------------------------------------------------------
			camera.pos.x += player.vel.x * elapsed;
			if (player.pos.x < 6.0f) {
				camera.pos.x = 6.0f;
			} else if (player.pos.x > 14.0f) {
				camera.pos.x = 14.0f;
			}

			//have the camera vertically follow the player (for level 1)
			//camera.pos.y = 2.5f + (player.pos.y - 1.0f);

			//enemy update --------------------------------------------------------------
			for (Enemy& enemies : Vector_Enemies) {
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
							enemies.target = player.pos;
							enemies.vel.x = 2.5f;
							enemies.alerted = true;
							enemies.walking = true;
						}
					} else {
						if (enemies.pos.x - enemies.sight_range <= player.pos.x && enemies.pos.x >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
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
							should_quit = true;
						}
					} else {
						if (enemies.pos.x - enemies.catch_range <= player.pos.x && enemies.pos.x >= player.pos.x && (abs(enemies.pos.y - player.pos.y) <= 0.5f)) {
							should_quit = true;
						}
					}
				}

			//detect footsteps
			for (int i = 0; i < num_enemies; i++){
<<<<<<< HEAD
				float h_diff = enemies.pos.x - player.pos.x;
				float v_diff = enemies.pos.y - (player.pos.y - 0.5f * player.size.y);
=======
				float h_diff = enemies[i].pos.x - player.pos.x;
				float v_diff = (enemies[i].pos.y + 0.35f * enemies[i].size.y) - (player.pos.y - 0.5f * player.size.y);
>>>>>>> 6219c84498ff75eab82048478fabe69662850f2a
				float sound = 0.0f;
				if ((player.vel.x == 1.0f || player.vel.x == -1.0f) && !player.jumping && !player.behind_door) {
					sound = 0.5f * player.walk_sound;
				} else if ((player.vel.x == 2.0f || player.vel.x == -2.0f) && !player.jumping && !player.behind_door) {
					sound = 0.5f * player.run_sound;
				}

				if (sqrt(h_diff * h_diff + v_diff * v_diff) <= sound) {
					if (player.pos.x > enemies.pos.x) {
						enemies.target = (player.pos + enemies.pos)/2.0f;
						enemies.vel.x = 2.5f;
						enemies.alerted = true;
						enemies.walking = true;
						enemies.face_right = true;
					} else {
						enemies.target = (player.pos + enemies.pos)/2.0f;
						enemies.vel.x = -2.5f;
						enemies.alerted = true;
						enemies.walking = true;
						enemies.face_right = false;
					}
				}
			}

			// projectile update ----------------------------------------------------------------
			if (mouse.remaining_time == 1.0f) {
				for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end() ; ++i) {
					
					for (Enemy& enemy : Vector_Enemies) {
						//enemies
<<<<<<< HEAD
						float h_diff = enemy.pos.x - i->x;
						float v_diff = enemy.pos.y - i->y;
=======
						float h_diff = enemies[j].pos.x - i->x;
						float v_diff = (enemies[j].pos.y + 0.35f * enemies[j].size.y) - i->y;
>>>>>>> 6219c84498ff75eab82048478fabe69662850f2a
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

					// float h_diff_l = ceilingLight.pos.x - i->x;
					// float v_diff_l = (ceilingLight.pos.y + 0.5f * ceilingLight.size.y) - i->y;
					// if (sqrt(h_diff_l*h_diff_l + v_diff_l*v_diff_l) <= 2.0f) {
					// 	ceilingLight.light_on = false;
					// }
				}       
			}

			for (Enemy& enemy : Vector_Enemies) {
				if (enemy.vel.x > 0.0f) {
			        enemy.flashlight.dir = 0.0f;
					enemy.flashlight.pos = enemy.pos + glm::vec2(enemy.flashlight.size.y - 0.35f, 0.0f);
					//rotate_light(enemy.flashlight);
				} else if (enemy.vel.x < 0.0f) {
				    enemy.flashlight.dir = PI;
				    enemy.flashlight.pos = enemy.pos - glm::vec2(enemy.flashlight.size.y + 0.60f, 0.0f);
				    //rotate_light(enemy.flashlight);
				}
			}
		}

			//level win -----------------------------------------------------------
			if (player.pos.x >= 20.0f) {
				should_quit = true;
			}

		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		{ //draw game state:
			std::vector< Vertex > verts;
			std::vector< Vertex > tri_verts;


			//---- Functions ----
			auto draw_sprite = [&verts](SpriteInfo const &sprite, glm::vec2 const &at, glm::vec2 size, glm::u8vec4 tint = glm::u8vec4(0xff, 0xff, 0xff, 0xff), float angle = 0.0f) {
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
			for (int i = 0; i < num_doors; i++){
				draw_sprite(door[i].sprite_empty, door[i].pos, door[i].size);
			}

			//draw player -----------------------------------------------------------
			glm::vec2 player_size = player.size;
			if (!player.face_right) {
				player_size.x *= -1.0f;
			}
			if (player.behind_door == false) {
				draw_sprite(player.sprite_stand, player.pos, player_size);
			}
			
			//draw enemies -----------------------------------------------------------
			for (Enemy& enemy : Vector_Enemies){
				if (enemy.face_right) {
					enemy.size.x *= -1.0f;
				}
				draw_sprite(enemy.sprite_stand, enemy.pos, enemy.size);     
				if (enemy.alerted) {
				glm::vec2 alert_pos = glm::vec2(enemy.pos.x, 
							enemy.pos.y + 0.51f*enemy.size.y + 0.51f*enemy.alert_size.y );
					draw_sprite(enemy.sprite_alert, alert_pos, enemy.alert_size);
				}
				//}

				//draw lights --------------------------------------------------------------
				if (enemy.flashlight.light_on) {
					draw_triangle(enemy.flashlight.vectors[0], enemy.flashlight.vectors[1], enemy.flashlight.vectors[2], 
						glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				}
			}

      		for (Light& light : Vector_Lights) {
      			if (light.light_on) {
      				draw_triangle(ceilingLight.vectors[0], ceilingLight.vectors[1], ceilingLight.vectors[2], 
					glm::vec2(1.0f), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
      			}
      		}

			//draw platforms -----------------------------------------------------------
			for (int i = 0; i < num_platforms; i++){
				draw_sprite(platforms[i].sprite, platforms[i].pos, platforms[i].size);
			}

			//draw sounds ---------------------------------------------------------------
			if (!player.aiming && mouse.remaining_time > 0.0f) {
				mouse.remaining_time -= elapsed;
				for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end(); ++i) {
					if (i->y == 0.5) {
						draw_sprite(mouse.sprite_throw, *i, glm::vec2(player.throw_sound * (1.0f - mouse.remaining_time)));
					} else {
						draw_sprite(mouse.sprite_shoot, *i, glm::vec2(player.throw_sound * (1.0f - mouse.remaining_time)));
					}
				}

				if (mouse.remaining_time <= 0.0f) {
					mouse.remaining_time = 0.0f;
					player.projectiles_pos.clear();
				}
			}

			if (player.aiming) {
				for (auto i = player.projectiles_pos.begin(); i != player.projectiles_pos.end(); ++i) {
					if (i->y == 0.5) {
						draw_sprite(mouse.sprite_throw, *i, glm::vec2(player.throw_sound));
					} else {
						draw_sprite(mouse.sprite_shoot, *i, glm::vec2(player.throw_sound));
					}
				}

				if (player.num_projectiles > 0) {
					if (player.ability_mode == 0) {
						float y1 = player.pos.y;
						float y2 = 5.0f;
						float y3 = 0.5f;
						float x1 = player.pos.x;
						float x2 = 0.5f*(mouse.pos.x + player.pos.x);
						float x3 = mouse.pos.x;
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
						for (float x = player.pos.x; x > mouse.pos.x; x -= 0.3f) {
							draw_sprite(mouse.sprite_throw, glm::vec2(x, a*x*x + b*x + c), glm::vec2(0.03f * player.throw_sound));
						}
						for (float x = player.pos.x; x < mouse.pos.x; x += 0.3f) {
							draw_sprite(mouse.sprite_throw, glm::vec2(x, a*x*x + b*x + c), glm::vec2(0.03f * player.throw_sound));
						}
						draw_sprite(mouse.sprite_throw, glm::vec2(mouse.pos.x, 0.5f), glm::vec2(player.throw_sound));
					} else {
						float slope = (7.0f - player.pos.y) / (mouse.pos.x - player.pos.x);
						for (float x = player.pos.x; x > mouse.pos.x; x -= 0.3f) {
							draw_sprite(mouse.sprite_throw, glm::vec2(x, (x - player.pos.x) * slope + player.pos.y), glm::vec2(0.03f * player.throw_sound));
						}
						for (float x = player.pos.x; x < mouse.pos.x; x += 0.3f) {
							draw_sprite(mouse.sprite_throw, glm::vec2(x, (x - player.pos.x) * slope + player.pos.y), glm::vec2(0.03f * player.throw_sound));
						}
						draw_sprite(mouse.sprite_throw, glm::vec2(mouse.pos.x, 7.0f), glm::vec2(player.throw_sound));
					}
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
