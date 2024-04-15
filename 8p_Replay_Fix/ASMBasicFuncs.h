#pragma once
#include <Windows.h>
#include <io.h>
#define relative_address(frm, to) (int)(((int)to - (int)frm) - 5)
#define PAGE_SIZE               (4096)
#define PAGE_ALIGN(Va)          ((ULONG_PTR)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTES_TO_PAGES(Size)    (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define ROUND_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

#include <tlhelp32.h>
#include <Psapi.h>
#include <cstdint>
#pragma comment( lib, "psapi.lib" )


const char HexArray[] = "0123456789ABCDEF0123456789abcdef";
int StringtoByteArray(BYTE* bCode, char* Code)
{
	int iSize = 0, i, j;
	for (i = 0; Code[i]; i++) {
		for (j = 0; HexArray[j]; j++) {
			if (Code[i] == HexArray[j]) {
				if (iSize % 2 == 0) {
					bCode[iSize / 2] = (j % 0x10) * 0x10;
				}
				else {
					bCode[iSize / 2] += j % 0x10;
				}
				iSize++;
				break;
			}
		}
	}
	if (iSize % 2) {
		return 0;
	}
	return (iSize / 2);
}

char* D2C(uint32_t dwordValue)
{
	static char hex_string[5];
	sprintf(hex_string, "%c%c%c%c", dwordValue & 0xFF, (dwordValue >> 8) & 0xFF, (dwordValue >> 16) & 0xFF, (dwordValue >> 24) & 0xFF);
	return hex_string;
}



bool MemPageRightsFromString(DWORD* Protect, const char* Rights)
{
	if (strlen(Rights) >= 2)
	{
		*Protect = 0;

		// Check for the PAGE_GUARD flag
		if (Rights[0] == 'G' || Rights[0] == 'g')
		{
			*Protect |= PAGE_GUARD;
			Rights++;
		}

		if (_strcmpi(Rights, "Execute") == 0)
			*Protect |= PAGE_EXECUTE;
		else if (_strcmpi(Rights, "ExecuteRead") == 0)
			*Protect |= PAGE_EXECUTE_READ;
		else if (_strcmpi(Rights, "ExecuteReadWrite") == 0)
			*Protect |= PAGE_EXECUTE_READWRITE;
		else if (_strcmpi(Rights, "ExecuteWriteCopy") == 0)
			*Protect |= PAGE_EXECUTE_WRITECOPY;
		else if (_strcmpi(Rights, "NoAccess") == 0)
			*Protect |= PAGE_NOACCESS;
		else if (_strcmpi(Rights, "ReadOnly") == 0)
			*Protect |= PAGE_READONLY;
		else if (_strcmpi(Rights, "ReadWrite") == 0)
			*Protect |= PAGE_READWRITE;
		else if (_strcmpi(Rights, "WriteCopy") == 0)
			*Protect |= PAGE_WRITECOPY;

		return (*Protect != 0);
	}
	else
	{
		return false;
	}

}

bool MemSetPageRights(DWORD Address, const char* Rights)
{
	// Align address to page base
	Address = PAGE_ALIGN(Address);

	// String -> bit mask
	DWORD protect;
	if (!MemPageRightsFromString(&protect, Rights))
		return false;

	DWORD oldProtect;
	return !!VirtualProtectEx(GetCurrentProcess(), (void*)Address, PAGE_SIZE, protect, &oldProtect);
}




void MakePageWritable(unsigned long ulAddress, unsigned long ulSize)
{
	MEMORY_BASIC_INFORMATION* mbi = new MEMORY_BASIC_INFORMATION;
	VirtualQuery((void*)ulAddress, mbi, ulSize);
	if (mbi->Protect != PAGE_EXECUTE_READWRITE)
	{
		unsigned long* ulProtect = new unsigned long;
		VirtualProtect((void*)ulAddress, ulSize, PAGE_EXECUTE_READWRITE, ulProtect);
		delete ulProtect;
	}

	delete mbi;
}

MODULEINFO GetModuleInfo(char* szModule)
{
	MODULEINFO modinfo = { 0 };
	HMODULE hModule = GetModuleHandleA(szModule);
	if (hModule == 0)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}

void WriteToMemory(uintptr_t addressToWrite, char* valueToWrite, int byteNum)
{

	unsigned long OldProtection;
	VirtualProtect((LPVOID)(addressToWrite), byteNum, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy((LPVOID)addressToWrite, valueToWrite, byteNum);
	VirtualProtect((LPVOID)(addressToWrite), byteNum, OldProtection, NULL);
}

bool WriteToMemory2(DWORD dwAddress, char* Code)
{
	BYTE bCode[255] = { 0 };
	int iSize = StringtoByteArray(bCode, Code);
	if (iSize)
	{
		MakePageWritable(dwAddress, iSize);
		__try
		{
			RtlCopyMemory((void*)dwAddress, bCode, iSize);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
		return true;
	}
	return false;
}


DWORD FindPattern(char* module, char* pattern, char* mask)
{
	//Get all module related information
	MODULEINFO mInfo = GetModuleInfo(module);

	//Assign our base and module size
	//Having the values right is ESSENTIAL, this makes sure
	//that we don't scan unwanted memory and leading our game to crash
	DWORD base = (DWORD)mInfo.lpBaseOfDll;
	DWORD size = (DWORD)mInfo.SizeOfImage;

	//Get length for our mask, this will allow us to loop through our array
	DWORD patternLength = (DWORD)strlen(mask);

	for (DWORD i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (DWORD j = 0; j < patternLength; j++)
		{
			//if we have a ? in our mask then we have true by default, 
			//or if the bytes match then we keep searching until finding it or not
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}

		//found = true, our entire pattern was found
		//return the memory addy so we can write to it
		if (found)
		{
			return base + i;
		}
	}
	return NULL;
}

void PatchJmp(unsigned long ulAddress, void* Function, unsigned Nops)
{
	MakePageWritable(ulAddress, Nops + 5); //Make memory writable before any modifications are attempted
	*(unsigned char*)ulAddress = 0xE9; //E9 is the opcode for a long jump
	*(unsigned long*)(ulAddress + 1) = relative_address(ulAddress, Function);//The next 4 bytes indicate the DISPLACEMENT of the destination
	memset((void*)(ulAddress + 5), 0x90, Nops); //nop the rest of the opcode
}

void PatchCall(unsigned long ulAddress, void* Function, unsigned Nops)
{
	MakePageWritable(ulAddress, Nops + 5); //Make memory writable before any modifications are attempted
	*(unsigned char*)ulAddress = 0xE8; //E8 is the opcode for a call
	*(unsigned long*)(ulAddress + 1) = relative_address(ulAddress, Function);//The next 4 bytes indicate the DISPLACEMENT of the destination
	memset((void*)(ulAddress + 5), 0x90, Nops); //nop the rest of the opcode
}

void PatchPush(unsigned long ulAddress, void* Function, unsigned Nops)
{
	MakePageWritable(ulAddress, Nops + 5); //Make memory writable before any modifications are attempted
	*(unsigned char*)ulAddress = 0x68; //68 is the opcode for a push
	*(unsigned long*)(ulAddress + 1) = (DWORD)Function;
	memset((void*)(ulAddress + 5), 0x90, Nops); //nop the rest of the opcode
}

void PatchDWORDPtrAddr(unsigned long ulAddress, void* Function, unsigned Nops)
{
	MakePageWritable(ulAddress, Nops + 4); //Make memory writable before any modifications are attempted
	*(unsigned long*)(ulAddress) = (DWORD)Function;
	memset((void*)(ulAddress + 5), 0x90, Nops); //nop the rest of the opcode
}

template <typename t> auto get_ptr(DWORD address)
{
	return *reinterpret_cast<t*>(address);
}