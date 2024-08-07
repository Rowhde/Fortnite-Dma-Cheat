#include <iostream>
#include <vector>
#include <thread>

#include <memprocfs.h>

#include "offsets.h"

inline c_memory memory = c_memory(0);
inline c_memory view_memory = c_memory(0);

inline c_keyboard keyboard = c_keyboard(nullptr);

inline bool ptr_valid(uint64_t ptr)//very bad practice
{
	return ptr > 0x1000000;
}

struct quat_t
{
	double x;
	double y;
	double z;
	double w;
};

struct transform_t
{
	quat_t rot;
	vec3_t translation;
	char pad[4];
	vec3_t scale;
	char pad1[4];

	D3DMATRIX to_matrix_with_scale()
	{
		D3DMATRIX m;
		m._41 = translation.x;
		m._42 = translation.y;
		m._43 = translation.z;

		float x2 = rot.x + rot.x;
		float y2 = rot.y + rot.y;
		float z2 = rot.z + rot.z;

		float xx2 = rot.x * x2;
		float yy2 = rot.y * y2;
		float zz2 = rot.z * z2;
		m._11 = (1.0f - (yy2 + zz2)) * scale.x;
		m._22 = (1.0f - (xx2 + zz2)) * scale.y;
		m._33 = (1.0f - (xx2 + yy2)) * scale.z;

		float yz2 = rot.y * z2;
		float wx2 = rot.w * x2;
		m._32 = (yz2 - wx2) * scale.z;
		m._23 = (yz2 + wx2) * scale.y;

		float xy2 = rot.x * y2;
		float wz2 = rot.w * z2;
		m._21 = (xy2 - wz2) * scale.y;
		m._12 = (xy2 + wz2) * scale.x;

		float xz2 = rot.x * z2;
		float wy2 = rot.w * y2;
		m._31 = (xz2 + wy2) * scale.z;
		m._13 = (xz2 - wy2) * scale.x;

		m._14 = 0.0f;
		m._24 = 0.0f;
		m._34 = 0.0f;
		m._44 = 1.0f;

		return m;
	}
};

inline static D3DMATRIX matrix_multiplication(D3DMATRIX pM1, D3DMATRIX pM2)
{
	D3DMATRIX pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}

enum class e_bone_ids : int
{
	HumanBase = 0,
	HumanPelvis = 2,
	HumanLThigh1 = 71,
	HumanLThigh2 = 77,
	HumanLThigh3 = 72,
	HumanLCalf = 74,
	HumanLFoot2 = 73,
	HumanLFoot = 86,
	HumanLToe = 76,
	HumanRThigh1 = 78,
	HumanRThigh2 = 84,
	HumanRThigh3 = 79,
	HumanRCalf = 81,
	HumanRFoot2 = 82,
	HumanRFoot = 87,
	HumanRToe = 83,
	HumanSpine1 = 7,
	HumanSpine2 = 5,
	HumanSpine3 = 2,
	HumanLCollarbone = 9,
	HumanLUpperarm = 35,
	HumanLForearm1 = 36,
	HumanLForearm23 = 10,
	HumanLForearm2 = 34,
	HumanLForearm3 = 33,
	HumanLPalm = 32,
	HumanLHand = 11,
	HumanRCollarbone = 98,
	HumanRUpperarm = 64,
	HumanRForearm1 = 65,
	HumanRForearm23 = 39,
	HumanRForearm2 = 63,
	HumanRForearm3 = 62,
	HumanRHand = 40,
	HumanRPalm = 58,
	HumanNeck = 67,
	HumanHead = 109,
	HumanChest = 66
};

class c_entity
{
private:
public:
	

	uint64_t player_state = 0, pawn = 0, mesh = 0, bone_array = 0, bone_array_cache = 0, player_weapon = 0, weapon_data = 0, ftext_ptr = 0, ftext_data = 0, local_player_weapon = 0, local_weapon_data = 0, local_ftext_ptr = 0, root_component = 0;
	
	
	
	DWORD_PTR test_platform = 0;
	int team_index = 0, kills = 0, level = 0, pNameLength = 0, ftext_length = 0;
	BYTE weapontier = 0;
	transform_t component_to_world{}; 
	transform_t base_transform{}, head_transform{}, pelvis_transform{}, chest_transform{}, spine1_transform{}, spine2_transform{};
	vec3_t base_pos{}, head_pos{}, pelvis_pos{}, chest_pos{}, spine1_pos{}, spine2_pos{};

	transform_t left_hand_transform{}, right_hand_transform{}, left_collar_bone_transform{}, right_collar_bone_transform{}, right_thigh_transform{}, left_thigh_transform{}, left_calf_transform{}, right_calf_transform{};
	vec3_t left_hand_pos{}, right_hand_pos{}, left_collar_bone_pos{}, right_collar_bone_pos{}, right_thigh_pos{}, left_thigh_pos{}, left_calf_pos{}, right_calf_pos{};

	float last_submit_time = 0.0f, last_render_time_on_screen = 0.0f;

	wchar_t platform[64];

	wchar_t* name_buffer = nullptr;
	uint64_t name_structure_ptr = 0, name_encrypted_buffer_ptr = 0;
	int name_length = 0;
	std::string formatted_name = "";
	c_entity(uint64_t in_player_state) : player_state(in_player_state) { }

	void prepare_address()
	{
		memory.prepare_scatter(player_state + offsets::pawn_private, &pawn);
		memory.prepare_scatter(player_state + offsets::team_index, &team_index);
		memory.prepare_scatter(player_state + offsets::name_structure, &name_structure_ptr);

		if (globals::visuals::killesp) {

		 memory.prepare_scatter(player_state + offsets::KillScore, &kills);

		}
		if (globals::visuals::levelesp)
			memory.prepare_scatter(player_state + offsets::level, &level);

		if (globals::visuals::draw_platform) {
			memory.prepare_scatter(player_state + offsets::platformoffset, &test_platform);
		}
		if (globals::aimbot::weapom_cfg){
			memory.prepare_scatter(globals::local_pawn + offsets::current_weapon, &local_player_weapon);
		}
			
		
		
		
				
	}

	void prepare_a()
	{

		memory.prepare_scatter(pawn + offsets::mesh, &mesh);
		memory.prepare_scatter(name_structure_ptr + 0x8, &name_encrypted_buffer_ptr);
		memory.prepare_scatter(name_structure_ptr + 0x10, &name_length);
		if (globals::aimbot::weapom_cfg) {
			memory.prepare_scatter(local_player_weapon + offsets::weapon_data, &local_weapon_data);
		}
		if (globals::visuals::weapon_esp) {
			memory.prepare_scatter(pawn + offsets::current_weapon, &player_weapon); //currentweapon
		}
		if (globals::visuals::draw_platform) {

			memory.prepare_scatter(test_platform, platform, sizeof(platform));

		}
	


			

			
		
		
		
		
	
		
		
	}
	
	void prepare_b()
	{
		
		memory.prepare_scatter(mesh + offsets::bone_array, &bone_array);
		memory.prepare_scatter(mesh + offsets::bone_array_cache, &bone_array_cache);
		memory.prepare_scatter(mesh + offsets::component_to_world, &component_to_world);

		
		
		memory.prepare_scatter(mesh + offsets::last_submit_time, &last_submit_time);
		memory.prepare_scatter(mesh + offsets::last_render_time_on_screen, &last_render_time_on_screen);
		
		if (globals::visuals::weapon_esp) {
			memory.prepare_scatter(player_weapon + offsets::weapon_data, &weapon_data); //weapon_data
		}
		if (globals::aimbot::weapom_cfg) {
			memory.prepare_scatter(local_weapon_data + offsets::ftext_ptr, &local_ftext_ptr);
		}
		name_buffer = new wchar_t[name_length];

		if (ptr_valid(name_encrypted_buffer_ptr))
			memory.prepare_scatter(name_encrypted_buffer_ptr, name_buffer, name_length * sizeof(wchar_t));
			
		

		
			
		
	}

	void prepare_c()
	{
		if (globals::visuals::weapon_esp) {
			memory.prepare_scatter(weapon_data + 0x73, &weapontier);
			memory.prepare_scatter(weapon_data + offsets::ftext_ptr, &ftext_ptr); //ftext ptr
		}
			
		
		

		memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanBase * 0x60), &base_transform);
		memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanHead * 0x60), &head_transform);
		if (globals::visuals::skeletons)
		{
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanPelvis * 0x60), &pelvis_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanChest * 0x60), &chest_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanSpine3 * 0x60), &spine1_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanSpine1 * 0x60), &spine2_transform);

			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanLHand * 0x60), &left_hand_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanRHand * 0x60), &right_hand_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanLForearm1 * 0x60), &left_collar_bone_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanRForearm1 * 0x60), &right_collar_bone_transform);

			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanRThigh2 * 0x60), &right_thigh_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanLThigh2 * 0x60), &left_thigh_transform);

			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanLCalf * 0x60), &left_calf_transform);
			memory.prepare_scatter(get_bone_array() + ((int)e_bone_ids::HumanRCalf * 0x60), &right_calf_transform);
        }
		
	}
	void prepare_d()
	{
		
			memory.prepare_scatter(ftext_ptr + 0x28, &ftext_data);
			memory.prepare_scatter(ftext_ptr + 0x30, &ftext_length);
		
		
		
	}
	
	vec3_t resolve_matrix(transform_t transform)
	{
		D3DMATRIX matrix = matrix_multiplication(transform.to_matrix_with_scale(), component_to_world.to_matrix_with_scale());

		return vec3_t{ matrix._41, matrix._42, matrix._43 };
	}

	void extract_bone_data()
	{
		base_pos = resolve_matrix(base_transform);
		head_pos = resolve_matrix(head_transform);
		pelvis_pos = resolve_matrix(pelvis_transform);
		chest_pos = resolve_matrix(chest_transform);
		spine1_pos = resolve_matrix(spine1_transform);
		spine2_pos = resolve_matrix(spine2_transform);

		left_hand_pos = resolve_matrix(left_hand_transform);
		right_hand_pos = resolve_matrix(right_hand_transform);

		left_collar_bone_pos = resolve_matrix(left_collar_bone_transform);
		right_collar_bone_pos = resolve_matrix(right_collar_bone_transform);

		right_thigh_pos = resolve_matrix(right_thigh_transform);
		left_thigh_pos = resolve_matrix(left_thigh_transform);

		left_calf_pos = resolve_matrix(left_calf_transform);
		right_calf_pos = resolve_matrix(right_calf_transform);

	}

	bool is_visible()
	{
		return last_render_time_on_screen + 0.15f >= last_submit_time;
	}

	uint64_t get_bone_array()
	{
		if (!ptr_valid(bone_array))
			return bone_array_cache;

		return bone_array;
	}
	void process_name_decryption()
	{
		if (name_length <= 0 || name_length > 128)
			return;

		if (!ptr_valid(name_structure_ptr) || !ptr_valid(name_encrypted_buffer_ptr))
			return;

		int v25 = name_length - 1;
		int v26 = 0;
		uint16_t* name_buffer_ptr = (uint16_t*)name_buffer;

		int last_used_char = name_length - 1;;
		for (int i = (v25) & 3; ; *name_buffer_ptr++ += i & 7)
		{
			if (v26 >= last_used_char)
				break;

			i += 3;
			++v26;
		}

		std::wstring temp_wstring(name_buffer);
		formatted_name = std::string(temp_wstring.begin(), temp_wstring.end());
	}
};

inline std::vector<c_entity> g_entities{};

class c_entity_list
{
private:
public:
	uint64_t array_start = 0;
	int array_size = 0;

	std::vector<c_entity> entities{};
	std::vector<c_entity> tmp_entities{};

	c_entity_list(uint64_t in_array_start, int in_array_size) : array_start(in_array_start), array_size(in_array_size) { }

	void update()
	{
		tmp_entities.clear();

		memory.initialize_scatter();

		for (int i = 0; i < array_size; i++)
			memory.prepare_scatter<uint64_t>(array_start + (i * 0x8));

		memory.dispatch_read();

		for (int i = 0; i < array_size; i++)
		{
			uint64_t player = memory.read_scatter<uint64_t>(array_start + (i * 0x8));
			if (ptr_valid(player))
				tmp_entities.push_back(c_entity(player));
		}

		memory.uninitialize_scatter();

		memory.initialize_scatter();
		for (auto& entity : tmp_entities)
			entity.prepare_address();
		memory.dispatch_read(true);

		memory.initialize_scatter();
		for (auto& entity : tmp_entities)
			entity.prepare_a();
		memory.dispatch_read(true);

		memory.initialize_scatter();
		for (auto& entity : tmp_entities)
			entity.prepare_b();
		memory.dispatch_read(true);

		memory.initialize_scatter();
		for (auto& entity : tmp_entities)
			entity.prepare_c();
		memory.dispatch_read(true);

		for (auto& entity : tmp_entities)
		{
			entity.extract_bone_data();
			entity.process_name_decryption();
		}

		entities = tmp_entities;
		g_entities = entities;
	}
};