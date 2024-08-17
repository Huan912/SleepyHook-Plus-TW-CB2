#include <Windows.h>
#include "Main.h"
#include "MetaHook.h"
#include "Utils.h"

std::string m_sHostIPAddress;
int m_iHostPort;

void WriteBytes(PVOID address, void* val, int bytes) {
	DWORD d, ds;
	VirtualProtect(address, bytes, PAGE_EXECUTE_READWRITE, &d);
	memcpy(address, val, bytes);
	VirtualProtect(address, bytes, d, &ds);
}

namespace GameGuard
{
	int (__cdecl* oGetStateCode)();
	int __cdecl GetStateCode() {
		return 1877;
	}
}

namespace HookFuncs
{
	int(__thiscall* oIpRedirector)(void* pThis, unsigned long ip, u_short port, char a4);
	int __fastcall IpRedirector(void* pThis, void* edx, unsigned long ip, u_short port, char a4) {
		auto ulIP = inet_addr(m_sHostIPAddress.c_str());
		auto usPort = ntohs(m_iHostPort);
		return oIpRedirector(pThis, ulIP, usPort, a4);
	}

	int(__cdecl* oIpRedirector2)(unsigned long ip, u_short port);
	int __cdecl IpRedirector2(unsigned long ip, u_short port) {
		auto ulIP = inet_addr(m_sHostIPAddress.c_str());
		auto usPort = ntohs(m_iHostPort);
		return oIpRedirector2(ulIP, usPort);
	}

	int(__thiscall* oSharedDictCheck)(int* thisptr, char* a2);
	int __fastcall SharedDictCheck_Hook(int* thisptr, void* edx, char* a2) {
		return 0;
	}
}

void GamePatcher() {
	/*
	Utils::AttachConsole();
	Utils::ConsolePrint("Host IP: %s\n", m_sHostIPAddress.c_str());
	Utils::ConsolePrint("Host Port: %d\n", m_iHostPort);
	*/
	std::string sIpAddr = CommandLine()->GetParmValue("-ip");
	int nPort = CommandLine()->GetParmValue("-port", 0);
	m_sHostIPAddress = sIpAddr;
	m_iHostPort = nPort;

	DWORD dwHardWare = (DWORD)GetModuleHandleA("hw.dll");

	//Ip Redirector
	MH_InlineHook((void*)(dwHardWare + 0x242840), HookFuncs::IpRedirector, (void*&)HookFuncs::oIpRedirector);
	MH_InlineHook((void*)(dwHardWare + 0xE0A40), HookFuncs::IpRedirector2, (void*&)HookFuncs::oIpRedirector2);

	//GameGuard Bypass
	MH_InlineHook((void*)(dwHardWare + 0x248070), GameGuard::GetStateCode, (void*&)GameGuard::oGetStateCode);
	WriteBytes((void*)(dwHardWare + 0xDA544), (void*)"\xEB", 1);

	if (CommandLine()->CheckParm("-nossl") != NULL)
	{
		//Disable SSL Certificate Init
		WriteBytes((void*)(dwHardWare + 0x244297), (void*)"\x90\x90\x90\x90\x90", 5);
		WriteBytes((void*)(dwHardWare + 0x2429E9), (void*)"\x90\x90\x90\x90\x90\x90\x90\xEB", 8);
	}



	//Patch SharedDict Check
	//MH_InlineHook((void*)(dwHardWare + 0xC0D0D8), HookFuncs::SharedDictCheck_Hook, (void*&)HookFuncs::oSharedDictCheck);
}