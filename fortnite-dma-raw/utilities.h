#pragma once

#include "vector.h"

namespace utilities
{
	extern void update_camera_data();
	extern vec2_t world_to_screen(vec3_t world);
	extern float distance_from_crosshair(const vec2_t& vec);
	extern void get_desktop_resolution(int& horizontal, int& vertical);
}
