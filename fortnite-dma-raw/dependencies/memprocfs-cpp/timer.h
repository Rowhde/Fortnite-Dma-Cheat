#pragma once
#include <time.h>

class c_timer
{
private:
	clock_t start, end, duration;

public:
	c_timer() { }

	void initialize()
	{
		start = clock();
	}

	float get_duration()
	{
		end = clock();
		duration = end - start;
		return (float)duration / CLOCKS_PER_SEC;
	}

	void reset()
	{
		start = clock();
	}
};