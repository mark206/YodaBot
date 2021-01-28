#ifndef CIRCULAR_BUFFER_h
#define CIRCULAR_BUFFER_h

#define READINGS 3

class CircularBuffer {
public:
	CircularBuffer(int size);
	void addElement(uint32_t newElement);
	void init();
	boolean eventsExceeded();

private:
	uint32_t* array;
	int index = 0;
	int num_elements = READINGS;
};

#endif