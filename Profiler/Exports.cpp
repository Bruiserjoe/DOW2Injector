#include "Exports.h"

namespace Profile {

	unsigned __int64 __stdcall MeasureCurrentCount() {
		return 0;
	}

	unsigned __int64 __stdcall MeasureFrequency() {
		return 0;
	}
}

CallGraph::CallGraph(const char*) {
}

CallGraph::~CallGraph() {
}

CallGraph& CallGraph::operator=(const CallGraph&) {
	return *this;
}

void CallGraph::DumpLog() {
}

void CallGraph::Visit() {
}
