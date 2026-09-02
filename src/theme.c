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
