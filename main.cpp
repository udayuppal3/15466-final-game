// ADAPTED FROM JIM MCCANN'S BASE1 CODE FOR 15-466 COMPUTER GAME PROGRAMMING

#include "load_save_png.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>

const float PI = 3.1415f;
static GLuint compile_shader(GLenum type, std::string const &source);
static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader);

float dot(glm::vec2 a, glm::vec2 b) {
	return (a.x * b.x + a.y * b.y);
}

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game1: Text/Tiles";
		glm::uvec2 size = glm::uvec2(1200, 700);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

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
	SDL_ShowCursor(SDL_DISABLE);

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
  struct {
    glm::vec2 pos = glm::vec2(6.0f, 3.5f);
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

    int num_projectiles = 0;
  } player;

  struct Enemy {
    glm::vec2 pos = glm::vec2(10.0f, 1.0f);
    glm::vec2 vel = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.5, 1.0f);
    glm::vec2 alert_size = glm::vec2(0.2, 0.4);

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
 
    bool face_right = true;
    bool alerted = false;
    bool walking = false;

    glm::vec2 waypoints [2] = { glm::vec2(10.0f, 1.0f), glm::vec2(4.0f, 1.0f) };
    glm::vec2 target = glm::vec2(0.0f, 0.0f);
    float wait_timers [2] = { 5.0f, 5.0f };
    int curr_index  = 0;
    float remaining_wait = 5.0f;
    float sight_range = 4.0f;
    float hear_range = 3.0f;
    float catch_range = 0.5f;
  };

  struct Light {
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(1.0f, 3.0f);
    float dir = PI * 1.5f;
    float angle = PI * 0.25f;
    float range = 3.0f;

    SpriteInfo sprite = {
      glm::vec2(1945.0f/3503.0f, 1289.0f/1689.0f),
      glm::vec2(2585.0f/3503.0f, 1.0f),
    };

    bool light_on = true;
  };

  struct Door {
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(1.0f, 1.5f);

    SpriteInfo sprite_empty = {
      glm::vec2(0.0f, 481.0f/1689.0f),
      glm::vec2(740.0f/3503.0f, 1.0f),
    };
    SpriteInfo sprite_used = {
      glm::vec2(0.0f, 481.0f/1689.0f),
      glm::vec2(740.0f/3503.0f, 1.0f),
    };

    bool in_use = false;
  };

  struct Platform {
    glm::vec2 pos = glm::vec2(10.0f, 0.25f);
    glm::vec2 size = glm::vec2(20.0f, 0.5f);

    SpriteInfo sprite = {
      glm::vec2(0.0f),
      glm::vec2(1.0f, 481.0f/1689.0f),
    };
  };

  Light flashlight;
  Light ceilingLight;
  Platform platform;
  Enemy enemy;
  Door door;

  flashlight.dir = PI * 1.5f;
  ceilingLight.pos = glm::vec2(10.0f, 2.7f);
  ceilingLight.size = glm::vec2(3.0f, 10.3f);
  door.pos = glm::vec2(5.0f, 1.25f);

	//------------ game loop ------------

	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				mouse.pos.x = (evt.motion.x + 0.5f) / float(config.size.x) * 2.0f - 1.0f;
				mouse.pos.y = (evt.motion.y + 0.5f) / float(config.size.y) *-2.0f + 1.0f;
			} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
        			if (evt.key.keysym.sym == SDLK_w) {
				       	if (!player.jumping && !player.behind_door && evt.key.state == SDL_PRESSED) {
                  player.jumping = true;
					        player.vel.y = 6.0f;
          			}
        			} else if (evt.key.keysym.sym == SDLK_a) {
          				if (evt.key.state == SDL_PRESSED) {
            					if (player.shifting) {
              						player.vel.x = -2.5f;
                          player.face_right = false;
            					} else {
              						player.vel.x = -1.0f;
                          player.face_right = false;
            					}
          				} else {
            					if (player.vel.x == -1.0f || player.vel.x == -2.5f) {
              						player.vel.x = 0.0f;
            					}
          				}
        			} else if (evt.key.keysym.sym == SDLK_d) {
          				if (evt.key.state == SDL_PRESSED) {
						if (player.shifting) {
							player.vel.x = 2.5f;
                          player.face_right = true;
						} else {
							player.vel.x = 1.0f;
                          player.face_right = true;
						}
					} else {
						if (player.vel.x == 1.0f || player.vel.x == 2.5f) {
							player.vel.x = 0.0f;
						}
					}
				} else if (evt.key.keysym.sym == SDLK_q) {
					if (evt.key.state == SDL_PRESSED) {
						player.ability_mode = 0;
					}
				} else if (evt.key.keysym.sym == SDLK_e) {
					if (evt.key.state == SDL_PRESSED) {
						player.ability_mode = 1;
					}
				} else if (evt.key.keysym.sym == SDLK_LSHIFT) {
					if (evt.key.state == SDL_PRESSED) {
						if (player.vel.x == 1.0f) {
              player.face_right = true;
							player.vel.x = 2.5f;
						} else if (player.vel.x == -1.0f) {
              player.face_right = false;
							player.vel.x = -2.5f;
						}
						player.shifting = true;
					} else {
						if (player.vel.x == 2.5f) {
              player.face_right = true;
							player.vel.x = 1.0f;
						} else if (player.vel.x == -2.5f) {
              player.face_right = false;
							player.vel.x = -1.0f;
						}
						player.shifting = false;
					}
				} else if (evt.key.keysym.sym == SDLK_SPACE) {
					// handle space press
					// check interractable state
					if (evt.type == SDL_KEYDOWN && evt.key.repeat == 0) {
						if (player.pos.x + player.size.x / 2 < door.pos.x + door.size.x / 2
								&& player.pos.x - player.size.x / 2 > door.pos.x - door.size.x / 2
								&& player.pos.y + player.size.y / 2 < door.pos.y + door.size.y / 2) {
							player.behind_door = !player.behind_door;
						}
					}
				}
			} else if (evt.type == SDL_QUIT) {
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
    	// Compute vectors
    	float width = 0.0f, height = 0.0f;
    	glm::vec2 A;
    	glm::vec2 B;
    	glm::vec2 C;
    	if (light.dir == 0.0f) {
    		width = light.pos.y;
    		height = light.pos.x;
    	}
    	else if (light.dir == (0.5f * PI)) {
    		width = light.pos.x;
    		height = light.pos.y;
    	}
    	else if (light.dir == PI) {
    		width = light.pos.y;
    		height = light.pos.x;
    	}
    	else if (light.dir == (1.5f * PI)) {
    		width = light.pos.x;
    		height = light.pos.y;
    	}

    	A = light.pos + glm::vec2(0.0f, 0.5f * height);
    	B = light.pos - glm::vec2(0.5f * width, 0.5f * height);
    	C = light.pos - glm::vec2(-0.5f * width, 0.5f * height);;


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


	{ //update game state:
      
      //check if player is in light
      if ((!player.behind_door) && (check_visibility(flashlight) || check_visibility(ceilingLight))) {
        player.visible = true;
    }


	if (player.behind_door == false) {
      // player update
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
      if (player.pos.y < 1.0f) {
        player.jumping = false;
        player.pos.y = 1.0f;
        player.vel.y = 0.0f;
      }

      //camera update
      camera.pos.x += player.vel.x * elapsed;
      if (player.pos.x < 6.0f) {
        camera.pos.x = 6.0f;
      } else if (player.pos.x > 14.0f) {
        camera.pos.x = 14.0f;
      }

      //enemy update ------------------------------------------------------------
      if (!enemy.alerted) {
        if (!enemy.walking) {
          enemy.remaining_wait -= elapsed;
          if (enemy.remaining_wait <= 0.0f) {
            enemy.walking = true;
            enemy.face_right = !enemy.face_right;
            enemy.curr_index = (enemy.curr_index + 1) % 2;
            if (enemy.face_right) {
              enemy.vel.x = 1.0f;
            } else {
              enemy.vel.x = -1.0f;
            }
          }
        } else {
          enemy.pos += enemy.vel * elapsed;
          if ((enemy.face_right && enemy.pos.x > enemy.waypoints[enemy.curr_index].x) ||
              (!enemy.face_right && enemy.pos.x < enemy.waypoints[enemy.curr_index].x)) {
            enemy.pos = enemy.waypoints[enemy.curr_index];
            enemy.remaining_wait = enemy.wait_timers[enemy.curr_index];
            enemy.walking = false;
            enemy.vel.x = 0.0f;
          }
        }
      } else {
        if (!enemy.walking) {
          enemy.remaining_wait -= elapsed;
          if (enemy.remaining_wait <= 0.0f) {
            enemy.alerted = false;
            enemy.walking = true;
            enemy.face_right = (enemy.waypoints[enemy.curr_index].x > enemy.pos.x);
            if (enemy.face_right) {
              enemy.vel.x = 1.0f;
            } else {
              enemy.vel.x = -1.0f;
            }
          }
        } else {
          enemy.pos += enemy.vel * elapsed;
          if ((enemy.face_right && enemy.pos.x > enemy.target.x) ||
              (!enemy.face_right && enemy.pos.x < enemy.target.x)) {
            enemy.pos = enemy.target;
            enemy.remaining_wait = 10.0f;
            enemy.walking = false;
            enemy.vel.x = 0.0f;
          }
        }
      }
      if (player.visible && !player.behind_door) {
        if (enemy.face_right) {
          if (enemy.pos.x <= player.pos.x && enemy.pos.x + enemy.sight_range >= player.pos.x) {
            enemy.target = player.pos;
            enemy.vel.x = 2.5f;
            enemy.alerted = true;
            enemy.walking = true;
          }
        } else {
          if (enemy.pos.x - enemy.sight_range <= player.pos.x && enemy.pos.x >= player.pos.x) {
            enemy.target = player.pos;
            enemy.vel.x = -2.5f;
            enemy.alerted = true;
            enemy.walking = true;
          }
        } 
      }

      if (!player.behind_door) {
        if (enemy.face_right) {
          if (enemy.pos.x <= player.pos.x && enemy.pos.x + enemy.catch_range >= player.pos.x) {
            should_quit = true;
          }
        } else {
          if (enemy.pos.x - enemy.catch_range <= player.pos.x && enemy.pos.x >= player.pos.x) {
            should_quit = true;
          }
        }
      }      

      if (enemy.face_right)
        flashlight.pos = enemy.pos + glm::vec2(1.4f, 0.0f);
      else
        flashlight.pos = enemy.pos - glm::vec2(1.4f, 0.05f);


      //level win -----------------------------------------------------------
      if (player.pos.x >= 19.0) {
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

      //draw doors ------------------------------------------------------------------
			draw_sprite(door.sprite_empty, door.pos, door.size);
			
      //draw player -----------------------------------------------------------
      glm::vec2 player_size = player.size;
      if (!player.face_right) {
        player_size.x *= -1.0f;
      }
      if (player.behind_door == false) {
				draw_sprite(player.sprite_stand, player.pos, player_size);
			}
			
      //draw enemies -----------------------------------------------------------
      glm::vec2 enemy_size = enemy.size;
      if (enemy.face_right) {
        enemy_size.x *= -1.0f;
      }
      draw_sprite(enemy.sprite_stand, enemy.pos, enemy_size);     
      if (enemy.alerted) {
      glm::vec2 alert_pos = glm::vec2(enemy.pos.x, 
            enemy.pos.y + 0.51f*enemy.size.y + 0.51f*enemy.alert_size.y );
        draw_sprite(enemy.sprite_alert, alert_pos, enemy.alert_size);
      }

      //draw lights --------------------------------------------------------------
      if (enemy.face_right)
        draw_sprite(flashlight.sprite, enemy.pos + glm::vec2(1.4f, 0.0f), flashlight.size, glm::u8vec4(0xff, 0xff, 0xff, 0xff), flashlight.dir + PI);
      else
        draw_sprite(flashlight.sprite, enemy.pos - glm::vec2(1.4f, 0.05f), flashlight.size, glm::u8vec4(0xff, 0xff, 0xff, 0xff), flashlight.dir);

      draw_sprite(ceilingLight.sprite, ceilingLight.pos, ceilingLight.size);

			
      //draw platforms -----------------------------------------------------------
      draw_sprite(platform.sprite, platform.pos, platform.size);

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
		}

		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

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
