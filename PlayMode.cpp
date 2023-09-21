#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint boxsphere_meshes_for_lit_color_texture_program = 0;

Load< MeshBuffer > boxsphere_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("boxsphere.pnct"));
	boxsphere_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > boxsphere_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("boxsphere.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = boxsphere_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = boxsphere_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > bgm_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("bgm.opus"));
});

Load< Sound::Sample > win_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("win.opus"));
});

Load< Sound::Sample > lose_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("lose.opus"));
});

PlayMode::PlayMode() : scene(*boxsphere_scene) {
	//get pointers to leg for convenience:

	for (auto &transform : scene.transforms) {
		if (transform.name == "Sphere") sphere = &transform;
		else if(transform.name == "Sphere1") sphere1 = &transform;
		else if (transform.name == "Target") target = &transform;
		else if (transform.name == "LeftAnkle") leftAnkle = &transform;
		else if (transform.name == "RightAnkle") rightAnkle = &transform;
	}
	if (target == nullptr) throw std::runtime_error("Target not found.");
	if (leftAnkle == nullptr) throw std::runtime_error("LeftAnkle not found.");
	if (rightAnkle == nullptr) throw std::runtime_error("RightAnkle not found.");
	if (sphere == nullptr) throw std::runtime_error("Sphere not found.");

	leftAnkle_base_rotation = leftAnkle->rotation;
	rightAnkle_base_rotation = rightAnkle->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	bgm = Sound::loop(*bgm_sample, 1.0f);// Sound::loop_3D(*bgm_sample, 1.0f, get_leg_tip_position(), 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
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

	if(lose || win) return;

	//slowly rotates through [0,1):
	wobble += elapsed * 2;
	wobble -= std::floor(wobble);

	leftAnkle->rotation = leftAnkle_base_rotation * glm::angleAxis(
		glm::radians(30.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
	rightAnkle->rotation = rightAnkle_base_rotation * glm::angleAxis(
		glm::radians(-30.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	sphere_speed.z -= 9.8f * elapsed;
	sphere->position += sphere_speed * elapsed;
	if (sphere->position.z < 1.0f) {//hit ground
		sphere->position.z = 2.f - sphere->position.z;
		sphere_speed.z *= -1.0f;
	}
	if (sphere->position.z > 19.f) {//hit ceiling
		sphere->position.z = 38.f - sphere->position.z;
		sphere_speed.z *= -1.0f;
	}
	if (sphere->position.x < -9.f) {//hit left wall
		sphere->position.x = -18.f - sphere->position.x;
		sphere_speed.x *= -1.0f;
	}
	if (sphere->position.x > 9.f) {//hit right wall
		sphere->position.x = 18.f - sphere->position.x;
		sphere_speed.x *= -1.0f;
	}
	if (sphere->position.y < -9.f) {//hit back wall
		sphere->position.y = -18.f - sphere->position.y;
		sphere_speed.y *= -1.0f;
	}
	if (sphere->position.y > 9.f) {//hit front wall
		sphere->position.y = 18.f - sphere->position.y;
		sphere_speed.y *= -1.0f;
	}

	sphere1->position += sphere1_speed * elapsed;
	if (sphere1->position.z < 1.0f) {//hit ground
		sphere1->position.z = 2.f - sphere1->position.z;
		sphere1_speed.z *= -1.0f;
	}
	if (sphere1->position.z > 19.f) {//hit ceiling
		sphere1->position.z = 38.f - sphere1->position.z;
		sphere1_speed.z *= -1.0f;
	}
	if (sphere1->position.x < -9.f) {//hit left wall
		sphere1->position.x = -18.f - sphere1->position.x;
		sphere1_speed.x *= -1.0f;
	}
	if (sphere1->position.x > 9.f) {//hit right wall
		sphere1->position.x = 18.f - sphere1->position.x;
		sphere1_speed.x *= -1.0f;
	}
	if (sphere1->position.y < -9.f) {//hit back wall
		sphere1->position.y = -18.f - sphere1->position.y;
		sphere1_speed.y *= -1.0f;
	}
	if (sphere1->position.y > 9.f) {//hit front wall
		sphere1->position.y = 18.f - sphere1->position.y;
		sphere1_speed.y *= -1.0f;
	}

	target->position += target_speed * elapsed;
	if (target->position.x < -9.f) {//hit left wall
		target->position.x = -18.f - target->position.x;
		target_speed.x *= -1.0f;
	}
	if (target->position.x > 9.f) {//hit right wall
		target->position.x = 18.f - target->position.x;
		target_speed.x *= -1.0f;
	}
	if (target->position.y < -9.f) {//hit back wall
		target->position.y = -18.f - target->position.y;
		target_speed.y *= -1.0f;
	}
	if (target->position.y > 9.f) {//hit front wall
		target->position.y = 18.f - target->position.y;
		target_speed.y *= -1.0f;
	}
	if (target->position.z < 1.f) {//hit ground
		target->position.z = 2.f - target->position.z;
		target_speed.z *= -1.0f;
	}
	if (target->position.z > 19.f) {//hit ceiling
		target->position.z = 38.f - target->position.z;
		target_speed.z *= -1.0f;
	}

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
		if(camera->transform->position.x < -9.f){
			camera->transform->position.x = -9.f;
		}
		if(camera->transform->position.x > 9.f){
			camera->transform->position.x = 9.f;
		}
		if(camera->transform->position.y < -9.f){
			camera->transform->position.y = -9.f;
		}
		if(camera->transform->position.y > 9.f){
			camera->transform->position.y = 9.f;
		}
		if(camera->transform->position.z < 1.f){
			camera->transform->position.z = 1.f;
		}
		if(camera->transform->position.z > 19.f){
			camera->transform->position.z = 19.f;
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	if(glm::length(target->position - camera->transform->position) < 1.f){
		win = true;
		Sound::play(*win_sample, 1.0f, 0.0f);
		std::cout << "You win!" << std::endl;
	}
	if(glm::length(sphere->position - camera->transform->position) < 1.5f || glm::length(sphere1->position - camera->transform->position) < 1.5f){
		lose = true;
		Sound::play(*lose_sample, 1.0f, 0.0f);
		std::cout << "You lose!" << std::endl;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(80.f * glm::vec3(1.0f, 1.0f, 0.95f)));
	glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 10.f)));
	glUniform3fv(lit_color_texture_program->CAMERA_POSITION_vec3, 1, glm::value_ptr(camera->transform->position));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

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
		std::string message = "Balls = Death, Wings = Success!";
		if(win){
			message = "You win!";
		}
		if(lose){
			message = "You lose!";
		}
		lines.draw_text(message,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(message,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

