#pragma once

#include <windows.h>
#include <stdint.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>

#include <vmmdll.h>

#include "logging.h"

#include "xorstr.h"

#define INTERNAL_STRINGIFY(NAME) #NAME
#define STRINGIFY(NAME) INTERNAL_STRINGIFY(NAME)
#define CURRENT_LINE STRINGIFY(__LINE__)

inline const char* extract_filename(const char* path)
{
	const char* separator = std::strrchr(path, '/');
	if (!separator)
		separator = std::strrchr(path, '\\');

	return separator ? separator + 1 : path;
}

#define CURRENT_FILE __FILE__
#define CURRENT_FILENAME extract_filename(CURRENT_FILE)

#define VMM_VERSION "5.2"

inline bool log_errors = true;

inline VMM_HANDLE vmm_handle = nullptr;

struct section_data_t
{
	uint64_t start, end;

	const char* name;

	uint32_t characteristics;
};

struct user_map_data_t
{
	uint32_t future_use1[2];
	uint32_t future_use2[2];
	unsigned long long va_reg_hive;
	LPSTR usz_text, usz_sid;
	LPWSTR wsz_text, wsz_sid;
};

enum class e_registry_type
{
	none = REG_NONE,
	sz = REG_SZ,
	expand_sz = REG_EXPAND_SZ,
	binary = REG_BINARY,
	dword = REG_DWORD,
	dword_little_endian = REG_DWORD_LITTLE_ENDIAN,
	dword_big_endian = REG_DWORD_BIG_ENDIAN,
	link = REG_LINK,
	multi_sz = REG_MULTI_SZ,
	resource_list = REG_RESOURCE_LIST,
	full_resource_descriptor = REG_FULL_RESOURCE_DESCRIPTOR,
	resource_requirements_list = REG_RESOURCE_REQUIREMENTS_LIST,
	qword = REG_QWORD,
	qword_little_endian = REG_QWORD_LITTLE_ENDIAN
};

class c_process;
class c_module;
class c_memory;
class c_registry;
class c_keyboard;

#pragma region device

class c_device
{
private:
	std::vector<const char*> o_args{};

public:
	bool connected = false;

	uint64_t id = 0, major_version = 0, minor_version = 0;

	const char* id_to_name();

	c_device(std::vector<const char*> in_args) : o_args(in_args) {};

	/*
		attmpts to initialize vmm with class initialized arguments
	*/
	bool connect();

	/*
		closes the vmm handle
	*/
	void disconnect();

	/*
		returns a list of windows users
	*/
	std::vector<user_map_data_t> get_users();

	/*
		returns a list of services
	*/
	std::vector<PVMMDLL_MAP_SERVICEENTRY> get_service_list();

	/*
		attempts to find a proecss using its name c_process:failed indiciates failure
	*/
	c_process process_from_name(const char* name);

	/*
		creates a c_process instance using a known process id
	*/
	c_process process_from_id(int id);

	/*
		returns a list of process id's
	*/
	std::vector<DWORD> get_pid_list();

	/*
		returns a list of process instances
	*/
	std::vector<c_process> get_process_list();

	/*
		returns registry data found at a path
	*/
	c_registry get_registry_data(const char* path, e_registry_type type);

	/*
		creates a c_keyboard instance to get keyboard status from target machine
	*/
	c_keyboard get_keyboard();

	int error(const char* error_message);
};

#pragma endregion device

#pragma region process

enum dtb_status_t
{
	failed = 0,
	success,
	fix_unnecessary,
	plugin_failed,
	list_filed,
	read_failed
};

inline const char* dtb_status_to_string(dtb_status_t status)
{
	switch (status)
	{
	case failed:
		return "failed";
	case success:
		return "success";
	case fix_unnecessary:
		return "fix unnecessary";
	case plugin_failed:
		return "plugin failed";
	case list_filed:
		return "list filed";
	case read_failed:
		return "read failed";
	default:
		return "unknown";
	}
}

class c_process
{
private:
	const char* name = nullptr;

public:
	bool failed = false;

	DWORD process_id = 0;

	VMMDLL_PROCESS_INFORMATION information{};

	c_process(const char* in_name);
	c_process(DWORD in_id);

	/*
		attmpts to find a module using it's name under a process
	*/
	c_module module_from_name(const char* module_name);

	/*
		returns a list of modules
	*/
	std::vector<c_module> get_module_list();

	/*
		returns a c_memory instance initialized with the process id
	*/
	c_memory get_memory();

	/*
		attmpts to correct directory table base, see dtb_status_t for return types
	*/
	dtb_status_t fix_directory_table_base();
};

#pragma endregion process

#pragma region memory

struct scatter_data_t
{
	uint64_t address;
	void* var;
	size_t size;
};

class c_memory
{
private:
	std::vector<scatter_data_t> scatter_buffers{};

public:
	DWORD process_id = 0;

	c_memory(DWORD in_pid) : process_id(in_pid) {};

	VMMDLL_SCATTER_HANDLE scatter_handle = NULL;

	int scatters = 0;

	/*
		reads memory at specified address, amount of bytes to read is defined by the template typename
	*/
	template<typename t>
	t read(uint64_t address, uint64_t flags = -1, bool with_kernel = false)
	{
		if (flags == -1)
			flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO;

		DWORD pid = process_id;
		if (with_kernel)
			pid = process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY;

		t data;
		bool result = VMMDLL_MemReadEx(vmm_handle, pid, address, (PBYTE)&data, sizeof(data), 0, flags);

		if (log_errors && !result)
			logger::error(xs("c_memory::read():%s:%s VMMDLL_MemReadEx failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return data;
	}

	template<typename T>
	bool read_array(uintptr_t address, T out[], size_t len)

	{

		for (size_t i = 0; i < len; ++i)
		{
			out[i] = read<T>(address + i * sizeof(T));
		}
		return true; // you can add additional checks to verify successful reads
	}
	/*
		read memory at a specified address to the buffer passed in the second argument, in_size is defaulted to 0 and will get the size it's self unless you specify otherwise
	*/
	template<typename t>
	bool read_raw(uint64_t address, t buffer, uint64_t in_size = 0, uint64_t flags = -1, bool with_kernel = false)
	{
		uint64_t size = in_size;
		if (in_size == 0)
			size = sizeof(*buffer);

		if (flags == -1)
			flags = VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO;

		DWORD pid = process_id;
		if (with_kernel)
			pid = process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY;

		bool result = VMMDLL_MemReadEx(vmm_handle, pid, address, (PBYTE)buffer, size, 0, flags);

		if (log_errors && !result)
			logger::error(xs("c_memory::read_raw():%s:%s VMMDLL_MemReadEx failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return result;
	}
	template<typename T>
	bool read_rawdog(PVOID address, T buffer, uint64_t size, uint64_t flags = -1, bool with_kernel = false)
	{
		DWORD pid = process_id;
		if (with_kernel)
			pid = process_id | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY;

		return VMMDLL_MemReadEx(vmm_handle, pid, reinterpret_cast<uint64_t>(address), reinterpret_cast<PBYTE>(buffer), size, 0, flags);
	}

	// Explicit specialization for char*
	template<>
	bool read_rawdog<char*>(PVOID address, char* buffer, uint64_t size, uint64_t flags, bool with_kernel)
	{
		return VMMDLL_MemReadEx(vmm_handle, process_id, reinterpret_cast<uint64_t>(address), reinterpret_cast<PBYTE>(buffer), size, 0, flags);
	}

	// Explicit specialization for wchar_t*
	template<>
	bool read_rawdog<wchar_t*>(PVOID address, wchar_t* buffer, uint64_t size, uint64_t flags, bool with_kernel)
	{
		return VMMDLL_MemReadEx(vmm_handle, process_id, reinterpret_cast<uint64_t>(address), reinterpret_cast<PBYTE>(buffer), size, 0, flags);
	}

	
	/*
		writes memory at a specified address, amount of bytes to write is defined by the template typename
	*/
	template<typename t>
	bool write(uint64_t address, t data)
	{
		bool result = VMMDLL_MemWrite(vmm_handle, process_id, address, (PBYTE)&data, sizeof(data));

		if (log_errors && !result)
			logger::error(xs("c_memory::write():%s:%s VMMDLL_MemWrite failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return result;
	}

	/*
		write memory at a specified address, amount of bytes to write is the size of the buffer provided
	*/
	template<typename t>
	bool write_raw(uint64_t address, t buffer)
	{
		bool result = VMMDLL_MemWrite(vmm_handle, process_id, address, (PBYTE)buffer, sizeof(*buffer));

		if (log_errors && !result)
			logger::error(xs("c_memory::write_raw():%s:%s VMMDLL_MemWrite failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return result;
	}

	/*
		reads a chain of pointers as a type defined by the first template typename, the last value type is defined by the second template typename
	*/
	template<typename t1, typename t2>
	t1 read_chain(uint64_t address, std::vector<uint64_t> offsets)
	{
		t1 current = read<t1>(address + offsets.at(0));
		if (current == 0)
			return {};

		for (auto i = 1u; i < offsets.size() - 1; ++i)
		{
			current = read<t1>(current + offsets[i]);

			if (current == 0)
				return {};
		}

		return read<t2>(current + offsets.back());
	}

	/*
		searches for a signature of bytes from range_start to range_end, "?" is used as a wildcard
	*/
	uint64_t find_signature(const char* signature, uint64_t range_start, uint64_t range_end, bool caching = true);

	/*
		clear cached modules to avoid wasted memory
	*/
	void clear_cached_modules();

	/*
		initializes a scatter handle
	*/
	bool initialize_scatter();

	/*
		uninitializes our scatter handle
	*/
	void uninitialize_scatter();

	/*
		dispatches all reads/writes to the device
	*/
	bool dispatch();

	/*
		dispatches all reads to the device and then loops our vector of scatter data to write the values we passed
	*/
	bool dispatch_read(bool uninitialize = false);

	template<typename t>
	bool prepare_scatter(uint64_t address, t* buffer, DWORD in_size = 0)
	{
		if (!scatter_handle)
			return false;

		DWORD size = sizeof(t);
		if (in_size != 0)
			size = in_size;

		bool result = VMMDLL_Scatter_Prepare(scatter_handle, address, size);
		if (result)
		{
			scatters++;
			scatter_buffers.push_back({ address, buffer, size });
		}
		else if (log_errors)
			logger::error(xs("c_memory::prepare_scatter():%s:%s VMMDLL_Scatter_Prepare failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		return result;
	}

	/* return type -> template
	*  prepares a scatter at specified address
	*  second argument "ret" will be set as the return boolean
	*  returns default template on error
	*/
	template<typename t>
	t prepare_scatter(uint64_t address)
	{
		if (!scatter_handle || address == 0)
			return t();

		bool result = VMMDLL_Scatter_Prepare(scatter_handle, address, sizeof(t));
		if (!result && log_errors)
			logger::error(xs("c_memory::prepare_scatter():%s:%s VMMDLL_Scatter_Prepare failed\n"), CURRENT_FILENAME, CURRENT_LINE);

		if (result)
			scatters++;
	}
	
	template<typename t>
	void read_scatter(uint64_t address, t* buffer, size_t size)
	{
		if (!scatter_handle || scatters < 1 || address == 0 || !buffer)
			return;

		DWORD callback = 0;
		BYTE* byte_array = new BYTE[size];
		bool result = VMMDLL_Scatter_Read(scatter_handle, address, size, byte_array, &callback);

		if (!result)
		{
			if (log_errors)
				logger::error(xs("c_memory::read_scatter():%s:%s VMMDLL_Scatter_Read failed\n"), CURRENT_FILENAME, CURRENT_LINE);

			delete[] byte_array;

			return;
		}

		memcpy(buffer, byte_array, size);

		delete[] byte_array;
	}

	template<typename t>
	t read_scatter(uint64_t address)
	{
		if (!scatter_handle || scatters < 1 || address == 0)
			return t();

		t result;

		DWORD callback = 0;
		BYTE byte_array[sizeof(t)];
		bool aresult = VMMDLL_Scatter_Read(scatter_handle, address, sizeof(t), byte_array, &callback);

		if (!aresult)
		{
			if (log_errors)
				logger::error(xs("c_memory::read_scatter():%s:%s VMMDLL_Scatter_Read failed\n"), CURRENT_FILENAME, CURRENT_LINE);

			delete[] byte_array;

			return t();
		}

		memcpy(&result, byte_array, sizeof(t));

		return result;
	}
	template<typename t>
	void read_scatter_array(uint64_t address, t* buffer, size_t size)
	{
		if (!scatter_handle || scatters < 1 || address == 0 || !buffer)
			return;

		DWORD callback = 0;
		BYTE* byte_array = new BYTE[size];
		bool result = VMMDLL_Scatter_Read(scatter_handle, address, size, byte_array, &callback);

		if (!result)
		{
			if (log_errors)
				logger::error(xs("c_memory::read_scatter():%s:%s VMMDLL_Scatter_Read failed\n"), CURRENT_FILENAME, CURRENT_LINE);

			delete[] byte_array;

			return;
		}

		memcpy(buffer, byte_array, size);

		delete[] byte_array;
	}

	template<typename t>
	t read_scatter_array(uint64_t address)
	{
		if (!scatter_handle || scatters < 1 || address == 0)
			return t();

		t result;

		DWORD callback = 0;
		BYTE byte_array[sizeof(t)];
		bool aresult = VMMDLL_Scatter_Read(scatter_handle, address, sizeof(t), byte_array, &callback);

		if (!aresult)
		{
			if (log_errors)
				logger::error(xs("c_memory::read_scatter():%s:%s VMMDLL_Scatter_Read failed\n"), CURRENT_FILENAME, CURRENT_LINE);

			delete[] byte_array;

			return t();
		}

		memcpy(&result, byte_array, sizeof(t));

		return result;
	}
	/*
		prepares a memory write to be dispatched to the card
	*/
	template<typename t>
	void prepare_write(uint64_t address, t buffer)
	{
		if (!scatter_handle)
			return;

		bool result = VMMDLL_Scatter_PrepareWrite(scatter_handle, address, (PBYTE)&buffer, sizeof(buffer));

		if (result)
			scatters++;
		else if (log_errors)
			logger::error(xs("c_memory::prepare_write():%s:%s VMMDLL_Scatter_PrepareWrite failed\n"), CURRENT_FILENAME, CURRENT_LINE);
	}

	bool is_in_section(uint64_t address, section_data_t section_data);
};

#pragma endregion memory

#pragma region module

struct function_data_t
{
	LPSTR name;
	uint64_t address;
};

class c_module
{
private:
	DWORD process_id = 0;

public:
	bool failed = false;

	uint64_t base = 0;
	uint32_t size = 0;

	const char* name = nullptr;

	c_module(const char* mname, DWORD in_id, bool with_kernel_memory = false);

	/*
		returns a vector of every section in the module
	*/
	std::vector<section_data_t> get_sections();

	/*
		returns the address of specified function in the module
	*/
	uint64_t get_function(const char* function_name);

	/*
		returns a list of functions in the module
	*/
	std::vector<function_data_t> get_functions();
};

#pragma endregion module

#pragma region registry

class c_registry
{
private:
	BYTE buffer[0x128]{};

	bool failed = false;

public:
	c_registry(const char* path, e_registry_type type);

	const char* get_string();
	int get_int();
};

#pragma endregion registry

#pragma region keyboard

class c_keyboard
{
private:
	c_process process = c_process((DWORD)INT_MIN);

	uint64_t gafAsyncKeyStateExport = 0;
	uint8_t state_bitmap[64]{};
	uint8_t previous_state_bitmap[256 / 8]{};
	uint8_t previous_key_state_bitmap[64] = { 0 };
	uint64_t win32kbase = 0;

public:
	c_device* device = nullptr;

	c_keyboard(c_device* device_in);

	void update_keys();
	bool key_down(uint8_t virtual_key_code);
	bool key_was_down(uint8_t virtual_key_code);
};

#pragma endregion keyboard