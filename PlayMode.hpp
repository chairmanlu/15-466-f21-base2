#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, more, less, space, w, a, s, d;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform *ball = nullptr;
	Scene::Transform *arrow = nullptr;
	Scene::Transform *ball_tracker = nullptr;
	glm::quat ball_base_rotation;
	glm::quat tracker_base_rotation;

	glm::vec3 velocity = glm::vec3(0.0f,0.0f,0.0f);

//	float total_rot_x = 0.0f;
//	float total_rot_y = 0.0f;
	
	bool in_play = false;

	size_t strokes = 0;

	float shot_mag = 5.0f;
	float shot_theta = 90.0f; //Angle between shot and +x
	float shot_phi = 0.0f; //Angle between shot and x-y plane

	glm::vec3 total_rot = glm::vec3(0.0f,0.0f,0.0f);

	glm::vec3 start_pos;

	float init_height = 0.0f;
	float wobble = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;
	Scene::Camera *camera2 = nullptr;

};
