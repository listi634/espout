/**
 * @file screen.c
 * @brief Screen interface implementation.
 */

#include "screen.h"
#include "string.h"

void screen_init(screen_t *screen, screen_id_t id, const char *name)
{
    if (screen != NULL) {
        screen->id = id;
        screen->name = name;
        screen->view = NULL;
        screen->on_create = NULL;
        screen->on_activate = NULL;
        screen->on_deactivate = NULL;
        screen->on_destroy = NULL;
        screen->on_event = NULL;
        screen->on_update = NULL;
        screen->user_data = NULL;
    }
}
