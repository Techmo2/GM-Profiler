#ifndef DATABASE_h
#define DATABASE_h

#include <mutex>
#include <stdio.h>
#include <iostream>
#include <boost\thread\mutex.hpp>
#include <boost\thread\thread.hpp>


#pragma comment(lib,"ws2_32.lib")

class StatServer {
	boost::mutex mtx;
	const int port;

};


#endif DATABASE_h