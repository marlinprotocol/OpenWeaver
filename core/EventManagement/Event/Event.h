class Event {
private:
	long durationInTicks;

public:
	Event();
	long getDurationInTicks();
	void execute();
};