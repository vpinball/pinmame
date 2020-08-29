#ifndef SN76496_H
#define SN76496_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define MAX_76496 4

struct SN76489interface
{
    int num;	/* total number of 76496 in the machine */
    double baseclock[MAX_76496];
    int volume[MAX_76496];
};

struct SN76489Ainterface
{
    int num;	/* total number of 76496 in the machine */
    double baseclock[MAX_76496];
    int volume[MAX_76496];
};

struct SN76494interface
{
    int num;	/* total number of 76496 in the machine */
    double baseclock[MAX_76496];
    int volume[MAX_76496];
};

struct SN76496interface
{
    int num;	/* total number of 76496 in the machine */
    double baseclock[MAX_76496];
    int volume[MAX_76496];
};

int SN76489_sh_start(const struct MachineSound *msound);
int SN76489A_sh_start(const struct MachineSound *msound);
int SN76494_sh_start(const struct MachineSound *msound);
int SN76496_sh_start(const struct MachineSound *msound);
int gamegear_sh_start(const struct MachineSound *msound);
int smsiii_sh_start(const struct MachineSound *msound);

WRITE_HANDLER(SN76489_0_w);
WRITE_HANDLER( SN76489_1_w );
WRITE_HANDLER( SN76489_2_w );
WRITE_HANDLER( SN76489_3_w );

WRITE_HANDLER(SN76489A_0_w);
WRITE_HANDLER( SN76489A_1_w );
WRITE_HANDLER( SN76489A_2_w );
WRITE_HANDLER( SN76489A_3_w );

WRITE_HANDLER(SN76494_0_w);
WRITE_HANDLER( SN76494_1_w );
WRITE_HANDLER( SN76494_2_w );
WRITE_HANDLER( SN76494_3_w );

WRITE_HANDLER(SN76496_0_w);
WRITE_HANDLER( SN76496_1_w );
WRITE_HANDLER( SN76496_2_w );
WRITE_HANDLER( SN76496_3_w );

#endif
