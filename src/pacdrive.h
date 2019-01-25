#include <windows.h>

typedef int (WINAPI *PACINITIALIZE) (void);
typedef void (WINAPI *PACSHUTDOWN) (void);
typedef int (WINAPI *PAC64SETLEDSTATES) (int id, int group, char data);
typedef int (WINAPI *PAC64SETLEDSTATE) (int id, int group, int port, int state);
typedef int (WINAPI *PACSETLEDSTATES) (int id, short data);

extern PACINITIALIZE PacInitialize;
extern PACSHUTDOWN PacShutdown;
extern PAC64SETLEDSTATES Pac64SetLEDStates;
extern PAC64SETLEDSTATE Pac64SetLEDState;
extern PACSETLEDSTATES PacSetLEDStates;

int LoadPacDrive(void);
void UnloadPacDrive(void);
void PacDriveSetOutput(const int outputNum, const unsigned char Value);
void PacDriveUpdateOutputs(void);
