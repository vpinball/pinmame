/***************************************************************************

	Generic (PC-style) IDE controller implementation

***************************************************************************/

#include "harddisk.h"


#define MAX_IDE_CONTROLLERS			1

struct ide_interface
{
	void 	(*interrupt)(int state);
};

int ide_controller_init(int which, struct ide_interface *intf, const char *diskname1, const char *diskname2);
void ide_controller_reset(int which);

READ32_HANDLER( ide_controller32_0_r );
WRITE32_HANDLER( ide_controller32_0_w );

READ16_HANDLER( ide_controller16_0_r );
WRITE16_HANDLER( ide_controller16_0_w );
