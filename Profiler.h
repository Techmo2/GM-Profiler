#pragma once

#ifndef PROFILER_h
#define PROFILER_h

#include <cstdint>
#include <unordered_map>
#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include "buffer.h"

// Stores execution time information for a function
class Profile {
public:
	std::chrono::time_point<std::chrono::steady_clock> current_start;
	std::chrono::time_point<std::chrono::steady_clock> current_end;
	Buffer<double, 10000> time_samples;
	std::string name;

	bool running;

	Profile() {

	}

	// Initialize the profile and allocate the space for the samples on the heap
	Profile(std::string _name) {
		name = _name;
		running = false;
	}
};

class Profiler {
private:
	int max_profiles;
	int num_profiles;
	Profile *profiles;
	std::unordered_map<std::string, Profile*> profile_index; // Hash table to provide a pointer to a profile for a given string

public:
	Profile* GetProfileByName(std::string name);
	Profile* GetProfileByIndex(int index);
	int ExportProfile(Profile* _profile, std::string path); // Writes the profile to the disk to be loaded by the profile viewer
	int ExportAllProfiles(std::string path);
	int ProfileStart(Profile* _profile); // Records the starting time of a profile
	int ProfileStop(Profile* _profile); // Records the stopping time of a profile and computes the elapsed time. Updates the average execution time.
	int AddProfile(std::string name); // Adds a profile to the array of profiles, returns the new profile's index
	Profiler(int _max_profiles);
	~Profiler();
};

#endif
