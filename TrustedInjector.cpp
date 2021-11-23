#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <utility>
#include <string>
#include <vector>

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

static const char *dllsToCheck[3] = {
	"kernel32",
	"ntdll",
	"KernelBase"
};

std::vector<std::string> getExportedFunctions(const char *dllName) {
	HMODULE hLibrary = LoadLibraryEx(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hLibrary == 0)
		return {};
	
	BYTE *pLibraryBase = reinterpret_cast<BYTE *>(hLibrary);
	
	PIMAGE_NT_HEADERS header = reinterpret_cast<PIMAGE_NT_HEADERS>(pLibraryBase + reinterpret_cast<PIMAGE_DOS_HEADER>(hLibrary)->e_lfanew);
	
	PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(pLibraryBase + header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	
	BYTE **functionNames = reinterpret_cast<BYTE **>(pLibraryBase + exports->AddressOfNames);

	std::vector<std::string> exportedFunctions;
	for (size_t i = 0; i < exports->NumberOfNames; i++) {
		std::string functionName = reinterpret_cast<char *>(pLibraryBase + reinterpret_cast<unsigned long long>(functionNames[i]));
		exportedFunctions.push_back(functionName);
	}

	return exportedFunctions;
}

void disableTrustedHooks(HANDLE csgo) {
	printf("Bypassing trusted mode hooks.\n");
	for (size_t i = 0; i < 3; i++) {
		std::vector<std::string> functionNames = getExportedFunctions(dllsToCheck[i]);
		for (const std::string &functionName : functionNames) {
			LPVOID pAddress = GetProcAddress(LoadLibrary(dllsToCheck[i]), functionName.c_str());
			uint8_t cleanBytes[5];
			memcpy(cleanBytes, pAddress, 5);
			uint8_t dirtyBytes[5];
			ReadProcessMemory(csgo, pAddress, dirtyBytes, 5, NULL);

			// If the good version doesnt match the game version, then csgo is hooking it...
			if (cleanBytes[0] != dirtyBytes[0]) {
				// Overwrite the hook with our good version.
				WriteProcessMemory(csgo, pAddress, cleanBytes, 5, NULL);
				printf("Disabled %s hook.\n", functionName.c_str());
			}
		}
	}
}

int main(size_t argc, char **argv) {
	// Don't know if this really helps anything, but it looks cool so fuck it.
	randomizeConsoleTitle();

	if (argc < 2) {
		error("No subcommands specified.\n\nUsage:\n    ./TrustedInjector.exe bypass\n    - or -\n    ./TrustedInjector.exe <path to dll file>\n");
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

	// Get a handle to the game process.
	HANDLE csgo = OpenProcess(PROCESS_ALL_ACCESS, FALSE, csgoPid);
	if (csgo == NULL) {
		error("Failed to open handle to CS:GO.");
	}

	disableTrustedHooks(csgo);

	if (shouldInject) {
		printf("Injecting DLL into CS:GO.\n");

		// Send unhook message for supported cheats.
		SendMessage(wndProc, WM_NULL, 0xC560, 0x0D11);

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
