/**
 * @file ir_config.h
 * @brief Global remote control profile configurations.
 */

#ifndef IR_CONFIG_H
#define IR_CONFIG_H

#include <stdint.h>

/**
 * @brief Enumeration of all supported application key actions (Global Scope).
 */
typedef enum {
    IR_KEY_UNKNOWN = 0,
    IR_KEY_ON,
    IR_KEY_OFF,
    IR_KEY_PCI,
    IR_KEY_HDMI,
    IR_KEY_SOURCE,
    IR_KEY_OK,
    IR_KEY_UP,
    IR_KEY_DOWN,
    IR_KEY_LEFT,
    IR_KEY_RIGHT,
    IR_KEY_BACK,
    IR_KEY_MENU,
    IR_KEY_AUTO,
    IR_KEY_ECO_BLANK,
    IR_KEY_FREEZE,
    IR_KEY_ASPECT,
    IR_KEY_PAGE_UP,
    IR_KEY_PAGE_DOWN,
    IR_KEY_SMART_ECO,
    IR_KEY_VOLUME_UP,
    IR_KEY_VOLUME_DOWN,
    IR_KEY_MUTE,
    IR_KEY_ZOOM_UP,
    IR_KEY_ZOOM_DOWN,
    IR_KEY_QUICK_INSTALL
} ir_key_t;

/**
 * @brief Mapping entry structure connecting a specific HEX code to a key action.
 */
typedef struct {
    uint32_t hex_code;
    ir_key_t key_id;
    const char *key_name;
} ir_lookup_entry_t;

/* --- BENQ REMOTE CONTROL PROFILE --- */
static const ir_lookup_entry_t s_ir_profile_benq[] = {
    { 0xB04F3000, IR_KEY_ON,            "ON" },
    { 0xB14E3000, IR_KEY_OFF,           "OFF" },
    { 0xBE413000, IR_KEY_PCI,           "PCI" },
    { 0xA7583000, IR_KEY_HDMI,          "HDMI" },
    { 0xFB043000, IR_KEY_SOURCE,        "SOURCE" },
    { 0xEA153000, IR_KEY_OK,            "OK" },
    { 0xF40B3000, IR_KEY_UP,            "UP" },
    { 0xF30C3000, IR_KEY_DOWN,          "DOWN" },
    { 0xF20D3000, IR_KEY_LEFT,          "LEFT" },
    { 0xF10E3000, IR_KEY_RIGHT,         "RIGHT" },
    { 0x7A853000, IR_KEY_BACK,          "BACK" },
    { 0xF00F3000, IR_KEY_MENU,          "MENU" },
    { 0xF7083000, IR_KEY_AUTO,          "AUTO" },
    { 0xF8073000, IR_KEY_ECO_BLANK,     "ECO_BLANK" },
    { 0xFC033000, IR_KEY_FREEZE,        "FREEZE" },
    { 0xEC133000, IR_KEY_ASPECT,        "ASPECT" },
    { 0xFA053000, IR_KEY_PAGE_UP,       "PAGE_UP" },
    { 0xF9063000, IR_KEY_PAGE_DOWN,     "PAGE_DOWN" },
    { 0xCF303000, IR_KEY_SMART_ECO,     "SMART_ECO" },
    { 0x7D823000, IR_KEY_VOLUME_UP,     "VOLUME_UP" },
    { 0x7C833000, IR_KEY_VOLUME_DOWN,   "VOLUME_DOWN" },
    { 0xEB143000, IR_KEY_MUTE,          "MUTE" },
    { 0xE7183000, IR_KEY_ZOOM_UP,       "ZOOM_UP" },
    { 0xE6193000, IR_KEY_ZOOM_DOWN,     "ZOOM_DOWN" },
    { 0x6B943000, IR_KEY_QUICK_INSTALL, "QUICK_INSTALL" }
};

/* --- Implement other profiles ---
static const ir_lookup_entry_t s_ir_profile_sony[] = {
    { 0x10, IR_KEY_ON, "SONY_ON" },
    ...
};
*/

#endif /* IR_CONFIG_H */