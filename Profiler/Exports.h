#pragma once

/*
Exports to match original Profiler.dll
CREDITS to maximumgame on github
*/

#define EXPORT __declspec(dllexport)

namespace Profile {

	unsigned __int64 EXPORT __stdcall MeasureCurrentCount();
	unsigned __int64 EXPORT __stdcall MeasureFrequency();
};


class EXPORT CallGraph
{
public:
	__thiscall CallGraph(const char* a);
	__thiscall ~CallGraph();
	CallGraph& __thiscall operator=(const CallGraph& other);

	void __thiscall DumpLog();
	void __thiscall Visit();
};