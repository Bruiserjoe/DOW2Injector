#pragma once

/*
Exports to match original Profiler.dll
CREDITS to maximumgame on github
*/

#define EXPORT __declspec(dllexport)

namespace Profile {

	unsigned __int64 EXPORT MeasureCurrentCount()
	{
		return 0;
	}

	unsigned __int64 EXPORT MeasureFrequency()
	{
		return 0;
	}
};


class EXPORT CallGraph
{
public:
	__thiscall CallGraph(const char* a) {};
	__thiscall ~CallGraph() {};

	void __thiscall DumpLog() {};
	void virtual __thiscall Visit() {};
};