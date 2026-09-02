#include <string.h>
#include <windows.h>
#include "theme.h"

static void styleFrame(Ihandle *frame) {
    IupSetAttribute(frame, "BGCOLOR", UI_BG);
    IupSetAttribute(frame, "TITLECOLOR", UI_ACCENT);
    IupSetAttribute(frame, "TITLEBGCOLOR", UI_BG);
    IupSetAttribute(frame, "TITLEALIGNMENT", "ALEFT");
    IupSetAttribute(frame, "FRAME", "CROSSTITLE");
    IupSetAttribute(frame, "FRAMECOLOR", UI_ACCENT);
    IupSetAttribute(frame, "TITLELINECOLOR", UI_ACCENT);
    IupSetAttribute(frame, "FRAMEWIDTH", "1");
}

static void styleButton(Ihandle *btn) {
    IupSetAttribute(btn, "BGCOLOR", UI_SURFACE);
    IupSetAttribute(btn, "FGCOLOR", UI_TEXT);
    IupSetAttribute(btn, "HLCOLOR", UI_HOVER);
    IupSetAttribute(btn, "PSCOLOR", UI_PRESS);
    IupSetAttribute(btn, "TEXTHLCOLOR", UI_TEXT);
    IupSetAttribute(btn, "TEXTPSCOLOR", UI_TEXT);
    IupSetAttribute(btn, "BORDERCOLOR", UI_SAGE);
    IupSetAttribute(btn, "BORDERHLCOLOR", UI_ACCENT);
    IupSetAttribute(btn, "BORDERPSCOLOR", UI_ACCENT);
    IupSetAttribute(btn, "BORDERWIDTH", "1");
    IupSetAttribute(btn, "SHOWBORDER", "YES");
}

static void styleToggle(Ihandle *toggle, int enabled) {
    IupSetAttribute(toggle, "BGCOLOR", UI_BG);
    IupSetAttribute(toggle, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "HLCOLOR", UI_HOVER);
    IupSetAttribute(toggle, "PSCOLOR", UI_PRESS);
    IupSetAttribute(toggle, "TEXTHLCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "TEXTPSCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "BORDERWIDTH", "0");
    IupSetAttribute(toggle, "ALIGNMENT", "ALEFT:ACENTER");
    IupSetAttribute(toggle, "EXPAND", "NO");
    IupSetAttribute(toggle, "ACTIVE", "YES");
}

static void styleText(Ihandle *ih, int enabled) {
    IupSetAttribute(ih, "READONLY", "NO");
    IupSetAttribute(ih, "ACTIVE", "YES");
    IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
}

static void preferWinDarkMode(void) {
    HMODULE ux = LoadLibraryA("uxtheme.dll");
    FARPROC setMode;
    if (!ux) {
        return;
    }
    setMode = GetProcAddress(ux, (LPCSTR)(ULONG_PTR)135);
    if (setMode) {
        /* PreferredAppMode ForceDark */
        ((int (WINAPI *)(int))setMode)(2);
    }
}

Ihandle *clumzyFrame(Ihandle *child) {
    Ihandle *frame = IupFlatFrame(child);
    styleFrame(frame);
    return frame;
}

Ihandle *clumzyButton(const char *title) {
    Ihandle *btn = IupFlatButton(title);
    styleButton(btn);
    return btn;
}

Ihandle *clumzyToggle(const char *title) {
    Ihandle *toggle = IupFlatToggle(title);
    styleToggle(toggle, 1);
    return toggle;
}

Ihandle *clumzyLabel(const char *title) {
    Ihandle *label;
    if (!title) {
        label = IupLabel(NULL);
        IupSetAttribute(label, "BGCOLOR", UI_BG);
        return label;
    }
    label = IupFlatLabel(title);
    IupSetAttribute(label, "FGCOLOR", UI_ACCENT);
    IupSetAttribute(label, "ALIGNMENT", "ALEFT:ACENTER");
    return label;
}

Ihandle *clumzyList(void) {
    Ihandle *list = IupList(NULL);
    IupSetAttribute(list, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(list, "FGCOLOR", UI_TEXT);
    return list;
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
    preferWinDarkMode();
    IupSetGlobal("DLGBGCOLOR", UI_BG);
    IupSetGlobal("DLGFGCOLOR", UI_TEXT);
    IupSetGlobal("TXTBGCOLOR", UI_INPUT_BG);
    IupSetGlobal("TXTFGCOLOR", UI_TEXT);
    IupSetGlobal("TXTHLCOLOR", UI_ACCENT);
    IupSetGlobal("IMAGEAUTOSCALE", "NO");
}

void clumzyApplyWindowDarkMode(void *hwnd) {
    BOOL dark = TRUE;
    HMODULE dwm;
    HMODULE ux;
    FARPROC setAttr;
    FARPROC allow;

    preferWinDarkMode();
    if (!hwnd) {
        return;
    }
    dwm = LoadLibraryA("dwmapi.dll");
    if (dwm) {
        setAttr = GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (setAttr) {
            typedef HRESULT (WINAPI *DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
            ((DwmSetWindowAttribute_t)setAttr)((HWND)hwnd, 20, &dark, sizeof(dark));
            ((DwmSetWindowAttribute_t)setAttr)((HWND)hwnd, 19, &dark, sizeof(dark));
        }
    }
    ux = LoadLibraryA("uxtheme.dll");
    if (ux) {
        allow = GetProcAddress(ux, (LPCSTR)(ULONG_PTR)133);
        if (allow) {
            ((BOOL (WINAPI *)(HWND, BOOL))allow)((HWND)hwnd, TRUE);
        }
    }
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
        if (strcmp(cls, "dialog") == 0) {
            IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
        }
        if (strcmp(cls, "flatframe") == 0) {
            styleFrame(ih);
        }
    } else if (strcmp(cls, "flatlabel") == 0) {
        if (!IupGetAttribute(ih, "IMAGE")) {
            IupSetAttribute(ih, "FGCOLOR", UI_ACCENT);
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
        styleText(ih, 1);
    } else if (strcmp(cls, "flattoggle") == 0) {
        styleToggle(ih, 1);
        copyActionToFlat(ih);
    } else if (strcmp(cls, "toggle") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
    } else if (strcmp(cls, "flatbutton") == 0) {
        styleButton(ih);
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

/* Native Win32 edits paint white when ACTIVE=NO or READONLY=YES.
   Keep them active and only mute colors. Flat controls stay drawn by IUP. */
static void enableOne(Ihandle *ih, int enabled) {
    const char *cls = IupGetClassName(ih);
    if (!cls) {
        return;
    }
    if (strcmp(cls, "text") == 0) {
        IupSetAttribute(ih, "CANFOCUS", "YES");
        styleText(ih, enabled);
    } else if (strcmp(cls, "flattoggle") == 0 || strcmp(cls, "toggle") == 0) {
        styleToggle(ih, enabled);
        if (strcmp(cls, "toggle") == 0) {
            IupSetAttribute(ih, "ACTIVE", "YES");
            IupSetAttribute(ih, "BGCOLOR", UI_BG);
            IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
        }
    } else if (strcmp(cls, "flatbutton") == 0) {
        styleButton(ih);
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    } else if (strcmp(cls, "flatlabel") == 0) {
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_ACCENT : UI_SAGE);
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
