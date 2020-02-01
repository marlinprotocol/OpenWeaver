template <class Type>
Queue<Type>::Queue() {
	current = -1;
	totalInsertions = 0;
	oneParseDone = false;
}

template <class Type>
int Queue<Type>::getQueueSize() {
	return MAX;
}

template <class Type>
void Queue<Type>::insert(Type item) {
	if(!oneParseDone && current + 1 == MAX) oneParseDone = true;
	current = (current + 1) % MAX;
	arr[current] = item;
	totalInsertions++;
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

template <class T>
int Queue<T>::getTotalInsertions() {
	return totalInsertions;
}

template <class T>
int Queue<T>::getOldestItemIndex() {
	if(totalInsertions == 0) return -1;
	if(!oneParseDone) return 0;

	return (current + 1) % MAX;
}

template <class T>
T& Queue<T>::getItemAtIndex(int index) {
	if(index<0 || index>=MAX || (!oneParseDone && index>current)) {
		LOG(FATAL) << "Invalid index in Circular Array accessed. "
				   << "Index accessed: " << index
				   << ", Current: " << current
				   << ", TotalInsertions: " << totalInsertions;
	}

	return arr[index];
}