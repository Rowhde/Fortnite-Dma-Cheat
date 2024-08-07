#include "render.h"
#include <vector>
#include <dwmapi.h>

#include "utilities.h"
#include "globals.h"

#include "entity_list.h"

#pragma region imgui

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};


bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
		return false;
	  initialize_font();
	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

class Weapon
{
public:

	int32_t ammo_count;
	int32_t max_ammo;

	int held_weapon_type;

	BYTE tier;
	std::string weapon_name;
	std::string buildplan;

};
std::string LocalPlayerWeapon;

std::vector<Weapon> Weapon_Act;
std::vector<Weapon> Temp_Weapon;
class item {
public:
	uintptr_t
		Actor;

	std::string
		Name;
	bool
		isVehicle,
		isChest,
		isPickup,
		isAmmoBox;
	float
		distance;


};
std::vector<item> item_pawns;
enum G_INFO : int {
	G_THREAD_FAILED = 0,
	G_INFO_SETUP_SUCCESSFUL = 1
};
int HeldWeaponType;

enum EFortWeaponType : int
{
	Rifle = 0,
	Shotgun = 1,
	Smg = 2,
	Sniper = 3
};
struct WeaponInformation
{
	int32_t ammo_count;
	int32_t max_ammo;

	BYTE tier;
	std::string weapon_name;
	std::string buildplan;

};
std::string LCP;

WeaponInformation WINFO;
	
struct FovVertex
{
	float x, y, z;
	D3DCOLOR color;
	static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};
void drawFOVCircle(float centerX, float centerY, float radius, int segments, c_color color)
{
	std::vector<vertice_t> verts;
	float angle = 0.0f;

	for (int i = 0; i < segments; ++i)
	{
		float x = centerX + radius * cos(angle);
		float y = centerY + radius * sin(angle);

		verts.push_back({ x, y, 0.0f, 1.0f, color.to_int() });

		angle += (2.0f * M_PI) / segments;
	}

	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->SetTexture(0, nullptr);
	device->DrawPrimitiveUP(D3DPT_LINESTRIP, segments - 1, verts.data(), sizeof(vertice_t));
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		
		break;
	}

	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::string DecryptName(__int64 PlayerState, uintptr_t PnameStucture, int pNameLength, uintptr_t pNameEncryptedBuffer)
{
	 
	_WORD* pNameBufferPointer;
	int i;      // ecx
	char v25; // al
	int v26;  // er8
	int v29;  // eax

	char16_t* pNameBuffer;

	
	
	if (pNameLength <= 0)
		return "AI/NONE";

	pNameBuffer = new char16_t[pNameLength];
	
	memory.read_array(pNameEncryptedBuffer, pNameBuffer, pNameLength);

	v25 = pNameLength - 1;
	v26 = 0;
	pNameBufferPointer = (_WORD*)pNameBuffer;

	for (i = (v25) & 3;; *pNameBufferPointer++ += i & 7) {
		v29 = pNameLength - 1;
		if (!(_DWORD)pNameLength)
			v29 = 0;

		if (v26 >= v29)
			break;

		i += 3;
		++v26;
	}

	std::u16string temp_wstring(pNameBuffer);
	delete[] pNameBuffer;
	return std::string(temp_wstring.begin(), temp_wstring.end());
}

void embraceTheDarkness()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
	style.CellPadding = ImVec2(6.00f, 6.00f);
	style.ItemSpacing = ImVec2(6.00f, 6.00f);
	style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 1;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = 1;
	style.TabBorderSize = 1;
	style.WindowRounding = 7;
	style.ChildRounding = 4;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ScrollbarRounding = 9;
	style.GrabRounding = 3;
	style.LogSliderDeadzone = 4;
	style.TabRounding = 4;
}
std::chrono::steady_clock::time_point tb_begin;
std::chrono::steady_clock::time_point tb_end;
bool has_clicked;

int tb_time_since;


static int keystatusss = 0;
static const char* keyNamesssss[] =
{
	"",
	"Left Mouse",
	"Right Mouse",
	"Cancel",
	"Middle Mouse",
	"Mouse 5",
	"Mouse 4",
	"",
	"Backspace",
	"Tab",
	"",
	"",
	"Clear",
	"Enter",
	"",
	"",
	"Shift",
	"Control",
	"Alt",
	"Pause",
	"Caps",
	"",
	"",
	"",
	"",
	"",
	"",
	"Escape",
	"",
	"",
	"",
	"",
	"Space",
	"Page Up",
	"Page Down",
	"End",
	"Home",
	"Left",
	"Up",
	"Right",
	"Down",
	"",
	"",
	"",
	"Print",
	"Insert",
	"Delete",
	"",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"",
	"",
	"",
	"",
	"",
	"Numpad 0",
	"Numpad 1",
	"Numpad 2",
	"Numpad 3",
	"Numpad 4",
	"Numpad 5",
	"Numpad 6",
	"Numpad 7",
	"Numpad 8",
	"Numpad 9",
	"Multiply",
	"Add",
	"",
	"Subtract",
	"Decimal",
	"Divide",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
};
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}
void ChangeKeyyy(void* blank)
{
	keystatusss = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				globals::aimbot::aimbot_key = i;
				keystatusss = 0;
				return;
			}
		}
	}
}
void changetrigger(void* blank)
{
	keystatusss = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				globals::triggerbot::triggerbot_key = i;
				keystatusss = 0;
				return;
			}
		}
	}
}
void HotkeyButtonnnnn(int aimkeyyyyy, void* changekeyyyyy, int statusssss)
{
	const char* preview_value = NULL;
	if (aimkeyyyyy >= 0 && aimkeyyyyy < IM_ARRAYSIZE(keyNamesssss))
		Items_ArrayGetter(keyNamesssss, aimkeyyyyy, &preview_value);

	std::string menukeys;
	if (preview_value == NULL)
		menukeys = "Select Key";
	else
		menukeys = preview_value;

	if (statusssss == 1)
	{
		menukeys = "Press the key";
	}
	if (ImGui::Button(menukeys.c_str(), ImVec2(125, 20)))
	{
		if (statusssss == 0)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)changekeyyyyy, nullptr, 0, nullptr);
		}
	}
}
inline char* wchar_to_char(const wchar_t* pwchar)
{
	
	int currentCharIndex = 0;
	char currentChar = pwchar[currentCharIndex];

	while (currentChar != '\0')
	{
		currentCharIndex++;
		currentChar = pwchar[currentCharIndex];
	}

	const int charCount = currentCharIndex + 1;

	char* filePathC = (char*)malloc(sizeof(char) * charCount);

	for (int i = 0; i < charCount; i++)
	{
		char character = pwchar[i];

		*filePathC = character;

		filePathC += sizeof(char);

	}
	filePathC += '\0';

	filePathC -= (sizeof(char) * charCount);

	return filePathC;
}
static std::string GetNameFromIndex(int key)
{
	
	uint32_t ChunkOffset = (uint32_t)((int)(key) >> 16);
	uint16_t NameOffset = (uint16_t)key;

	uint64_t NamePoolChunk = memory.read<uint64_t>(base_address + 0x11947880 + (8 * ChunkOffset) + 16) + (unsigned int)(2 * NameOffset);
	uint16_t nameEntry = memory.read<uint16_t>(NamePoolChunk);
	int nameLength = nameEntry >> 6;
	char buff[1024] = {};

	char* v2; // rdi
	int v4; // er8
	__int64 result; // rax
	__int64 v6; // rdx
	unsigned int v7; // er8

	v2 = buff;
	v4 = 14;
	result = nameLength;
	if (v4)
	{
		memory.read_rawdog(PVOID(NamePoolChunk + 2), (PVOID)buff, 2 * nameLength);
		v6 = (unsigned int)result;
		do
		{
			v7 = v4 + 45297;
			*v2 = v7 + ~*v2;
			result = v7 << 8;
			v4 = result | (v7 >> 8);
			++v2;
			--v6;
		} while (v6);

		buff[nameLength] = '\0'; // null terminator
		return std::string(buff);
	}
	else
	{
		return std::string("");
	}
}

bool chests = 1;
bool testt_darw_loot = 0;

bool ammo_box = 0;
bool pickups = 1;
bool uncommon = 0;
bool commom = 0;
bool rare = 0;
bool epic = 0;
bool lengendery = 0;
bool mythic = 0;

bool vehicle = 0;
bool Lamma = 0;
int max_distance = 100;
int max_distance1 = 100;
int max_distance2 = 250;
int max_distance3 = 500;
int max_distance4 = 100;

void do_imgui()
{
	embraceTheDarkness();
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	
	ImGui::NewFrame();

	static bool open = true;
	if (GetAsyncKeyState(VK_INSERT) & 0x1)
		open = !open;
	
	if (open)
	{
		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("AIM X - (DMA)", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		if (ImGui::BeginTabBar("Tabs"))
		{
			if (ImGui::BeginTabItem("Aimbot"))
			{
				ImGui::Checkbox("Aimbot", &globals::aimbot::aimbot);
				if (globals::aimbot::aimbot)
				{
					ImGui::SliderFloat("FOV SIZE", &globals::aimbot::aimbot_fov, 1.0f, 500.0f, nullptr, 1.0f);
					ImGui::Checkbox("Draw FOV", &globals::visuals::drawfov);
					ImGui::SliderFloat("Smoothness", &globals::aimbot::smoothing, 0.1f, 100.0f, nullptr, 1.0f);
					ImGui::SliderFloat("KmBox Multiplication", &globals::aimbot::kmbox_times, 1.0f, 5.0f, nullptr, 1.0f);
					ImGui::Checkbox("Prediction NOT FINISHED", &globals::aimbot::prediction);
					ImGui::Text("aimbot key");
					HotkeyButtonnnnn(globals::aimbot::aimbot_key, ChangeKeyyy, keystatusss);
					

				}
				//ImGui::Checkbox("Weapon Configs", &globals::aimbot::weapom_cfg);

				if (globals::aimbot::weapom_cfg)
				{
					globals::aimbot::aimbot = false;
					if (ImGui::BeginTabBar("Weapon Configs Tabs"))
					{
						if (ImGui::BeginTabItem("shotgun"))
						{
							ImGui::Checkbox("Shotgun aimbot", &shotgun::aimbot);
							if (shotgun::aimbot) {
								ImGui::SliderFloat("FOV SIZE", &shotgun::aim_fov, 1.0f, 500.0f, nullptr, 1.0f);
								ImGui::SliderFloat("Smoothness", &shotgun::smooth, 0.1f, 100.0f, nullptr, 1.0f);
								ImGui::Checkbox("Draw FOV", &shotgun::render_fov);

							}
							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("rifle"))
						{
							ImGui::Checkbox("rifle aimbot", &rifle::aimbot);
							if (rifle::aimbot) {
								ImGui::SliderFloat("FOV SIZE", &rifle::aim_fov, 1.0f, 500.0f, nullptr, 1.0f);
								ImGui::SliderFloat("Smoothness", &rifle::smooth, 0.1f, 100.0f, nullptr, 1.0f);
								ImGui::Checkbox("Draw FOV", &rifle::render_fov);

							}
							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("smg"))
						{
							ImGui::Checkbox("smg aimbot", &smg::aimbot);
							if (rifle::aimbot) {
								ImGui::SliderFloat("FOV SIZE", &smg::aim_fov, 1.0f, 500.0f, nullptr, 1.0f);
								ImGui::SliderFloat("Smoothness", &smg::smooth, 0.1f, 100.0f, nullptr, 1.0f);
								ImGui::Checkbox("Draw FOV", &smg::render_fov);

							}
							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("sniper"))
						{
							ImGui::Checkbox("sniper aimbot", &sniper::aimbot);
							if (rifle::aimbot) {
								ImGui::SliderFloat("FOV SIZE", &sniper::aim_fov, 1.0f, 500.0f, nullptr, 1.0f);
								ImGui::SliderFloat("Smoothness", &sniper::smooth, 0.1f, 100.0f, nullptr, 1.0f);
								ImGui::Checkbox("Draw FOV", &sniper::render_fov);

							}
							ImGui::EndTabItem();
						}
						ImGui::EndTabBar();
					}

				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Triggerbot"))
			{
				ImGui::Checkbox("Triggerbot", &globals::triggerbot::triggerbot);
				if (globals::triggerbot::triggerbot)
				{
					//ImGui::Checkbox("shotgun only", &globals::triggerbot::shotgun_only);
					ImGui::SliderFloat("Delay", &globals::triggerbot::custom_delay, 1.0f, 50.0f, nullptr, 1.0f);
					ImGui::SliderFloat("Maximum Distance", &globals::triggerbot::maximum_distance, 1.0f, 300.0f, nullptr, 1.0f);
				//	ImGui::Text("trigger key");
				//	HotkeyButtonnnnn(globals::triggerbot::triggerbot_key, changetrigger, keystatusss);
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Visuals"))
			{
				ImGui::Spacing();
				ImGui::Checkbox("Draw Box", &globals::visuals::box);
				if (globals::visuals::box) {
					ImGui::SliderInt("Box Thickness", &globals::visuals::box_thickness, 0, 6);
				}
				ImGui::Checkbox("Draw Skeleton", &globals::visuals::skeletons);
				ImGui::Checkbox("Draw Distance", &globals::visuals::distancesp);
				ImGui::Checkbox("Draw Name", &globals::visuals::name_esp);
				ImGui::Checkbox("Draw Snapline", &globals::visuals::snap_lines);
				ImGui::Checkbox("Draw Weapon", &globals::visuals::weapon_esp);
				ImGui::Checkbox("Draw Platform", &globals::visuals::draw_platform);
				ImGui::Checkbox("Draw Kills", &globals::visuals::killesp);
				ImGui::Checkbox("Draw Level", &globals::visuals::levelesp);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings"))
			{
				ImGui::Spacing();
				
				

				ImGui::EndTabItem();
			}
			

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}
#pragma endregion imgui
auto InScreen(vec2_t screen_position) -> bool {

	if (screen_position.x > 0 && screen_position.x < globals::resolution_x && screen_position.y > 0 && screen_position.y < globals::resolution_y)
		return true;
	else
		return false;

}
uintptr_t RootComponent(uintptr_t actor)
{

	return memory.read<uintptr_t>(actor + 0x198);
}
vec3_t GetLocation(uintptr_t actor)
{

	return memory.read<vec3_t>(RootComponent(actor) + 0x128);
}

void filled_box(int x, int y, int width, int height, c_color c)
{
	vertice_t pVertex[4] = { { x, y + height, 0.0f, 1.0f, c.to_int() }, { x, y, 0.0f, 1.0f, c.to_int() }, { x + width, y + height, 0.0f, 1.0f, c.to_int() }, { x + width, y, 0.0f, 1.0f, c.to_int() } };
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertex, sizeof(vertice_t));
}

void bordered_box(int x, int y, int width, int height, int thickness, c_color c)
{
	filled_box(x, y, width, thickness, c);
	filled_box(x, y, thickness, height, c);
	filled_box(x + width - thickness, y, thickness, height, c);
	filled_box(x, y + height - thickness, width, thickness, c);
}

void line(vec2_t a, vec2_t b, c_color c)
{
	vertice_t verts[2] =
	{
		{a.x, a.y, 0.0f, 1.0f, c.to_int()},
		{b.x, b.y, 0.0f, 1.0f, c.to_int()}
	};

	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	device->SetTexture(0, nullptr);

	device->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(vertice_t));

	device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}
void line2(ImVec2 a, vec2_t b, c_color c)
{
	vertice_t verts[2] =
	{
		{a.x, a.y, 0.0f, 1.0f, c.to_int()},
		{b.x, b.y, 0.0f, 1.0f, c.to_int()}
	};

	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	device->SetTexture(0, nullptr);

	device->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(vertice_t));
}

void initialize_font()
{
	D3DXCreateFontA(
		device,
		22, // Font size
		0,
		FW_NORMAL, // Regular font weight
		0,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH,
		"Fortnite", // Font name
		&font
	);
}

void text(std::string text, int x, int y, c_color color, bool center)
{
	vec2_t text_size = measure_text(text);

	if (center) {
		x -= text_size.x / 0.5;
	}

	RECT rect;
	SetRect(&rect, x, y, x, y);

	font->DrawTextA(NULL, text.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, color.to_int());
}

vec2_t measure_text(std::string text)
{
	RECT rect;
	font->DrawTextA(0, text.c_str(), text.size(), &rect, DT_CALCRECT, 0);

	return vec2_t{ (float)(rect.right - rect.left), (float)(rect.bottom - rect.top) };
}

/*
static auto Weapon_Cache() -> void {

	for (;; ) {


		Temp_Weapon.clear();


		if (globals::local_pawn) {



			uint64_t player_weapon = memory.read<uint64_t>(globals::local_pawn + offset::Current_Weapon); // 	struct AFortWeapon* CurrentWeapon;

			if (ptr_valid(player_weapon)) {





				uint64_t weapon_data = memory.read<uint64_t>(player_weapon + 0x4a8);	//struct UFortWeaponItemDefinition* WeaponData;


				if (ptr_valid(weapon_data)) {

					uint64_t ftext_ptr = memory.read<uint64_t>(weapon_data + 0x90);

					if (ptr_valid(ftext_ptr)) {
						uint64_t ftext_data = memory.read<uint64_t>(ftext_ptr + 0x28);
						int ftext_length = memory.read<int>(ftext_ptr + 0x30);
						if (ftext_length > 0 && ftext_length < 50) {
							wchar_t* ftext_buf = new wchar_t[ftext_length];

							memory.read_raw(ftext_data, ftext_buf, ftext_length * sizeof(wchar_t));
							std::wstring wstr_buf(ftext_buf);



							LocalPlayerWeapon = std::string(wstr_buf.begin(), wstr_buf.end());

							wchar_t* WeaponName = ftext_buf;

							delete[] ftext_buf;


							


							if (wcsstr(WeaponName, H(L"Assault Rifle")) || wcsstr(WeaponName, H(L"Havoc Suppressed Assault Rifle")) || wcsstr(WeaponName, H(L"Red-Eye Assault Rifle"))
								|| wcsstr(WeaponName, H(L"Suppressed Assault Rifle")) || wcsstr(WeaponName, H(L"Striker Burst Rifle")) || wcsstr(WeaponName, H(L"Burst Assault Rifle"))
								|| wcsstr(WeaponName, H(L"Ranger Assault Rifle")) || wcsstr(WeaponName, H(L"Flapjack Rifle")) || wcsstr(WeaponName, H(L"Heavy Assault Rifle"))
								|| wcsstr(WeaponName, H(L"MK-Seven Assault Rifle")) || wcsstr(WeaponName, H(L"MK-Alpha Assault Rifle")) || wcsstr(WeaponName, H(L"Combat Assault Rifle"))
								|| wcsstr(WeaponName, H(L"Tactical Assault Rifle")) || wcsstr(WeaponName, H(L"Hammer Assault Rifle")) || wcsstr(WeaponName, H(L"Striker AR")) || wcsstr(WeaponName, H(L"Sideways Rifle")) || wcsstr(WeaponName, H(L"AR")) || wcsstr(WeaponName, H(L"Nemesis AR")) || wcsstr(WeaponName, H(L"Makeshift Rifle"))) {
								HeldWeaponType = EFortWeaponType::Rifle;
								if (Util->debug)
								{
									std::cout << "Holding AR" << std::endl;

								}
								if (Universals->weapon_cfg)
								{



									Universals->aimbot = rifle::aimbot;
									Universals->render_field_of_view = rifle::render_fov;
									Universals->field_of_view = rifle::aim_fov;
									Universals->smooth = rifle::smooth;
								}
							}
							if (wcsstr(WeaponName, H(L"Shotgun"))) {

								if (Util->debug)
								{
									std::cout << "Holding Shotgun" << std::endl;

								}
								HeldWeaponType = EFortWeaponType::Shotgun;
								if (Universals->weapon_cfg)
								{

									Universals->aimbot = shotgun::aimbot;
									Universals->render_field_of_view = shotgun::render_fov;
									Universals->field_of_view = shotgun::aim_fov;
									Universals->smooth = shotgun::smooth;
								}


							}
							if (wcsstr(WeaponName, H(L"Smg")) || wcsstr(WeaponName, H(L"Submachine Gun")) || wcsstr(WeaponName, H(L"Hyper SMG")) || wcsstr(WeaponName, H(L"Thunder Burst SMG")) || wcsstr(WeaponName, H(L"SMG")) || wcsstr(WeaponName, H(L"Combat Smg")) || wcsstr(WeaponName, H(L"Ranger Pistol")) || wcsstr(WeaponName, H(L"Pistol"))) {
								HeldWeaponType = EFortWeaponType::Smg;
								if (Util->debug)
								{
									std::cout << H("Holding SMG") << std::endl;

								}
								if (Universals->weapon_cfg)
								{

									Universals->aimbot = smg::aimbot;
									Universals->render_field_of_view = smg::render_fov;
									Universals->field_of_view = smg::aim_fov;
									Universals->smooth = smg::smooth;
								}


							}
							if (wcsstr(WeaponName, H(L"Hunting Rifle")) || wcsstr(WeaponName, H(L"Heavy Sniper Rifle")) || wcsstr(WeaponName, H(L"Suppressed Sniper Rifle"))
								|| wcsstr(WeaponName, H(L"Storm Scout")) || wcsstr(WeaponName, H(L"Bolt-Action Sniper Rifle")) || wcsstr(WeaponName, H(L"Automatic Sniper Rifle"))
								|| wcsstr(WeaponName, H(L"DMR")) || wcsstr(WeaponName, H(L"Thermal DMR")) || wcsstr(WeaponName, H(L"Hunter Bolt-Action Sniper"))
								|| wcsstr(WeaponName, H(L"Crossbow")) || wcsstr(WeaponName, H(L"Reaper Sniper Rifle")) || wcsstr(WeaponName, H(L"Mechanical Bow"))) {
								HeldWeaponType = EFortWeaponType::Sniper;
								if (Util->debug)
								{
									std::cout << "Holding SNIPER" << std::endl;

								}
								if (Universals->weapon_cfg)
								{

									Universals->aimbot = sniper::aimbot;
									Universals->render_field_of_view = sniper::render_fov;
									Universals->field_of_view = sniper::aim_fov;
									Universals->smooth = sniper::smooth;

								}

							}
						}
					}
				}
			}
		}

		Weapon cached_info{};

		Temp_Weapon.push_back(cached_info);
	}
	Weapon_Act.clear();
	Weapon_Act = Temp_Weapon;
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
*/

float WorldGravityZ = -2800.0f;
uintptr_t NetConnection = 0x518; /** The net connection this controller is communicating on, nullptr for local players on server */
uintptr_t RotationInput = NetConnection + 0x8; //size of NetConnection (0x8)

bool memory_event(FVector newpos)
{
	memory.write<FVector>(globals::local_player_controller + RotationInput, newpos); //write Vectors to control rotation
	return true;
}


void moveto(vec2_t target, int smooth)
{
	
	vec2_t center;
	center.x = globals::resolution_x / 2;
	center.y = globals::resolution_y / 2;

	

	if (target.x != 0)
	{
		if (target.x > center.x)
		{
			target.x = -(center.x - target.x);
			target.x /= smooth;
			if (target.x + center.x > center.x * 2)
				target.x = 0;
		}

		if (target.x < center.x)
		{
			target.x = target.x - center.x;
			target.x /= smooth;
			if (target.x + center.x < 0)
				target.x = 0;
		}
	}
	if (target.y != 0)
	{
		if (target.y > center.y)
		{
			target.y = -(center.y - target.y);
			target.y /= smooth;
			if (target.y + center.y > center.y * 2)
				target.y = 0;
		}

		if (target.y < center.y)
		{
			target.y = target.y - center.y;
			target.y /= smooth;
			if (target.y + center.y < 0)
				target.y = 0;
		}
	}
	memory_event(FVector(-target.y / 5, target.x / 5, 0));
}
float CalculateElevationDifference(const vec3_t& localplayer, const vec3_t& target) {
	return localplayer.z - target.z; // Positive if shooter is above, negative if below
}
struct mouse_
{
	void mouse_aim(double x, double y, int smooth)
	{
		float ScreenCenterX = globals::resolution_x / 2;
		float ScreenCenterY = globals::resolution_y / 2;
		int AimSpeed = smooth;
		float TargetX = 0;
		float TargetY = 0;

		if (x != 0)
		{
			if (x > ScreenCenterX)
			{
				TargetX = -(ScreenCenterX - x);
				TargetX /= AimSpeed;
				if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
			}

			if (x < ScreenCenterX)
			{
				TargetX = x - ScreenCenterX;
				TargetX /= AimSpeed;
				if (TargetX + ScreenCenterX < 0) TargetX = 0;
			}
		}

		if (y != 0)
		{
			if (y > ScreenCenterY)
			{
				TargetY = -(ScreenCenterY - y);
				TargetY /= AimSpeed;
				if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
			}

			if (y < ScreenCenterY)
			{
				TargetY = y - ScreenCenterY;
				TargetY /= AimSpeed;
				if (TargetY + ScreenCenterY < 0) TargetY = 0;
			}
		}

		
			mouse_event(MOUSEEVENTF_MOVE, TargetX, TargetY, NULL, NULL);
		

		return;
	}
}; std::unique_ptr<mouse_> mouse = std::make_unique<mouse_>();


vec3_t PredictLocation(vec3_t target, vec3_t targetVelocity, float projectileSpeed, float projectileGravityScale, float distance)
{
	float horizontalTime = distance / projectileSpeed;
	float verticalTime = distance / projectileSpeed;

	target.x += targetVelocity.x * horizontalTime;
	target.y += targetVelocity.y * horizontalTime;
	target.z += targetVelocity.z * verticalTime +
		abs(WorldGravityZ * projectileGravityScale) * 0.5f * (verticalTime * verticalTime);

	return target;
}
void aimbot()
{
	if (!globals::aimbot::aimbot)
		return;

	float best_fov = FLT_MAX;
	c_entity best_target = c_entity(0);
	float projectileSpeed = 0;
	float projectileGravityScale = 0;

	uint64_t root_component;
	vec3_t Velocity;
	for (auto& entity : g_entities)
	{
		if (!ptr_valid(entity.pawn) || !ptr_valid(entity.mesh))
			continue;

		if (entity.team_index == globals::local_team)
			continue;

		if (globals::visuals::visible_check && !entity.is_visible())
			continue;
		if (globals::aimbot::prediction) {
			root_component = memory.read<uint64_t>(entity.pawn + 0x198);

			Velocity = memory.read<vec3_t>(root_component + 0x168);
		}
		

		vec3_t hitbox = entity.head_pos;

		vec2_t screen_pos = utilities::world_to_screen(hitbox);
		if (vec2_invalid(screen_pos))
			continue;

		float distance = utilities::distance_from_crosshair(screen_pos);
		if (distance < best_fov)
		{
			best_fov = distance;
			best_target = entity;
		}
	}
	vec3_t hitbox = best_target.head_pos;
	
	//uintptr_t currentweapon = memory.read<uintptr_t>(globals::local_pawn + 0xa20);
	//float ProjectileSpeed = memory.read<float>(currentweapon + 0x174);

 //   PredictLocation(globals::location, best_target.head_pos, idk, ProjectileSpeed)
	//

	//

	//vec2_t hitbox_screen = utilities::world_to_screen(hitbox);



	//
	if (globals::aimbot::prediction) {
		
			projectileSpeed = 50000.0f;
			projectileGravityScale = 0.12f;
		
	}
	if (projectileSpeed) {
		hitbox = PredictLocation(hitbox, Velocity, projectileSpeed, projectileGravityScale, globals::location.Distance(hitbox)); //dmr
	}
	

	vec2_t screen_pos = utilities::world_to_screen(hitbox);

	int center_x = globals::resolution_x / 2;
	int center_y = globals::resolution_y / 2;

	float fov_val = globals::aimbot::aimbot_fov;

	if (best_fov > fov_val)
		return;

	bool aimbot_key = keyboard.key_down(globals::aimbot::aimbot_key);	
	if (!aimbot_key)
		return;

	

	
	
	
	if (vec2_invalid(screen_pos))
		return;

	

	if (globals::aimbot::kmbox)
	{
		bool needs_left = screen_pos.x < center_x;
		bool needs_down = screen_pos.y < center_y;

		float angle = std::atan2(screen_pos.y - center_y, screen_pos.x - center_x);
		float distance = std::sqrt(std::pow(screen_pos.x - center_x, 2) + std::pow(screen_pos.y - center_y, 2));
		float scaling_factor_x = globals::aimbot::smoothing * globals::aimbot::kmbox_times;
		float scaling_factor_y = globals::aimbot::smoothing * globals::aimbot::kmbox_times;

		int amount_to_move_x = static_cast<int>(std::round(std::abs(distance * std::cos(angle)) / scaling_factor_x));
		int amount_to_move_y = static_cast<int>(std::round(std::abs(distance * std::sin(angle)) / scaling_factor_y));

		if (amount_to_move_x < 1)
			amount_to_move_x = 1;

		if (amount_to_move_y < 1)
			amount_to_move_y = 1;

		if (abs(screen_pos.x - center_x) > 4)
		{
			if (needs_left)
			{
				globals::should_move_x = true;
				globals::move_x = -amount_to_move_x;
			}
			else
			{
				globals::should_move_x = true;
				globals::move_x = amount_to_move_x;
			}
		}

		if (abs(screen_pos.y - center_y) > 4)
		{
			if (needs_down)
			{
				globals::should_move_y = true;
				globals::move_y = amount_to_move_y;
			}
			else
			{
				globals::should_move_y = true;
				globals::move_y = -amount_to_move_y;
			}
		}
	}
	else if (globals::aimbot::memoryaim) {
		moveto(screen_pos, globals::aimbot::smoothing);
	}
	else if (globals::aimbot::mouseaim)
	{
		mouse->mouse_aim(screen_pos.x, screen_pos.y, globals::aimbot::smoothing);
	}
	
}
auto IsShootable(vec3_t lur, vec3_t wl) -> bool {

	if (lur.x >= wl.x - 20 && lur.x <= wl.x + 20 && lur.y >= wl.y - 20 && lur.y <= wl.y + 20 && lur.z >= wl.z - 30 && lur.z <= wl.z + 30)
		return true;
	else
		return false;

}

void render_items()
{
	if (!ptr_valid(globals::local_player_controller))
		return;

	utilities::update_camera_data();


	

	aimbot();

	for (auto& entity : g_entities)
	{
		if (!ptr_valid(entity.pawn) || !ptr_valid(entity.mesh))
			continue;

		if (entity.team_index == globals::local_team)
			continue;

		vec2_t screen_base = utilities::world_to_screen(entity.base_pos);

		vec3_t head_real = entity.head_pos;
		head_real.z += 12.0f;

		vec2_t screen_top = utilities::world_to_screen(head_real);

		vec2_t screen_head = utilities::world_to_screen(entity.head_pos);

		auto distance = globals::location.Distance(entity.base_pos) / 100.0f;

		

		if (vec2_invalid(screen_base) && vec2_invalid(screen_top))
			continue;

		int box_height = screen_base.y - screen_top.y;
		int box_width = box_height / 2.15;

		
		c_color esp_color = c_color(204, 40, 237);

		if (globals::visuals::visible_check)
			if (entity.is_visible()) {
				esp_color = c_color(229, 247, 64);
			}
		    if (!entity.is_visible()) {
			    esp_color = c_color(204, 40, 237);
		    }
	
		

		if (globals::visuals::distancesp)
		{
			std::string dist_str = "[" + std::to_string(static_cast<int>(distance)) + "m]";
			vec2_t text_size = measure_text(dist_str);

			text_size.x /= 2; // Adjust for centering
			
				text(dist_str, static_cast<int>(screen_base.x - text_size.x), static_cast<int>(screen_base.y + 5), esp_color, false);
			
			

		}

		if (globals::visuals::box)
			bordered_box(screen_top.x - box_width / 2, screen_top.y, box_width, box_height, globals::visuals::box_thickness , esp_color);

		if (globals::visuals::name_esp) {

			vec2_t text_size = measure_text(entity.formatted_name);

			text_size.x /= 2; // Adjust for centering

			text(entity.formatted_name, static_cast<int>(screen_top.x - text_size.x), static_cast<int>(screen_top.y - text_size.y - 5), esp_color, false);
		}

			

		if (globals::visuals::snap_lines) {
			ImVec2 middleOfScreen = ImVec2(globals::resolution_x / 2, globals::resolution_y / 2);

			line2(middleOfScreen, screen_head, esp_color);
		}
			
		
		if (globals::visuals::drawfov) {
			

			drawFOVCircle(globals::resolution_x / 2, globals::resolution_y / 2, globals::aimbot::aimbot_fov, 50, c_color(204, 40, 237));  // Centered at the screen center, radius 50, 50 segments, red color

			
		}
		
		//if (globals::exploit::noreload) {
		//	

		//	

		//		bool bIsReloadingWeapon = memory.read<bool>(0xa20 + 0x360);
		//		uintptr_t Mesh = memory.read<uintptr_t>(globals::local_pawn + 0x318);

		//		
		//			memory.write<float>(Mesh + 0xa68, 999);
		//		
		//		

		//	

		//		
		//	
		//}
		//if (globals::exploit::freeze)
		//{
		//	memory.write<bool>(entity.local_player_weapon + 0x12C9, true);
		//}
		//if (globals::exploit::bigplayer) {
		//	

		//	
		//	memory.write<vec3_t>(entity.mesh + 0x158, { 3, 3, 3 });
		//	memory.write<vec3_t>(entity.mesh + 0x158, { 3, 3, 3 });
		//	memory.write<vec3_t>(entity.mesh + 0x158, { 3, 3, 3 });
		//	
		//}
		if (globals::visuals::levelesp) {
			

			std::string ud = "Level [" + std::to_string(static_cast<int>(entity.level)) + "]";
			vec2_t text_size = measure_text(ud);

			text_size.x /= 2; // Adjust for centering

			
				text(ud, static_cast<int>(screen_base.x - text_size.x), static_cast<int>(screen_base.y + 50), esp_color, false);
			
		}

		if (globals::visuals::killesp) {


			std::string killsud = "Kills [" + std::to_string(static_cast<int>(entity.kills)) + "]";
			vec2_t text_size = measure_text(killsud);

			text_size.x /= 2; // Adjust for centering
			
				text(killsud, static_cast<int>(screen_base.x - text_size.x), static_cast<int>(screen_base.y + 35), esp_color, false);
			

			
		}

		if (globals::triggerbot::triggerbot) {

				POINT cursor;
				GetCursorPos(&cursor);
				if (IsShootable(memory.read<vec3_t>(globals::local_player_controller + 0x2560), entity.head_pos)) {   // LocationUnderReticle       //in fortnitegame_classes
					if (distance <= globals::triggerbot::maximum_distance) {
						/*bool aimbot_key = keyboard.key_down(globals::triggerbot::triggerbot_key);
						if (!aimbot_key)
							return;*/

						if (has_clicked) {
							tb_begin = std::chrono::steady_clock::now();
							has_clicked = 0;
						}
						tb_end = std::chrono::steady_clock::now();
						tb_time_since = std::chrono::duration_cast<std::chrono::milliseconds>(tb_end - tb_begin).count();
						if (tb_time_since >= globals::triggerbot::custom_delay) {
	
							mouse_event(MOUSEEVENTF_LEFTDOWN, DWORD(NULL), DWORD(NULL), DWORD(0x0002), ULONG_PTR(NULL));
							mouse_event(MOUSEEVENTF_LEFTUP, DWORD(NULL), DWORD(NULL), DWORD(0x0004), ULONG_PTR(NULL));
							has_clicked = 1;
						}
					}
				}
		}

		if (globals::aimbot::weapom_cfg)
	{ 
		

		if (ptr_valid(entity.local_player_weapon)) {





			//struct UFortWeaponItemDefinition* WeaponData;


			if (ptr_valid(entity.local_weapon_data)) {

				

				if (ptr_valid(entity.local_ftext_ptr)) {
					uint64_t ftext_data = memory.read<uint64_t>(entity.local_ftext_ptr + 0x28);
					int ftext_length = memory.read<int>(entity.local_ftext_ptr + 0x30);
					if (ftext_length > 0 && ftext_length < 50) {
						wchar_t* ftext_buf = new wchar_t[ftext_length];
						memory.read_raw(ftext_data, ftext_buf, ftext_length * sizeof(wchar_t));




						wchar_t* WeaponName = ftext_buf;
						LocalPlayerWeapon = *WeaponName;


						delete[] ftext_buf;


						if (wcsstr(WeaponName, (L"Assault Rifle")) || wcsstr(WeaponName, (L"Havoc Suppressed Assault Rifle")) || wcsstr(WeaponName, (L"Red-Eye Assault Rifle"))
							|| wcsstr(WeaponName, (L"Suppressed Assault Rifle")) || wcsstr(WeaponName, (L"Striker Burst Rifle")) || wcsstr(WeaponName, (L"Burst Assault Rifle"))
							|| wcsstr(WeaponName, (L"Ranger Assault Rifle")) || wcsstr(WeaponName, (L"Flapjack Rifle")) || wcsstr(WeaponName, (L"Heavy Assault Rifle"))
							|| wcsstr(WeaponName, (L"MK-Seven Assault Rifle")) || wcsstr(WeaponName, (L"MK-Alpha Assault Rifle")) || wcsstr(WeaponName, (L"Combat Assault Rifle"))
							|| wcsstr(WeaponName, (L"Tactical Assault Rifle")) || wcsstr(WeaponName, (L"Hammer Assault Rifle")) || wcsstr(WeaponName, (L"Sideways Rifle")) || wcsstr(WeaponName, (L"Makeshift Rifle"))) {
							HeldWeaponType = EFortWeaponType::Rifle;
							if (globals::aimbot::weapom_cfg)
							{

								globals::aimbot::aimbot = rifle::aimbot;
								globals::visuals::drawfov = rifle::render_fov;
								globals::aimbot::aimbot_fov = rifle::aim_fov;
								globals::aimbot::smoothing = rifle::smooth;
							}
						}
						if (wcsstr(WeaponName, (L"Shotgun"))) {
							HeldWeaponType = EFortWeaponType::Shotgun;
							if (globals::aimbot::weapom_cfg)
							{

								globals::aimbot::aimbot = shotgun::aimbot;
								globals::visuals::drawfov = shotgun::render_fov;
								globals::aimbot::aimbot_fov = shotgun::aim_fov;
								globals::aimbot::smoothing = shotgun::smooth;
							}


						}
						if (wcsstr(WeaponName, (L"Smg")) || wcsstr(WeaponName, (L"Submachine Gun")) || wcsstr(WeaponName, (L"Combat Smg")) || wcsstr(WeaponName, (L"Pistol"))) {
							HeldWeaponType = EFortWeaponType::Smg;
							if (globals::aimbot::weapom_cfg)
							{

								globals::aimbot::aimbot = smg::aimbot;
								globals::visuals::drawfov = smg::render_fov;
								globals::aimbot::aimbot_fov = smg::aim_fov;
								globals::aimbot::smoothing = smg::smooth;
							}


						}
						if (wcsstr(WeaponName, (L"Hunting Rifle")) || wcsstr(WeaponName, (L"Heavy Sniper Rifle")) || wcsstr(WeaponName, (L"Suppressed Sniper Rifle"))
							|| wcsstr(WeaponName, (L"Storm Scout")) || wcsstr(WeaponName, (L"Bolt-Action Sniper Rifle")) || wcsstr(WeaponName, (L"Automatic Sniper Rifle"))
							|| wcsstr(WeaponName, (L"DMR")) || wcsstr(WeaponName, (L"Thermal DMR")) || wcsstr(WeaponName, (L"Hunter Bolt-Action Sniper"))
							|| wcsstr(WeaponName, (L"Crossbow")) || wcsstr(WeaponName, (L"Mechanical Bow"))) {
							HeldWeaponType = EFortWeaponType::Sniper;
							if (globals::aimbot::weapom_cfg)
							{

								globals::aimbot::aimbot = sniper::aimbot;
								globals::visuals::drawfov = sniper::render_fov;
								globals::aimbot::aimbot_fov = sniper::aim_fov;
								globals::aimbot::smoothing = sniper::smooth;

							}

						}
					}
				}
			}
		}
		}

		int offsetplatform = 15;
		if (globals::visuals::draw_platform) {
			
			

			// Use read_raw with the correct type and size
			if (entity.platform) {
				std::wstring balls_sus(entity.platform);
				std::string str_platform(balls_sus.begin(), balls_sus.end());
				vec2_t DistanceTextSize2 = measure_text(str_platform.c_str());

				if (str_platform.find(("XBL")) != std::string::npos || str_platform.find(("XSX")) != std::string::npos) {
					char XboxText[] = "Xbox";

					vec2_t DistanceTextSize2 = measure_text(XboxText);

					DistanceTextSize2.x /= 2;


					text(XboxText, static_cast<int>(screen_top.x - DistanceTextSize2.x), static_cast<int>(screen_top.y - DistanceTextSize2.y - 20), c_color(0, 255, 0), false);
				}
				else if (str_platform.find(("PSN")) != std::string::npos || str_platform.find(("PS5")) != std::string::npos) {
					char PSNText[] = "PlayStation";

					vec2_t DistanceTextSize2 = measure_text(PSNText);
					DistanceTextSize2.x /= 2;
					text(PSNText, static_cast<int>(screen_top.x - DistanceTextSize2.x), static_cast<int>(screen_top.y - DistanceTextSize2.y - 20), c_color(0, 0, 255), false);
				}
				else if (str_platform.find(("WIN")) != std::string::npos) {
					char WINText[] = "Windows";

					vec2_t DistanceTextSize2 = measure_text(WINText);
					DistanceTextSize2.x /= 2;
					text(WINText, static_cast<int>(screen_top.x - DistanceTextSize2.x), static_cast<int>(screen_top.y - DistanceTextSize2.y - 20), c_color(255, 255, 255), false);
				}
				else if (str_platform.find(("SWH")) != std::string::npos || str_platform.find(("SWT")) != std::string::npos) {
					char SWHText[] = "Switch";

					vec2_t DistanceTextSize2 = measure_text(SWHText);
					DistanceTextSize2.x /= 2;
					text(SWHText, static_cast<int>(screen_top.x - DistanceTextSize2.x), static_cast<int>(screen_top.y - DistanceTextSize2.y - 20), c_color(255, 0, 0), false);
				}
				else {
					DistanceTextSize2.x /= 2;
					
				    text(str_platform.c_str(), static_cast<int>(screen_top.x - DistanceTextSize2.x), static_cast<int>(screen_top.y - DistanceTextSize2.y - 20), esp_color, false);
				}
			}
		}

		

		if (globals::visuals::skeletons)
		{
			vec2_t screen_right_hand = utilities::world_to_screen(entity.right_hand_pos);
			vec2_t screen_right_collar_bone = utilities::world_to_screen(entity.right_collar_bone_pos);
			vec2_t screen_left_hand = utilities::world_to_screen(entity.left_hand_pos);
			vec2_t screen_left_collar_bone = utilities::world_to_screen(entity.left_collar_bone_pos);
			vec2_t screen_chest = utilities::world_to_screen(entity.chest_pos);
			vec2_t screen_spine2 = utilities::world_to_screen(entity.spine2_pos);
			vec2_t screen_spine1 = utilities::world_to_screen(entity.spine1_pos);
			vec2_t screen_right_thigh = utilities::world_to_screen(entity.right_thigh_pos);
			vec2_t screen_left_thigh = utilities::world_to_screen(entity.left_thigh_pos);
			vec2_t screen_right_calf = utilities::world_to_screen(entity.right_calf_pos);
			vec2_t screen_left_calf = utilities::world_to_screen(entity.left_calf_pos);

			

			

			

			line(screen_right_collar_bone, screen_right_hand, esp_color);
			line(screen_left_collar_bone, screen_left_hand, esp_color);
			line(screen_chest, screen_left_collar_bone, esp_color);
			line(screen_chest, screen_right_collar_bone, esp_color);
			line(screen_head, screen_chest, esp_color);
			line(screen_spine2, screen_chest, esp_color);
			line(screen_spine1, screen_spine2, esp_color);
			line(screen_spine1, screen_right_thigh, esp_color);
			line(screen_spine1, screen_left_thigh, esp_color);
			line(screen_right_thigh, screen_right_calf, esp_color);
			line(screen_left_thigh, screen_left_calf, esp_color);
		}

		if (globals::visuals::weapon_esp)
		{
			if (entity.pawn)
			{
				WeaponInformation held_weapon{};

				

				if (ptr_valid(entity.player_weapon)) {

					

					if (ptr_valid(entity.weapon_data)) {
						held_weapon.tier = entity.weapontier;

						

						if (ptr_valid(entity.ftext_ptr)) {
							
							if (entity.ftext_length > 0 && entity.ftext_length < 50) {
								wchar_t* ftext_buf = new wchar_t[entity.ftext_length];

								memory.read_raw(entity.ftext_data, ftext_buf, entity.ftext_length * sizeof(wchar_t));
								std::wstring wstr_buf(ftext_buf);

								held_weapon.weapon_name = std::string(wstr_buf.begin(), wstr_buf.end());

								delete[] ftext_buf;
							}
						}

					}

					WINFO = held_weapon;


					std::string final = ("[") + WINFO.weapon_name + ("]");

					vec2_t TextSize = measure_text(final.c_str());

					vec2_t text_size = measure_text(final.c_str());
					int add;
					if (globals::visuals::distancesp)
					{
						add = 20;
					}
					else
					{
						add = 5;
					}

					if (WINFO.tier == 0)
					{
						if (strstr(WINFO.weapon_name.c_str(), ("Pickaxe")) != nullptr)
						{
							
								text(final.c_str(), screen_base.x - (text_size.x / 2), screen_base.y + 20, c_color(211, 227, 215), false);
							
						}
						else
						{
							std::string fina1l = ("[Building Plan]");

							vec2_t text_size1 = measure_text(fina1l.c_str());

							
								text(fina1l.c_str(), screen_base.x - (text_size1.x / 2), screen_base.y + 20, c_color(211, 227, 215), false);
							
						}
					}
					if (WINFO.tier == 1)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(170, 165, 169), false);

					}
					if (WINFO.tier == 2)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(30, 255, 0), false);
		

					}
					if (WINFO.tier == 3)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(0, 112, 221), false);
			

					}
					if (WINFO.tier == 4)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(163, 53, 238), false);
			

					}
					if (WINFO.tier == 5)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(255, 128, 0), false);
						

					}
					if (WINFO.tier == 6)
					{
						text(final.c_str(), (screen_base.x) - (TextSize.x / 2), (screen_base.y + 20), c_color(255, 255, 0), false);
					

					}

				}
			}
		}
	}
}
HWND hwnd;
auto hijack() -> bool {
	

	hwnd = FindWindowA(("MedalOverlayClass"), ("MedalOverlay")); //CiceroUIWndFrame

	if (!hwnd)
	{
		system("cls");

		Sleep(500);
		MessageBoxA(0, ("{!} Overlay Failed. Contact Support with error {3}"), (""), MB_ICONINFORMATION);
		Sleep(1000);
		system("cls");


	}

	int WindowWidth = GetSystemMetrics(SM_CXSCREEN);
	int WindowHeight = GetSystemMetrics(SM_CYSCREEN);

	if (!SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, WindowWidth, WindowHeight, SWP_SHOWWINDOW))
	{
		return false;
	}

	if (!SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA))
	{
		return false;
	}

	if (!SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT))
	{
		return false;
	}

	MARGINS Margin = { -1 };
	if (FAILED(DwmExtendFrameIntoClientArea(hwnd, &Margin)))
	{
		return false;
	}

	ShowWindow(hwnd, SW_SHOW);

	UpdateWindow(hwnd);

	ShowWindow(hwnd, SW_HIDE);

	return true;
}

// Call this function at the beginning of your program or rendering loop
std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime;
int frameCount = 0;

// Call this function at the beginning of your program or rendering loop
void InitializeFPSCounter() {
	lastFrameTime = std::chrono::high_resolution_clock::now();
}

// Call this function in your rendering loop to update the FPS counter
std::string UpdateFPSCounter() {
	auto currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsedSeconds = currentTime - lastFrameTime;

	// Calculate FPS
	double fps = 1.0 / elapsedSeconds.count();

	// Increment the frame count and update last frame time
	frameCount++;
	lastFrameTime = currentTime;

	// Convert FPS to string
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(2) << fps;
	return ss.str();
}

// Call this function to draw the FPS counter on the screen
void DrawFPSCounter(std::string fpsString) {
	// Assuming you have a valid font and position to draw the text
	// Customize the position, font, and color according to your requirements
	// For simplicity, let's assume you have an ImGui-like library for text rendering

	
	text(fpsString, 10, 10, c_color(255, 255, 255), false);
}
bool doOnce = true;
void window_thread()
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = wndproc;
	wc.hInstance = GetModuleHandleA(0);
	wc.hCursor = 0l;
	wc.hbrBackground = (HBRUSH)RGB(0, 0, 0);
	wc.lpszClassName = (LPSTR)"hihihihiA12";

	RegisterClassEx(&wc);

	// Use WS_OVERLAPPEDWINDOW style for a resizable and movable window
	hwnd = ::CreateWindowA(wc.lpszClassName, (LPSTR)"hihihihiA13", WS_OVERLAPPEDWINDOW, 0, 0, globals::resolution_x, globals::resolution_y, NULL, NULL, wc.hInstance, NULL);
	//hijack();

	ShowWindow(hwnd, SW_SHOW);

	if ((d3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return;

	ZeroMemory(&pp, sizeof(pp));
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	pp.AutoDepthStencilFormat = D3DFMT_D16;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.EnableAutoDepthStencil = TRUE;
	pp.Windowed = TRUE;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device) < 0)
		return;

	::UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(device);


	static bool did_initialize = false;
	static bool done = false;

	//InitializeFPSCounter();

	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}

		if (!did_initialize)
		{
			initialize_font();

			did_initialize = true;
		}

		if (done)
			break;

		device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

		device->BeginScene();

		
		/*if (doOnce) {
			std::thread([&]() { for (;; ) { CacheLevels(); } }).detach();
		}*/
		
		

		do_imgui();

		render_items();

		//LevelDrawing();

		device->EndScene();
		device->Present(NULL, NULL, NULL, NULL);
	}
}