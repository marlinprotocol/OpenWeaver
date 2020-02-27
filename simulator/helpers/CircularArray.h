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
	int totalInsertions;

public:
	Queue();
	int getQueueSize();
	void insert(T item);
	void printArray(); 
	int getTotalInsertions();
	int getOldestItemIndex();
	T& getItemAtIndex(int index);
};

#include "./CircularArray.ipp"

#endif /*CIRCULARARRAY_H_*/
