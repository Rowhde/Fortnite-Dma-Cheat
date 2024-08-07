#include "input.h"

#include "render.h"
#include "globals.h"

#include "entity_list.h"
#include "utilities.h"
#include "auth.hpp"

#include "lazy.h"
#include <Windows.h>
#include <setupapi.h>
#include <devguid.h>
uint64_t game_state;
int error(const char* message)
{
	printf("%s", message);

	VMMDLL_CloseAll();

	system("pause");

	return 0;
}

int input_thread()
{
	while (!GetAsyncKeyState(VK_END))
	{
		if (globals::should_move_x)
		{
			kmbox::move_x(globals::move_x);

			globals::should_move_x = false;
		}

		if (globals::should_move_y)
		{
			kmbox::move_y(globals::move_y);

			globals::should_move_y = false;
		}
		if (globals::should_click)
		{
			kmbox::left_click();

			kmbox::release_left_click();

			globals::should_click = false;

		}
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}

	return 0;
}
void loadoffsets(uint64_t base) {

	for (;;)
	{
		uint64_t gworld = memory.read<uint64_t>(base + offsets::gworld);
		if (!ptr_valid(gworld))
			continue;

		globals::gworld = gworld;

		game_state = memory.read<uint64_t>(gworld + offsets::game_state);
		if (!ptr_valid(game_state))
			continue;

		

		uint64_t game_instance = memory.read<uint64_t>(gworld + offsets::game_instance);
		if (!ptr_valid(game_instance))
			continue;

		


		uint64_t local_players = memory.read<uint64_t>(game_instance + offsets::local_players);
		if (!ptr_valid(local_players))
			continue;



		uint64_t local_player = memory.read<uint64_t>(local_players);
		if (!ptr_valid(local_player))
			continue;

		

		uint64_t player_controller = memory.read<uint64_t>(local_player + offsets::player_controller);
		if (!ptr_valid(player_controller))
			continue;

		globals::local_player_controller = player_controller;


		uint64_t pawn = memory.read<uint64_t>(player_controller + offsets::pawn);

		uint64_t local_state = memory.read<uint64_t>(pawn + offsets::player_state);

		globals::local_team = memory.read<int>(local_state + offsets::team_index);
	}
	Sleep(5000);
}
std::string GetCOMPortByDescription(const std::string& targetDescription) {
	HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return "";

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); ++i) {
		char buf[512];
		DWORD nSize = 0;

		if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)buf, sizeof(buf), &nSize) && nSize > 0) {
			buf[nSize] = '\0';
			std::string deviceDescription = buf;

			size_t comPos = deviceDescription.find("COM");
			size_t endPos = deviceDescription.find(")", comPos);

			if (comPos != std::string::npos && endPos != std::string::npos && deviceDescription.find(targetDescription) != std::string::npos) {
				SetupDiDestroyDeviceInfoList(hDevInfo);
				return deviceDescription.substr(comPos, endPos - comPos);
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return "";
}
int main()
{
 
	/*KeyAuthApp.init();
	if (!KeyAuthApp.data.success) {
		std::cout << (" Auth Error: Failed to init") << std::endl;
		LI_FN(Sleep)(2000);
		return 0;
	}
	KeyAuthApp.check();
	std::cout << ("\n  Auth Status: ") << KeyAuthApp.data.message << std::endl;
	LI_FN(Sleep)(2000);
	LI_FN(system)("CLS");
	std::cout << ("\n Enter Key: ");
	std::string key;
	std::cin >> key;
	KeyAuthApp.license(key);
	if (!KeyAuthApp.data.success)
	{
		std::cout << (" Status: ") << KeyAuthApp.data.message;
		LI_FN(Sleep)(1500);
		exit(0);
	}
	KeyAuthApp.check();
	if (!KeyAuthApp.data.success)
	{
	
		std::cout << ("\n Status: ") << KeyAuthApp.data.message;
		LI_FN(Sleep)(1500);
		exit(0);
	}
	LI_FN(Sleep)(2000);
	LI_FN(system)("CLS");

	LI_FN(SetConsoleTitleA)(("Aim-X (DMA)"));
	KeyAuthApp.check();
	if (!KeyAuthApp.data.success)
	{
		std::cout << ("\n Status: ") << KeyAuthApp.data.message;
		(Sleep)(1500);
		exit(0);
	}*/
	
	system("cls");
	utilities::get_desktop_resolution(globals::resolution_x, globals::resolution_y);

	//int portnum;
	//std::wstring PORT;

	//logger::info("Enter port number:\n");
	//logger::info(">\n");

	//std::cin >> portnum;

	//// Convert port number to string and concatenate with "COM"
	//PORT = L"COM" + std::to_wstring(portnum);

	//// Convert wide string to narrow string
	//int bufferSize = WideCharToMultiByte(CP_ACP, 0, PORT.c_str(), -1, NULL, 0, NULL, NULL);
	//std::string narrowPort(bufferSize, 0);
	//WideCharToMultiByte(CP_ACP, 0, PORT.c_str(), -1, &narrowPort[0], bufferSize, NULL, NULL);

	//// Now you can use narrowPort.c_str() as LPCSTR
	//kmbox::initialize(narrowPort.c_str());

	//logger::info("kmbox connection result -> %d\n", kmbox::connected);

	
	std::string targetDescription = "USB-SERIAL CH340";

	std::string comPort = GetCOMPortByDescription(targetDescription);

	if (!comPort.empty()) {
		std::cout << "Found COM port: " << comPort << std::endl;

		// Now you can use kmbox::initialize with the discovered COM port number
		
	}
	else {
		std::cout << "COM port not found for description: " << targetDescription << std::endl;
	}

	kmbox::initialize(comPort.c_str());

	std::thread inthread = std::thread(input_thread);

	
	
	std::vector<const char*> args = { "", "-device", "FPGA" };

	c_device device = c_device(args);

	logger::info("attempting to connect to device...\n");

	if (!device.connect())
		return device.error("failed to connect to device\n");
	else
		logger::info("connected to device, id -> %lli | version -> %lli.%lli\n\n", device.id, device.major_version, device.minor_version);

	c_process process = device.process_from_name("FortniteClient-Win64-Shipping.exe");
	if (process.failed)
		return device.error("failed to find fortnite\n");
	else
		logger::info("found fortnite, process id -> %d\n", process.process_id);

	process.fix_directory_table_base();

	c_module mod = process.module_from_name("FortniteClient-Win64-Shipping.exe");
	if (mod.failed)
		return device.error("failed to find fortnite\n");
	else
		logger::info("found fortnite, base address -> 0x%llx\n\n", mod.base);

	memory = process.get_memory();
	view_memory = process.get_memory();

	keyboard = device.get_keyboard();

	base_address = mod.base;

	logger::info("loop started, press VK_END to exit\n");

	std::thread ithread = std::thread(window_thread);

	std::thread([&]() { for (;; ) { loadoffsets(mod.base); } }).detach();

	
	
	
	while (!GetAsyncKeyState(VK_END))
	{
		keyboard.update_keys();

		int player_count;
		uint64_t player_array;

		memory.initialize_scatter();

		memory.prepare_scatter(game_state + offsets::player_array_size, &player_count);
		memory.prepare_scatter(game_state + offsets::player_array, &player_array);
		memory.prepare_scatter(globals::local_player_controller + 0x338, &globals::local_pawn);

		memory.dispatch_read();
		
		if (!ptr_valid(player_array))
			continue;

		c_entity_list entity_list = c_entity_list(player_array, player_count);

		entity_list.update();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
	}

	VMMDLL_CloseAll();

	ithread.join();
	inthread.join();

	return 0;
}