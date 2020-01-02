#include "./CircularArray.h"

template <class Type>
Queue<Type>::Queue() {
	current = -1;
	oneParseDone = false;
}

template <class Type>
void Queue<Type>::insert(Type item) {
	if(!oneParseDone && current + 1 == MAX) oneParseDone = true;
	current = (current + 1) % MAX;
	arr[current] = item;
}

template <class T>
void Queue<T>::printArray() {
	if(current == -1) {
		LOG(INFO) << "Circular Array is empty";
		return;
	}
	
	int i = current;
	
	while(i != -1) {
		LOG(INFO) << arr[i];
		i--;
	}

	if(!oneParseDone) return;

	i = MAX - 1;

	while(i != current) {
		LOG(INFO) << arr[i];
		i--;
	}
}