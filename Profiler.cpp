#include "Profiler.h"

Profile * Profiler::GetProfileByName(std::string name)
{
	auto result = profile_index.find(name);
	if (result != profile_index.end())
		return result->second;

	return nullptr;
}

Profile * Profiler::GetProfileByIndex(int index)
{
	if (index < num_profiles)
		return &profiles[index];

	return nullptr;
}

int Profiler::ExportProfile(Profile * _profile, std::string path) 
{
	std::ofstream file((path + _profile->name + ".gpf").c_str(), std::ios::binary);
	file.write((char*)((*_profile).time_samples.buffer), sizeof(double) * _profile->time_samples.occupied);
	file.close();


	return 0;
}

int Profiler::ExportAllProfiles(std::string path) 
{
	for (int i = 0; i < num_profiles; i++) {
		Profile p = profiles[i];
		ExportProfile(&p, path);
	}

	return 0;
}

int Profiler::ProfileStart(Profile * _profile)
{
	_profile->running = true;
	_profile->current_start = std::chrono::steady_clock::now();
	return 0;
}

int Profiler::ProfileStop(Profile * _profile)
{
	_profile->current_end = std::chrono::steady_clock::now();

	std::chrono::duration<double> diff = _profile->current_end - _profile->current_start;

	double seconds = diff.count();

	_profile->time_samples.push(seconds);
	_profile->running = false;

	return 0;
}

int Profiler::AddProfile(std::string name)
{
	// Don't overflow the profiles array
	if (num_profiles >= max_profiles)
		return 1;

	// Add a new profile to the array and increment the counter
	profiles[num_profiles++] = Profile(name);

	// Add an entry to the hash table with the name, and a pointer to the profile
	profile_index.insert({ name, &profiles[num_profiles - 1] });

	return num_profiles - 1;
}

Profiler::Profiler(int _max_profiles)
{
	max_profiles = _max_profiles;
	num_profiles = 0;
	profiles = new Profile[_max_profiles];
}

Profiler::~Profiler()
{
	delete [] profiles;
}
