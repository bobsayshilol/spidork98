#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <go32.h>
#include <sys/farptr.h>

// TODO: is this safe to replace with a word read?
#define read_keyboard_state() ( \
    (_farpeekb(_dos_ds, 0x052A + 2) << 24) | /* IUYTREWQ */ \
    (_farpeekb(_dos_ds, 0x052A + 3) << 16) | /* DSAe.[PO */ \
    (_farpeekb(_dos_ds, 0x052A + 6) <<  8) | /* ... .?>< */ \
    (_farpeekb(_dos_ds, 0x052A + 7) <<  0)   /* ..drlu.. */ \
  )

#define KB_STATE_Q (1 << 24)
#define KB_STATE_W (1 << 25)
#define KB_STATE_E (1 << 26)
#define KB_STATE_A (1 << 21)
#define KB_STATE_S (1 << 22)
#define KB_STATE_D (1 << 23)
#define KB_STATE_ENTER (1 << 20)
#define KB_STATE_SPACE (1 << 12)
#define KB_STATE_DOWN (1 << 5)
#define KB_STATE_UP (1 << 2)
#define KB_STATE_LEFT (1 << 3)
#define KB_STATE_RIGHT (1 << 4)

#endif
