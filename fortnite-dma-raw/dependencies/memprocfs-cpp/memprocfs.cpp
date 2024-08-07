#include "memprocfs.h"

#pragma region conversions

inline LPSTR CC_TO_LPSTR(const char* in)
{
	LPSTR out = new char[strlen(in) + 1];
	strcpy_s(out, strlen(in) + 1, in);

	return out;
}

inline LPWSTR CC_TO_LPWSTR(const wchar_t* in)
{
	LPWSTR out = new wchar_t[wcslen(in) + 1];
	wcscpy_s(out, wcslen(in) + 1, in);

	return out;
}

inline LPWSTR LPSTR_TO_LPWSTR(LPSTR in)
{
	int size = MultiByteToWideChar(CP_ACP, 0, in, -1, NULL, 0);
	LPWSTR out = new wchar_t[size];
	MultiByteToWideChar(CP_ACP, 0, in, -1, out, size);

	return out;
}

inline const char* LPWSTR_TO_CC(LPWSTR in)
{
	char buffer[500];
	wcstombs(buffer, in, 500);

	return buffer;
}

#pragma endregion conversions

#pragma region device

const char* c_device::id_to_name()
{
	switch (id)
	{
	case 0:
		return xs("SP605 / FT601");
	case 1:
		return xs("PCIeScreamer R1");
	case 2:
		return xs("AC701 / FT601");
	case 3:
		return xs("PCIeScreamer R2");
	case 4:
		return xs("ScreamerM2");
	case 5:
		return xs("NeTV2 RawUDP");
	case 6:
		return xs("Unsupported");
	case 7:
		return xs("Unsupported");
	case 8:
		return xs("FT2232H #1");
	case 9:
		return xs("Enigma X1");
	case 10:
		return xs("Enigma X1 (FutureUse)");
	case 11:
		return xs("ScreamerM2x4");
	case 12:
		return xs("PCIeSquirrel");
	case 13:
		return xs("Device #13N");
	case 14:
		return xs("Device #14T");
	case 15:
		return xs("Device #15N");
	case 16:
		return xs("Device #16T");
	}

	return xs("Unknown");
}

bool c_device::connect()
{
	std::vector<LPSTR> args{};
	for (auto arg : o_args)
		args.push_back((LPSTR)arg);

	vmm_handle = VMMDLL_Initialize(args.size(), args.data());
	if (vmm_handle)
	{
		VMMDLL_ConfigGet(vmm_handle, LC_OPT_FPGA_FPGA_ID, &id);
		VMMDLL_ConfigGet(vmm_handle, LC_OPT_FPGA_VERSION_MAJOR, &major_version);
		VMMDLL_ConfigGet(vmm_handle, LC_OPT_FPGA_VERSION_MINOR, &minor_version);

		connected = true;

		return true;
	}
	else
	{
		if (log_errors)
			logger::error(xs("c_device::connect():%s:%s -> VMMDLL_Initialize failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return false;
	}

	return false;
}

void c_device::disconnect()
{
	VMMDLL_Close(vmm_handle);

	connected = false;
}

/* return type -> std::vector<user_map_data_t>
*  gets all users found on the target machine
*/
std::vector<user_map_data_t> c_device::get_users()
{
	std::vector<user_map_data_t> user_map_list{};

	PVMMDLL_MAP_USER pUserMap = nullptr;
	DWORD PcbUserMap = 0;

	bool result = VMMDLL_Map_GetUsersU(vmm_handle, &pUserMap);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_device::get_users():%s:%s failed invalid version\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (!pUserMap)
	{
		if (log_errors)
			logger::error(xs("c_device::get_users():%s:%s user map is nullptr\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (pUserMap->dwVersion != VMMDLL_MAP_USER_VERSION)
	{
		if (log_errors)
			logger::error(xs("c_device::get_users():%s:%s failed invalid version\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	for (int i = 0; i < pUserMap->cMap; i++)
	{
		PVMMDLL_MAP_USERENTRY entry = &pUserMap->pMap[i];

		user_map_data_t data{};
		memcpy(data.future_use1, entry->_FutureUse1, sizeof(entry->_FutureUse1));
		memcpy(data.future_use2, entry->_FutureUse2, sizeof(entry->_FutureUse2));

		data.usz_sid = entry->uszSID;
		data.usz_text = entry->uszText;
		data.va_reg_hive = entry->vaRegHive;
		data.wsz_sid = entry->wszSID;
		data.wsz_text = entry->wszText;

		user_map_list.push_back(data);
	}

	return user_map_list;
}

/* return type -> std::vector<PVMMDLL_MAP_SERVICEENTRY>
*  gets all services running on target machine
*/
std::vector<PVMMDLL_MAP_SERVICEENTRY> c_device::get_service_list()
{
	std::vector<PVMMDLL_MAP_SERVICEENTRY> service_data_list{};

	PVMMDLL_MAP_SERVICE pServiceMap = nullptr;

	bool result = VMMDLL_Map_GetServicesU(vmm_handle, &pServiceMap);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_device::get_service_list():%s:%s failed GetServicesU call\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (!pServiceMap)
	{
		if (log_errors)
			logger::error(xs("c_device::get_service_list():%s:%s service map is nullptr\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (pServiceMap->dwVersion != VMMDLL_MAP_SERVICE_VERSION)
	{
		if (log_errors)
			logger::error(xs("c_device::get_service_list():%s:%s invalid version\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	for (int i = 0; i < pServiceMap->cMap; i++)
		service_data_list.push_back(pServiceMap->pMap + i);

	return service_data_list;
}

c_process c_device::process_from_name(const char* name)
{
	return c_process(name);
}

c_process c_device::process_from_id(int id)
{
	return c_process(id);
}

std::vector<DWORD> c_device::get_pid_list()
{
	std::vector<DWORD> pids{};

	ULONG64 pcPIDs = 0;
	bool result = VMMDLL_PidList(vmm_handle, NULL, &pcPIDs);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_device::get_pid_list():%s:%s -> VMMDLL_PidList failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	std::vector<DWORD> pid_buffer(pcPIDs);
	result = VMMDLL_PidList(vmm_handle, pid_buffer.data(), &pcPIDs);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_device::get_pid_list():%s:%s -> VMMDLL_PidList failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	for (const DWORD& pid : pid_buffer)
		pids.push_back(pid);

	return pids;
}

std::vector<c_process> c_device::get_process_list()
{
	std::vector<c_process> process_list{};

	const auto pids = get_pid_list();
	if (pids.empty())
		return {};

	for (const auto pid : pids)
		process_list.emplace_back(process_from_id(pid));

	return process_list;
}

c_registry c_device::get_registry_data(const char* path, e_registry_type type)
{
	return c_registry(path, type);
}

c_keyboard c_device::get_keyboard()
{
	return c_keyboard(this);
}

int c_device::error(const char* error_message)
{
	logger::error(error_message);

	disconnect();

	system(xs("pause"));

	return 0;
}

#pragma endregion device

#pragma region process

void get_process_information(VMMDLL_PROCESS_INFORMATION& information, DWORD process_id)
{
	SIZE_T cbProcessInformation = sizeof(VMMDLL_PROCESS_INFORMATION);
	ZeroMemory(&information, sizeof(VMMDLL_PROCESS_INFORMATION));
	information.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	information.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

	bool result = VMMDLL_ProcessGetInformation(vmm_handle, process_id, &information, &cbProcessInformation);
	if (!result && log_errors)
		logger::error(xs("get_process_information():%s:%s -> VMMDLL_ProcessGetInformation failed\n"), CURRENT_FILENAME, CURRENT_LINE);
}

c_process::c_process(const char* in_name)
{
	if (strcmp(in_name, "") == 0)
		return;

	name = in_name;

	if (!VMMDLL_PidGetFromName(vmm_handle, CC_TO_LPSTR(name), &process_id))
		failed = true;
	else
		get_process_information(information, process_id);
}

c_process::c_process(DWORD in_id)
{
	process_id = in_id;

	if (process_id == INT_MIN)
		return;

	get_process_information(information, process_id);
}

c_module c_process::module_from_name(const char* module_name)
{
	return c_module(module_name, process_id);
}

std::vector<c_module> c_process::get_module_list()
{
	std::vector<c_module> map_data_list{};

	DWORD cbPteMap = 0;
	PVMMDLL_MAP_PTE pPteMap = nullptr;

	bool result = VMMDLL_Map_GetPteU(vmm_handle, process_id, TRUE, &pPteMap);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_process::get_module_list():%s:%s -> VMMDLL_Map_GetPteU failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (!pPteMap)
	{
		if (log_errors)
			logger::error(xs("c_process::get_module_list():%s:%s pte map map is nullptr\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (pPteMap->dwVersion != VMMDLL_MAP_PTE_VERSION)
		return {};

	for (int i = 0; i < pPteMap->cMap; i++)
	{
		PVMMDLL_MAP_PTEENTRY entry = &pPteMap->pMap[i];

		map_data_list.push_back(c_module(entry->uszText, process_id, false));
	}

	pPteMap = NULL;

	return map_data_list;
}

c_memory c_process::get_memory()
{
	return c_memory(process_id);
}

struct process_info_t
{
	DWORD index;
	DWORD process_id;
	uint64_t dtb;
};

//default the file size to something high just incase it fails to find it
uint64_t cb_file_size = 0x80000;

//callback for VfsFileListU
void cb_add_file(_Inout_ HANDLE h, _In_ LPSTR uszName, _In_ ULONG64 cb, _In_opt_ PVMMDLL_VFS_FILELIST_EXINFO pExInfo)
{
	if (strcmp(uszName, xs("dtb.txt")) == 0)
		cb_file_size = cb;
}

/* return type -> bool
*  attempts to correct the directory table base often mangled by malware and anti cheat solutions
*/
dtb_status_t c_process::fix_directory_table_base()
{
	if (!VMMDLL_InitializePlugins(vmm_handle))
	{
		if (log_errors)
			logger::error(xs("c_process::fix_directory_table_base():%s:%s failed InitializePlugins call\n"), CURRENT_FILENAME, CURRENT_LINE);

		return dtb_status_t::plugin_failed;
	}

	//have to sleep a little or we try reading the file before the plugin initializes fully
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	//try to get the module normally
	c_module mod = module_from_name(name);
	if (!mod.failed)
		return dtb_status_t::fix_unnecessary;

	logger::info(xs("progress -> "));

	int iterations = 0;

	//constantly check the progress_percent file to see once it completes finding all of the dtb's
	while (true)
	{
		BYTE bytes[4] = { 0 };
		DWORD i = 0;
		auto nt = VMMDLL_VfsReadU(vmm_handle, CC_TO_LPSTR(xs("\\misc\\procinfo\\progress_percent.txt")), bytes, sizeof(bytes) - 1, &i, 0);
		int val = atoi((LPSTR)bytes);

		printf(xs("%d "), val);

		if (nt == VMMDLL_STATUS_SUCCESS && val == 100)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		iterations++;

		if (iterations > 50)
			break;
	}

	iterations = 0;

	printf(xs("\n"));

	VMMDLL_VFS_FILELIST2 VfsFileList;
	VfsFileList.dwVersion = VMMDLL_VFS_FILELIST_VERSION;
	VfsFileList.h = 0;
	VfsFileList.pfnAddDirectory = 0;
	VfsFileList.pfnAddFile = cb_add_file;//set the callback so we can get the size to allocate our buffer

	bool result = VMMDLL_VfsListU(vmm_handle, CC_TO_LPSTR(xs("\\misc\\procinfo\\")), &VfsFileList);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_process::fix_directory_table_base():%s:%s failed VfsListU call\n"), CURRENT_FILENAME, CURRENT_LINE);

		return dtb_status_t::list_filed;
	}

	//read the data from the txt and parse it
	//this isn't a great way of doing it
	const size_t buffer_size = cb_file_size;
	BYTE* bytes = new BYTE[buffer_size];
	DWORD j = 0;
	auto nt = VMMDLL_VfsReadU(vmm_handle, CC_TO_LPSTR(xs("\\misc\\procinfo\\dtb.txt")), bytes, buffer_size - 1, &j, 0);
	if (nt != VMMDLL_STATUS_SUCCESS)
	{
		if (log_errors)
			logger::error(xs("c_process::fix_directory_table_base():%s:%s failed VfsReadW call\n"), CURRENT_FILENAME, CURRENT_LINE);

		delete[] bytes;
		return dtb_status_t::read_failed;
	}

	std::vector<uint64_t> possible_dtbs;
	char* pLineStart = reinterpret_cast<char*>(bytes);
	for (size_t i = 0; i < 1000; ++i)
	{
		char* pLineEnd = strchr(pLineStart, '\n');
		if (pLineEnd == nullptr)
			break;

		*pLineEnd = '\0';

		process_info_t info = {};
		int numFields = sscanf(pLineStart, xs("%X %X %llX"), &info.index, &info.process_id, &info.dtb);

		if (numFields == 3 && info.process_id == 0)//parts that lack a name or have a NULL pid are suspects
			possible_dtbs.push_back(info.dtb);

		pLineStart = pLineEnd + 1;
	}

	logger::info(xs("attempting eac bypass...\n\n"));

	for (auto dtb : possible_dtbs)
	{
		VMMDLL_ConfigSet(vmm_handle, VMMDLL_OPT_PROCESS_DTB | process_id, dtb);

		mod = module_from_name(name);
		if (!mod.failed)
		{
			delete[] bytes;
			return dtb_status_t::success;
		}
	}

	delete[] bytes;

	return dtb_status_t::failed;
}

#pragma region memory

static const char* hexdigits =
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\001\002\003\004\005\006\007\010\011\000\000\000\000\000\000"
"\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\012\013\014\015\016\017\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
"\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000";

static uint8_t get_byte(const char* hex)
{
	return (uint8_t)((hexdigits[hex[0]] << 4) | (hexdigits[hex[1]]));
}

struct sig_cache_t
{
	uint64_t start;
	uint64_t end;
	std::vector<uint8_t> buffer{};
};

inline std::vector<sig_cache_t> sig_cache{};

uint64_t c_memory::find_signature(const char* signature, uint64_t range_start, uint64_t range_end, bool caching)
{
	if (!signature || signature[0] == '\0' || range_start >= range_end)
		return 0;

	std::vector<uint8_t> buffer(range_end - range_start);

	bool skip_read = false;

	if (caching)
	{
		for (auto& cache : sig_cache)
		{
			if (cache.start == range_start && cache.end == range_end)
			{
				buffer = cache.buffer;

				skip_read = true;

				break;
			}
		}
	}

	if (!skip_read)
	{
		if (!VMMDLL_MemReadEx(vmm_handle, process_id, range_start, buffer.data(), buffer.size(), 0, VMMDLL_FLAG_NOCACHE))
		{
			if (log_errors)
				logger::error(xs("c_memory::find_signature():%s:%s -> VMMDLL_MemReadEx failed\n"), CURRENT_FILENAME, CURRENT_LINE);

			return 0;
		}

		if (caching)
			sig_cache.push_back({ range_start, range_end, buffer });
	}

	const char* pat = signature;
	uint64_t first_match = 0;
	for (uint64_t i = range_start; i < range_end; i++)
	{
		if (*pat == '?' || buffer[i - range_start] == get_byte(pat))
		{
			if (!first_match)
				first_match = i;

			if (!pat[2])
				break;

			pat += (*pat == '?') ? 2 : 3;
		}
		else
		{
			pat = signature;
			first_match = 0;
		}
	}

	return first_match;
}

void c_memory::clear_cached_modules()
{
	sig_cache.clear();
}

bool c_memory::initialize_scatter()
{
	scatter_handle = VMMDLL_Scatter_Initialize(vmm_handle, process_id, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING_IO);
	if (log_errors && !scatter_handle)
		logger::error(xs("c_memory::initialize_scatter():%s:%s -> VMMDLL_Scatter_Initialize failed\n"), CURRENT_FILENAME, CURRENT_LINE);

	scatters = 0;

	return scatter_handle;
}

void c_memory::uninitialize_scatter()
{
	scatter_buffers.clear();

	VMMDLL_Scatter_CloseHandle(scatter_handle);
}

bool c_memory::dispatch()
{
	if (!scatter_handle || scatters < 1)
		return false;

	bool result = VMMDLL_Scatter_Execute(scatter_handle);

	if (log_errors && !result)
		logger::error(xs("c_memory::dispatch():%s:%s -> VMMDLL_Scatter_Execute failed\n"), CURRENT_FILENAME, CURRENT_LINE);

	return result;
}

bool c_memory::dispatch_read(bool uninitialize)
{
	if (!scatter_handle || scatters < 1)
		return false;

	bool result = VMMDLL_Scatter_ExecuteRead(scatter_handle);
	if (log_errors && !result)
		logger::error(xs("c_memory::dispatch_read():%s:%s -> VMMDLL_Scatter_ExecuteRead failed\n"), CURRENT_FILENAME, CURRENT_LINE);

	if (scatter_buffers.empty())
		return result;

	for (auto& data : scatter_buffers)
	{
		read_scatter(data.address, data.var, data.size);
	}

	if (uninitialize)
		uninitialize_scatter();

	return result;
}

bool c_memory::is_in_section(uint64_t address, section_data_t section_data)
{
	return address > section_data.start && address < section_data.end;
}

#pragma endregion memory

#pragma region module

c_module::c_module(const char* mname, DWORD in_pid, bool with_kernel_memory)
{
	if (!mname)
		return;

	if (with_kernel_memory)
		process_id = in_pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY;
	else
		process_id = in_pid;

	PVMMDLL_MAP_MODULEENTRY module_entry = nullptr;
	failed = !VMMDLL_Map_GetModuleFromNameU(vmm_handle, process_id, CC_TO_LPSTR(mname), &module_entry);

	if (log_errors && failed)
	{
		logger::error(xs("Battle-Eye Not Found\n"), CURRENT_FILENAME, CURRENT_LINE);

		return;
	}

	name = mname;
	base = module_entry->vaBase;
	size = module_entry->cbImageSize;
}

std::vector<section_data_t> c_module::get_sections()
{
	std::vector<section_data_t> sections_list{};

	LPWSTR module_name_w = LPSTR_TO_LPWSTR(CC_TO_LPSTR(name));
	LPSTR module_name_lw = CC_TO_LPSTR(name);

	uint64_t base = VMMDLL_ProcessGetModuleBaseW(vmm_handle, process_id, module_name_w);

	DWORD sections;
	BOOL result = VMMDLL_ProcessGetSectionsU(vmm_handle, process_id, module_name_lw, NULL, 0, &sections);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_module::get_sections():%s:%s -> VMMDLL_ProcessGetSectionsU failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	PIMAGE_SECTION_HEADER section_headers = (PIMAGE_SECTION_HEADER)LocalAlloc(LMEM_ZEROINIT, sections * sizeof(IMAGE_SECTION_HEADER));
	if (!section_headers)
		return {};

	result = VMMDLL_ProcessGetSectionsU(vmm_handle, process_id, module_name_lw, section_headers, sections, &sections);
	if (result)
	{
		for (int i = 0; i < sections; i++)
		{
			sections_list.push_back({
				base + section_headers[i].VirtualAddress,
				base + section_headers[i].VirtualAddress + section_headers[i].Misc.VirtualSize,
				(const char*)section_headers[i].Name,
				section_headers[i].Characteristics
				});
		}
	}
	else
		if (log_errors)
			logger::error(xs("c_module::get_sections():%s:%s -> VMMDLL_ProcessGetSectionsU failed\n"), CURRENT_FILENAME, CURRENT_LINE);

	LocalFree(section_headers);

	return sections_list;
}

uint64_t c_module::get_function(const char* function_name)
{
	return VMMDLL_ProcessGetProcAddressU(vmm_handle, process_id, CC_TO_LPSTR(name), CC_TO_LPSTR(function_name));
}

std::vector<function_data_t> c_module::get_functions()
{
	std::vector<function_data_t> functions{};

	PVMMDLL_MAP_EAT pEatMap = nullptr;
	PVMMDLL_MAP_EATENTRY pEatMapEntry;
	bool result = VMMDLL_Map_GetEATU(vmm_handle, process_id, CC_TO_LPSTR(name), &pEatMap);
	if (!result)
	{
		if (log_errors)
			logger::error(xs("c_module::get_exports():%s:%s -> VMMDLL_Map_GetEATU failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (!pEatMap)
	{
		if (log_errors)
			logger::error(xs("c_module::get_functions():%s:%s eat map is nullptr\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	if (pEatMap->dwVersion != VMMDLL_MAP_EAT_VERSION)
	{
		VMMDLL_MemFree(pEatMap);

		pEatMap = NULL;

		if (log_errors)
			logger::error(xs("c_module::get_exports():%s:%s -> version mismatch\n"), CURRENT_FILENAME, CURRENT_LINE);

		return {};
	}

	for (int i = 0; i < pEatMap->cMap; i++)
	{
		pEatMapEntry = pEatMap->pMap + i;

		functions.push_back({ pEatMapEntry->uszFunction, pEatMapEntry->vaFunction });
	}

	VMMDLL_MemFree(pEatMap);

	pEatMap = nullptr;

	return functions;
}

#pragma endregion module

#pragma region registry

c_registry::c_registry(const char* path, e_registry_type type)
{
	DWORD dtype = (DWORD)type;
	DWORD size = sizeof(buffer);
	failed = !VMMDLL_WinReg_QueryValueExU(vmm_handle, CC_TO_LPSTR(path), &dtype, buffer, &size);

	if (log_errors && failed)
		logger::error(xs("c_registry::c_registry():%s:%s -> VMMDLL_WinReg_QueryValueExU failed\n"), CURRENT_FILENAME, CURRENT_LINE);
}

const char* c_registry::get_string()
{
	return LPWSTR_TO_CC((LPWSTR)buffer);
}

int c_registry::get_int()
{
	int out;
	std::stringstream s(get_string());
	s >> out;

	return out;
}

#pragma endregion registry

#pragma region keyboard

c_keyboard::c_keyboard(c_device* device_in)
{
	device = device_in;

	if (!device)
		return;

	int current_build = device->get_registry_data(xs("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentBuild"), e_registry_type::sz).get_int();
	bool is_win11 = current_build >= 22000;

	if (is_win11)
	{
		logger::info(xs("initializing keyboard for windows 11\n\n"));

		process = device->process_from_name("csrss.exe");
		if (process.failed)
			return;

		c_memory memory = process.get_memory();

		uint64_t tmp = VMMDLL_ProcessGetModuleBaseU(vmm_handle, process.process_id, (LPSTR)"win32ksgd.sys");
		uint64_t gSessionGlobalSlots = tmp + 0x3110;
		uint64_t Session1_UserSessionState = memory.read<uint64_t>(memory.read<uint64_t>(memory.read<uint64_t>(gSessionGlobalSlots)));
		gafAsyncKeyStateExport = Session1_UserSessionState + 0x3690;
	}
	else
	{
		logger::info(xs("initializing keyboard for windows 10\n\n"));

		process = device->process_from_name(xs("winlogon.exe"));
		if (process.failed)
			return;

		gafAsyncKeyStateExport = VMMDLL_ProcessGetProcAddressU(vmm_handle, process.process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY, CC_TO_LPSTR(xs("win32kbase.sys")), CC_TO_LPSTR(xs("gafAsyncKeyState")));
		if (!gafAsyncKeyStateExport)
		{
			logger::error("failed to get KeyBoard\n");

			return;
		}
		logger::info("KeyBoard -> 0x%llx\n", gafAsyncKeyStateExport);
	
		/*PVMMDLL_MAP_EAT pEatMap = NULL;
		PVMMDLL_MAP_EATENTRY pEatMapEntry;
		bool result = VMMDLL_Map_GetEATU(vmm_handle, process.process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY, (LPSTR)"win32kbase.sys", &pEatMap);
		if (!result)
			return;

		if (pEatMap->dwVersion != VMMDLL_MAP_EAT_VERSION)
		{
			VMMDLL_MemFree(pEatMap);
			pEatMap = NULL;

			return;
		}

		for (int i = 0; i < pEatMap->cMap; i++)
		{
			pEatMapEntry = pEatMap->pMap + i;

			if (strcmp(pEatMapEntry->uszFunction, xs("gafAsyncKeyState")) == 0)
			{
				gafAsyncKeyStateExport = pEatMapEntry->vaFunction;

				break;
			}
		}

		VMMDLL_MemFree(pEatMap);

		pEatMap = NULL;*/
	
	}
}

void c_keyboard::update_keys()
{
	if (process.failed)
		return;

	memcpy(previous_key_state_bitmap, state_bitmap, 64);

	VMMDLL_MemReadEx(vmm_handle, process.process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY, gafAsyncKeyStateExport, (PBYTE)&state_bitmap, 64, NULL, VMMDLL_FLAG_NOCACHE);

	for (int vk = 0; vk < 256; ++vk)
		if ((state_bitmap[(vk * 2 / 8)] & 1 << vk % 4 * 2) && !(previous_key_state_bitmap[(vk * 2 / 8)] & 1 << vk % 4 * 2))
			previous_state_bitmap[vk / 8] |= 1 << vk % 8;
}

bool c_keyboard::key_down(uint8_t virtual_key_code)
{
	return state_bitmap[(virtual_key_code * 2 / 8)] & 1 << virtual_key_code % 4 * 2;;
}

bool c_keyboard::key_was_down(uint8_t vk)
{
	bool result = previous_state_bitmap[vk / 8] & 1 << vk % 8;

	previous_state_bitmap[vk / 8] &= ~(1 << vk % 8);

	return result;
}

#pragma endregion keyboard