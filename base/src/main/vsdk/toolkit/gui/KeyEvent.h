#ifndef __KEY_EVENT__
#define __KEY_EVENT__

#include "vsdk/toolkit/gui/PresentationElement.h"
class KeyEvent : public PresentationElement {
public:
    static const int MASK_CTRL = 0x0001;
    static const int MASK_LCTRL = 0x0002;
    static const int MASK_RCTRL = 0x0004;
    static const int MASK_ALT = 0x0008;
    static const int MASK_LALT = 0x0010;
    static const int MASK_RALT = 0x0020;
    static const int MASK_ALTGR = 0x0040;
    static const int MASK_SHIFT = 0x0080;
    static const int MASK_LSHIFT = 0x0100;
    static const int MASK_RSHIFT = 0x0200;
    static const int MASK_WINKEY = 0x0400;

    static const int KEY_NONE = 0x0000;
    static const int KEY_A = 0x0001;
    static const int KEY_B = 0x0002;
    static const int KEY_C = 0x0003;
    static const int KEY_D = 0x0004;
    static const int KEY_E = 0x0005;
    static const int KEY_F = 0x0006;
    static const int KEY_G = 0x0007;
    static const int KEY_H = 0x0008;
    static const int KEY_I = 0x0009;
    static const int KEY_J = 0x000A;
    static const int KEY_K = 0x000B;
    static const int KEY_L = 0x000C;
    static const int KEY_M = 0x000D;
    static const int KEY_N = 0x000E;
    static const int KEY_O = 0x000F;
    static const int KEY_P = 0x0010;
    static const int KEY_Q = 0x0011;
    static const int KEY_R = 0x0012;
    static const int KEY_S = 0x0013;
    static const int KEY_T = 0x0014;
    static const int KEY_U = 0x0015;
    static const int KEY_V = 0x0016;
    static const int KEY_W = 0x0017;
    static const int KEY_X = 0x0018;
    static const int KEY_Y = 0x0019;
    static const int KEY_Z = 0x001A;
    static const int KEY_a = 0x001B;
    static const int KEY_b = 0x001C;
    static const int KEY_c = 0x001D;
    static const int KEY_d = 0x001E;
    static const int KEY_e = 0x001F;
    static const int KEY_f = 0x0020;
    static const int KEY_g = 0x0021;
    static const int KEY_h = 0x0022;
    static const int KEY_i = 0x0023;
    static const int KEY_j = 0x0024;
    static const int KEY_k = 0x0025;
    static const int KEY_l = 0x0026;
    static const int KEY_m = 0x0027;
    static const int KEY_n = 0x0028;
    static const int KEY_o = 0x0029;
    static const int KEY_p = 0x002A;
    static const int KEY_q = 0x002B;
    static const int KEY_r = 0x002C;
    static const int KEY_s = 0x002D;
    static const int KEY_t = 0x002E;
    static const int KEY_u = 0x002F;
    static const int KEY_v = 0x0030;
    static const int KEY_w = 0x0031;
    static const int KEY_x = 0x0032;
    static const int KEY_y = 0x0033;
    static const int KEY_z = 0x0034;
    static const int KEY_0 = 0x0035;
    static const int KEY_1 = 0x0036;
    static const int KEY_2 = 0x0037;
    static const int KEY_3 = 0x0038;
    static const int KEY_4 = 0x0039;
    static const int KEY_5 = 0x003A;
    static const int KEY_6 = 0x003B;
    static const int KEY_7 = 0x003C;
    static const int KEY_8 = 0x003D;
    static const int KEY_9 = 0x003E;
    static const int KEY_NUM0 = 0x003F;
    static const int KEY_NUM1 = 0x0040;
    static const int KEY_NUM2 = 0x0041;
    static const int KEY_NUM3 = 0x0042;
    static const int KEY_NUM4 = 0x0043;
    static const int KEY_NUM5 = 0x0044;
    static const int KEY_NUM6 = 0x0045;
    static const int KEY_NUM7 = 0x0046;
    static const int KEY_NUM8 = 0x0047;
    static const int KEY_NUM9 = 0x0048;
    static const int KEY_F1 = 0x0049;
    static const int KEY_F2 = 0x004A;
    static const int KEY_F3 = 0x004B;
    static const int KEY_F4 = 0x004C;
    static const int KEY_F5 = 0x004D;
    static const int KEY_F6 = 0x004E;
    static const int KEY_F7 = 0x004F;
    static const int KEY_F8 = 0x0050;
    static const int KEY_F9 = 0x0051;
    static const int KEY_F10 = 0x0052;
    static const int KEY_F11 = 0x0053;
    static const int KEY_F12 = 0x0054;
    static const int KEY_ESC = 0x0055;
    static const int KEY_PRINTSCREEN = 0x0056;
    static const int KEY_BACKSPACE = 0x0057;
    static const int KEY_INSERT = 0x0058;
    static const int KEY_DELETE = 0x0059;
    static const int KEY_PAGEUP = 0x005A;
    static const int KEY_PAGEDOWN = 0x005B;
    static const int KEY_HOME = 0x005C;
    static const int KEY_END = 0x005D;
    static const int KEY_SPACE = 0x005E;
    static const int KEY_LSHIFT = 0x005F;
    static const int KEY_RSHIFT = 0x0060;
    static const int KEY_LALT = 0x0061;
    static const int KEY_RALT = 0x0062;
    static const int KEY_ALTGR = 0x0063;
    static const int KEY_LCTRL = 0x0064;
    static const int KEY_RCTRL = 0x0065;
    static const int KEY_UP = 0x0066;
    static const int KEY_DOWN = 0x0067;
    static const int KEY_LEFT = 0x0068;
    static const int KEY_RIGHT = 0x0069;
    static const int KEY_NUMSLASH = 0x006A;
    static const int KEY_NUMASTERISK = 0x006B;
    static const int KEY_NUMMINUS = 0x006C;
    static const int KEY_NUMPLUS = 0x006D;
    static const int KEY_NUMLOCK = 0x006E;
    static const int KEY_NUMENTER = 0x006F;
    static const int KEY_ENTER = 0x0070;
    static const int KEY_CAPSLOCK = 0x0071;
    static const int KEY_TAB = 0x0072;
    static const int KEY_COMMA = 0x0073;
    static const int KEY_PERIOD = 0x0074;
    static const int KEY_NUMPERIOD = 0x0075;
    static const int KEY_EQUALS = 0x0076;
    static const int KEY_MINUS = 0x0077;

    KeyEvent()
        : keycode(KEY_NONE), unicodeId(KEY_NONE), modifierMask(0)
    {
    }

    int keycode;
    char unicodeId;
    int modifierMask;

    static const char* getKeyName(int key);
};


#endif
