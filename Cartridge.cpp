#include <iostream>
#include "gstdlib.h"
#include "cartridge.h"
#include "Profiler.h"

Profiler *profiler;
bool _init = false;

Cartridge("GProfiler v0.1b")

Function(CreateProfile) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CheckType(1, Type::STRING); // Make sure the first given argument is a string

	if (_init) {
		LUA->PushNumber(profiler->AddProfile(LUA->GetString(1)));
		return 1;
	}
	else {
		FatalError("Must initialize the profiler before adding a profile\n");
		return 0;
	}

	LUA->Pop(); 

}

Function(StartProfile) {

	Profile* p = nullptr;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);

	if (LUA->IsType(1, Type::STRING)) {
		p = profiler->GetProfileByName(LUA->GetString(1));
	}
	if (LUA->IsType(1, Type::NUMBER)) {
		p = profiler->GetProfileByIndex(LUA->GetNumber(1));
	}
	else {
		Error("Invalid arguments given\n");
		return 0;
	}
	LUA->Pop();

	if (p == nullptr) {
		Error("Tried to start a profile that does not exist\n");
		return 0;
	}
	

	profiler->ProfileStart(p);

	return 0;
}

Function(ProfilerInit) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CheckType(1, Type::NUMBER);

	profiler = new Profiler(LUA->GetNumber(1));
	LUA->Pop();
	print("Profiler initialized\n");
	_init = true;

	return 0;
}

Function(StopProfile) {
	Profile* p = nullptr;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	if (LUA->IsType(1, Type::STRING)) {
		p = profiler->GetProfileByName(LUA->GetString(1));
	}
	else if (LUA->IsType(1, Type::NUMBER)) {
		p = profiler->GetProfileByIndex(LUA->GetNumber(1));
	}
	else {
		Error("Invalid arguments given\n");
		return 0;
	}
	LUA->Pop();

	if (p == nullptr) {
		Error("Tried to stop a profile that does not exist\n");
		return 0;
	}

	profiler->ProfileStop(p);

	return 0;
}

Start() {
	print("\nGProfiler v0.1b by Techmo\n");

	RegisterFunction(ProfilerInit);
	RegisterFunction(CreateProfile);
	RegisterFunction(StartProfile);
	RegisterFunction(StopProfile);

	return 0;
}

Stop() {
	print("Writing profile data to disk\n");
	profiler->ExportAllProfiles("profiles/");

	//profiler->~Profiler();

	return 0;
}