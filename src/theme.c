#include <string.h>
#include "theme.h"

Ihandle *clumzyFrame(Ihandle *child) {
    Ihandle *frame = IupFlatFrame(child);
    IupSetAttribute(frame, "BGCOLOR", UI_BG);
    IupSetAttribute(frame, "TITLECOLOR", UI_TEXT);
    IupSetAttribute(frame, "TITLEBGCOLOR", UI_BG);
    IupSetAttribute(frame, "TITLEALIGNMENT", "ALEFT");
    IupSetAttribute(frame, "FRAME", "CROSSTITLE");
    IupSetAttribute(frame, "FRAMECOLOR", UI_BORDER);
    IupSetAttribute(frame, "TITLELINECOLOR", UI_BORDER);
    IupSetAttribute(frame, "FRAMEWIDTH", "1");
    return frame;
}

Ihandle *clumzyButton(const char *title) {
    Ihandle *btn = IupFlatButton(title);
    IupSetAttribute(btn, "BGCOLOR", UI_SURFACE);
    IupSetAttribute(btn, "FGCOLOR", UI_TEXT);
    IupSetAttribute(btn, "HLCOLOR", UI_HOVER);
    IupSetAttribute(btn, "PSCOLOR", UI_PRESS);
    IupSetAttribute(btn, "TEXTHLCOLOR", UI_TEXT);
    IupSetAttribute(btn, "TEXTPSCOLOR", UI_TEXT);
    IupSetAttribute(btn, "BORDERCOLOR", UI_BORDER);
    IupSetAttribute(btn, "BORDERHLCOLOR", UI_ACCENT);
    IupSetAttribute(btn, "BORDERPSCOLOR", UI_ACCENT);
    IupSetAttribute(btn, "BORDERWIDTH", "1");
    IupSetAttribute(btn, "SHOWBORDER", "YES");
    return btn;
}

Ihandle *clumzyToggle(const char *title) {
    Ihandle *toggle = IupToggle(title, NULL);
    IupSetAttribute(toggle, "FGCOLOR", UI_TEXT);
    IupSetAttribute(toggle, "BGCOLOR", UI_BG);
    return toggle;
}

void clumzySetAction(Ihandle *ih, Icallback cb) {
    IupSetCallback(ih, "ACTION", cb);
    IupSetCallback(ih, "FLAT_ACTION", cb);
}

void clumzyPinImageLabel(Ihandle *label, Ihandle *image, int pad) {
    int w, h;
    if (!label || !image) {
        return;
    }
    IupSetAttribute(image, "AUTOSCALE", "NO");
    w = IupGetInt(image, "WIDTH");
    h = IupGetInt(image, "HEIGHT");
    IupSetAttribute(label, "EXPAND", "NO");
    IupSetfAttribute(label, "PADDING", "%dx%d", pad, pad);
    IupSetfAttribute(label, "RASTERSIZE", "%dx%d", w + pad * 2, h + pad * 2);
}

void applyClumzyGlobals(void) {
    IupSetGlobal("DLGBGCOLOR", UI_BG);
    IupSetGlobal("DLGFGCOLOR", UI_TEXT);
    IupSetGlobal("TXTBGCOLOR", UI_INPUT_BG);
    IupSetGlobal("TXTFGCOLOR", UI_TEXT);
    IupSetGlobal("IMAGEAUTOSCALE", "NO");
}

static void copyActionToFlat(Ihandle *ih) {
    Icallback cb = IupGetCallback(ih, "ACTION");
    if (cb) {
        IupSetCallback(ih, "FLAT_ACTION", cb);
    }
}

static void themeOne(Ihandle *ih) {
    const char *cls;

    if (!ih) {
        return;
    }
    cls = IupGetClassName(ih);
    if (!cls) {
        return;
    }

    if (strcmp(cls, "dialog") == 0 ||
        strcmp(cls, "vbox") == 0 ||
        strcmp(cls, "hbox") == 0 ||
        strcmp(cls, "zbox") == 0 ||
        strcmp(cls, "cbox") == 0 ||
        strcmp(cls, "fill") == 0 ||
        strcmp(cls, "flatframe") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        if (strcmp(cls, "flatframe") == 0) {
            IupSetAttribute(ih, "TITLECOLOR", UI_TEXT);
            IupSetAttribute(ih, "TITLEBGCOLOR", UI_BG);
            IupSetAttribute(ih, "FRAMECOLOR", UI_BORDER);
            IupSetAttribute(ih, "TITLELINECOLOR", UI_BORDER);
        }
    } else if (strcmp(cls, "label") == 0) {
        if (IupGetAttribute(ih, "IMAGE")) {
            IupSetAttribute(ih, "EXPAND", "NO");
            IupSetAttribute(ih, "BGCOLOR", UI_BG);
            return;
        }
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
    } else if (strcmp(cls, "text") == 0 || strcmp(cls, "list") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
        IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
    } else if (strcmp(cls, "toggle") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
    } else if (strcmp(cls, "flatbutton") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_SURFACE);
        IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
        IupSetAttribute(ih, "HLCOLOR", UI_HOVER);
        IupSetAttribute(ih, "PSCOLOR", UI_PRESS);
        IupSetAttribute(ih, "BORDERCOLOR", UI_BORDER);
        IupSetAttribute(ih, "BORDERHLCOLOR", UI_ACCENT);
        IupSetAttribute(ih, "BORDERPSCOLOR", UI_ACCENT);
        copyActionToFlat(ih);
    }
}

static int lockedTextKAny(Ihandle *ih, int c) {
    (void)c;
    return IupGetInt(ih, "CLUMZY_LOCKED") ? IUP_IGNORE : IUP_CONTINUE;
}

static int lockedTextChanged(Ihandle *ih) {
    Icallback prev;

    if (IupGetInt(ih, "CLUMZY_LOCKED")) {
        const char *locked = IupGetAttribute(ih, "CLUMZY_LOCKVALUE");
        const char *cur = IupGetAttribute(ih, "VALUE");
        if (locked && (!cur || strcmp(locked, cur) != 0)) {
            IupStoreAttribute(ih, "VALUE", locked);
        }
        return IUP_DEFAULT;
    }
    prev = (Icallback)IupGetAttribute(ih, "__CLUMZY_VC");
    return prev ? prev(ih) : IUP_DEFAULT;
}

static void installTextLockGuards(Ihandle *ih) {
    if (IupGetInt(ih, "CLUMZY_GUARDS")) {
        return;
    }
    IupSetInt(ih, "CLUMZY_GUARDS", 1);
    IupSetAttribute(ih, "__CLUMZY_VC", (char*)IupGetCallback(ih, "VALUECHANGED_CB"));
    IupSetCallback(ih, "VALUECHANGED_CB", (Icallback)lockedTextChanged);
    IupSetCallback(ih, "K_ANY", (Icallback)lockedTextKAny);
}

/* Native Win32 edits/toggles paint white when ACTIVE=NO or READONLY=YES.
   Keep them active and only mute colors. */
static void enableOne(Ihandle *ih, int enabled) {
    const char *cls = IupGetClassName(ih);
    if (!cls) {
        return;
    }
    if (strcmp(cls, "text") == 0) {
        IupSetAttribute(ih, "READONLY", "NO");
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "CANFOCUS", "YES");
        IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    } else if (strcmp(cls, "toggle") == 0) {
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    } else if (strcmp(cls, "flatbutton") == 0) {
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "BGCOLOR", UI_SURFACE);
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
        IupSetAttribute(ih, "HLCOLOR", UI_HOVER);
        IupSetAttribute(ih, "PSCOLOR", UI_PRESS);
        IupSetAttribute(ih, "BORDERCOLOR", UI_BORDER);
    } else if (strcmp(cls, "label") == 0 && !IupGetAttribute(ih, "IMAGE")) {
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    }
}

static void enableWalk(Ihandle *ih, int enabled) {
    Ihandle *child;
    enableOne(ih, enabled);
    child = IupGetNextChild(ih, NULL);
    while (child) {
        enableWalk(child, enabled);
        child = IupGetBrother(child);
    }
}

void clumzySetControlsEnabled(Ihandle *root, int enabled) {
    if (!root) {
        return;
    }
    IupSetInt(root, "CLUMZY_ENABLED", enabled);
    enableWalk(root, enabled);
}

int clumzyGetControlsEnabled(Ihandle *root) {
    return root ? IupGetInt(root, "CLUMZY_ENABLED") : 0;
}

static void refreshWalk(Ihandle *ih) {
    Ihandle *child;
    if (!ih) {
        return;
    }
    if (IupGetAttribute(ih, "CLUMZY_ENABLED")) {
        enableWalk(ih, IupGetInt(ih, "CLUMZY_ENABLED"));
    }
    child = IupGetNextChild(ih, NULL);
    while (child) {
        refreshWalk(child);
        child = IupGetBrother(child);
    }
}

void clumzyRefreshControlsEnabled(Ihandle *root) {
    refreshWalk(root);
}

void clumzyLockText(Ihandle *ih, int locked) {
    if (!ih) {
        return;
    }
    installTextLockGuards(ih);
    IupSetInt(ih, "CLUMZY_LOCKED", locked);
    IupSetAttribute(ih, "READONLY", "NO");
    IupSetAttribute(ih, "ACTIVE", "YES");
    IupSetAttribute(ih, "CANFOCUS", locked ? "NO" : "YES");
    IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(ih, "FGCOLOR", locked ? UI_TEXT_MUTE : UI_TEXT);
    if (locked) {
        IupStoreAttribute(ih, "CLUMZY_LOCKVALUE", IupGetAttribute(ih, "VALUE"));
    }
}

void applyClumzyTheme(Ihandle *root) {
    Ihandle *child;

    if (!root) {
        return;
    }
    themeOne(root);
    child = IupGetNextChild(root, NULL);
    while (child) {
        applyClumzyTheme(child);
        child = IupGetBrother(child);
    }
}
