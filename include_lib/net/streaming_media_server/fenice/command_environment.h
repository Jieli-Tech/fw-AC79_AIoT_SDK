#ifndef _COMMAND_ENVIRONMENTH
#define _COMMAND_ENVIRONMENTH

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include "prefs.h"
#include "fnc_log.h"
#include "types.h"

uint32 command_environment(int, char **);
#endif
