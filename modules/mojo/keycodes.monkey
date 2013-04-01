
'***** KeyDown/KeyHit Constants *****

Const KEY_LMB=1
Const KEY_RMB=2
Const KEY_MMB=3

Const KEY_BACKSPACE=8
Const KEY_TAB=9
Const KEY_ENTER=13
Const KEY_SHIFT=16
Const KEY_CONTROL=17
Const KEY_ESCAPE=27
Const KEY_SPACE=32
Const KEY_PAGEUP=33
Const KEY_PAGEDOWN=34
Const KEY_END=35
Const KEY_HOME=36
Const KEY_LEFT=37
Const KEY_UP=38
Const KEY_RIGHT=39
Const KEY_DOWN=40
Const KEY_INSERT=45
Const KEY_DELETE=46

Const KEY_0=48,KEY_1=49,KEY_2=50,KEY_3=51,KEY_4=52
Const KEY_5=53,KEY_6=54,KEY_7=55,KEY_8=56,KEY_9=57

Const KEY_A=65,KEY_B=66,KEY_C=67,KEY_D=68,KEY_E=69
Const KEY_F=70,KEY_G=71,KEY_H=72,KEY_I=73,KEY_J=74
Const KEY_K=75,KEY_L=76,KEY_M=77,KEY_N=78,KEY_O=79
Const KEY_P=80,KEY_Q=81,KEY_R=82,KEY_S=83,KEY_T=84
Const KEY_U=85,KEY_V=86,KEY_W=87,KEY_X=88,KEY_Y=89
Const KEY_Z=90

Const KEY_F1=112
Const KEY_F2=113
Const KEY_F3=114
Const KEY_F4=115
Const KEY_F5=116
Const KEY_F6=117
Const KEY_F7=118
Const KEY_F8=119
Const KEY_F9=120
Const KEY_F10=121
Const KEY_F11=122
Const KEY_F12=123

'TODO: add these for applicable targets
'Const KEY_LSHIFT=160
'Const KEY_RSHIFT=161
'Const KEY_LCONTROL=162
'Const KEY_RCONTROL=163
'Const KEY_LALT=164
'Const KEY_RALT=165

Const KEY_SEMICOLON=186
Const KEY_EQUALS=187
Const KEY_COMMA=188
Const KEY_MINUS=189
Const KEY_PERIOD=190
Const KEY_SLASH=191
Const KEY_TILDE=192
Const KEY_OPENBRACKET=219
Const KEY_BACKSLASH=220
Const KEY_CLOSEBRACKET=221
Const KEY_QUOTES=222

Const KEY_JOY0=$100

Const KEY_JOY0_A=$100		'32 joy0 states
Const KEY_JOY0_B=$101
Const KEY_JOY0_X=$102
Const KEY_JOY0_Y=$103
Const KEY_JOY0_LB=$104
Const KEY_JOY0_RB=$105
Const KEY_JOY0_BACK=$106
Const KEY_JOY0_START=$107
Const KEY_JOY0_LEFT=$108
Const KEY_JOY0_UP=$109
Const KEY_JOY0_RIGHT=$10a
Const KEY_JOY0_DOWN=$10b

Const KEY_JOY1_A=$120		'32 joy1 states

Const KEY_JOY2_A=$140		'32 joy2 states

Const KEY_JOY3_A=$160		'32 joy3 states

Const KEY_TOUCH0=$180		'32 touch states

Const KEY_BACK=$1a0			'back button (android)
Const KEY_MENU=$1a1			'menu button (android)
Const KEY_SEARCH=$1a2		'search button (android)

Const KEY_CLOSE=$1b0		'close button (glfw/desktop xna)

'***** GetChar constants *****

'Special char codes for 'unprintable' keys
'ascii-ish
'
Const CHAR_BACKSPACE=8
Const CHAR_TAB=9
Const CHAR_ENTER=13
Const CHAR_ESCAPE=27
Const CHAR_DELETE=127

'non-ascii
Const CHAR_PAGEUP=KEY_PAGEUP | $10000
Const CHAR_PAGEDOWN=KEY_PAGEDOWN | $10000
Const CHAR_END=KEY_END | $10000
Const CHAR_HOME=KEY_HOME | $10000
Const CHAR_LEFT=KEY_LEFT | $10000
Const CHAR_UP=KEY_UP | $10000
Const CHAR_RIGHT=KEY_RIGHT | $10000
Const CHAR_DOWN=KEY_DOWN | $10000
Const CHAR_INSERT=KEY_INSERT | $10000

'***** JoyDown/JoyHit constants *****

Const JOY_A=0
Const JOY_B=1
Const JOY_X=2
Const JOY_Y=3
Const JOY_LB=4
Const JOY_RB=5
Const JOY_BACK=6
Const JOY_START=7
Const JOY_LEFT=8
Const JOY_UP=9
Const JOY_RIGHT=10
Const JOY_DOWN=11

'***** MouseDown/MouseHit constants *****

Const MOUSE_LEFT=0
Const MOUSE_RIGHT=1
Const MOUSE_MIDDLE=2

