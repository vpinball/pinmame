#ifndef INC_PROC_DRIVERS
#define INC_PROC_DRIVERS

const int PROC_MAX_PATTER_INTERVAL_MS = 100;
const long int CLOCKS_PER_MS = CLOCKS_PER_SEC / 1000;

class CoilDriver {
 private:
	int num;
	int patterDetectionEnable;
	clock_t timeLastChanged;
	int numPatterOn;
	int avgOnTime;
	int numPatterOff;
	int avgOffTime;
	int patterActive;
	int reqPatterState;
	int pulseTime;
	int useDefaultPulseTime;
	int patterOnTime;
	int patterOffTime;
	int useDefaultPatterTimes;

	void ResetPatter(void);
	void Drive(int state);
	void Patter(int msOn, int msOff);

 public:
	CoilDriver(void);
	void SetNum(int num);
	void SetPulseTime(int ms);
	void SetPatterTimes(int msOn, int msOff);
	void SetPatterDetectionEnable(int enable);
	int GetPatterDetectionEnable();
        int GetPulseTime();
	void CheckEndPatter(void);
	void RequestDrive(int state);
};

#endif /* INC_PROC_DRIVERS */
