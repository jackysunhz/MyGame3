#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

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
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *sphere = nullptr;
	Scene::Transform *sphere1 = nullptr;
	glm::vec3 sphere_speed = glm::vec3(7.0f, 9.0f, -15.f);
	glm::vec3 sphere1_speed = glm::vec3(6.0f, -3.0f, 8.f);

	Scene::Transform *target = nullptr;
	glm::vec3 target_speed = glm::vec3(4.f, 2.f, 3.f);
	Scene::Transform *leftAnkle = nullptr;
	Scene::Transform *rightAnkle = nullptr;

	glm::quat leftAnkle_base_rotation;
	glm::quat rightAnkle_base_rotation;
	float wobble = 0.0f;

	std::shared_ptr< Sound::PlayingSample > bgm;
	std::shared_ptr< Sound::Sample> win_sound;
	std::shared_ptr< Sound::Sample> lose_sound;
	
	//camera:
	Scene::Camera *camera = nullptr;

	bool win = false;
	bool lose = false;

};
