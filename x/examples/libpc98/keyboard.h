#ifndef KEYBOARD_H
#define KEYBOARD_H

#ifndef WEB_BUILD

#include <go32.h>
#include <sys/farptr.h>

// TODO: is this safe to replace with a word read?
#define read_keyboard_state() ( \
    (_farpeekb(_dos_ds, 0x052A + 2) << 24) | /* IUYTREWQ */ \
    (_farpeekb(_dos_ds, 0x052A + 3) << 16) | /* DSAe.[PO */ \
    (_farpeekb(_dos_ds, 0x052A + 6) <<  8) | /* ... .?>< */ \
    (_farpeekb(_dos_ds, 0x052A + 7) <<  0)   /* ..drlu.. */ \
  )

#else

#include "types.h"
u32 read_keyboard_state();

#endif

#define KB_STATE_Q (1U << 24)
#define KB_STATE_W (1U << 25)
#define KB_STATE_E (1U << 26)
#define KB_STATE_R (1U << 27)
#define KB_STATE_T (1U << 28)
#define KB_STATE_Y (1U << 29)
#define KB_STATE_U (1U << 30)
#define KB_STATE_I (1U << 31)
#define KB_STATE_O (1U << 16)
#define KB_STATE_P (1U << 17)
#define KB_STATE_A (1U << 21)
#define KB_STATE_S (1U << 22)
#define KB_STATE_D (1U << 23)
#define KB_STATE_ENTER (1U << 20)
#define KB_STATE_SPACE (1U << 12)
#define KB_STATE_DOWN (1U << 5)
#define KB_STATE_UP (1U << 2)
#define KB_STATE_LEFT (1U << 3)
#define KB_STATE_RIGHT (1U << 4)

#endif
