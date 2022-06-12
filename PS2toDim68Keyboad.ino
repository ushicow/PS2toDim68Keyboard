/*  Simple keyboard to serial port at 300 baud Dimension 68000
 *   by USHIRODA, Atsushi 

  Based PS2KeyAdvanced library example

  Advanced support PS2 Keyboard to get every key code byte from a PS2 Keyboard
  for testing purposes.

  IMPORTANT WARNING

    If using a DUE or similar board with 3V3 I/O you MUST put a level translator
    like a Texas Instruments TXS0102 or FET circuit as the signals are
    Bi-directional (signals transmitted from both ends on same wire).

    Failure to do so may damage your Arduino Due or similar board.

  Test History
    May 2022 Uno using Arduino V1.8.19, BTC Model 5100C PS/2 Keyboard

  This is for a LATIN style keyboard using Scan code set 2. See various
  websites on what different scan code sets use. Scan Code Set 2 is the
  default scan code set for PS2 keyboards on power up.

  Will support most keyboards even ones with multimedia keys or even 24 function keys.

  The circuit:
   * KBD Clock (PS2 pin 1) to an interrupt pin on Arduino ( this example pin 3 )
   * KBD Data (PS2 pin 5) to a data pin ( this example pin 4 )
   * +5V from Arduino to PS2 pin 4
   * GND from Arduino to PS2 pin 3

   The connector to mate with PS2 keyboard is a 6 pin Female Mini-Din connector
   PS2 Pins to signal
    1       KBD Data
    3       GND
    4       +5V
    5       KBD Clock

   The connector to mate with Dimension 68000 is a 5 pin Male DIN connector
   Dim68K Pins to signal
    1       Signal From Arduino Pin 2
    4       Ground
    5       +5V Power to Arduino

   Dimension 68000 J3 Keyboard connector pinout
   View of Front of Connector at rear of SYSTEM UNIT
         ---
       /     \
      /       \
   2 | o     o | 1
      \ o   o / 
     5 \  o  / 4
         ---
          3

   Keyboard has 5V and GND connected see plenty of examples and
   photos around on Arduino site and other sites about the PS2 Connector.

 Interrupts

   Clock pin from PS2 keyboard MUST be connected to an interrupt
   pin, these vary with the different types of Arduino

  PS2KeyAdvanced requires both pins specified for begin()

    keyboard.begin( data_pin, irq_pin );

  Valid irq pins:
     Arduino Uno:  2, 3
     Arduino Due:  All pins, except 13 (LED)
     Arduino Mega: 2, 3, 18, 19, 20, 21
     Teensy 2.0:   All pins, except 13 (LED)
     Teensy 2.0:   5, 6, 7, 8
     Teensy 1.0:   0, 1, 2, 3, 4, 6, 7, 16
     Teensy++ 2.0: 0, 1, 2, 3, 18, 19, 36, 37
     Teensy++ 1.0: 0, 1, 2, 3, 18, 19, 36, 37
     Sanguino:     2, 10, 11

  Read method Returns an UNSIGNED INT containing
        Make/Break status
        Caps status
        Shift, CTRL, ALT, ALT GR, GUI keys
        Flag for function key not a displayable/printable character
        8 bit key code

  Code Ranges (bottom byte of unsigned int)
        0       invalid/error
        1-1F    Functions (Caps, Shift, ALT, Enter, DEL... )
        1A-1F   Functions with ASCII control code
                    (DEL, BS, TAB, ESC, ENTER, SPACE)
        20-61   Printable characters noting
                    0-9 = 0x30 to 0x39 as ASCII
                    A to Z = 0x41 to 0x5A as upper case ASCII type codes
                    8B Extra European key
        61-A0   Function keys and other special keys (plus F2 and F1)
                    61-78 F1 to F24
                    79-8A Multimedia
                    8B NOT included
                    8C-8E ACPI power
                    91-A0 and F2 and F1 - Special multilingual
        A8-FF   Keyboard communications commands (note F2 and F1 are special
                codes for special multi-lingual keyboards)

    By using these ranges it is possible to perform detection of any key and do
    easy translation to ASCII/UTF-8 avoiding keys that do not have a valid code.

    Top Byte is 8 bits denoting as follows with defines for bit code

        Define name bit     description
        PS2_BREAK   15      1 = Break key code
                   (MSB)    0 = Make Key code
        PS2_SHIFT   14      1 = Shift key pressed as well (either side)
                            0 = NO shift key
        PS2_CTRL    13      1 = Ctrl key pressed as well (either side)
                            0 = NO Ctrl key
        PS2_CAPS    12      1 = Caps Lock ON
                            0 = Caps lock OFF
        PS2_ALT     11      1 = Left Alt key pressed as well
                            0 = NO Left Alt key
        PS2_ALT_GR  10      1 = Right Alt (Alt GR) key pressed as well
                            0 = NO Right Alt key
        PS2_GUI      9      1 = GUI key pressed as well (either)
                            0 = NO GUI key
        PS2_FUNCTION 8      1 = FUNCTION key non-printable character (plus space, tab, enter)
                            0 = standard character key

  Error Codes
     Most functions return 0 or 0xFFFF as error, other codes to note and
     handle appropriately
        0xAA   keyboard has reset and passed power up tests
               will happen if keyboard plugged in after code start
        0xFC   Keyboard General error or power up fail

  See PS2Keyboard.h file for returned definitions of Keys

  Note defines starting
            PS2_KEY_* are the codes this library returns
            PS2_*     remaining defines for use in higher levels

  To get the key as ASCII/UTF-8 single byte character conversion requires use
  of PS2KeyMap library AS WELL.

  Written by Paul Carpenter, PC Services <sales@pcserviceselectronics.co.uk>
*/

#include <PS2KeyAdvanced.h>

/* Keyboard constants  Change to suit your Arduino
   define pins used for data and clock from keyboard */
#define DATAPIN 4
#define IRQPIN  3

#define DEBUG 0
#define BAUD 300
#define VOID -1

uint16_t c;
uint8_t m;
const byte keycode[0x70][8] = {
//     Non,Shift, Cntl,Cn+Sh,  Alt,Alt+S,Alt+C,A+C+S
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 00 ERROR
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 01 NUM
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 02 SCROLL
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 03 CAPS
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 04 PRTSCR
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 05 PAUSE
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 06 Left Shift
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 07 Right Shift
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 08 Left Cntl
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 09 Right Alt
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 0A Left Cntl
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 0B Right Alt
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 0C Left GUI
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 0D Righ GUI
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 0E Menu
    { 0x8c, 0x8d, 0x8d, 0x8d, 0x8c, 0x8d, 0x8d, 0x8d }, // 0F Break
    { 0x2a, 0x1f, 0xbc, 0xbc, 0xaa, 0x9f, 0xbc, 0xbc }, // 10 SysRq
    { 0x17, 0x1c, 0xfb, 0xfb, 0x97, 0x9c, 0xfb, 0xfb }, // 11 HOM
    { 0x11, 0x0c, 0xbe, 0xbe, 0x91, 0x8c, 0xbe, 0xbe }, // 12 END
    { 0x19, 0x1e, 0xfc, 0xfc, 0x99, 0x9e, 0xfc, 0xfc }, // 13 PGU
    { 0x13, 0x0e, 0xbf, 0xbf, 0x93, 0x8e, 0xbf, 0xbf }, // 14 PGD
    { 0x14, 0x0f, 0xac, 0xac, 0x94, 0x8f, 0xac, 0xac }, // 15 LFT
    { 0x16, 0x1b, 0xbb, 0xbb, 0x96, 0x9b, 0xbb, 0xbb }, // 16 RGT
    { 0x18, 0x1d, 0xfd, 0xfd, 0x98, 0x9d, 0xfd, 0xfd }, // 17 UP
    { 0x12, 0x0d, 0xfe, 0xfe, 0x92, 0x8d, 0xfe, 0xfe }, // 18 DWN
    { 0x10, 0x0a, 0xe0, 0xe0, 0x90, 0x8a, 0xe0, 0xe0 }, // 19 INS
    { 0x8b, 0x0b, 0x7f, 0x7f, 0x8b, 0x8b, 0xff, 0xff }, // 1A DEL
    { 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb }, // 1B ESC
    { 0xc8, 0xc8, 0x7f, 0x7f, 0xc8, 0xc8, 0xff, 0xff }, // 1C Backspace
    { 0xc9, 0xaa, 0xab, 0xab, 0xc9, 0xaa, 0xab, 0xab }, // 1D TAB
    { 0xcd, 0xcd, 0xca, 0xca, 0xcd, 0xcd, 0xca, 0xca }, // 1E Enter
    { 0x20, 0x20, 0x20, 0x20, 0xa0, 0xa0, 0xa0, 0xa0 }, // 1F Space
    { 0x10, 0x0a, 0xe0, 0xe0, 0x90, 0x8a, 0xe0, 0xe0 }, // 20 Keypad 0 INS
    { 0x11, 0x0c, 0xbe, 0xbe, 0x91, 0x8c, 0xbe, 0xbe }, // 21 Keypad 1 END
    { 0x12, 0x0d, 0xfe, 0xfe, 0x92, 0x8d, 0xfe, 0xfe }, // 22 Keypad 2 DWN
    { 0x13, 0x0e, 0xbf, 0xbf, 0x93, 0x8e, 0xbf, 0xbf }, // 23 Keypad 3 PGD
    { 0x14, 0x0f, 0xac, 0xac, 0x94, 0x8f, 0xac, 0xac }, // 24 Keypad 4 LFT
    { 0x15, 0x1a, 0xba, 0xba, 0x95, 0x9a, 0xba, 0xba }, // 25 Keypad 5
    { 0x16, 0x1b, 0xbb, 0xbb, 0x96, 0x9b, 0xbb, 0xbb }, // 26 Keypad 6 RGT
    { 0x17, 0x1c, 0xfb, 0xfb, 0x97, 0x9c, 0xfb, 0xfb }, // 27 Keypad 7 HOM
    { 0x18, 0x1d, 0xfd, 0xfd, 0x98, 0x9d, 0xfd, 0xfd }, // 28 Keypad 8 UP
    { 0x19, 0x1e, 0xfc, 0xfc, 0x99, 0x9e, 0xfc, 0xfc }, // 29 Keypad 9 PGU
    { 0x8b, 0x0b, 0x7f, 0x7f, 0x8b, 0x8b, 0xff, 0xff }, // 2A Keypad . DEL
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 2B Keypad
    { 0x2b, 0x2b, 0x2b, 0x2b, 0xab, 0xab, 0xab, 0xab }, // 2C Keypad +
    { 0x2d, 0x2d, 0x2d, 0x2d, 0xad, 0xad, 0xad, 0xad }, // 2D Keypad -
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 2E Keypad *
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 2F Keypad /
    { 0x30, 0x29, 0x30, 0x29, 0xb0, 0xa9, 0xb0, 0xa9 }, // 30 '0)"
    { 0x31, 0x21, 0x31, 0x21, 0xb1, 0xa1, 0xb1, 0xa1 }, // 31 "1!"
    { 0x32, 0x40, 0x32, 0xc0, 0xb2, 0xc0, 0xb2, 0xc0 }, // 32 "2@"
    { 0x33, 0x23, 0x33, 0x23, 0xb3, 0xa3, 0xb3, 0xa3 }, // 33 "3#"
    { 0x34, 0x24, 0x34, 0x24, 0xb4, 0xa4, 0xb4, 0xa4 }, // 34 "4$"
    { 0x35, 0x25, 0x35, 0x25, 0xb5, 0xa5, 0xb5, 0xa5 }, // 35 "5%"
    { 0x36, 0x5e, 0x36, 0xde, 0xb6, 0xde, 0xb6, 0xde }, // 36 "6^"
    { 0x37, 0x26, 0x37, 0x26, 0xb7, 0xa6, 0xb7, 0xa6 }, // 37 "7&"
    { 0x38, 0x2a, 0x38, 0x2a, 0xb8, 0xaa, 0xb8, 0xaa }, // 38 "8*"
    { 0x39, 0x28, 0x39, 0x28, 0xb9, 0xa8, 0xb9, 0xa8 }, // 39 "9("
    { 0x27, 0x22, 0x27, 0x22, 0xa7, 0xa2, 0xa7, 0xa2 }, // 3A "'""
    { 0x2c, 0x3c, 0x2c, 0x3c, 0xac, 0xbc, 0xac, 0xbc }, // 3B ",<"
    { 0x2d, 0x5f, 0xdf, 0xdf, 0xad, 0xdf, 0xdf, 0xdf }, // 3C "-_"
    { 0x2e, 0x3e, 0x2e, 0x3e, 0xae, 0xbe ,0xae, 0xbe }, // 3D ".>"
    { 0x2f, 0x3f, 0x2f, 0x3f, 0xaf, 0xbf, 0xaf, 0xbf }, // 3E "/?"
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 3F
    { 0x60, 0x7e, 0x60, 0x7e, 0xe0, 0xfe, 0xe0, 0xfe }, // 40 "`~"
    { 0x61, 0x41, 0xc1, 0xc1, 0xe1, 0xc1, 0xc1, 0xc1 }, // 41 "A"
    { 0x62, 0x42, 0xc2, 0xc2, 0xe2, 0xc2, 0xc2, 0xc2 }, // 42 "B"
    { 0x63, 0x43, 0xc3, 0xc3, 0xe3, 0xc3, 0xc3, 0xc3 }, // 43 "C"
    { 0x64, 0x44, 0xc4, 0xc4, 0xe4, 0xc4, 0xc4, 0xc4 }, // 44 "D"
    { 0x65, 0x45, 0xc5, 0xc5, 0xe5, 0xc5, 0xc5, 0xc5 }, // 45 "E"
    { 0x66, 0x46, 0xc6, 0xc6, 0xe6, 0xc6, 0xc6, 0xc6 }, // 46 "F"
    { 0x67, 0x47, 0xc7, 0xc7, 0xe7, 0xc7, 0xc7, 0xc7 }, // 47 "G"
    { 0x68, 0x48, 0xc8, 0xc8, 0xe8, 0xc8, 0xc8, 0xc8 }, // 48 "H"
    { 0x69, 0x49, 0xc9, 0xc9, 0xe9, 0xc9, 0xc9, 0xc9 }, // 49 "I"
    { 0x6a, 0x4a, 0xca, 0xca, 0xea, 0xca, 0xca, 0xca }, // 4A "J"
    { 0x6b, 0x4b, 0xcb, 0xcb, 0xeb, 0xcb, 0xcb, 0xcb }, // 4B "K"
    { 0x6c, 0x4c, 0xcc, 0xcc, 0xec, 0xcc, 0xcc, 0xcc }, // 4C "L"
    { 0x6d, 0x4d, 0xcd, 0xcd, 0xed, 0xcd, 0xcd, 0xcd }, // 4D "M"
    { 0x6e, 0x4e, 0xce, 0xce, 0xee, 0xce, 0xce, 0xce }, // 4E "N"
    { 0x6f, 0x4f, 0xcf, 0xcf, 0xef, 0xcf, 0xcf, 0xcf }, // 4F "O"
    { 0x70, 0x50, 0xd0, 0xd0, 0xf0, 0xd0, 0xd0, 0xd0 }, // 50 "P"
    { 0x71, 0x51, 0xd1, 0xd1, 0xf1, 0xd1, 0xd1, 0xd1 }, // 51 "Q"
    { 0x72, 0x52, 0xd2, 0xd2, 0xf2, 0xd2, 0xd2, 0xd2 }, // 52 "R"
    { 0x73, 0x53, 0xd3, 0xd3, 0xf3, 0xd3, 0xd3, 0xd3 }, // 53 "S"
    { 0x74, 0x54, 0xd4, 0xd4, 0xf4, 0xd4, 0xd4, 0xd4 }, // 54 "T"
    { 0x75, 0x55, 0xd5, 0xd5, 0xf5, 0xd5, 0xd5, 0xd5 }, // 55 "U"
    { 0x76, 0x56, 0xd6, 0xd6, 0xf6, 0xd6, 0xd6, 0xd6 }, // 56 "V"
    { 0x77, 0x57, 0xd7, 0xd7, 0xf7, 0xd7, 0xd7, 0xd7 }, // 57 "W"
    { 0x78, 0x58, 0xd8, 0xd8, 0xf8, 0xd8, 0xd8, 0xd8 }, // 58 "X"
    { 0x79, 0x59, 0xd9, 0xd9, 0xf9, 0xd9, 0xd9, 0xd9 }, // 59 "Y"
    { 0x7a, 0x5a, 0xda, 0xda, 0xfa, 0xda, 0xda, 0xda }, // 5A "Z"
    { 0x3b, 0x3a, 0x3b, 0x3a, 0xbb, 0xba, 0xbb, 0xba }, // 5B ";:"
    { 0x5c, 0x7c, 0xdc, 0xdc, 0xdc, 0xfc, 0xdc, 0xfc }, // 5C "\|"
    { 0x5b, 0x7b, 0xdb, 0xdb, 0xdb, 0xfb, 0xdb, 0xdb }, // 5D "[{"
    { 0x5d, 0x7d, 0xdd, 0xdd, 0xdd, 0xfd, 0xdd, 0xdd }, // 5E "]}"
    { 0x3d, 0x2b, 0x3d, 0x2b, 0xbd, 0xab, 0xbd, 0xab }, // 5F "=+"
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 60
    { 0x00, 0xa0, 0x9a, 0x9a, 0x80, 0xa0, 0x9a, 0x9a }, // 61 F1
    { 0x01, 0xa1, 0x9b, 0x9b, 0x81, 0xa1, 0x9b, 0x9b }, // 62 F2
    { 0x02, 0xa2, 0x9c, 0x9c, 0x82, 0xa2, 0x9c, 0xc9 }, // 63 F3
    { 0x03, 0xa3, 0x9d, 0x9d, 0x83, 0xa3, 0x9d, 0x9d }, // 64 F4
    { 0x04, 0xa4, 0x9e, 0x9e, 0x84, 0xa4, 0x9e, 0x9e }, // 65 F5
    { 0x05, 0xa5, 0x9f, 0x9f, 0x85, 0xa5, 0x9f, 0x9f }, // 66 F6
    { 0x06, 0xa6, 0xae, 0xae, 0x86, 0xa6, 0xae, 0xae }, // 67 F7
    { 0x07, 0xa7, 0xaf, 0xaf, 0x87, 0xa7, 0xaf, 0xaf }, // 68 F8
    { 0x08, 0xa8, 0x8e, 0x8e, 0x88, 0xa8, 0x8e, 0x8e }, // 69 F9
    { 0x09, 0xa9, 0x8f, 0x8f, 0x89, 0xa9, 0x8f, 0x8f }, // 6A F10
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 6B F11
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 6C F12
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 6D F13
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 6E F14
    { VOID, VOID, VOID, VOID, VOID, VOID, VOID, VOID }, // 6F F15
};

PS2KeyAdvanced keyboard;


void setup( )
{
// Configure the keyboard library
keyboard.begin( DATAPIN, IRQPIN );
Serial.begin( BAUD );
keyboard.resetKey();
keyboard.setNoBreak( 1 );         // No break codes for keys (when key released)
keyboard.setNoRepeat( 1 );        // Don't repeat shift ctrl etc
#if DEBUG
Serial.println( "PS2 Advanced Key Simple Test:" );
#endif
}


void loop( )
{
  bool caps;
  
if( keyboard.available( ) )
  {
  // read the next key
  c = keyboard.read( );
  if( c > 0 )
    {
    m = 0;
#if DEBUG
    Serial.print( c, HEX );
    Serial.print("->");
#endif
    caps = c & PS2_CAPS;
    if ( c & PS2_SHIFT ) m |= 1;
    if ( c & PS2_CTRL ) m |= 2;
    if ( c & PS2_ALT ) m |= 4;
    if ( c == 6 ) c = 0x0f; // BRK
    c &= 0xff;
    if ( (c >= 'A') && (c <= 'Z') ) {
      if (caps) m ^= 1;
    }
#if DEBUG
    if ((c >= 0x0f) && (c < 0x70)) Serial.print( keycode[c][m], HEX );
    Serial.println();
#else
    if ((c >= 0x0f) && (c < 0x70)) Serial.write( keycode[c][m] );
#endif
    }
  }
}
