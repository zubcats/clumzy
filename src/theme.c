#include <string.h>
#include <windows.h>
#include "theme.h"

typedef HRESULT (WINAPI *SetWindowTheme_t)(HWND, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
typedef int (WINAPI *SetPreferredAppMode_t)(int);
typedef BOOL (WINAPI *AllowDarkModeForWindow_t)(HWND, BOOL);

static SetWindowTheme_t pSetWindowTheme;
static DwmSetWindowAttribute_t pDwmSetWindowAttribute;
static SetPreferredAppMode_t pSetPreferredAppMode;
static AllowDarkModeForWindow_t pAllowDarkModeForWindow;
static int darkApisLoaded;
static HWND darkenedHwnd;

static void loadDarkApis(void) {
    HMODULE ux;
    HMODULE dwm;
    if (darkApisLoaded) {
        return;
    }
    darkApisLoaded = 1;
    ux = LoadLibraryA("uxtheme.dll");
    dwm = LoadLibraryA("dwmapi.dll");
    if (ux) {
        pSetWindowTheme = (SetWindowTheme_t)GetProcAddress(ux, "SetWindowTheme");
        pSetPreferredAppMode = (SetPreferredAppMode_t)GetProcAddress(ux, (LPCSTR)(ULONG_PTR)135);
        pAllowDarkModeForWindow = (AllowDarkModeForWindow_t)GetProcAddress(ux, (LPCSTR)(ULONG_PTR)133);
    }
    if (dwm) {
        pDwmSetWindowAttribute = (DwmSetWindowAttribute_t)GetProcAddress(dwm, "DwmSetWindowAttribute");
    }
}

static void preferWinDarkMode(void) {
    loadDarkApis();
    if (pSetPreferredAppMode) {
        pSetPreferredAppMode(2);
    }
}

static void allowDark(HWND hwnd) {
    if (pAllowDarkModeForWindow && hwnd) {
        pAllowDarkModeForWindow(hwnd, TRUE);
    }
}

static void stripVisualStyles(HWND hwnd) {
    if (pSetWindowTheme && hwnd) {
        pSetWindowTheme(hwnd, L"", L"");
    }
}

static void syncFrameTitle(Ihandle *box) {
    Ihandle *title = (Ihandle*)IupGetAttribute(box, "__CLUMZY_FRAME_TITLE");
    const char *text = IupGetAttribute(box, "TITLE");
    if (title && text) {
        IupStoreAttribute(title, "TITLE", text);
        IupSetAttribute(title, "FGCOLOR", UI_ACCENT);
    }
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

static void styleText(Ihandle *ih, int enabled) {
    IupSetAttribute(ih, "READONLY", "NO");
    IupSetAttribute(ih, "ACTIVE", "YES");
    IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
}

static void copyActionToFlat(Ihandle *ih) {
    Icallback cb = IupGetCallback(ih, "ACTION");
    if (cb) {
        IupSetCallback(ih, "FLAT_ACTION", cb);
    }
}

Ihandle *clumzyFrame(Ihandle *child) {
    Ihandle *title = IupLabel("");
    Ihandle *box = IupVbox(title, child, NULL);
    IupSetAttribute(title, "FGCOLOR", UI_ACCENT);
    IupSetInt(title, "__CLUMZY_FRAME_TITLE_LBL", 1);
    IupSetAttribute(box, "NCMARGIN", "4x4");
    IupSetAttribute(box, "NCGAP", "2x2");
    IupSetInt(box, "__CLUMZY_FRAME", 1);
    IupSetAttribute(box, "__CLUMZY_FRAME_TITLE", (char*)title);
    return box;
}

Ihandle *clumzyButton(const char *title) {
    Ihandle *btn = IupFlatButton(title);
    styleButton(btn);
    return btn;
}

Ihandle *clumzyToggle(const char *title) {
    return IupToggle(title, NULL);
}

Ihandle *clumzyLabel(const char *title) {
    return IupLabel(title);
}

Ihandle *clumzyList(void) {
    return IupList(NULL);
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

void clumzyInitDarkMode(void) {
    preferWinDarkMode();
}

void applyClumzyGlobals(void) {
    preferWinDarkMode();
    IupSetGlobal("DLGBGCOLOR", UI_BG);
    IupSetGlobal("TXTBGCOLOR", UI_INPUT_BG);
    IupSetGlobal("TXTFGCOLOR", UI_TEXT);
    IupSetGlobal("TXTHLCOLOR", UI_ACCENT);
    IupSetGlobal("IMAGEAUTOSCALE", "NO");
}

void clumzyApplyWindowDarkMode(void *hwnd) {
    BOOL dark = TRUE;

    preferWinDarkMode();
    if (!hwnd || (HWND)hwnd == darkenedHwnd) {
        return;
    }
    darkenedHwnd = (HWND)hwnd;
    allowDark((HWND)hwnd);
    if (pDwmSetWindowAttribute) {
        pDwmSetWindowAttribute((HWND)hwnd, 20, &dark, sizeof(dark));
        pDwmSetWindowAttribute((HWND)hwnd, 19, &dark, sizeof(dark));
    }
}

static int ancestorEnabled(Ihandle *ih) {
    while (ih) {
        if (IupGetAttribute(ih, "CLUMZY_ENABLED")) {
            return IupGetInt(ih, "CLUMZY_ENABLED");
        }
        ih = IupGetParent(ih);
    }
    return 1;
}

static void skinAfterMap(Ihandle *ih) {
    const char *cls;
    HWND hwnd;
    Ihandle *child;

    if (!ih) {
        return;
    }
    cls = IupGetClassName(ih);
    hwnd = (HWND)IupGetAttribute(ih, "HWND");

    if (cls && strcmp(cls, "dialog") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "BACKGROUND", UI_BG);
        if (hwnd) {
            clumzyApplyWindowDarkMode(hwnd);
        }
    } else if (cls && hwnd && strcmp(cls, "text") == 0) {
        stripVisualStyles(hwnd);
        allowDark(hwnd);
        styleText(ih, ancestorEnabled(ih));
        InvalidateRect(hwnd, NULL, TRUE);
    }

    child = IupGetNextChild(ih, NULL);
    while (child) {
        skinAfterMap(child);
        child = IupGetBrother(child);
    }
}

void clumzyOnMapped(Ihandle *ih) {
    if (!ih || IupGetInt(ih, "__CLUMZY_SKINNING")) {
        return;
    }
    IupSetInt(ih, "__CLUMZY_SKINNING", 1);
    skinAfterMap(ih);
    IupSetInt(ih, "__CLUMZY_SKINNING", 0);
}

void clumzySyncToggleImages(Ihandle *root) {
    (void)root;
}

void clumzySyncList(Ihandle *ih) {
    (void)ih;
}

void clumzyStoreListItem(Ihandle *ih, const char *id, const char *text) {
    if (ih) {
        IupStoreAttribute(ih, id, text);
    }
}

void clumzySetListAttributes(Ihandle *ih, const char *attrs) {
    if (ih && attrs) {
        IupSetAttributes(ih, attrs);
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

    if (strcmp(cls, "dialog") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "BACKGROUND", UI_BG);
    } else if (strcmp(cls, "flatbutton") == 0) {
        styleButton(ih);
        copyActionToFlat(ih);
    } else if (strcmp(cls, "text") == 0) {
        styleText(ih, 1);
    } else if (strcmp(cls, "label") == 0) {
        if (IupGetAttribute(ih, "IMAGE")) {
            IupSetAttribute(ih, "EXPAND", "NO");
        } else {
            IupSetAttribute(ih, "FGCOLOR", UI_TEXT);
        }
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

static void enableOne(Ihandle *ih, int enabled) {
    const char *cls = IupGetClassName(ih);
    if (!cls) {
        return;
    }
    if (strcmp(cls, "text") == 0) {
        IupSetAttribute(ih, "CANFOCUS", "YES");
        styleText(ih, enabled);
    } else if (strcmp(cls, "toggle") == 0) {
        IupSetAttribute(ih, "ACTIVE", "YES");
    } else if (strcmp(cls, "flatbutton") == 0) {
        styleButton(ih);
        IupSetAttribute(ih, "ACTIVE", "YES");
        IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    } else if (strcmp(cls, "label") == 0 && !IupGetAttribute(ih, "IMAGE")) {
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
    HWND hwnd;
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
    hwnd = (HWND)IupGetAttribute(ih, "HWND");
    if (hwnd) {
        stripVisualStyles(hwnd);
        allowDark(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
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
