#include <Windows.h>
#include "Main.h"
#include "MetaHook.h"
#define FILEYSTEM_DLL "FileSystem_Nar.dll"
IFileSystem* g_pFileSystem;
BlobFootprint_t g_blobfootprintClient;

int SafeStrnicmp(const char* str1, const char* str2, size_t n) {
	if (str1 == nullptr || str2 == nullptr) {
		return str1 == str2 ? 0 : (str1 == nullptr ? -1 : 1);
	}

	while (n > 0) {
		int diff = std::tolower(static_cast<unsigned char>(*str1)) - std::tolower(static_cast<unsigned char>(*str2));
		if (diff != 0 || *str1 == '\0')
			return diff;
		++str1;
		++str2;
		--n;
	}
	return 0;
}

void SetEngineDLL(const char*& pszEngineDLL)
{
	pszEngineDLL = "hw.dll";
	registry->WriteString("EngineDLL", pszEngineDLL);
}

BOOL OnVideoModeFailed(void)
{
	registry->WriteInt("ScreenWidth", 640);
	registry->WriteInt("ScreenHeight", 480);
	registry->WriteInt("ScreenBPP", 16);
	registry->WriteString("EngineDLL", "sw.dll");

	return (MessageBoxA(NULL, "The specified video mode is not supported.\nThe game will now run in software mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) == IDOK);
}

typedef struct LauncherString {
	int		Size;
	char	Name[MAX_PATH];
	LauncherString(const char* Value = "") : Size(0) {
		Size = sprintf_s(Name, "%s", Value);
	}
};

typedef struct LauncherParam {
	LauncherString 	Name;
	LauncherString 	Value;
};

void SetValue(LPVOID Info, int& Index, int Size, ULONG* Value) {
	memcpy((void*)((ULONG)Info + Index), Value, Size);
	Index = Index + Size;
}

void FillGameSharedMemroy()
{
	HANDLE hFile = CreateFileMappingA(NULL, NULL, PAGE_READWRITE, NULL, 32768, "CSO.SharedDict");
	if (!hFile)
		return;
	LPVOID lpMemInfo = MapViewOfFile(hFile, SECTION_MAP_EXECUTE | FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!lpMemInfo) {
		CloseHandle(hFile);
		return;
	}

	LauncherParam ArrayInfo[] = {
		{ "launched", "1" },
		{ "mode", "" },
		//{ "passport", "username@password" }, //This prevents us from needing to Hook AuthManager.
		{ "region", "3" },
		{ "type", "0" },
	};

	ULONG	Number = sizeof(ArrayInfo) / sizeof(LauncherParam);;
	ULONG	Value = Number;
	int		Index = 4;
	SetValue(lpMemInfo, Index, 4, &Value);

	//Fill the Shared Memory
	for (auto i = 0; i < Number; i++)
	{
		Value = ArrayInfo[i].Name.Size;
		SetValue(lpMemInfo, Index, 4, &Value);
		SetValue(lpMemInfo, Index, ArrayInfo[i].Name.Size, (ULONG*)ArrayInfo[i].Name.Name);

		Value = ArrayInfo[i].Value.Size;
		SetValue(lpMemInfo, Index, 4, &Value);
		SetValue(lpMemInfo, Index, ArrayInfo[i].Value.Size, (ULONG*)ArrayInfo[i].Value.Name);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE hObject = NULL;
	if (!IsDebuggerPresent())
	{
		hObject = CreateMutexA(NULL, FALSE, "NexonCSOMutex");
		DWORD dwStatus = WaitForSingleObject(hObject, 0);
		if (dwStatus && dwStatus != WAIT_ABANDONED)
		{
			MessageBoxA(NULL, "Could not launch game.\nOnly one instance of this game can be run at a time.", "Error", MB_ICONERROR);
			return 0;
		}
	}

	WSAData WSAData;
	WSAStartup(2, &WSAData);
	registry->Init();
	CommandLine()->CreateCmdLine(GetCommandLineA());
	CommandLine()->AppendParm("-nomaster", NULL);

	char szFileName[256];
	Sys_GetExecutableName(szFileName, sizeof(szFileName));
	char* szExeName = strrchr(szFileName, '\\') + 1;
	if (_stricmp(szExeName, "hl.exe") && CommandLine()->CheckParm("-game") == NULL)
	{
		szExeName[strlen(szExeName) - 4] = '\0';
		CommandLine()->AppendParm("-game", szExeName);
	}

	const char* _szGameName;
	static char szGameName[32];
	const char* szGameStr = CommandLine()->CheckParm("-game", &_szGameName);
	strcpy_s(szGameName, _szGameName);

	if (szGameStr && !SafeStrnicmp(&szGameStr[6], "czero", 5))
		CommandLine()->AppendParm("-forcevalve", NULL);

	if (registry->ReadInt("CrashInitializingVideoMode", FALSE))
	{
		registry->WriteInt("CrashInitializingVideoMode", FALSE);

		if (strcmp(registry->ReadString("EngineDLL", "hw.dll"), "hw.dll"))
		{
			if (registry->ReadInt("EngineD3D", FALSE))
			{
				registry->WriteInt("EngineD3D", FALSE);

				if (MessageBoxA(NULL, "The game has detected that the previous attempt to start in D3D video mode failed.\nThe game will now run attempt to run in openGL mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) != IDOK)
					return 0;
			}
			else
			{
				registry->WriteString("EngineDLL", "sw.dll");

				if (MessageBoxA(NULL, "The game has detected that the previous attempt to start in openGL video mode failed.\nThe game will now run in software mode.", "Video mode change failure", MB_OKCANCEL | MB_ICONWARNING) != IDOK)
					return 0;
			}

			registry->WriteInt("ScreenWidth", 640);
			registry->WriteInt("ScreenHeight", 480);
			registry->WriteInt("ScreenBPP", 16);
		}
	}

	while (true)
	{
		HINTERFACEMODULE hFileSystem = Sys_LoadModule(FILEYSTEM_DLL);
		if(!hFileSystem)
			break;
		//MH_Init(szGameName); //MetaHook Init
		CreateInterfaceFn fsCreateInterface = (CreateInterfaceFn)Sys_GetFactory(hFileSystem);
		g_pFileSystem = (IFileSystem*)fsCreateInterface(FILESYSTEM_INTERFACE_VERSION, NULL);
		g_pFileSystem->Mount();
		g_pFileSystem->AddSearchPath(Sys_GetBinPathName(), "ROOT");
		g_pFileSystem->AddSearchPath(Sys_GetBinPathName(), "BIN");
		CommandLine()->AppendParm("-lang", "tw"); //Language
		CommandLine()->AppendParm("-game", "cstrike");

		static char szNewCommandParams[2048];
		const char* pszEngineDLL;
		int iResult = ENGINE_RESULT_NONE;

		szNewCommandParams[0] = 0;
		SetEngineDLL(pszEngineDLL);

		g_blobfootprintClient.m_hDll = NULL;

		IEngine* engineAPI = NULL;
		HINTERFACEMODULE hEngine = NULL;
		bool bUseBlobDLL = false;

		if (FIsBlob(pszEngineDLL))
		{
			Sys_CloseDEP();
			NLoadBlobFile(pszEngineDLL, &g_blobfootprintClient, (void**)&engineAPI);
			bUseBlobDLL = true;
		}
		else
		{
			FillGameSharedMemroy();
			hEngine = Sys_LoadModule(pszEngineDLL);
			MessageBoxA(NULL, "Please set BP at ret code of MessageBoxA API", "", MB_OK); // if you want to debug the game, you can set a breakpoint here
			if (!hEngine)
			{
				static char msg[512];
				sprintf_s(msg, "Could not load %s.\nPlease try again at a later time.", pszEngineDLL);
				MessageBoxA(NULL, msg, "Fatal Error", MB_ICONERROR);
				break;
			}

			CreateInterfaceFn engineCreateInterface = (CreateInterfaceFn)Sys_GetFactory(hEngine);
			engineAPI = (IEngine*)engineCreateInterface(VENGINE_LAUNCHER_API_VERSION, NULL);

			if (!engineCreateInterface || !engineAPI)
				Sys_FreeModule(hEngine);
		}

		if (engineAPI)
		{
			MH_LoadEngine(bUseBlobDLL ? NULL : (HMODULE)hEngine);
			iResult = engineAPI->Run(hInstance, Sys_GetLongPathName(), CommandLine()->GetCmdLine(), szNewCommandParams, Sys_GetFactoryThis(), Sys_GetFactory(hFileSystem));
			MH_ExitGame(iResult);

			if (bUseBlobDLL)
				FreeBlob(&g_blobfootprintClient);
			else
				Sys_FreeModule(hEngine);
		}

		if (iResult == ENGINE_RESULT_NONE || iResult > ENGINE_RESULT_UNSUPPORTEDVIDEO)
			break;

		bool bContinue;

		switch (iResult)
		{
		case ENGINE_RESULT_RESTART:
		{
			bContinue = true;
			break;
		}

		case ENGINE_RESULT_UNSUPPORTEDVIDEO:
		{
			bContinue = OnVideoModeFailed();
			break;
		}
		}

		CommandLine()->RemoveParm("-sw");
		CommandLine()->RemoveParm("-startwindowed");
		CommandLine()->RemoveParm("-windowed");
		CommandLine()->RemoveParm("-window");
		CommandLine()->RemoveParm("-full");
		CommandLine()->RemoveParm("-fullscreen");
		CommandLine()->RemoveParm("-soft");
		CommandLine()->RemoveParm("-software");
		CommandLine()->RemoveParm("-gl");
		CommandLine()->RemoveParm("-d3d");
		CommandLine()->RemoveParm("-w");
		CommandLine()->RemoveParm("-width");
		CommandLine()->RemoveParm("-h");
		CommandLine()->RemoveParm("-height");
		CommandLine()->RemoveParm("-novid");

		if (strstr(szNewCommandParams, "-game"))
			CommandLine()->RemoveParm("-game");

		if (strstr(szNewCommandParams, "+load"))
			CommandLine()->RemoveParm("+load");

		CommandLine()->AppendParm(szNewCommandParams, NULL);

		g_pFileSystem->Unmount();
		Sys_FreeModule(hFileSystem);
		MH_Shutdown();

		if (!bContinue)
			break;
	}

	registry->Shutdown();
	if (hObject)
	{
		ReleaseMutex(hObject);
		CloseHandle(hObject);
	}

	WSACleanup();
	MH_Shutdown();
	TerminateProcess(GetCurrentProcess(), 1);
	return 1;
}