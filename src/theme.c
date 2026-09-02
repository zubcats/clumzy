#include <string.h>
#include <windows.h>
#include "theme.h"

#define CHECK_PX 14
#define CLUMZY_LABEL "__CLUMZY_LABEL"
#define CLUMZY_FG "__CLUMZY_FG"

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

static void skinHwndTree(HWND hwnd) {
    HWND child;
    if (!hwnd) {
        return;
    }
    allowDark(hwnd);
    stripVisualStyles(hwnd);
    child = FindWindowExA(hwnd, NULL, NULL, NULL);
    while (child) {
        skinHwndTree(child);
        child = FindWindowExA(hwnd, child, NULL, NULL);
    }
}

static void ensureCheckImages(void) {
    unsigned char offPix[CHECK_PX * CHECK_PX];
    unsigned char onPix[CHECK_PX * CHECK_PX];
    Ihandle *off;
    Ihandle *on;
    int x, y, i;

    if (IupGetHandle("clumzy_check_off")) {
        return;
    }
    for (y = 0; y < CHECK_PX; ++y) {
        for (x = 0; x < CHECK_PX; ++x) {
            i = y * CHECK_PX + x;
            offPix[i] = (x == 0 || y == 0 || x == CHECK_PX - 1 || y == CHECK_PX - 1) ? 1 : 0;
            onPix[i] = offPix[i];
        }
    }
    for (i = 0; i < 4; ++i) {
        onPix[(8 + i) * CHECK_PX + (3 + i)] = 2;
    }
    for (i = 0; i < 6; ++i) {
        onPix[(10 - i) * CHECK_PX + (6 + i)] = 2;
    }

    off = IupImage(CHECK_PX, CHECK_PX, offPix);
    on = IupImage(CHECK_PX, CHECK_PX, onPix);
    IupSetAttribute(off, "0", UI_SURFACE);
    IupSetAttribute(off, "1", UI_SAGE);
    IupSetAttribute(off, "2", UI_ACCENT);
    IupSetAttribute(on, "0", UI_SURFACE);
    IupSetAttribute(on, "1", UI_SAGE);
    IupSetAttribute(on, "2", UI_ACCENT);
    IupSetAttribute(off, "AUTOSCALE", "NO");
    IupSetAttribute(on, "AUTOSCALE", "NO");
    IupSetHandle("clumzy_check_off", off);
    IupSetHandle("clumzy_check_on", on);
}

static void syncToggleImage(Ihandle *ih) {
    IupSetAttribute(ih, "IMAGE", IupGetInt(ih, "VALUE") ? "clumzy_check_on" : "clumzy_check_off");
}

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

static int isLabelButton(Ihandle *ih) {
    return IupGetInt(ih, CLUMZY_LABEL);
}

static const char *labelFg(Ihandle *label, int enabled) {
    const char *fg = IupGetAttribute(label, CLUMZY_FG);
    if (fg && fg[0]) {
        return enabled ? fg : UI_TEXT_MUTE;
    }
    return enabled ? UI_ACCENT : UI_SAGE;
}

static void styleLabelButton(Ihandle *label, int enabled) {
    const char *fg = labelFg(label, enabled);
    IupSetAttribute(label, CLUMZY_LABEL, "1");
    IupSetAttribute(label, "BGCOLOR", UI_BG);
    IupSetAttribute(label, "FGCOLOR", fg);
    IupSetAttribute(label, "HLCOLOR", UI_BG);
    IupSetAttribute(label, "PSCOLOR", UI_BG);
    IupSetAttribute(label, "TEXTHLCOLOR", fg);
    IupSetAttribute(label, "TEXTPSCOLOR", fg);
    IupSetAttribute(label, "BORDERWIDTH", "0");
    IupSetAttribute(label, "SHOWBORDER", "NO");
    IupSetAttribute(label, "ALIGNMENT", "ALEFT:ACENTER");
    IupSetAttribute(label, "ACTIVE", "YES");
}

/* CHECKSIZE=0 so IUP paints BGCOLOR. The system checkbox skips that fill. */
static void styleToggle(Ihandle *toggle, int enabled) {
    ensureCheckImages();
    IupSetAttribute(toggle, "CHECKSIZE", "0");
    IupSetAttribute(toggle, "BGCOLOR", UI_BG);
    IupSetAttribute(toggle, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "HLCOLOR", UI_HOVER);
    IupSetAttribute(toggle, "PSCOLOR", UI_PRESS);
    IupSetAttribute(toggle, "TEXTHLCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "TEXTPSCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
    IupSetAttribute(toggle, "BORDERWIDTH", "0");
    IupSetAttribute(toggle, "SHOWBORDER", "NO");
    IupSetAttribute(toggle, "ALIGNMENT", "ALEFT:ACENTER");
    IupSetAttribute(toggle, "IMAGEPOSITION", "LEFT");
    IupSetAttribute(toggle, "EXPAND", "NO");
    IupSetAttribute(toggle, "ACTIVE", "YES");
    syncToggleImage(toggle);
}

static void styleText(Ihandle *ih, int enabled) {
    IupSetAttribute(ih, "READONLY", "NO");
    IupSetAttribute(ih, "ACTIVE", "YES");
    IupSetAttribute(ih, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
}

static int toggleFlatAction(Ihandle *ih, int state) {
    Icallback prev;
    syncToggleImage(ih);
    prev = IupGetCallback(ih, "ACTION");
    return prev ? ((int (*)(Ihandle *, int))prev)(ih, state) : IUP_DEFAULT;
}

static void copyToggleAction(Ihandle *ih) {
    IupSetCallback(ih, "FLAT_ACTION", (Icallback)toggleFlatAction);
}

static void copyActionToFlat(Ihandle *ih) {
    Icallback cb = IupGetCallback(ih, "ACTION");
    if (cb) {
        IupSetCallback(ih, "FLAT_ACTION", cb);
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
    label = IupFlatButton(title);
    IupSetAttribute(label, "CANFOCUS", "NO");
    styleLabelButton(label, 1);
    return label;
}

Ihandle *clumzyList(void) {
    Ihandle *list = IupList(NULL);
    styleText(list, 1);
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

void clumzyInitDarkMode(void) {
    preferWinDarkMode();
}

void applyClumzyGlobals(void) {
    preferWinDarkMode();
    ensureCheckImages();
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

/* After map: dark title bar + untheme native edits/combos only. Do not
 * change SIZE or walk every HWND — that retriggered MAP/SHOW and hung. */
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
    } else if (cls && hwnd && (strcmp(cls, "text") == 0 || strcmp(cls, "list") == 0)) {
        skinHwndTree(hwnd);
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
    Ihandle *child;
    const char *cls;
    if (!root) {
        return;
    }
    cls = IupGetClassName(root);
    if (cls && strcmp(cls, "flattoggle") == 0) {
        syncToggleImage(root);
    }
    child = IupGetNextChild(root, NULL);
    while (child) {
        clumzySyncToggleImages(child);
        child = IupGetBrother(child);
    }
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

    if (strcmp(cls, "dialog") == 0 ||
        strcmp(cls, "vbox") == 0 ||
        strcmp(cls, "hbox") == 0 ||
        strcmp(cls, "zbox") == 0 ||
        strcmp(cls, "cbox") == 0 ||
        strcmp(cls, "fill") == 0 ||
        strcmp(cls, "flatframe") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        if (strcmp(cls, "dialog") == 0) {
            IupSetAttribute(ih, "BACKGROUND", UI_BG);
        }
        if (strcmp(cls, "flatframe") == 0) {
            styleFrame(ih);
        }
    } else if (strcmp(cls, "label") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        if (IupGetAttribute(ih, "IMAGE")) {
            IupSetAttribute(ih, "EXPAND", "NO");
        }
    } else if (strcmp(cls, "text") == 0 || strcmp(cls, "list") == 0) {
        styleText(ih, 1);
    } else if (strcmp(cls, "flattoggle") == 0) {
        styleToggle(ih, 1);
        copyToggleAction(ih);
    } else if (strcmp(cls, "flatbutton") == 0) {
        if (isLabelButton(ih)) {
            styleLabelButton(ih, 1);
        } else {
            styleButton(ih);
            copyActionToFlat(ih);
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
    if (strcmp(cls, "text") == 0 || strcmp(cls, "list") == 0) {
        IupSetAttribute(ih, "CANFOCUS", "YES");
        styleText(ih, enabled);
    } else if (strcmp(cls, "flattoggle") == 0) {
        styleToggle(ih, enabled);
    } else if (strcmp(cls, "flatbutton") == 0) {
        if (isLabelButton(ih)) {
            styleLabelButton(ih, enabled);
        } else {
            styleButton(ih);
            IupSetAttribute(ih, "ACTIVE", "YES");
            IupSetAttribute(ih, "FGCOLOR", enabled ? UI_TEXT : UI_TEXT_MUTE);
        }
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
