#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

#define GRAVITY -20.0f
#define FRICTION 0.8f
#define AIR_FRICTION 0.9f
#define EPS 0.5f

#define ROT_SPEED 100.0f

//Can't figure out collision detection so just gonna hack it
#define WALL_RIGHT 20.3f
#define WALL_LEFT -1.5f
#define WALL_TOP 45.7f
#define WALL_BOTTOM 0.5f
#define WALL_HEIGHT -18.5f

#define DROP_HEIGHT 5.0f
#define OOB_THRES -50.0f
#define VEL_THRES 0.5f

#define HOLE_X 9.6f
#define HOLE_Y 40.5f
#define HOLE_RADIUS 0.3f

#define WALL_FRICTION -0.7f

#define MIN_POWER 1.0f
#define MAX_POWER 20.0f

#define SCALE_FACTOR -0.2f

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hole1.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hole1.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Ball") ball = &transform;
		if (transform.name == "Arrow") arrow = &transform;
		if (transform.name == "Ball Tracker") ball_tracker = &transform;
	}
	if (ball == nullptr) throw std::runtime_error("Ball not found.");
	if (arrow == nullptr) throw std::runtime_error("Arrow not found.");
	if (ball_tracker == nullptr) throw std::runtime_error("Ball tracker not found.");
//
	ball_base_rotation = ball->rotation;
	tracker_base_rotation = ball_tracker->rotation;
	ball_tracker->position = ball->position;
	init_height = ball->position.z;
	start_pos = ball->position;

	//get pointer to camera for convenience:
//	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			a.downs += 1;
			a.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			d.downs += 1;
			d.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			w.downs += 1;
			w.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			s.downs += 1;
			s.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			more.downs += 1;
			more.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			less.downs += 1;
			less.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_q) {
			more.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_e) {
			less.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			a.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			d.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			w.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			s.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	velocity += glm::vec3(0.0f,0.0f,GRAVITY * elapsed);//Apply gravity

	ball->position += velocity * elapsed;

	auto withinEps = [&](float a, float b){
		return abs(a-b) <= EPS && a < b;
	};

	if(velocity.x > 0 && withinEps(WALL_RIGHT, ball->position.x) && ball->position.z < WALL_HEIGHT){
		ball->position.x = WALL_RIGHT;
		velocity.x *= WALL_FRICTION;
		ball_base_rotation = ball->rotation;
		total_rot.x = 0;
		total_rot.y = 0;
	}

	if(velocity.x < 0 && withinEps(ball->position.x,WALL_LEFT) && ball->position.z < WALL_HEIGHT){
		ball->position.x = WALL_LEFT;
		velocity.x *= WALL_FRICTION;
		ball_base_rotation = ball->rotation;
		total_rot.x = 0;
		total_rot.y = 0;
	}

	if(velocity.y < 0 && withinEps(ball->position.y,WALL_BOTTOM) && ball->position.z < WALL_HEIGHT){
		ball->position.y = WALL_BOTTOM;
		velocity.y *= WALL_FRICTION;
		ball_base_rotation = ball->rotation;
		total_rot.x = 0;
		total_rot.y = 0;
	}

	if(velocity.y > 0 && withinEps(WALL_TOP,ball->position.y) && ball->position.z < WALL_HEIGHT){
		ball->position.y = WALL_TOP;
		velocity.y *= WALL_FRICTION;
		ball_base_rotation = ball->rotation;
		total_rot.x = 0;
		total_rot.y = 0;
	}

	bool in_bounds = ball->position.x <= WALL_RIGHT && ball->position.x >= WALL_LEFT &&
	                 ball->position.y <= WALL_TOP && ball->position.y >= WALL_BOTTOM;

	float dist = sqrt(pow(ball->position.x - HOLE_X,2.0f) + pow(ball->position.y - HOLE_Y,2.0f));

	if(velocity.z < 0 && ball->position.z < init_height && in_bounds && dist > HOLE_RADIUS){
		//Ball on ground
		ball->position.z = init_height;
		velocity.z *= WALL_FRICTION;
		velocity *= pow(FRICTION,elapsed);

		if(glm::length(velocity) < VEL_THRES){
			velocity = glm::vec3(0.0f,0.0f,0.0f);
			in_play = false;
		}
	}
	else{
		velocity *= pow(AIR_FRICTION,elapsed);
	}

	if(!in_bounds && ball->position.z < OOB_THRES){
		ball->position = start_pos + glm::vec3(0.0f,0.0f,DROP_HEIGHT);
		velocity *= 0.0f;
		strokes++;
	}

	if(dist < HOLE_RADIUS && ball->position.z < init_height){
		velocity.x = 0;
		velocity.y = 0;
	}

	if(dist < HOLE_RADIUS && ball->position.z < OOB_THRES){
		std::cout << "Finished in " << strokes << " strokes." << std::endl;
		exit(0);
	}

	if(withinEps(ball->position.z,init_height)){
		total_rot.x += ROT_SPEED * velocity.x * elapsed;
		total_rot.y += ROT_SPEED * velocity.y * elapsed;
	}

	while(total_rot.x >= 360.0f){
		total_rot.x -= 360.0f;
	}
	while(total_rot.y >= 360.0f){
		total_rot.y -= 360.0f;
	}

	float rot = sqrt(pow(total_rot.x,2.0f) + pow(total_rot.y,2.0f));

	if(rot != 0){
		ball->rotation = ball_base_rotation * glm::angleAxis(
			glm::radians(rot),
			glm::normalize(glm::vec3(-1.0f * total_rot.y, total_rot.x, 0.0f))
		);
	}


	if(in_play){
		ball_tracker->position = glm::vec3(-99999.0f,-99999.0f,-99999.0f);
	}
	else{
		ball_tracker->position = ball->position;
		if (left.pressed && !right.pressed) shot_theta += 1.0f;
		if (!left.pressed && right.pressed) shot_theta -= 1.0f;
		if (!down.pressed && up.pressed) shot_phi += 1.0f;
		if (down.pressed && !up.pressed) shot_phi -= 1.0f;
		if (more.pressed && !less.pressed) shot_mag += 1.0f;
		if (!more.pressed && less.pressed) shot_mag -= 1.0f;

		while(shot_theta < 0.0f){
			shot_theta += 360.0f;
		}
		while(shot_theta >= 360.0f){
			shot_theta -= 360.0f;
		}
		if(shot_phi < 0.0f){
			shot_phi = 0.0f;
		}
		if(shot_phi > 90.0f){
			shot_phi = 90.0f;
		}
		if(shot_mag < MIN_POWER){
			shot_mag = MIN_POWER;
		}
		if(shot_mag > MAX_POWER){
			shot_mag = MAX_POWER;
		}

		//Polar to cartesion formula from Wikipedia
		glm::vec3 shot_dir(shot_mag * cos(shot_theta * float(M_PI) / 180.0f) * cos(shot_phi * float(M_PI) / 180.0f),
		                   shot_mag * sin(shot_theta * float(M_PI) / 180.0f) * cos(shot_phi * float(M_PI) / 180.0f),
		                   shot_mag * sin(shot_phi * float(M_PI) / 180.0f));

		glm::quat tmp_rotation = tracker_base_rotation * glm::angleAxis(
			glm::radians(shot_phi),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);

		ball_tracker->rotation = tmp_rotation * glm::angleAxis(
			glm::radians(shot_theta),
			glm::vec3(-1.0f, 0.0f, 0.0f)
		);

		arrow->scale.z = SCALE_FACTOR * shot_mag;

		if(space.pressed){
			in_play = true;
			velocity = shot_dir;
			strokes++;
		}
	}

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (a.pressed && !d.pressed) move.x =-1.0f;
		if (!a.pressed && d.pressed) move.x = 1.0f;
		if (s.pressed && !w.pressed) move.y =-1.0f;
		if (!s.pressed && w.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	a.downs = 0;
	s.downs = 0;
	d.downs = 0;
	w.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f,1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Arrow keys to aim ball. Q and E to increase/decrease power. WASD and mouse control camera.",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Arrow keys to aim ball. Q and E to increase/decrease power. WASD and mouse control camera.",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
