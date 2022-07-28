#ifndef __TVS_PREFERENCE_H__
#define __TVS_PREFERENCE_H__

#define TVS_PREFERENCE_CURRENT_MODE   "mode"
#define TVS_PREFERENCE_VOLUME         "volume"
#define TVS_PREFERENCE_SANDBOX         "_sandbox"
#define TVS_PREF_ENV_TYPE  "_env"
#define TVS_PREF_ARS_BITRATE  "_bitrate"


int tvs_preference_module_init();

int tvs_preference_get_number_value(const char *name, int *value, int default_value);

const char *tvs_preference_get_string_value(const char *name);

int tvs_preference_set_string_value(const char *name, const char *value);

int tvs_preference_set_number_value(const char *name, int value);

int tvs_preference_set_int_array_value(const char *name, const int *numbers, int count);

int tvs_preference_get_int_array_value(const char *name, int **array);


#endif
