/* Copyright (c) 2012 Toni Georgiev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _GKKEYDEFS_H_
#define _GKKEYDEFS_H_


#ifdef _WIN32

/*
	Key codes and modifiers begin
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GK_KEY_UNKNOWN		0
#define GK_KEY_BACKSPACE	VK_BACK
#define GK_KEY_TAB			VK_TAB
#define GK_KEY_CLEAR		VK_CLEAR
#define GK_KEY_RETURN		VK_RETURN
#define GK_KEY_PAUSE		VK_PAUSE
#define GK_KEY_ESCAPE		VK_ESCAPE
#define GK_KEY_SPACE		VK_SPACE
#define GK_KEY_QUOTE		222
#define GK_KEY_COMMA		188
#define GK_KEY_MINUS		189
#define GK_KEY_PERIOD		190
#define GK_KEY_SLASH		191
#define GK_KEY_NUM0			48
#define GK_KEY_NUM1			49
#define GK_KEY_NUM2			50
#define GK_KEY_NUM3			51
#define GK_KEY_NUM4			52
#define GK_KEY_NUM5			53
#define GK_KEY_NUM6			54
#define GK_KEY_NUM7			55
#define GK_KEY_NUM8			56
#define GK_KEY_NUM9			57
#define GK_KEY_SEMICOLON	186
#define GK_KEY_EQUALS		187
#define GK_KEY_LEFTBRACKET	219
#define GK_KEY_BACKSLASH	220
#define GK_KEY_RIGHTBRACKET 221
#define GK_KEY_BACKQUOTE	192
#define GK_KEY_A			65
#define GK_KEY_B			66
#define GK_KEY_C			67
#define GK_KEY_D			68
#define GK_KEY_E			69
#define GK_KEY_F			70
#define GK_KEY_G			71
#define GK_KEY_H			72
#define GK_KEY_I			73
#define GK_KEY_J			74
#define GK_KEY_K			75
#define GK_KEY_L			76
#define GK_KEY_M			77
#define GK_KEY_N			78
#define GK_KEY_O			79
#define GK_KEY_P			80
#define GK_KEY_Q			81
#define GK_KEY_R			82
#define GK_KEY_S			83
#define GK_KEY_T			84
#define GK_KEY_U			85
#define GK_KEY_V			86
#define GK_KEY_W			87
#define GK_KEY_X			88
#define GK_KEY_Y			89
#define GK_KEY_Z			90
#define GK_KEY_DEL			VK_DELETE
#define GK_KEY_KP0			96
#define GK_KEY_KP1			97
#define GK_KEY_KP2			98
#define GK_KEY_KP3			99
#define GK_KEY_KP4			100
#define GK_KEY_KP5			101
#define GK_KEY_KP6			102
#define GK_KEY_KP7			103
#define GK_KEY_KP8			104
#define GK_KEY_KP9			105
#define GK_KEY_KP_PERIOD	110
#define GK_KEY_KP_DIVIDE	111
#define GK_KEY_KP_MULTIPLY	106
#define GK_KEY_KP_MINUS		109
#define GK_KEY_KP_PLUS		107
#define GK_KEY_UP			38
#define GK_KEY_DOWN			40
#define GK_KEY_RIGHT		39
#define GK_KEY_LEFT			37
#define GK_KEY_INSERT		45
#define GK_KEY_HOME			36
#define GK_KEY_END			35
#define GK_KEY_PAGEUP		33
#define GK_KEY_PAGEDOWN		34
#define GK_KEY_F1			VK_F1
#define GK_KEY_F2			VK_F2
#define GK_KEY_F3			VK_F3
#define GK_KEY_F4			VK_F4
#define GK_KEY_F5			VK_F5
#define GK_KEY_F6			VK_F6
#define GK_KEY_F7			VK_F7
#define GK_KEY_F8			VK_F8
#define GK_KEY_F9			VK_F9
#define GK_KEY_F10			VK_F10
#define GK_KEY_F11			VK_F11
#define GK_KEY_F12			VK_F12
#define GK_KEY_F13			VK_F13
#define GK_KEY_F14			VK_F14
#define GK_KEY_F15			VK_F15
#define GK_KEY_NUMLOCK		VK_NUMLOCK
#define GK_KEY_CAPSLOCK		VK_CAPITAL
#define GK_KEY_SCROLLOCK	VK_SCROLL
#define GK_KEY_RSHIFT		VK_RSHIFT
#define GK_KEY_LSHIFT		VK_LSHIFT
#define GK_KEY_RCTRL		VK_RCONTROL
#define GK_KEY_LCTRL		VK_LCONTROL
#define GK_KEY_RALT			VK_RMENU
#define GK_KEY_LALT			VK_LMENU
#define GK_KEY_BREAK		VK_PAUSE

#define GK_KEY_MOD_NONE		0x0000
#define GK_KEY_MOD_LSHIFT	0x0001
#define GK_KEY_MOD_RSHIFT	0x0002
#define GK_KEY_MOD_LCTRL	0x0040
#define GK_KEY_MOD_RCTRL	0x0080
#define GK_KEY_MOD_LALT		0x0100
#define GK_KEY_MOD_RALT		0x0200
#define GK_KEY_MOD_NUM		0x1000
#define GK_KEY_MOD_CAPS		0x2000
#define GK_KEY_MOD_SHIFT	(GK_KEY_MOD_LSHIFT|GK_KEY_MOD_SHIFT)
#define GK_KEY_MOD_CTRL		(GK_KEY_MOD_LCTRL|GK_KEY_MOD_RCTRL)
#define GK_KEY_MOD_ALT		(GK_KEY_MOD_LALT|GK_KEY_MOD_RALT)

#else

#define XK_LATIN1

#define GK_KEY_R            0x0052

#define GK_KEY_SPACE        XK_Space
#define GK_KEY_ESCAPE       0xff1b

#define GK_KEY_NUMLOCK		XK_Num_Lock
#define GK_KEY_CAPSLOCK		XK_Caps_Lock
#define GK_KEY_SCROLLOCK	XK_Scroll_Lock
#define GK_KEY_RSHIFT		XK_Shift_R
#define GK_KEY_LSHIFT		XK_Shift_L
#define GK_KEY_RCTRL		XK_Control_R
#define GK_KEY_LCTRL		XK_Control_L
#define GK_KEY_RALT		XK_Alt_R
#define GK_KEY_LALT		XK_Alt_L
#define GK_KEY_BREAK		XK_Break

#define GK_KEY_MOD_NONE		0x0000
#define GK_KEY_MOD_LSHIFT	0x0001
#define GK_KEY_MOD_RSHIFT	0x0002
#define GK_KEY_MOD_LCTRL	0x0040
#define GK_KEY_MOD_RCTRL	0x0080
#define GK_KEY_MOD_LALT		0x0100
#define GK_KEY_MOD_RALT		0x0200
#define GK_KEY_MOD_NUM		0x1000
#define GK_KEY_MOD_CAPS		0x2000
#define GK_KEY_MOD_SHIFT	(GK_KEY_MOD_LSHIFT|GK_KEY_MOD_SHIFT)
#define GK_KEY_MOD_CTRL		(GK_KEY_MOD_LCTRL|GK_KEY_MOD_RCTRL)
#define GK_KEY_MOD_ALT		(GK_KEY_MOD_LALT|GK_KEY_MOD_RALT)

#endif

#endif