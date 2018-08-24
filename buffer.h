#pragma once

#ifndef BUFFER_h
#define BUFFER_h

template<typename T, int len>
class Buffer {
public:
	T buffer[len];
	unsigned int length = len;
	unsigned int occupied = 0;
	unsigned int index = 0;
	
	int size() {
		return length;
	}

	void push(T item) {
		if (index >= length)
			index = 0;

		if (occupied < length)
			occupied++;

		buffer[index++] = item;
	}

	Buffer() {}
};

#endif