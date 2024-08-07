#include "utilities.h"
#include "globals.h"

#include <d3dx9.h>

#include "entity_list.h"

#define M_PI 3.14159265358979323846264338327950288

static D3DMATRIX to_matrix(vec3_t rot, vec3_t origin = vec3_t{ 0, 0, 0 })
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}


struct camera_data_t
{
	D3DMATRIX temp_matrix{};
	vec3_t rotation{}, location{};
	float field_of_view;
};

camera_data_t camera_data{};

void utilities::update_camera_data()
{
	camera_data_t new_camera_data{};

	uint64_t location_ptr = 0, rotation_ptr = 0;
	view_memory.initialize_scatter();
	view_memory.prepare_scatter(globals::gworld + 0x110, &location_ptr);
	view_memory.prepare_scatter(globals::gworld + 0x120, &rotation_ptr);
	view_memory.dispatch_read(true);

	struct fn_rot_t
	{
		double a; //0x0000
		char pad_0008[24]; //0x0008
		double b; //0x0020
		char pad_0028[424]; //0x0028
		double c; //0x01D0
	} fn_rot;

	float raw_field_of_view = 0;

	view_memory.initialize_scatter();
	view_memory.prepare_scatter(rotation_ptr, &fn_rot.a);
	view_memory.prepare_scatter(rotation_ptr + 0x20, &fn_rot.b);
	view_memory.prepare_scatter(rotation_ptr + 0x1d0, &fn_rot.c);
	view_memory.prepare_scatter(location_ptr, &new_camera_data.location);
	view_memory.prepare_scatter<float>(globals::local_player_controller + 0x394, &raw_field_of_view);
	view_memory.dispatch_read(true);

	new_camera_data.rotation.x = asin(fn_rot.c) * (180.0 / M_PI);
	new_camera_data.rotation.y = ((atan2(fn_rot.a * -1, fn_rot.b) * (180.0 / M_PI)) * -1) * -1;
	new_camera_data.field_of_view = raw_field_of_view * 90.f;

	//logger::info("location -> %f %f %f\n", new_camera_data.location.x, new_camera_data.location.y, new_camera_data.location.z);
	//logger::info("rotation -> %f %f %f\n", new_camera_data.rotation.x, new_camera_data.rotation.y, new_camera_data.rotation.z);
	//logger::info("field of view -> %f\n\n", new_camera_data.field_of_view);
	
	camera_data = new_camera_data;
	globals::location = camera_data.location;
	camera_data.temp_matrix = to_matrix(camera_data.rotation);
}

vec2_t utilities::world_to_screen(vec3_t world)
{
	vec3_t vAxisX = vec3_t{ camera_data.temp_matrix.m[0][0], camera_data.temp_matrix.m[0][1], camera_data.temp_matrix.m[0][2] };
	vec3_t vAxisY = vec3_t{ camera_data.temp_matrix.m[1][0], camera_data.temp_matrix.m[1][1], camera_data.temp_matrix.m[1][2] };
	vec3_t vAxisZ = vec3_t{ camera_data.temp_matrix.m[2][0], camera_data.temp_matrix.m[2][1], camera_data.temp_matrix.m[2][2] };

	vec3_t vDelta = { world.x - camera_data.location.x, world.y - camera_data.location.y, world.z - camera_data.location.z };
	vec3_t vTransformed = vec3_t{ dot(vDelta, vAxisY), dot(vDelta, vAxisZ), dot(vDelta, vAxisX) };

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float x = (globals::resolution_x / 2.0f) + vTransformed.x * (((globals::resolution_x / 2.0f) / tanf(camera_data.field_of_view * (float)M_PI / 360.f))) / vTransformed.z;
	float y = (globals::resolution_y / 2.0f) - vTransformed.y * (((globals::resolution_x / 2.0f) / tanf(camera_data.field_of_view * (float)M_PI / 360.f))) / vTransformed.z;

	return vec2_t{ x, y };
}

float utilities::distance_from_crosshair(const vec2_t& vec)
{
	const float center_x = globals::resolution_x / 2.0f;
	const float center_y = globals::resolution_y / 2.0f;

	const float dx = vec.x - center_x;
	const float dy = vec.y - center_y;

	return sqrt(dx * dx + dy * dy);
}

void utilities::get_desktop_resolution(int& horizontal, int& vertical)
{
	HWND wnd = GetDesktopWindow();

	RECT desktop;
	GetWindowRect(wnd, &desktop);

	horizontal = desktop.right;
	vertical = desktop.bottom;
}
