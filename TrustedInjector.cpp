#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <utility>
#include <string>

void randomizeConsoleTitle()
{
	srand(time(NULL));
	size_t length = (rand() % 32) + 16;
	char *buffer = static_cast<char *>(calloc(length + 1, sizeof(char)));
	for (size_t i = 0; i < length; i++) {
		buffer[i] = (rand() % 223) + 32;
	}
	SetConsoleTitle(buffer);
	free(buffer);
}

void error(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}

bool fileExists(const char *path) {
	FILE *pFile;
	fopen_s(&pFile, path, "r");
	if (pFile) {
		fclose(pFile);
		return true;
	}
	else {
		return false;
	}
}

void printBytes(uint8_t *bytes, size_t size) {
	for (size_t i = 0; i < size; i++) {
		printf("%02x ", bytes[i]);
	}
	printf("\n");
}

static const std::pair<const char *, const char *> hooks[17] = {
	{"LoadLibraryExW","kernel32"},
	{"VirtualAlloc", "kernel32"},
	{"FreeLibrary", "kernel32"},
	{"LoadLibraryExA", "kernel32"},
	{"LoadLibraryW", "kernel32"},
	{"LoadLibraryA", "kernel32"},
	{"VirtualAllocEx", "kernel32"},
	{"LdrLoadDll", "ntdll"},
	{"NtOpenFile", "ntdll"},
	{"VirtualProtect", "kernel32"},
	{"CreateProcessW", "kernel32"},
	{"CreateProcessA", "kernel32"},
	{"VirtualProtectEx", "kernel32"},
	{"FreeLibrary", "KernelBase"},
	{"LoadLibraryExA", "KernelBase"},
	{"LoadLibraryExW", "KernelBase"},
	{"ResumeThread", "KernelBase"}
};

void disableTrustedHooks(HANDLE csgo) {
	printf("Bypassing trusted mode hooks.\n");
	for (size_t i = 0; i < 17; i++) {
		byte goodBytes[6];
		LPVOID address = GetProcAddress(LoadLibrary(hooks[i].second), hooks[i].first);
		memcpy(goodBytes, address, 6);
		byte csgoBytes[6];
		ReadProcessMemory(csgo, address, csgoBytes, 6, NULL);
		if (csgoBytes[0] == 0xe9) {
			WriteProcessMemory(csgo, address, goodBytes, 6, NULL);
			printf("Disabled %s hook.\n", hooks[i].first);
		}
	}
}

int main(size_t argc, char **argv) {
	// Don't know if this really helps anything, but it looks cool so fuck it.
	randomizeConsoleTitle();

	if (argc < 2) {
		error("No subcommands specified.");
	}

	bool shouldInject = true;

	if (fileExists(argv[1]) == false) {
		if (std::string("bypass") != argv[1]) {
			error("Failed to open DLL.");
		}
		shouldInject = false;
	}

	char dllPath[MAX_PATH];
	if (shouldInject) {
		GetFullPathName(argv[1], MAX_PATH, dllPath, NULL);
		printf("Using DLL at '%s'\n", dllPath);
	}

	HWND wndProc = FindWindowA("VALVE001", "Counter-Strike: Global Offensive");
	if (wndProc == NULL) {
		error("Failed to get wndProc. Is CS:GO open?");
	}

	DWORD csgoPid;
	GetWindowThreadProcessId(wndProc, &csgoPid);
	if (csgoPid == NULL) {
		error("Failed to get CS:GO's PID.");
	}

	// Gets the address of NtOpenFile relative to ntdll. 
	// This offset is always going to be the same relative to the base address of any process.
	LPVOID ntOpenFile = GetProcAddress(GetModuleHandle("ntdll.dll"), "NtOpenFile");

	// Get a handle to the game process.
	HANDLE csgo = OpenProcess(PROCESS_ALL_ACCESS, FALSE, csgoPid);
	if (csgo == NULL) {
		error("Failed to open handle to CS:GO.");
	}

	disableTrustedHooks(csgo);

	if (shouldInject) {
		printf("Injecting DLL into CS:GO.\n");

		// Load the dll and call LoadLibrary from the process :yawn:
		LPVOID allocatedMem = VirtualAllocEx(csgo, NULL, sizeof(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		WriteProcessMemory(csgo, allocatedMem, dllPath, sizeof(dllPath), NULL);
		HANDLE hThread = CreateRemoteThread(csgo, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, allocatedMem, 0, 0);
		printf("DLL Injected.\n");

		// We done.
		CloseHandle(hThread);
	}

	CloseHandle(csgo);
	return 0;
}
