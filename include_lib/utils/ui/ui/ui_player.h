#ifndef UI_PLAYER_H
#define UI_PLAYER_H



#include "ui/control.h"
#include "ui/ui_core.h"






struct ui_player {
    struct element elm; 	//must be first
    int fd;
    const struct ui_player_info *info;
    const struct element_event_handler *handler;
};



#define ui_player_for_id(id) \
	(struct ui_player*)ui_core_get_element_by_id(id)



void register_ui_player_handler(const struct element_event_handler *handler);























#endif
