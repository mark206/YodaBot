#include "Arduino.h"
#include "CircularBuffer.h"
#include <TimeLib.h>			// For CircularBuffer.cpp

#define TIMEFRAME 5L

CircularBuffer::CircularBuffer(int arraySize)
{
	array = new uint32_t[arraySize];
}

void CircularBuffer::init()
{
	for (int i = 0; i < num_elements; i++)
	{
		array[i] = now() - TIMEFRAME - 10L;
	}
}

void CircularBuffer::addElement(uint32_t newElement)
{
	array[index] = newElement;
	index++;
	if (index >= num_elements)
	{
		index = 0;
	}
}

boolean CircularBuffer::eventsExceeded()
{
	uint32_t presentTime = now();
	boolean oldFound = false;
	for (int i = 0; i < num_elements; i++)
	{
		if (presentTime - array[i] > TIMEFRAME)
		{
			oldFound = true;
		}
	}
	return !oldFound;
}
