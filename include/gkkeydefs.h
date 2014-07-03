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


#ifdef GK_PLATFORM_WIN

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

#elif defined(GK_PLATFORM_ANDROID)

#include <android/keycodes.h>

#define GK_KEY_UNKNOWN		0
#define GK_KEY_BACKSPACE	GK_KEY_UNKNOWN
#define GK_KEY_TAB		AKEYCODE_TAB
#define GK_KEY_CLEAR		AKEYCODE_CLEAR
#define GK_KEY_RETURN		AKEYCODE_ENTER
#define GK_KEY_PAUSE		GK_KEY_UNKNOWN
#define GK_KEY_ESCAPE		AKEYCODE_BACK
#define GK_KEY_SPACE		AKEYCODE_SPACE
#define GK_KEY_QUOTE		GK_KEY_UNKNOWN
#define GK_KEY_COMMA		AKEYCODE_COMMA
#define GK_KEY_MINUS		AKEYCODE_MINUS
#define GK_KEY_PERIOD		AKEYCODE_PERIOD
#define GK_KEY_SLASH		AKEYCODE_SLASH
#define GK_KEY_NUM0		AKEYCODE_0
#define GK_KEY_NUM1		AKEYCODE_1
#define GK_KEY_NUM2		AKEYCODE_2
#define GK_KEY_NUM3		AKEYCODE_3
#define GK_KEY_NUM4		AKEYCODE_4
#define GK_KEY_NUM5		AKEYCODE_5
#define GK_KEY_NUM6		AKEYCODE_6
#define GK_KEY_NUM7		AKEYCODE_7
#define GK_KEY_NUM8		AKEYCODE_8
#define GK_KEY_NUM9		AKEYCODE_9
#define GK_KEY_SEMICOLON	AKEYCODE_SEMICOLON
#define GK_KEY_EQUALS		AKEYCODE_EQUALS
#define GK_KEY_LEFTBRACKET	AKEYCODE_LEFT_BRACKET
#define GK_KEY_BACKSLASH	AKEYCODE_BACKSLASH
#define GK_KEY_RIGHTBRACKET	AKEYCODE_RIGHT_BRACKET
#define GK_KEY_BACKQUOTE	AKEYCODE_APOSTROPHE
#define GK_KEY_A		AKEYCODE_A
#define GK_KEY_B		AKEYCODE_B
#define GK_KEY_C		AKEYCODE_C
#define GK_KEY_D		AKEYCODE_D
#define GK_KEY_E		AKEYCODE_E
#define GK_KEY_F		AKEYCODE_F
#define GK_KEY_G		AKEYCODE_G
#define GK_KEY_H		AKEYCODE_H
#define GK_KEY_I		AKEYCODE_I
#define GK_KEY_J		AKEYCODE_J
#define GK_KEY_K		AKEYCODE_K
#define GK_KEY_L		AKEYCODE_L
#define GK_KEY_M		AKEYCODE_M
#define GK_KEY_N		AKEYCODE_N
#define GK_KEY_O		AKEYCODE_O
#define GK_KEY_P		AKEYCODE_P
#define GK_KEY_Q		AKEYCODE_Q
#define GK_KEY_R		AKEYCODE_R
#define GK_KEY_S		AKEYCODE_S
#define GK_KEY_T		AKEYCODE_T
#define GK_KEY_U		AKEYCODE_U
#define GK_KEY_V		AKEYCODE_V
#define GK_KEY_W		AKEYCODE_W
#define GK_KEY_X		AKEYCODE_X
#define GK_KEY_Y		AKEYCODE_Y
#define GK_KEY_Z		AKEYCODE_Z
#define GK_KEY_DEL		AKEYCODE_DEL
#define GK_KEY_KP0		AKEYCODE_0
#define GK_KEY_KP1		AKEYCODE_1
#define GK_KEY_KP2		AKEYCODE_2
#define GK_KEY_KP3		AKEYCODE_3
#define GK_KEY_KP4		AKEYCODE_4
#define GK_KEY_KP5		AKEYCODE_5
#define GK_KEY_KP6		AKEYCODE_6
#define GK_KEY_KP7		AKEYCODE_7
#define GK_KEY_KP8		AKEYCODE_8
#define GK_KEY_KP9		AKEYCODE_9
#define GK_KEY_KP_PERIOD	AKEYCODE_PERIOD
#define GK_KEY_KP_DIVIDE	AKEYCODE_SLASH
#define GK_KEY_KP_MULTIPLY	AKEYCODE_STAR
#define GK_KEY_KP_MINUS		AKEYCODE_MINUS
#define GK_KEY_KP_PLUS		AKEYCODE_PLUS
#define GK_KEY_UP		AKEYCODE_DPAD_UP
#define GK_KEY_DOWN		AKEYCODE_DPAD_DOWN
#define GK_KEY_RIGHT		AKEYCODE_DPAD_RIGHT
#define GK_KEY_LEFT		AKEYCODE_DPAD_LEFT
#define GK_KEY_INSERT		GK_KEY_UNKNOWN
#define GK_KEY_HOME			GK_KEY_UNKNOWN
#define GK_KEY_END			GK_KEY_UNKNOWN
#define GK_KEY_PAGEUP		GK_KEY_UNKNOWN
#define GK_KEY_PAGEDOWN		GK_KEY_UNKNOWN
#define GK_KEY_F1			GK_KEY_UNKNOWN
#define GK_KEY_F2			GK_KEY_UNKNOWN
#define GK_KEY_F3			GK_KEY_UNKNOWN
#define GK_KEY_F4			GK_KEY_UNKNOWN
#define GK_KEY_F5			GK_KEY_UNKNOWN
#define GK_KEY_F6			GK_KEY_UNKNOWN
#define GK_KEY_F7			GK_KEY_UNKNOWN
#define GK_KEY_F8			GK_KEY_UNKNOWN
#define GK_KEY_F9			GK_KEY_UNKNOWN
#define GK_KEY_F10			AKEYCODE_HOME

#define GK_KEY_F11			GK_KEY_UNKNOWN
#define GK_KEY_F12			GK_KEY_UNKNOWN

#define GK_KEY_F13			GK_KEY_UNKNOWN
#define GK_KEY_F14			GK_KEY_UNKNOWN
#define GK_KEY_F15			GK_KEY_UNKNOWN

#define GK_KEY_NUMLOCK		GK_KEY_UNKNOWN
#define GK_KEY_CAPSLOCK		GK_KEY_UNKNOWN
#define GK_KEY_SCROLLOCK	GK_KEY_UNKNOWN
#define GK_KEY_RSHIFT		AKEYCODE_SHIFT_RIGHT
#define GK_KEY_LSHIFT		AKEYCODE_SHIFT_LEFT
#define GK_KEY_RCTRL		GK_KEY_UNKNOWN
#define GK_KEY_LCTRL		GK_KEY_UNKNOWN
#define GK_KEY_RALT		AKEYCODE_ALT_RIGHT
#define GK_KEY_LALT		AKEYCODE_ALT_LEFT
#define GK_KEY_BREAK		GK_KEY_UNKNOWN


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

#define GK_KEY_UNKNOWN		0
#define GK_KEY_BACKSPACE	22
#define GK_KEY_TAB			23
#define GK_KEY_CLEAR		GK_KEY_UNKNOWN
#define GK_KEY_RETURN		36
#define GK_KEY_PAUSE		127
#define GK_KEY_ESCAPE		9
#define GK_KEY_SPACE		65
#define GK_KEY_QUOTE		48
#define GK_KEY_COMMA		59
#define GK_KEY_MINUS		20
#define GK_KEY_PERIOD		60
#define GK_KEY_SLASH		61
#define GK_KEY_NUM0			19
#define GK_KEY_NUM1			10
#define GK_KEY_NUM2			11
#define GK_KEY_NUM3			12
#define GK_KEY_NUM4			13
#define GK_KEY_NUM5			14
#define GK_KEY_NUM6			15
#define GK_KEY_NUM7			16
#define GK_KEY_NUM8			17
#define GK_KEY_NUM9			18
#define GK_KEY_SEMICOLON	47
#define GK_KEY_EQUALS		21
#define GK_KEY_LEFTBRACKET	34
#define GK_KEY_BACKSLASH	51
#define GK_KEY_RIGHTBRACKET 35
#define GK_KEY_BACKQUOTE	49
#define GK_KEY_A			38
#define GK_KEY_B			56
#define GK_KEY_C			54
#define GK_KEY_D			40
#define GK_KEY_E			26
#define GK_KEY_F			41
#define GK_KEY_G			42
#define GK_KEY_H			43
#define GK_KEY_I			31
#define GK_KEY_J			44
#define GK_KEY_K			45
#define GK_KEY_L			46
#define GK_KEY_M			58
#define GK_KEY_N			57
#define GK_KEY_O			32
#define GK_KEY_P			33
#define GK_KEY_Q			24
#define GK_KEY_R			27
#define GK_KEY_S			39
#define GK_KEY_T			28
#define GK_KEY_U			30
#define GK_KEY_V			55
#define GK_KEY_W			25
#define GK_KEY_X			53
#define GK_KEY_Y			29
#define GK_KEY_Z			52
#define GK_KEY_DEL			119
#define GK_KEY_KP0			90
#define GK_KEY_KP1			87
#define GK_KEY_KP2			88
#define GK_KEY_KP3			89
#define GK_KEY_KP4			83
#define GK_KEY_KP5			84
#define GK_KEY_KP6			85
#define GK_KEY_KP7			79
#define GK_KEY_KP8			80
#define GK_KEY_KP9			81
#define GK_KEY_KP_PERIOD	91
#define GK_KEY_KP_DIVIDE	106
#define GK_KEY_KP_MULTIPLY	63
#define GK_KEY_KP_MINUS		82
#define GK_KEY_KP_PLUS		86
#define GK_KEY_UP			111
#define GK_KEY_DOWN			116
//
#define GK_KEY_RIGHT		114
#define GK_KEY_LEFT			113
#define GK_KEY_INSERT		118
#define GK_KEY_HOME			110
#define GK_KEY_END			115
#define GK_KEY_PAGEUP		112
#define GK_KEY_PAGEDOWN		117
#define GK_KEY_F1			67
#define GK_KEY_F2			68
#define GK_KEY_F3			69
#define GK_KEY_F4			70
#define GK_KEY_F5			71
#define GK_KEY_F6			72
#define GK_KEY_F7			73
#define GK_KEY_F8			74
#define GK_KEY_F9			75
#define GK_KEY_F10			76

#define GK_KEY_F11			GK_KEY_UNKNOWN
#define GK_KEY_F12			96

#define GK_KEY_F13			GK_KEY_UNKNOWN
#define GK_KEY_F14			GK_KEY_UNKNOWN
#define GK_KEY_F15			GK_KEY_UNKNOWN

#define GK_KEY_NUMLOCK		77
#define GK_KEY_CAPSLOCK		66
#define GK_KEY_SCROLLOCK	GK_KEY_UNKNOWN
#define GK_KEY_RSHIFT		62
#define GK_KEY_LSHIFT		50
#define GK_KEY_RCTRL		105
#define GK_KEY_LCTRL		37
#define GK_KEY_RALT			108
#define GK_KEY_LALT			64
#define GK_KEY_BREAK		127


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
