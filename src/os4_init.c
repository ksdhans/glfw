//========================================================================
// GLFW 3.4 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2016 Google Inc.
// Copyright (c) 2016-2017 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
// It is fine to use C99 in this file because it will not be built with VS
//========================================================================

#include "internal.h"

#ifndef CLIB4
#undef __STRICT_ANSI__
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

extern struct Library *DOSBase;
extern struct DOSIFace *IDOS;

struct GraphicsIFace *IGraphics = NULL;
struct Library *GfxBase = NULL;

struct IntuitionIFace *IIntuition = NULL;
struct Library *IntuitionBase = NULL;

struct KeymapIFace *IKeymap = NULL;
struct Library *KeymapBase = NULL;

struct Library *AIN_Base = NULL;
struct AIN_IFace *IAIN = NULL;

struct Device *TimerBase = NULL;
struct TimerIFace *ITimer = NULL;

struct TextClipIFace *ITextClip = NULL;
struct Library *TextClipBase = NULL;

struct IconIFace *IIcon = NULL;
struct Library *IconBase = NULL;

struct WorkbenchIFace *IWorkbench = NULL;
struct Library *WorkbenchBase = NULL;

#define MIN_LIB_VERSION 51

static void OS4_FindApplicationName();

//************************************************************************
//****                  OS4 internal functions                        ****
//************************************************************************

BOOL VARARGS68K showErrorRequester(const char *errMsgRaw, ...)
{
    va_list ap;
    va_startlinear(ap, errMsgRaw);
    ULONG *errMsgArgs = va_getlinearva(ap, ULONG *);

    Object *object = NULL;
    if (IIntuition)
    {
        object = IIntuition->NewObject(
            NULL, "requester.class",
            REQ_Type, REQTYPE_INFO,
            REQ_TitleText, "GLFW: FATAL ERROR",
            REQ_BodyText, errMsgRaw,
            REQ_VarArgs, errMsgArgs,
            REQ_Image, REQIMAGE_ERROR,
            REQ_GadgetText, "_Ok",
            TAG_DONE);
    }
    if (object)
    {
        IIntuition->IDoMethod(object, RM_OPENREQ, NULL, NULL, NULL, TAG_DONE);
        IIntuition->DisposeObject(object);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static struct Library *openLib(const char *libName, unsigned int minVers, struct Interface **iFacePtr)
{
    struct Library *libBase = IExec->OpenLibrary(libName, minVers);
    if (libBase)
    {
        struct Interface *iFace = IExec->GetInterface(libBase, "main", 1, NULL);
        if (!iFace)
        {
            // Failed
            //CloseLibrary(libBase);
            libBase = NULL; // Lets the code below know we've failed
        }

        if (iFacePtr)
        {
            // Write the interface pointer
            *iFacePtr = iFace;
        }
    }
    else
    {
        // Opening the library failed. Show the error requester
        const char errMsgRaw[] = "Couldn't open %s version %u+.\n";
        if (!showErrorRequester(errMsgRaw, libName, minVers))
        {
            // Use printf() as a backup
            printf(errMsgRaw, libName, minVers);
        }
    }

    return libBase;
}

static int loadLibraries(void)
{
    // graphics.library
    DOSBase = openLib("dos.library", MIN_LIB_VERSION, (struct Interface **)&IDOS);
    if (!DOSBase)
    {
        return 0;
    }

    // graphics.library
    GfxBase = openLib("graphics.library", 54, (struct Interface **)&IGraphics);
    if (!GfxBase)
    {
        return 0;
    }

    // intuition.library
    IntuitionBase = openLib("intuition.library", MIN_LIB_VERSION, (struct Interface **)&IIntuition);
    if (!IntuitionBase)
    {
        return 0;
    }

    // keymap.library
    KeymapBase = openLib("keymap.library", MIN_LIB_VERSION, (struct Interface **)&IKeymap);
    if (!KeymapBase)
    {
        return 0;
    }

    // Workbench.library
    WorkbenchBase = openLib("workbench.library", MIN_LIB_VERSION, (struct Interface **)&IWorkbench);
    if (!WorkbenchBase)
    {
        return 0;
    }

    // icon.library
    IconBase = openLib("icon.library", MIN_LIB_VERSION, (struct Interface **)&IIcon);
    if (!IconBase)
    {
        return 0;
    }

    // AmigaInput
    AIN_Base = openLib("AmigaInput.library", MIN_LIB_VERSION, (struct Interface **)&IAIN);
    if (!AIN_Base)
    {
        return 0;
    }

    // TextClip
    TextClipBase  = openLib("textclip.library", MIN_LIB_VERSION, (struct Interface **)&ITextClip);
    if (!TextClipBase)
    {
        return 0;
    }

    return 1;
}

static void closeLibraries(void)
{
    if (ITextClip) {
        IExec->DropInterface((struct Interface *)ITextClip);
    }
    if (TextClipBase)
    {
        IExec->CloseLibrary(TextClipBase);
    }

    if (IAIN)
    {
        IExec->DropInterface((struct Interface *)IAIN);
    }
    if (AIN_Base)
    {
        IExec->CloseLibrary(AIN_Base);
    }

    // Close workbench.library
    if (IWorkbench)
    {
        IExec->DropInterface((struct Interface *)IWorkbench);
        IWorkbench = NULL;
    }
    if (WorkbenchBase)
    {
        IExec->CloseLibrary((struct Library *)WorkbenchBase);
        WorkbenchBase = NULL;
    }

    // Close icon.library
    if (IIcon)
    {
        IExec->DropInterface((struct Interface *)IIcon);
        IIcon = NULL;
    }
    if (IconBase)
    {
        IExec->CloseLibrary((struct Library *)IconBase);
        IconBase = NULL;
    }

    // Close graphics.library
    if (IGraphics)
    {
        IExec->DropInterface((struct Interface *)IGraphics);
        IGraphics = NULL;
    }
    if (GfxBase)
    {
        IExec->CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }

    // Close intuition.library
    if (IIntuition)
    {
        IExec->DropInterface((struct Interface *)IIntuition);
        IIntuition = NULL;
    }
    if (IntuitionBase)
    {
        IExec->CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }

    // Close keymap.library
    if (IKeymap)
    {
        IExec->DropInterface((struct Interface *)IKeymap);
        IKeymap = NULL;
    }
    if (KeymapBase)
    {
        IExec->CloseLibrary(KeymapBase);
        KeymapBase = NULL;
    }
}

static const char* get_key_name(int key)
{
    switch (key)
    {
        // Printable keys
        case GLFW_KEY_A:            return "A";
        case GLFW_KEY_B:            return "B";
        case GLFW_KEY_C:            return "C";
        case GLFW_KEY_D:            return "D";
        case GLFW_KEY_E:            return "E";
        case GLFW_KEY_F:            return "F";
        case GLFW_KEY_G:            return "G";
        case GLFW_KEY_H:            return "H";
        case GLFW_KEY_I:            return "I";
        case GLFW_KEY_J:            return "J";
        case GLFW_KEY_K:            return "K";
        case GLFW_KEY_L:            return "L";
        case GLFW_KEY_M:            return "M";
        case GLFW_KEY_N:            return "N";
        case GLFW_KEY_O:            return "O";
        case GLFW_KEY_P:            return "P";
        case GLFW_KEY_Q:            return "Q";
        case GLFW_KEY_R:            return "R";
        case GLFW_KEY_S:            return "S";
        case GLFW_KEY_T:            return "T";
        case GLFW_KEY_U:            return "U";
        case GLFW_KEY_V:            return "V";
        case GLFW_KEY_W:            return "W";
        case GLFW_KEY_X:            return "X";
        case GLFW_KEY_Y:            return "Y";
        case GLFW_KEY_Z:            return "Z";
        case GLFW_KEY_1:            return "1";
        case GLFW_KEY_2:            return "2";
        case GLFW_KEY_3:            return "3";
        case GLFW_KEY_4:            return "4";
        case GLFW_KEY_5:            return "5";
        case GLFW_KEY_6:            return "6";
        case GLFW_KEY_7:            return "7";
        case GLFW_KEY_8:            return "8";
        case GLFW_KEY_9:            return "9";
        case GLFW_KEY_0:            return "0";
        case GLFW_KEY_SPACE:        return "SPACE";
        case GLFW_KEY_MINUS:        return "MINUS";
        case GLFW_KEY_EQUAL:        return "EQUAL";
        case GLFW_KEY_LEFT_BRACKET: return "LEFT BRACKET";
        case GLFW_KEY_RIGHT_BRACKET: return "RIGHT BRACKET";
        case GLFW_KEY_BACKSLASH:    return "BACKSLASH";
        case GLFW_KEY_SEMICOLON:    return "SEMICOLON";
        case GLFW_KEY_APOSTROPHE:   return "APOSTROPHE";
        case GLFW_KEY_GRAVE_ACCENT: return "GRAVE ACCENT";
        case GLFW_KEY_COMMA:        return "COMMA";
        case GLFW_KEY_PERIOD:       return "PERIOD";
        case GLFW_KEY_SLASH:        return "SLASH";
        case GLFW_KEY_WORLD_1:      return "WORLD 1";
        case GLFW_KEY_WORLD_2:      return "WORLD 2";

        // Function keys
        case GLFW_KEY_ESCAPE:       return "ESCAPE";
        case GLFW_KEY_F1:           return "F1";
        case GLFW_KEY_F2:           return "F2";
        case GLFW_KEY_F3:           return "F3";
        case GLFW_KEY_F4:           return "F4";
        case GLFW_KEY_F5:           return "F5";
        case GLFW_KEY_F6:           return "F6";
        case GLFW_KEY_F7:           return "F7";
        case GLFW_KEY_F8:           return "F8";
        case GLFW_KEY_F9:           return "F9";
        case GLFW_KEY_F10:          return "F10";
        case GLFW_KEY_F11:          return "F11";
        case GLFW_KEY_F12:          return "F12";
        case GLFW_KEY_F13:          return "F13";
        case GLFW_KEY_F14:          return "F14";
        case GLFW_KEY_F15:          return "F15";
        case GLFW_KEY_F16:          return "F16";
        case GLFW_KEY_F17:          return "F17";
        case GLFW_KEY_F18:          return "F18";
        case GLFW_KEY_F19:          return "F19";
        case GLFW_KEY_F20:          return "F20";
        case GLFW_KEY_F21:          return "F21";
        case GLFW_KEY_F22:          return "F22";
        case GLFW_KEY_F23:          return "F23";
        case GLFW_KEY_F24:          return "F24";
        case GLFW_KEY_F25:          return "F25";
        case GLFW_KEY_UP:           return "UP";
        case GLFW_KEY_DOWN:         return "DOWN";
        case GLFW_KEY_LEFT:         return "LEFT";
        case GLFW_KEY_RIGHT:        return "RIGHT";
        case GLFW_KEY_LEFT_SHIFT:   return "LEFT SHIFT";
        case GLFW_KEY_RIGHT_SHIFT:  return "RIGHT SHIFT";
        case GLFW_KEY_LEFT_CONTROL: return "LEFT CONTROL";
        case GLFW_KEY_RIGHT_CONTROL: return "RIGHT CONTROL";
        case GLFW_KEY_LEFT_ALT:     return "LEFT ALT";
        case GLFW_KEY_RIGHT_ALT:    return "RIGHT ALT";
        case GLFW_KEY_TAB:          return "TAB";
        case GLFW_KEY_ENTER:        return "ENTER";
        case GLFW_KEY_BACKSPACE:    return "BACKSPACE";
        case GLFW_KEY_INSERT:       return "INSERT";
        case GLFW_KEY_DELETE:       return "DELETE";
        case GLFW_KEY_PAGE_UP:      return "PAGE UP";
        case GLFW_KEY_PAGE_DOWN:    return "PAGE DOWN";
        case GLFW_KEY_HOME:         return "HOME";
        case GLFW_KEY_END:          return "END";
        case GLFW_KEY_KP_0:         return "KEYPAD 0";
        case GLFW_KEY_KP_1:         return "KEYPAD 1";
        case GLFW_KEY_KP_2:         return "KEYPAD 2";
        case GLFW_KEY_KP_3:         return "KEYPAD 3";
        case GLFW_KEY_KP_4:         return "KEYPAD 4";
        case GLFW_KEY_KP_5:         return "KEYPAD 5";
        case GLFW_KEY_KP_6:         return "KEYPAD 6";
        case GLFW_KEY_KP_7:         return "KEYPAD 7";
        case GLFW_KEY_KP_8:         return "KEYPAD 8";
        case GLFW_KEY_KP_9:         return "KEYPAD 9";
        case GLFW_KEY_KP_DIVIDE:    return "KEYPAD DIVIDE";
        case GLFW_KEY_KP_MULTIPLY:  return "KEYPAD MULTIPLY";
        case GLFW_KEY_KP_SUBTRACT:  return "KEYPAD SUBTRACT";
        case GLFW_KEY_KP_ADD:       return "KEYPAD ADD";
        case GLFW_KEY_KP_DECIMAL:   return "KEYPAD DECIMAL";
        case GLFW_KEY_KP_EQUAL:     return "KEYPAD EQUAL";
        case GLFW_KEY_KP_ENTER:     return "KEYPAD ENTER";
        case GLFW_KEY_PRINT_SCREEN: return "PRINT SCREEN";
        case GLFW_KEY_NUM_LOCK:     return "NUM LOCK";
        case GLFW_KEY_CAPS_LOCK:    return "CAPS LOCK";
        case GLFW_KEY_SCROLL_LOCK:  return "SCROLL LOCK";
        case GLFW_KEY_PAUSE:        return "PAUSE";
        case GLFW_KEY_LEFT_SUPER:   return "LEFT SUPER";
        case GLFW_KEY_RIGHT_SUPER:  return "RIGHT SUPER";
        case GLFW_KEY_MENU:         return "MENU";

        default:                    return "UNKNOWN";
    }
}

static void createKeyTables(void)
{
    memset(_glfw.os4.keycodes, -1, sizeof(_glfw.os4.keycodes));
    memset(_glfw.os4.scancodes, -1, sizeof(_glfw.os4.scancodes));
    memset(_glfw.os4.keynames, 0, sizeof(_glfw.os4.keynames));

    _glfw.os4.keycodes[0xb]             = GLFW_KEY_GRAVE_ACCENT;
    _glfw.os4.keycodes[0x1]             = GLFW_KEY_1;
    _glfw.os4.keycodes[0x2]             = GLFW_KEY_2;
    _glfw.os4.keycodes[0x3]             = GLFW_KEY_3;
    _glfw.os4.keycodes[0x4]             = GLFW_KEY_4;
    _glfw.os4.keycodes[0x5]             = GLFW_KEY_5;
    _glfw.os4.keycodes[0x6]             = GLFW_KEY_6;
    _glfw.os4.keycodes[0x7]             = GLFW_KEY_7;
    _glfw.os4.keycodes[0x8]             = GLFW_KEY_8;
    _glfw.os4.keycodes[0x9]             = GLFW_KEY_9;
    _glfw.os4.keycodes[0xa]             = GLFW_KEY_0;
    _glfw.os4.keycodes[0x40]            = GLFW_KEY_SPACE; //
    _glfw.os4.keycodes[0x3a]            = GLFW_KEY_MINUS;
    _glfw.os4.keycodes[0xc]             = GLFW_KEY_EQUAL;
    _glfw.os4.keycodes[0x10]            = GLFW_KEY_Q;
    _glfw.os4.keycodes[0x11]            = GLFW_KEY_W;
    _glfw.os4.keycodes[0x12]            = GLFW_KEY_E;
    _glfw.os4.keycodes[0x13]            = GLFW_KEY_R;
    _glfw.os4.keycodes[0x14]            = GLFW_KEY_T;
    _glfw.os4.keycodes[0x15]            = GLFW_KEY_Y;
    _glfw.os4.keycodes[0x16]            = GLFW_KEY_U;
    _glfw.os4.keycodes[0x17]            = GLFW_KEY_I;
    _glfw.os4.keycodes[0x18]            = GLFW_KEY_O;
    _glfw.os4.keycodes[0x19]            = GLFW_KEY_P;
    _glfw.os4.keycodes[0x1a]            = GLFW_KEY_LEFT_BRACKET;
    _glfw.os4.keycodes[0x1b]            = GLFW_KEY_RIGHT_BRACKET;
    _glfw.os4.keycodes[0x20]            = GLFW_KEY_A;
    _glfw.os4.keycodes[0x21]            = GLFW_KEY_S;
    _glfw.os4.keycodes[0x22]            = GLFW_KEY_D;
    _glfw.os4.keycodes[0x23]            = GLFW_KEY_F;
    _glfw.os4.keycodes[0x24]            = GLFW_KEY_G;
    _glfw.os4.keycodes[0x25]            = GLFW_KEY_H;
    _glfw.os4.keycodes[0x26]            = GLFW_KEY_J;
    _glfw.os4.keycodes[0x27]            = GLFW_KEY_K;
    _glfw.os4.keycodes[0x28]            = GLFW_KEY_L;
    //_glfw.os4.keycodes[KEY_SEMICOLON]  = GLFW_KEY_SEMICOLON;
    //_glfw.os4.keycodes[KEY_APOSTROPHE] = GLFW_KEY_APOSTROPHE;
    _glfw.os4.keycodes[0x31]            = GLFW_KEY_Z;
    _glfw.os4.keycodes[0x32]            = GLFW_KEY_X;
    _glfw.os4.keycodes[0x33]            = GLFW_KEY_C;
    _glfw.os4.keycodes[0x34]            = GLFW_KEY_V;
    _glfw.os4.keycodes[0x35]            = GLFW_KEY_B;
    _glfw.os4.keycodes[0x36]            = GLFW_KEY_N;
    _glfw.os4.keycodes[0x37]            = GLFW_KEY_M;
    _glfw.os4.keycodes[0x38]            = GLFW_KEY_COMMA;
    _glfw.os4.keycodes[0x39]            = GLFW_KEY_PERIOD;
    _glfw.os4.keycodes[0x3a]            = GLFW_KEY_SLASH;
    _glfw.os4.keycodes[0x0]             = GLFW_KEY_BACKSLASH;
    _glfw.os4.keycodes[RAWKEY_ESC]      = GLFW_KEY_ESCAPE;         //
    _glfw.os4.keycodes[RAWKEY_TAB]      = GLFW_KEY_TAB;            //
    _glfw.os4.keycodes[RAWKEY_LSHIFT]   = GLFW_KEY_LEFT_SHIFT;  //
    _glfw.os4.keycodes[RAWKEY_RSHIFT]   = GLFW_KEY_RIGHT_SHIFT; //
    _glfw.os4.keycodes[RAWKEY_LCTRL]    = GLFW_KEY_LEFT_CONTROL; //
    _glfw.os4.keycodes[RAWKEY_LCTRL]    = GLFW_KEY_RIGHT_CONTROL;
    _glfw.os4.keycodes[RAWKEY_LALT]     = GLFW_KEY_LEFT_ALT;      //
    _glfw.os4.keycodes[RAWKEY_RALT]     = GLFW_KEY_RIGHT_ALT;      //
    _glfw.os4.keycodes[RAWKEY_LCOMMAND] = GLFW_KEY_LEFT_SUPER; //
    _glfw.os4.keycodes[RAWKEY_RCOMMAND] = GLFW_KEY_RIGHT_SUPER;
    _glfw.os4.keycodes[RAWKEY_MENU]     = GLFW_KEY_MENU; //
    //_glfw.os4.keycodes[KEY_NUMLOCK]       = GLFW_KEY_NUM_LOCK;
    _glfw.os4.keycodes[RAWKEY_CAPSLOCK] = GLFW_KEY_CAPS_LOCK; //
    _glfw.os4.keycodes[RAWKEY_BREAK]    = GLFW_KEY_PRINT_SCREEN;
    _glfw.os4.keycodes[0x5f]            = GLFW_KEY_SCROLL_LOCK;
    _glfw.os4.keycodes[RAWKEY_BREAK]    = GLFW_KEY_PAUSE;
    _glfw.os4.keycodes[RAWKEY_DEL]      = GLFW_KEY_DELETE;          //
    _glfw.os4.keycodes[RAWKEY_BACKSPACE] = GLFW_KEY_BACKSPACE; //
    _glfw.os4.keycodes[RAWKEY_RETURN]   = GLFW_KEY_ENTER;        //
    _glfw.os4.keycodes[RAWKEY_HOME]     = GLFW_KEY_HOME;           //
    _glfw.os4.keycodes[RAWKEY_END]      = GLFW_KEY_END;             //
    _glfw.os4.keycodes[RAWKEY_PAGEUP]   = GLFW_KEY_PAGE_UP;      //
    _glfw.os4.keycodes[RAWKEY_PAGEDOWN] = GLFW_KEY_PAGE_DOWN;  //
    _glfw.os4.keycodes[RAWKEY_INSERT]   = GLFW_KEY_INSERT;       //
    _glfw.os4.keycodes[RAWKEY_CRSRLEFT] = GLFW_KEY_LEFT;
    _glfw.os4.keycodes[RAWKEY_CRSRRIGHT] = GLFW_KEY_RIGHT; //
    _glfw.os4.keycodes[RAWKEY_CRSRDOWN] = GLFW_KEY_DOWN;   //
    _glfw.os4.keycodes[RAWKEY_CRSRUP]   = GLFW_KEY_UP;       //
    _glfw.os4.keycodes[RAWKEY_F1]       = GLFW_KEY_F1;           //
    _glfw.os4.keycodes[RAWKEY_F2]       = GLFW_KEY_F2;           //
    _glfw.os4.keycodes[RAWKEY_F3]       = GLFW_KEY_F3;           //
    _glfw.os4.keycodes[RAWKEY_F4]       = GLFW_KEY_F4;           //
    _glfw.os4.keycodes[RAWKEY_F5]       = GLFW_KEY_F5;           //
    _glfw.os4.keycodes[RAWKEY_F6]       = GLFW_KEY_F6;           //
    _glfw.os4.keycodes[RAWKEY_F7]       = GLFW_KEY_F7;           //
    _glfw.os4.keycodes[RAWKEY_F8]       = GLFW_KEY_F8;           //
    _glfw.os4.keycodes[RAWKEY_F9]       = GLFW_KEY_F9;           //
    _glfw.os4.keycodes[RAWKEY_F10]      = GLFW_KEY_F10;         //
    _glfw.os4.keycodes[RAWKEY_F11]      = GLFW_KEY_F11;         //
    _glfw.os4.keycodes[RAWKEY_F12]      = GLFW_KEY_F12;         //
    _glfw.os4.keycodes[RAWKEY_F13]      = GLFW_KEY_F13;         //
    _glfw.os4.keycodes[RAWKEY_F14]      = GLFW_KEY_F14;         //
    _glfw.os4.keycodes[RAWKEY_F15]      = GLFW_KEY_F15;         //
    _glfw.os4.keycodes[RAWKEY_HELP]     = GLFW_KEY_F16;        // Mapped amiga HELP key with F16
    _glfw.os4.keycodes[0x5c]            = GLFW_KEY_KP_DIVIDE;
    _glfw.os4.keycodes[0x5d]            = GLFW_KEY_KP_MULTIPLY;
    _glfw.os4.keycodes[0x4a]            = GLFW_KEY_KP_SUBTRACT;
    _glfw.os4.keycodes[0x5e]            = GLFW_KEY_KP_ADD;
    _glfw.os4.keycodes[0xf]             = GLFW_KEY_KP_0;
    _glfw.os4.keycodes[0x1d]            = GLFW_KEY_KP_1;
    _glfw.os4.keycodes[0x1e]            = GLFW_KEY_KP_2;
    _glfw.os4.keycodes[0x1f]            = GLFW_KEY_KP_3;
    _glfw.os4.keycodes[0x2d]            = GLFW_KEY_KP_4;
    _glfw.os4.keycodes[0x2e]            = GLFW_KEY_KP_5;
    _glfw.os4.keycodes[0x2f]            = GLFW_KEY_KP_6;
    _glfw.os4.keycodes[0x3d]            = GLFW_KEY_KP_7;
    _glfw.os4.keycodes[0x3e]            = GLFW_KEY_KP_8;
    _glfw.os4.keycodes[0x3f]            = GLFW_KEY_KP_9;
    _glfw.os4.keycodes[0x3c]            = GLFW_KEY_KP_DECIMAL;
    /*
    _glfw.os4.keycodes[KEY_KPEQUAL]    = GLFW_KEY_KP_EQUAL;
    */
    _glfw.os4.keycodes[RAWKEY_ENTER]    = GLFW_KEY_KP_ENTER; //

    for (int scancode = 0; scancode < 512; scancode++) {
        if (_glfw.os4.keycodes[scancode] > 0)
            _glfw.os4.scancodes[_glfw.os4.keycodes[scancode]] = scancode;
    }

    for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; i++) {
        strncpy(_glfw.os4.keynames[i], get_key_name(_glfw.os4.keycodes[i]), 16);
    }
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

GLFWbool _glfwConnectOS4(int platformID, _GLFWplatform *platform)
{
    const _GLFWplatform os4 =
        {
            .platformID = GLFW_PLATFORM_OS4,
            .init = _glfwInitOS4,
            .terminate = _glfwTerminateOS4,
            .getCursorPos = _glfwGetCursorPosOS4,
            .setCursorPos = _glfwSetCursorPosOS4,
            .setCursorMode = _glfwSetCursorModeOS4,
            .setRawMouseMotion = _glfwSetRawMouseMotionOS4,
            .rawMouseMotionSupported = _glfwRawMouseMotionSupportedOS4,
            .createCursor = _glfwCreateCursorOS4,
            .createStandardCursor = _glfwCreateStandardCursorOS4,
            .destroyCursor = _glfwDestroyCursorOS4,
            .setCursor = _glfwSetCursorOS4,
            .getScancodeName = _glfwGetScancodeNameOS4,
            .getKeyScancode = _glfwGetKeyScancodeOS4,
            .setClipboardString = _glfwSetClipboardStringOS4,
            .getClipboardString = _glfwGetClipboardStringOS4,
            .initJoysticks = _glfwInitJoysticksOS4,
            .terminateJoysticks = _glfwTerminateJoysticksOS4,
            .pollJoystick = _glfwPollJoystickOS4,
            .getMappingName = _glfwGetMappingNameOS4,
            .updateGamepadGUID = _glfwUpdateGamepadGUIDOS4,
            .freeMonitor = _glfwFreeMonitorOS4,
            .getMonitorPos = _glfwGetMonitorPosOS4,
            .getMonitorContentScale = _glfwGetMonitorContentScaleOS4,
            .getMonitorWorkarea = _glfwGetMonitorWorkareaOS4,
            .getVideoModes = _glfwGetVideoModesOS4,
            .getVideoMode = _glfwGetVideoModeOS4,
            .getGammaRamp = _glfwGetGammaRampOS4,
            .setGammaRamp = _glfwSetGammaRampOS4,
            .createWindow = _glfwCreateWindowOS4,
            .destroyWindow = _glfwDestroyWindowOS4,
            .setWindowTitle = _glfwSetWindowTitleOS4,
            .setWindowIcon = _glfwSetWindowIconOS4,
            .getWindowPos = _glfwGetWindowPosOS4,
            .setWindowPos = _glfwSetWindowPosOS4,
            .getWindowSize = _glfwGetWindowSizeOS4,
            .setWindowSize = _glfwSetWindowSizeOS4,
            .setWindowSizeLimits = _glfwSetWindowSizeLimitsOS4,
            .setWindowAspectRatio = _glfwSetWindowAspectRatioOS4,
            .getFramebufferSize = _glfwGetFramebufferSizeOS4,
            .getWindowFrameSize = _glfwGetWindowFrameSizeOS4,
            .getWindowContentScale = _glfwGetWindowContentScaleOS4,
            .iconifyWindow = _glfwIconifyWindowOS4,
            .restoreWindow = _glfwRestoreWindowOS4,
            .maximizeWindow = _glfwMaximizeWindowOS4,
            .showWindow = _glfwShowWindowOS4,
            .hideWindow = _glfwHideWindowOS4,
            .requestWindowAttention = _glfwRequestWindowAttentionOS4,
            .focusWindow = _glfwFocusWindowOS4,
            .setWindowMonitor = _glfwSetWindowMonitorOS4,
            .windowFocused = _glfwWindowFocusedOS4,
            .windowIconified = _glfwWindowIconifiedOS4,
            .windowVisible = _glfwWindowVisibleOS4,
            .windowMaximized = _glfwWindowMaximizedOS4,
            .windowHovered = _glfwWindowHoveredOS4,
            .framebufferTransparent = _glfwFramebufferTransparentOS4,
            .getWindowOpacity = _glfwGetWindowOpacityOS4,
            .setWindowResizable = _glfwSetWindowResizableOS4,
            .setWindowDecorated = _glfwSetWindowDecoratedOS4,
            .setWindowFloating = _glfwSetWindowFloatingOS4,
            .setWindowOpacity = _glfwSetWindowOpacityOS4,
            .setWindowMousePassthrough = _glfwSetWindowMousePassthroughOS4,
            .pollEvents = _glfwPollEventsOS4,
            .waitEvents = _glfwWaitEventsOS4,
            .waitEventsTimeout = _glfwWaitEventsTimeoutOS4,
            .postEmptyEvent = _glfwPostEmptyEventOS4,
            .getEGLPlatform = _glfwGetEGLPlatformOS4,
            .getEGLNativeDisplay = _glfwGetEGLNativeDisplayOS4,
            .getEGLNativeWindow = _glfwGetEGLNativeWindowOS4,            
            .getRequiredInstanceExtensions = _glfwGetRequiredInstanceExtensionsOS4,
            .getPhysicalDevicePresentationSupport = _glfwGetPhysicalDevicePresentationSupportOS4,
            .createWindowSurface = _glfwCreateWindowSurfaceOS4,
        };

    *platform = os4;
    return GLFW_TRUE;
}

int _glfwInitOS4(void)
{
    memset(&_glfw.os4, 0, sizeof(_glfw.os4));
    loadLibraries();
    createKeyTables();

    if (!(_glfw.os4.userPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE))) {
        return GLFW_FALSE;
    }

    if (!(_glfw.os4.appMsgPort = IExec->AllocSysObjectTags(ASOT_PORT, TAG_DONE))) {
        return GLFW_FALSE;
    }

    _glfwPollMonitorsOS4();

    OS4_FindApplicationName();

    return GLFW_TRUE;
}

void _glfwTerminateOS4(void)
{
    if (_glfw.os4.appMsgPort) {
        struct Message *msg;

        while ((msg = IExec->GetMsg(_glfw.os4.appMsgPort))) {
            IExec->ReplyMsg((struct Message *) msg);
        }
        IExec->FreeSysObject(ASOT_PORT, _glfw.os4.appMsgPort);
    }

    if (_glfw.os4.userPort) {
        struct Message *msg;

        while ((msg = IExec->GetMsg(_glfw.os4.userPort))) {
            IExec->ReplyMsg((struct Message *) msg);
        }
        IExec->FreeSysObject(ASOT_PORT, _glfw.os4.userPort);
    }

    if (_glfw.os4.clipboardString) {
        _glfw_free(_glfw.os4.clipboardString);
        _glfw.os4.clipboardString = NULL;
    }
    closeLibraries();
}

/************************************************************************************/
/********************************* AmigaOS4 METHODS *********************************/
/************************************************************************************/

static void
OS4_FindApplicationName()
{
    size_t size;
    char pathBuffer[MAXPATHLEN];

    if (IDOS->GetCliProgramName(pathBuffer, MAXPATHLEN - 1)) {
        dprintf("GetCliProgramName: '%s'\n", pathBuffer);
    } else {
        dprintf("Failed to get CLI program name, checking task node\n");

        struct Task* me = IExec->FindTask(NULL);
        snprintf(pathBuffer, MAXPATHLEN, "%s", ((struct Node *)me)->ln_Name);
    }

    size = strlen(pathBuffer) + 1;

    _glfw.os4.appName = malloc(size);

    if (_glfw.os4.appName) {
        snprintf(_glfw.os4.appName, size, pathBuffer);
    }

    dprintf("Application name: '%s'\n", _glfw.os4.appName);
}