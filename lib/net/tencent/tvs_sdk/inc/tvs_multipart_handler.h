#ifndef __TVS_MULTIPART_HANDLER_H__
#define __TVS_MULTIPART_HANDLER_H__

#include "tvs_multipart_parser.h"

#include "tvs_directives_processor.h"

typedef struct {
    int has_tts;
    int tts_bytes;
    int control_type;
    tvs_directives_params directives_param;
} tvs_multi_param;

void tvs_init_multipart_callback(tvs_multipart_callback *multipart_callback);

void tvs_release_multipart_parser(void **multipart_info, bool down_channel);

void tvs_multipart_process_chunked(void **multipart_info, tvs_multipart_callback *multipart_callback,
                                   tvs_multi_param *param, void *ev_data, bool jump, bool down_channel);

void tvs_multipart_handler_init();

#endif
