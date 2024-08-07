#pragma once

#include <stdint.h>

namespace offsets
{
	inline uint64_t gworld = 0x11781328;
	inline uint64_t game_state = 0x158;
	inline uint64_t game_instance = 0x1d0;
	inline uint64_t platformoffset = 0x438;
	inline uint64_t KillScore = 0x10f4;//IntProperty Class FortniteGame.FortPlayerStateAthena.KillScore
	inline uint64_t level = 0x10f8;//IntProperty Class FortniteGame.FortPlayerStateAthena.SeasonLevelUIDisplay -> 0x10f0
	
	inline uint64_t player_array = 0x2a8;
	inline uint64_t player_array_size = player_array + 0x8;

	inline uint64_t local_players = 0x38;
	inline uint64_t player_controller = 0x30;
	inline uint64_t pawn = 0x338;//APlayerController->AcknowledgedPawn
	inline uint64_t name_structure = 0xAE8;//somewhere in playerstate padding

	inline uint64_t pawn_private = 0x308;
	inline uint64_t player_state = 0x2b0;

	inline uint64_t team_index = 0x10e0;

	inline uint64_t mesh = 0x318;//ACharacter->USkeletalMeshComponent
	inline uint64_t bone_array = 0x608;
	inline uint64_t bone_array_cache = bone_array + 0x10;
	inline uint64_t component_to_world = 0x230;

	inline uint64_t last_submit_time = 0x358;
	inline uint64_t last_render_time_on_screen = 0x360;

	inline uint64_t current_weapon = 0xa20; 
	inline uint64_t weapon_data = 0x4a8;
	inline uint64_t ftext_ptr = 0x90;
}