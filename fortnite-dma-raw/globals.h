#pragma once
#include <Windows.h>
namespace rifle
{

	inline bool aimbot = 1;
	inline bool ignore_downed = 1;
	inline bool ignore_bots = 0;
	inline bool render_fov = 0;

	inline bool visible_check = 1;
	inline bool target_line = 1;
	inline float aim_fov = 15;
	inline float smooth = 3;
	inline int hitbox = 0;
	inline int aimkey;
}

namespace shotgun
{

	inline bool aimbot = 1;
	inline bool ignore_downed = 1;
	inline bool ignore_bots = 0;
	inline bool render_fov = 0;

	inline bool visible_check = 1;
	inline bool target_line = 1;
	inline float aim_fov = 15;
	inline float smooth = 3;
	inline int hitbox = 0;
	inline int aimkey;



}

namespace smg
{

	inline bool aimbot = 1;
	inline bool ignore_downed = 1;
	inline bool ignore_bots = 0;
	inline bool render_fov = 0;

	inline bool visible_check = 1;
	inline bool target_line = 1;
	inline 	float aim_fov = 15;
	inline 	float smooth = 3;
	inline int hitbox = 0;
	inline int aimkey;


}

namespace sniper
{

	inline bool aimbot = 1;
	inline bool ignore_downed = 1;
	inline bool ignore_bots = 0;
	inline bool render_fov = 0;

	inline bool visible_check = 1;
	inline bool target_line = 1;
	inline	float aim_fov = 15;
	inline float smooth = 3;
	inline int hitbox = 0;
	inline int aimkey;



}
namespace globals
{
	inline bool SuperPreformance = false;

		namespace triggerbot
		{
			inline bool shotgun_only = false;
			inline bool triggerbot_key = VK_RBUTTON;
			inline bool triggerbot = false;
			inline float custom_delay = 1;
			inline float maximum_distance = 50;
		}
		namespace visuals
		{
			inline int box_thickness = 2;
			inline bool killesp = false;
			inline bool levelesp = false;
			inline bool visible_check = true;
			inline bool distancesp = true;
			inline bool draw_platform = false;
			inline bool box = true;
			inline bool skeletons = false;
			inline bool drawfov = false;
			inline bool name_esp = true;
			inline bool snap_lines = false;
			inline bool weapon_esp = false;
		}
		namespace aimbot
		{
			inline bool mouseaim = false;
			inline bool memoryaim = false;
			inline bool kmbox = true;
			inline bool aimbot = true;
			inline bool weapom_cfg = false;
			inline bool prediction = true;
			inline int hitbox = 0;

			inline int aimbot_key = VK_RBUTTON;
			inline float aimbot_fov = 300.0f;
			inline float smoothing = 2.0f;

			inline float kmbox_times = 3.0f;
		}
		

		namespace exploit
		{
			inline bool freeze = false;
			inline bool bigplayer = false;
			inline bool noreload = false;
		}
		
		
		
	

	inline bool is_aim_key_picker_active = false;

	inline int resolution_x = 0, resolution_y = 0;

	inline uint64_t gworld = 0, local_player_controller = 0;
	inline uintptr_t local_pawn = 0;

	inline vec3_t location{};

	

	inline int local_team = 0;

	inline bool should_click = false, should_move_x = false, should_move_y = false;
	inline int move_x = 0, move_y = 0;
}