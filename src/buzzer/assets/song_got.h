/**
 * @file song_got.h
 * @brief Game of Thrones main theme melody asset.
 */

#ifndef SONG_GOT_H
#define SONG_GOT_H

#include "buzzer/buzzer.h"
#include "buzzer/buzzer_notes.h"

static const buzzer_note_t song_got_notes[] = {
    /* Measure 1 - 4 */
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_E4, 16 },  { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_E4, 16 },  { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_E4, 16 },  { NOTE_F4, 16 },
    { NOTE_G4, 8 }, { NOTE_C4, 8 }, { NOTE_E4, 16 },  { NOTE_F4, 16 },

    /* Measure 5 - 12 */
    { NOTE_G4, -4 }, { NOTE_C4, -4 },
    { NOTE_DS4, 16 }, { NOTE_F4, 16 }, { NOTE_G4, 4 }, { NOTE_C4, 4 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_D4, -1 },
    { NOTE_F4, -4 }, { NOTE_AS3, -4 },
    { NOTE_DS4, 16 }, { NOTE_D4, 16 }, { NOTE_F4, 4 }, { NOTE_AS3, -4 },
    { NOTE_DS4, 16 }, { NOTE_D4, 16 }, { NOTE_C4, -1 },

    /* Repeat section from Measure 5 */
    { NOTE_G4, -4 }, { NOTE_C4, -4 },
    { NOTE_DS4, 16 }, { NOTE_F4, 16 }, { NOTE_G4, 4 }, { NOTE_C4, 4 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },
    { NOTE_D4, -1 },
    { NOTE_F4, -4 }, { NOTE_AS3, -4 },
    { NOTE_DS4, 16 }, { NOTE_D4, 16 }, { NOTE_F4, 4 }, { NOTE_AS3, -4 },
    { NOTE_DS4, 16 }, { NOTE_D4, 16 }, { NOTE_C4, -1 },
    { NOTE_G4, -4 }, { NOTE_C4, -4 },
    { NOTE_DS4, 16 }, { NOTE_F4, 16 }, { NOTE_G4, 4 }, { NOTE_C4, 4 }, { NOTE_DS4, 16 }, { NOTE_F4, 16 },

    /* Measure 15 - 27 */
    { NOTE_D4, -2 },
    { NOTE_F4, -4 }, { NOTE_AS3, -4 },
    { NOTE_D4, -8 }, { NOTE_DS4, -8 }, { NOTE_D4, -8 }, { NOTE_AS3, -8 },
    { NOTE_C4, -1 },
    { NOTE_C5, -2 },
    { NOTE_AS4, -2 },
    { NOTE_C4, -2 },
    { NOTE_G4, -2 },
    { NOTE_DS4, -2 },
    { NOTE_DS4, -4 }, { NOTE_F4, -4 },
    { NOTE_G4, -1 },

    /* Measure 28 - 35 */
    { NOTE_C5, -2 },
    { NOTE_AS4, -2 },
    { NOTE_C4, -2 },
    { NOTE_G4, -2 },
    { NOTE_DS4, -2 },
    { NOTE_DS4, -4 }, { NOTE_D4, -4 },
    { NOTE_C5, 8 }, { NOTE_G4, 8 }, { NOTE_GS4, 16 }, { NOTE_AS4, 16 },
    { NOTE_C5, 8 }, { NOTE_G4, 8 }, { NOTE_GS4, 16 }, { NOTE_AS4, 16 },
    { NOTE_C5, 8 }, { NOTE_G4, 8 }, { NOTE_GS4, 16 }, { NOTE_AS4, 16 },
    { NOTE_C5, 8 }, { NOTE_G4, 8 }, { NOTE_GS4, 16 }, { NOTE_AS4, 16 },

    /* Finale */
    { REST, 4 },
    { NOTE_GS5, 16 }, { NOTE_AS5, 16 }, { NOTE_C6, 8 }, { NOTE_G5, 8 }, { NOTE_GS5, 16 }, { NOTE_AS5, 16 },
    { NOTE_C6, 8 },  { NOTE_G5, 16 }, { NOTE_GS5, 16 }, { NOTE_AS5, 16 },
    { NOTE_C6, 8 },  { NOTE_G5, 8 },  { NOTE_GS5, 16 }, { NOTE_AS5, 16 },
    { NOTE_C6, 8 },  { NOTE_G5, 16 }, { NOTE_GS5, 16 }, { NOTE_AS5, 16 }
};

static const buzzer_melody_t song_got = {
    .name       = "Game of Thrones Theme",
    .tempo_bpm  = 85,
    .notes      = song_got_notes,
    .note_count = sizeof(song_got_notes) / sizeof(song_got_notes[0])
};

#endif /* SONG_GOT_H */