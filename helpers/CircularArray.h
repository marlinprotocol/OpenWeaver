#ifndef CIRCULARARRAY_H_
#define CIRCULARARRAY_H_

#include "./Logger/easylogging.h"

#define MAX 50

template <class T>
class Queue
{
private:
	T arr[MAX];
	int current;
	bool oneParseDone;

public:
	Queue();
	void insert(T item);
	void printArray(); 
};

#endif /*CIRCULARARRAY_H_*/
