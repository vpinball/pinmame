#include <windows.h>

typedef int (WINAPI *PACINITIALIZE) (void);
typedef void (WINAPI *PACSHUTDOWN) (void);
typedef int (WINAPI *PAC64SETLEDSTATES) (int id, int group, char data);
typedef int (WINAPI *PAC64SETLEDSTATE) (int id, int group, int port, int state);

extern PACINITIALIZE PacInitialize;
extern PACSHUTDOWN PacShutdown;
extern PAC64SETLEDSTATES Pac64SetLEDStates;
extern PAC64SETLEDSTATE Pac64SetLEDState;

int GetNumOutputs(void);
int GetNumGroups(void);

int LoadPacDrive(void);
void UnloadPacDrive(void);
void PacDriveSetBit(int BitNumber, int State);
void PacDriveSetOutput(int outputNum, unsigned char Value);
void UpdateOutputs(void);
void InitOutputs(void);
