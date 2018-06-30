#include "GarrysMod/Lua/Interface.h"
#include <stdio.h>
#include <vector>
#include "stattool.h"

using namespace GarrysMod::Lua;

STool::SysStats stats;
std::vector<STool::ProfilerNode> nodes;

FILETIME ft;
SYSTEMTIME st;

/*

require( "example" );

MsgN( TestFunction() );

MsgN( TestFunction( 24.75 ) );

*/

int MyExampleFunction(lua_State* state)
{
	if (LUA->IsType(1, Type::NUMBER))
	{
		char strOut[512];
		float fNumber = LUA->GetNumber(1);
		sprintf(strOut, "Thanks for the number - I love %f!!", fNumber);
		LUA->PushString(strOut);
		return 1;
	}

	LUA->PushString("This string is returned");
	return 1;

}

int SystemMemoryUsed(lua_State* state) {
	LUA->PushNumber(STool::querySystemMemoryUsage(stats));
	return 1;
}

int ProcessMemoryUsed(lua_State* state) {
	LUA->PushNumber(STool::queryProcessMemoryUsage(stats));
	return 1;
}

int SystemMemoryAvailable(lua_State* state) {
	LUA->PushNumber(STool::queryTotalPhysMemory(stats));
	return 1;
}

int SystemCPUUsage(lua_State* state) {
	LUA->PushNumber(STool::querySystemCPUUsage(stats));
	return 1;
}

int ProcessCPUUsage(lua_State* state) {
	LUA->PushNumber(STool::queryProcCPUUsage(stats));
	return 1;
}

int NumProcessors(lua_State* state) {
	LUA->PushNumber(STool::queryNumProcessors(stats));
	return 1;
}

int GameDiagVersion(lua_State* state) {
	LUA->PushString("v1.0 (written in C by Techmo)");
	return 1;
}

// Add a profiler node to the vector of nodes, then return the index of the node
int ProfilerCreate(lua_State* state) {
	if (LUA->IsType(1, Type::STRING)) {
		STool::ProfilerNode* n = new STool::ProfilerNode;

		n->name = LUA->GetString(1);
		n->num_calls = 0;
		n->time_elapsed = 0;
		n->time_end = 0;
		n->time_start = 0;

		nodes.push_back(*n);
		LUA->PushNumber(nodes.size() - 1);
	}
	return 1;
}

int ProfilerStart(lua_State* state) {
	if (LUA->IsType(1, Type::NUMBER)) {
		int index = LUA->GetNumber(1);
		STool::ProfilerNode* n = &nodes[index];
		
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		// Combine the 32 high order bits, and 32 low order bits of the time by bit shifting
		n->time_start = (__int64(ft.dwHighDateTime) << 32 | __int64(ft.dwLowDateTime));
	}
	return 1;
}

int ProfilerEnd(lua_State* state) {
	if (LUA->IsType(1, Type::NUMBER)) {
		int index = LUA->GetNumber(1);
		STool::ProfilerNode* n = &nodes[index];

		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);

		// Combine the 32 high order bits, and 32 low order bits of the time by bit shifting
		n->time_end = (__int64(ft.dwHighDateTime) << 32 | __int64(ft.dwLowDateTime));

		// Get the elapsed time
		n->time_elapsed = n->time_end - n->time_start;

		LUA->PushNumber(double(n->time_elapsed / 10000)); // Return elapsed time in milliseconds
	}
	return 1;
}


//
// Called when you module is opened
//

int pushFunction(lua_State* state, const char* name, GarrysMod::Lua::CFunc func) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);	// Push global table
	LUA->PushString(name);					// Push Name
	LUA->PushCFunction(func);			// Push function
	LUA->SetTable(-3);

	return 0;
}

/*
int cprint(lua_State* state, const char* str) {
	LUA->GetField(-1, "print");
	LUA->PushString(str);
	LUA->Call(1, 0);
	return 0;
}
*/

GMOD_MODULE_OPEN()
{
	//
	// Set Global[ "TextFunction" ] = MyExampleFunction
	//

	/*
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );	// Push global table
	LUA->PushString( "TestFunction" );					// Push Name
	LUA->PushCFunction( MyExampleFunction );			// Push function
	LUA->SetTable( -3 );	
	*/

	pushFunction(state, "TestFunction", MyExampleFunction);
	pushFunction(state, "SystemMemoryUsed", SystemMemoryUsed);
	pushFunction(state, "ProcessMemoryUsed", ProcessMemoryUsed);
	pushFunction(state, "SystemMemoryAvailable", SystemMemoryAvailable);
	pushFunction(state, "SystemCPUUsage", SystemCPUUsage);
	pushFunction(state, "ProcessCPUUsage", ProcessCPUUsage);
	pushFunction(state, "NumProcessors", NumProcessors);
	pushFunction(state, "GameDiagVersion", GameDiagVersion);
	pushFunction(state, "ProfilerCreate", ProfilerCreate);
	pushFunction(state, "ProfilerStart", ProfilerStart);
	pushFunction(state, "ProfilerEnd", ProfilerEnd);
	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE()
{
	return 0;
}