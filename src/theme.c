#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "theme.h"

#define CHECK_PX 14
#define CLUMZY_LABEL "__CLUMZY_LABEL"
#define CLUMZY_FG "__CLUMZY_FG"
#define CLUMZY_LIST "CLUMZY_LIST"

typedef HRESULT (WINAPI *SetWindowTheme_t)(HWND, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
typedef int (WINAPI *SetPreferredAppMode_t)(int);
typedef BOOL (WINAPI *AllowDarkModeForWindow_t)(HWND, BOOL);

static SetWindowTheme_t pSetWindowTheme;
static DwmSetWindowAttribute_t pDwmSetWindowAttribute;
static SetPreferredAppMode_t pSetPreferredAppMode;
static AllowDarkModeForWindow_t pAllowDarkModeForWindow;
static int darkApisLoaded;

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

/* CHECKSIZE=0 so IUP paints BGCOLOR. System checkbox skips the fill and stays white. */
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

static Ihandle *getDropList(Ihandle *drop) {
    return drop ? (Ihandle*)IupGetAttribute(drop, CLUMZY_LIST) : NULL;
}

static Ihandle *getListDrop(Ihandle *list) {
    return list ? (Ihandle*)IupGetAttribute(list, "CLUMZY_DROP") : NULL;
}

static void styleFlatList(Ihandle *list) {
    IupSetAttribute(list, "BGCOLOR", UI_INPUT_BG);
    IupSetAttribute(list, "FGCOLOR", UI_TEXT);
    IupSetAttribute(list, "HLCOLOR", UI_HOVER);
    IupSetAttribute(list, "PSCOLOR", UI_HOVER);
    IupSetAttribute(list, "TEXTPSCOLOR", UI_TEXT);
    IupSetAttribute(list, "BORDERCOLOR", UI_SAGE);
    IupSetAttribute(list, "BORDERWIDTH", "1");
    IupSetAttribute(list, "ALIGNMENT", "ALEFT:ACENTER");
}

static void styleDropButton(Ihandle *drop) {
    IupSetAttribute(drop, "BGCOLOR", UI_SURFACE);
    IupSetAttribute(drop, "FGCOLOR", UI_TEXT);
    IupSetAttribute(drop, "HLCOLOR", UI_HOVER);
    IupSetAttribute(drop, "PSCOLOR", UI_PRESS);
    IupSetAttribute(drop, "TEXTHLCOLOR", UI_TEXT);
    IupSetAttribute(drop, "TEXTPSCOLOR", UI_TEXT);
    IupSetAttribute(drop, "BORDERCOLOR", UI_SAGE);
    IupSetAttribute(drop, "BORDERHLCOLOR", UI_ACCENT);
    IupSetAttribute(drop, "BORDERPSCOLOR", UI_ACCENT);
    IupSetAttribute(drop, "BORDERWIDTH", "1");
    IupSetAttribute(drop, "SHOWBORDER", "YES");
    IupSetAttribute(drop, "DROPONARROW", "NO");
    IupSetAttribute(drop, "ARROWCOLOR", UI_SAGE);
    IupSetAttribute(drop, "ALIGNMENT", "ALEFT:ACENTER");
    IupSetAttribute(drop, "PADDING", "6x4");
    {
        const char *cols = IupGetAttribute(drop, "VISIBLECOLUMNS");
        if (cols && cols[0]) {
            IupSetfAttribute(drop, "SIZE", "%sx", cols);
        }
    }
}

static void syncDropList(Ihandle *drop) {
    Ihandle *list = getDropList(drop);
    const char *val;
    const char *title;
    char key[8];
    int i;
    int count = 0;
    int value;

    if (!drop || !list) {
        return;
    }
    styleDropButton(drop);
    styleFlatList(list);

    if (IupGetAttribute(drop, "1")) {
        for (i = 1; i <= 128; ++i) {
            sprintf(key, "%d", i);
            val = IupGetAttribute(drop, key);
            if (!val) {
                IupSetAttribute(list, key, NULL);
                break;
            }
            IupSetStrAttribute(list, key, val);
            count = i;
        }
    } else {
        count = IupGetInt(list, "COUNT");
    }

    val = IupGetAttribute(drop, "VISIBLECOLUMNS");
    if (val) {
        IupSetAttribute(list, "VISIBLECOLUMNS", val);
    }
    IupSetInt(list, "VISIBLELINES", count > 12 ? 12 : (count < 1 ? 1 : count));

    val = IupGetAttribute(drop, "VALUE");
    if (val && val[0] && strcmp(val, "0") != 0) {
        IupSetAttribute(list, "VALUE", val);
        value = atoi(val);
        if (value > 0) {
            sprintf(key, "%d", value);
            title = IupGetAttribute(list, key);
            if (!title) {
                title = IupGetAttribute(drop, key);
            }
            IupSetStrAttribute(drop, "TITLE", title ? title : "");
        }
    } else {
        IupSetAttribute(list, "VALUE", NULL);
        IupSetAttribute(drop, "TITLE", "");
    }
}

static int listPickCb(Ihandle *list, char *text, int item, int state) {
    Ihandle *drop;
    Icallback cb;

    if (state != 1) {
        return IUP_DEFAULT;
    }
    drop = IupGetAttributeHandle(IupGetDialog(list), "DROPBUTTON");
    if (!drop) {
        drop = getListDrop(list);
    }
    if (drop) {
        IupSetAttribute(drop, "SHOWDROPDOWN", "NO");
        IupSetStrAttribute(drop, "TITLE", text);
        IupSetInt(drop, "VALUE", item);
        cb = IupGetCallback(drop, "ACTION");
        if (cb) {
            return ((int (*)(Ihandle *, char *, int, int))cb)(drop, text, item, state);
        }
    }
    return IUP_DEFAULT;
}

static int dropDownCb(Ihandle *drop, int state) {
    Ihandle *list;
    Ihandle *dlg;

    if (state != 1) {
        return IUP_DEFAULT;
    }
    syncDropList(drop);
    list = getDropList(drop);
    if (list) {
        dlg = IupGetDialog(list);
        if (dlg) {
            IupSetAttribute(dlg, "BGCOLOR", UI_SURFACE);
            IupSetAttribute(dlg, "BACKGROUND", UI_SURFACE);
        }
    }
    return IUP_DEFAULT;
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
    Ihandle *list = IupFlatList();
    Ihandle *drop = IupDropButton(list);
    IupSetAttribute(drop, CLUMZY_LIST, (char*)list);
    IupSetAttribute(list, "CLUMZY_DROP", (char*)drop);
    IupSetAttribute(list, "EXPAND", "YES");
    IupSetCallback(list, "FLAT_ACTION", (Icallback)listPickCb);
    IupSetCallback(drop, "DROPDOWN_CB", (Icallback)dropDownCb);
    styleFlatList(list);
    styleDropButton(drop);
    return drop;
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
    if (!hwnd) {
        return;
    }
    allowDark((HWND)hwnd);
    if (pDwmSetWindowAttribute) {
        pDwmSetWindowAttribute((HWND)hwnd, 20, &dark, sizeof(dark));
        pDwmSetWindowAttribute((HWND)hwnd, 19, &dark, sizeof(dark));
    }
    SetWindowPos((HWND)hwnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
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

static void skinNativeControl(Ihandle *ih) {
    const char *cls = IupGetClassName(ih);
    HWND hwnd;
    int enabled;

    if (!ih || !cls) {
        return;
    }
    hwnd = (HWND)IupGetAttribute(ih, "HWND");
    enabled = ancestorEnabled(ih);

    /* Only untheme native edits/combos. Labels and checkboxes stay IUP-drawn. */
    if (hwnd && (strcmp(cls, "text") == 0 || strcmp(cls, "list") == 0)) {
        skinHwndTree(hwnd);
        styleText(ih, enabled);
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }
    if (strcmp(cls, "dialog") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
        IupSetAttribute(ih, "BACKGROUND", UI_BG);
        if (hwnd) {
            clumzyApplyWindowDarkMode(hwnd);
        }
    } else if (strcmp(cls, "flatframe") == 0) {
        styleFrame(ih);
    } else if (strcmp(cls, "flattoggle") == 0) {
        styleToggle(ih, enabled);
        copyToggleAction(ih);
    } else if (strcmp(cls, "flatbutton") == 0) {
        if (isLabelButton(ih)) {
            styleLabelButton(ih, enabled);
        } else {
            styleButton(ih);
        }
    } else if (strcmp(cls, "dropbutton") == 0) {
        syncDropList(ih);
    } else if (strcmp(cls, "flatlist") == 0) {
        styleFlatList(ih);
    } else if (strcmp(cls, "label") == 0) {
        IupSetAttribute(ih, "BGCOLOR", UI_BG);
    }
}

static void skinWalk(Ihandle *ih) {
    Ihandle *child;
    if (!ih) {
        return;
    }
    skinNativeControl(ih);
    child = IupGetNextChild(ih, NULL);
    while (child) {
        skinWalk(child);
        child = IupGetBrother(child);
    }
}

void clumzyOnMapped(Ihandle *ih) {
    skinWalk(ih);
}

void clumzySyncToggleImages(Ihandle *root) {
    Ihandle *child;
    if (!root) {
        return;
    }
    if (IupGetClassName(root) && strcmp(IupGetClassName(root), "flattoggle") == 0) {
        syncToggleImage(root);
    }
    child = IupGetNextChild(root, NULL);
    while (child) {
        clumzySyncToggleImages(child);
        child = IupGetBrother(child);
    }
}

void clumzySyncList(Ihandle *ih) {
    if (ih && IupGetClassName(ih) && strcmp(IupGetClassName(ih), "dropbutton") == 0) {
        syncDropList(ih);
    }
}

void clumzyStoreListItem(Ihandle *drop, const char *id, const char *text) {
    Ihandle *list = getDropList(drop);
    if (!drop) {
        return;
    }
    IupStoreAttribute(drop, id, text);
    if (list) {
        IupStoreAttribute(list, id, text);
    }
}

void clumzySetListAttributes(Ihandle *drop, const char *attrs) {
    Ihandle *list = getDropList(drop);
    if (!drop) {
        return;
    }
    IupSetAttributes(drop, attrs);
    if (list) {
        IupSetAttributes(list, attrs);
    }
    syncDropList(drop);
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
    } else if (strcmp(cls, "dropbutton") == 0) {
        syncDropList(ih);
    } else if (strcmp(cls, "flatlist") == 0) {
        styleFlatList(ih);
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
    } else if (strcmp(cls, "dropbutton") == 0) {
        styleDropButton(ih);
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
        skinHwndTree(hwnd);
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
