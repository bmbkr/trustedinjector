#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

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

int main(size_t argc, char **argv) {
	// Don't know if this really helps anything, but it looks cool so fuck it.
	randomizeConsoleTitle();

	if (argc < 2) {
		error("DLL path was not specified.");
	}

	if (fileExists(argv[1]) == false) {
		error("Failed to open DLL.");
	}

	char dllPath[MAX_PATH];
	GetFullPathName(argv[1], MAX_PATH, dllPath, NULL);
	printf("Using DLL at '%s'\n", dllPath);

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

	// Grab the hooked version of NtOpenFile from CS:GO's memory.
	uint8_t dirtyNtOpenFile[5];
	ReadProcessMemory(csgo, ntOpenFile, dirtyNtOpenFile, 5, NULL);

	// Get a clean version of NtOpenFile without any hooks.
	uint8_t cleanNtOpenFile[5];
	memcpy(cleanNtOpenFile, ntOpenFile, 5);

	// Is the first byte of CS:GO's NtOpenFile a JMP instruction?
	if (dirtyNtOpenFile[0] == 0xE9) {
		printf("Disabling CS:GO's NtOpenFile hook.\n");
		// Replace CS:GO's hooked NtOpenFile with a non-hooked one.
		WriteProcessMemory(csgo, ntOpenFile, cleanNtOpenFile, 5, NULL);
	}

	printf("Injecting DLL into CS:GO.\n");

	// Load the dll and call LoadLibrary from the process :yawn:
	LPVOID allocatedMem = VirtualAllocEx(csgo, NULL, sizeof(dllPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(csgo, allocatedMem, dllPath, sizeof(dllPath), NULL);
	HANDLE hThread = CreateRemoteThread(csgo, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, allocatedMem, 0, 0);
	printf("DLL Injected.\n");

	// We gone.
	CloseHandle(hThread);
	CloseHandle(csgo);
	return 0;
}
