#include <gk.h>

GK_BOOL init()
{
	/* Initialize game here */
	return GK_TRUE;
}

void cleanup()
{
	/* Cleanup game here */
}

GK_APP(init, cleanup)