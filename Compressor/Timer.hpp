#ifndef __TIMER__
#define __TIMER__

#include <stdio.h>

#ifndef __GNUC__
#include <windows.h>

class Timer {
private:
	LARGE_INTEGER _freq, _start, _stop;
	
public:
	Timer() {
		_freq.QuadPart = 0;
		_start.QuadPart = 0;
		_stop.QuadPart = 0;
		QueryPerformanceFrequency(&_freq);
		_freq.QuadPart /= 1000.0;
	}
	~Timer() {
	}
	
	void Start() {
		QueryPerformanceCounter(&_start);
	}

	double GetTimeInMs() {
		QueryPerformanceCounter(&_stop);
		return (double(_stop.QuadPart) - double(_start.QuadPart)) / double(_freq.QuadPart);
	}
};

#else

#include <sys/time.h>

class Timer {
private:
	double start;
	
public:
	Timer() {
		start = 0.;
	}
	~Timer() {
	}
	
	void Start() {
		struct timeval _start;
		gettimeofday(&_start,NULL);
		start = _start.tv_sec*1. +_start.tv_usec*1.e-6;
	}

	double GetTimeInMs() {
		struct timeval _stop;
		gettimeofday(&_stop,NULL);
		double stop = _stop.tv_sec*1. +_stop.tv_usec*1.e-6;
		return stop-start;
	}
};

#endif

#endif // TIMER