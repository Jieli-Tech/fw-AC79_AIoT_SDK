#ifndef __TVS_DIR_HANDLER_H__
#define __TVS_DIR_HANDLER_H__

#include "tvs_directives_processor.h"


void tvs_directives_parse_metadata(const char *name, const char *metadata, int metadata_size, bool down_channel, tvs_directives_params *params);

#endif

