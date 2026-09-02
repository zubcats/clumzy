#pragma once
#include "iup.h"

/* ZubCut charcoal / teal, as IUP "R G B" strings. */
#define UI_BG         "20 20 20"     /* #141414 */
#define UI_SURFACE    "43 43 43"     /* #2b2b2b */
#define UI_HOVER      "56 56 56"     /* #383838 */
#define UI_PRESS      "50 50 50"     /* #323232 */
#define UI_BORDER     "61 61 61"     /* #3d3d3d */
#define UI_TEXT       "232 234 237"  /* #e8eaed */
#define UI_TEXT_MUTE  "154 154 154"  /* #9a9a9a */
#define UI_INPUT_BG   "0 0 0"        /* #000000 */
#define UI_ACCENT     "49 110 105"   /* #316E69 */
#define UI_ACCENT_HL  "61 82 79"     /* #3d524f */
#define UI_SAGE       "93 112 110"   /* #5D706E */
#define UI_SEL_TEXT   "242 242 242"  /* #f2f2f2 */
#define UI_ERROR      "192 57 43"    /* #c0392b */

Ihandle *clumzyFrame(Ihandle *child);
Ihandle *clumzyButton(const char *title);
Ihandle *clumzyToggle(const char *title);
void clumzySetAction(Ihandle *ih, Icallback cb);
void clumzyPinImageLabel(Ihandle *label, Ihandle *image, int pad);
void applyClumzyGlobals(void);
void applyClumzyTheme(Ihandle *root);
void clumzySetControlsEnabled(Ihandle *root, int enabled);
int clumzyGetControlsEnabled(Ihandle *root);
void clumzyLockText(Ihandle *ih, int locked);
