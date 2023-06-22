// This is a Processing sketch, see https://processing.org/ to download the IDE

// Select the font, size and character ranges in the user configuration section
// of this sketch, which starts at line 120. Instructions start at line 50.


/*
Software License Agreement (FreeBSD License)
 
 Copyright (c) 2018 Bodmer (https://github.com/Bodmer)
 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 */

////////////////////////////////////////////////////////////////////////////////////////////////

// This is a processing sketch to create font files for the TFT_eSPI library:

// https://github.com/Bodmer/TFT_eSPI

// Coded by Bodmer January 2018, updated 10/2/19
// Version 0.8

// >>>>>>>>>>>>>>>>>>>>             INSTRUCTIONS             <<<<<<<<<<<<<<<<<<<<

// See comments below in code for specifying the font parameters (point size,
// unicode blocks to include etc). Ranges of characters (glyphs) and specific
// individual glyphs can be included in the created "*.vlw" font file.

// Created fonts are saved in the sketches "FontFiles" folder. Press Ctrl+K to
// see that folder location.

// 16 bit Unicode point codes in the range 0x0000 - 0xFFFF are supported.
// Codes 0-31 are control codes such as "tab" and "carraige return" etc.
// and 32 is a "space", these should NOT be included.

// The sketch will convert True Type (a .ttf or .otf file) file stored in the
// sketches "Data" folder as well as your computers' system fonts.

// To maximise rendering performance and the memory consumed only include the characters
// you will use. Characters at the start of the file will render faster than those at
// the end due to the buffering and file seeking overhead.

// The inclusion of "non-existant" characters in a font may give unpredicatable results
// when rendering with the TFT_eSPI library. The Processing sketch window that pops up
// to show the font characters will print "boxes" (also known as Tofu!) for non existant
// characters.

// Once created the files must be loaded into the ESP32 or ESP8266 SPIFFS memory
// using the Arduino IDE plugin detailed here:
// https://github.com/esp8266/arduino-esp8266fs-plugin
// https://github.com/me-no-dev/arduino-esp32fs-plugin

// When the sketch is run it will generate a file called "System_Font_List.txt" in the
// sketch "FontFiles" folder, press Ctrl+K to see it. Open the file in a text editor to
// view it. This list provides the font reference number needed below to locate that
// font on your system.

// The sketch also lists all the available system fonts to the console, you can increase
// the console line count (in preferences.txt) to stop some fonts scrolling out of view.
// See link in File>Preferences to locate "preferences.txt" file. You must close
// Processing then edit the file lines. If Processing is not closed first then the
// edits will be overwritten by defaults! Edit "preferences.txt" as follows for
// 3000 lines, then save, then run Processing again:

//     console.length=3000;             // Line 4 in file
//     console.scrollback.lines=3000;   // Line 7 in file


// Useful links:
/*

 https://en.wikipedia.org/wiki/Unicode_font
 
 https://www.gnu.org/software/freefont/
 https://www.gnu.org/software/freefont/sources/
 https://www.gnu.org/software/freefont/ranges/
 http://savannah.gnu.org/projects/freefont/
 
 http://www.google.com/get/noto/
 
 https://github.com/Bodmer/TFT_eSPI
 https://github.com/esp8266/arduino-esp8266fs-plugin
 https://github.com/me-no-dev/arduino-esp32fs-plugin
 
   >>>>>>>>>>>>>>>>>>>>         END OF INSTRUCTIONS         <<<<<<<<<<<<<<<<<<<< */


import java.awt.Desktop; // Required to allow sketch to open file windows


////////////////////////////////////////////////////////////////////////////////////////////////

//                       >>>>>>>>>> USER CONFIGURED PARAMETERS START HERE <<<<<<<<<<

// Use font number or name, -1 for fontNumber means use fontName below, a value >=0 means use system font number from list.
// When the sketch is run it will generate a file called "systemFontList.txt" in the sketch folder, press Ctrl+K to see it.
// Open the "systemFontList.txt" in a text editor to view the font files and reference numbers for your system.

int fontNumber = -1; // << Use [Number] in brackets from the fonts listed.

// OR use font name for ttf files placed in the "Data" folder or the font number seen in IDE Console for system fonts
//                                                  the font numbers are listed when the sketch is run.
//                |         1         2     |       Maximum filename size for SPIFFS is 31 including leading /
//                 1234567890123456789012345        and added point size and .vlw extension, so max is 25
String fontName = "Aura點陣宋";  // Manually crop the filename length later after creation if needed
                                     // Note: SPIFFS does NOT accept underscore in a filename!
String fontType = ".ttf";
//String fontType = ".otf";


// Define the font size in points for the TFT_eSPI font file
int  fontSize = 20;  //20

// Font size to use in the Processing sketch display window that pops up (can be different to above)
int displayFontSize = 28;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Next we specify which unicode blocks from the the Basic Multilingual Plane (BMP) are included in the final font file. //
// Note: The ttf/otf font file MAY NOT contain all possible Unicode characters, refer to the fonts online documentation. //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static final int[] unicodeBlocks = {
  // The list below has been created from the table here: https://en.wikipedia.org/wiki/Unicode_block
  // Remove // at start of lines below to include that unicode block, different code ranges can also be specified by
  // editing the start and end-of-range values. Multiple lines from the list below can be included, limited only by
  // the final font file size!

  // Block range,   //Block name, Code points, Assigned characters, Scripts
  // First, last,   //Range is inclusive of first and last codes
  0x0021, 0x007E, //Basic Latin, 128, 128, Latin (52 characters), Common (76 characters)
  //0x0080, 0x00FF, //Latin-1 Supplement, 128, 128, Latin (64 characters), Common (64 characters)
  //0x0100, 0x017F, //Latin Extended-A, 128, 128, Latin
  //0x0180, 0x024F, //Latin Extended-B, 208, 208, Latin
  //0x0250, 0x02AF, //IPA Extensions, 96, 96, Latin
  //0x02B0, 0x02FF, //Spacing Modifier Letters, 80, 80, Bopomofo (2 characters), Latin (14 characters), Common (64 characters)
  //0x0300, 0x036F, //Combining Diacritical Marks, 112, 112, Inherited
  //0x0370, 0x03FF, //Greek and Coptic, 144, 135, Coptic (14 characters), Greek (117 characters), Common (4 characters)
  //0x0400, 0x04FF, //Cyrillic, 256, 256, Cyrillic (254 characters), Inherited (2 characters)
  //0x0500, 0x052F, //Cyrillic Supplement, 48, 48, Cyrillic
  //0x0530, 0x058F, //Armenian, 96, 89, Armenian (88 characters), Common (1 character)
  //0x0590, 0x05FF, //Hebrew, 112, 87, Hebrew
  //0x0600, 0x06FF, //Arabic, 256, 255, Arabic (237 characters), Common (6 characters), Inherited (12 characters)
  //0x0700, 0x074F, //Syriac, 80, 77, Syriac
  //0x0750, 0x077F, //Arabic Supplement, 48, 48, Arabic
  //0x0780, 0x07BF, //Thaana, 64, 50, Thaana
  //0x07C0, 0x07FF, //NKo, 64, 59, Nko
  //0x0800, 0x083F, //Samaritan, 64, 61, Samaritan
  //0x0840, 0x085F, //Mandaic, 32, 29, Mandaic
  //0x0860, 0x086F, //Syriac Supplement, 16, 11, Syriac
  //0x08A0, 0x08FF, //Arabic Extended-A, 96, 73, Arabic (72 characters), Common (1 character)
  //0x0900, 0x097F, //Devanagari, 128, 128, Devanagari (124 characters), Common (2 characters), Inherited (2 characters)
  //0x0980, 0x09FF, //Bengali, 128, 95, Bengali
  //0x0A00, 0x0A7F, //Gurmukhi, 128, 79, Gurmukhi
  //0x0A80, 0x0AFF, //Gujarati, 128, 91, Gujarati
  //0x0B00, 0x0B7F, //Oriya, 128, 90, Oriya
  //0x0B80, 0x0BFF, //Tamil, 128, 72, Tamil
  //0x0C00, 0x0C7F, //Telugu, 128, 96, Telugu
  //0x0C80, 0x0CFF, //Kannada, 128, 88, Kannada
  //0x0D00, 0x0D7F, //Malayalam, 128, 117, Malayalam
  //0x0D80, 0x0DFF, //Sinhala, 128, 90, Sinhala
  //0x0E00, 0x0E7F, //Thai, 128, 87, Thai (86 characters), Common (1 character)
  //0x0E80, 0x0EFF, //Lao, 128, 67, Lao
  //0x0F00, 0x0FFF, //Tibetan, 256, 211, Tibetan (207 characters), Common (4 characters)
  //0x1000, 0x109F, //Myanmar, 160, 160, Myanmar
  //0x10A0, 0x10FF, //Georgian, 96, 88, Georgian (87 characters), Common (1 character)
  //0x1100, 0x11FF, //Hangul Jamo, 256, 256, Hangul
  //0x1200, 0x137F, //Ethiopic, 384, 358, Ethiopic
  //0x1380, 0x139F, //Ethiopic Supplement, 32, 26, Ethiopic
  //0x13A0, 0x13FF, //Cherokee, 96, 92, Cherokee
  //0x1400, 0x167F, //Unified Canadian Aboriginal Syllabics, 640, 640, Canadian Aboriginal
  //0x1680, 0x169F, //Ogham, 32, 29, Ogham
  //0x16A0, 0x16FF, //Runic, 96, 89, Runic (86 characters), Common (3 characters)
  //0x1700, 0x171F, //Tagalog, 32, 20, Tagalog
  //0x1720, 0x173F, //Hanunoo, 32, 23, Hanunoo (21 characters), Common (2 characters)
  //0x1740, 0x175F, //Buhid, 32, 20, Buhid
  //0x1760, 0x177F, //Tagbanwa, 32, 18, Tagbanwa
  //0x1780, 0x17FF, //Khmer, 128, 114, Khmer
  //0x1800, 0x18AF, //Mongolian, 176, 156, Mongolian (153 characters), Common (3 characters)
  //0x18B0, 0x18FF, //Unified Canadian Aboriginal Syllabics Extended, 80, 70, Canadian Aboriginal
  //0x1900, 0x194F, //Limbu, 80, 68, Limbu
  //0x1950, 0x197F, //Tai Le, 48, 35, Tai Le
  //0x1980, 0x19DF, //New Tai Lue, 96, 83, New Tai Lue
  //0x19E0, 0x19FF, //Khmer Symbols, 32, 32, Khmer
  //0x1A00, 0x1A1F, //Buginese, 32, 30, Buginese
  //0x1A20, 0x1AAF, //Tai Tham, 144, 127, Tai Tham
  //0x1AB0, 0x1AFF, //Combining Diacritical Marks Extended, 80, 15, Inherited
  //0x1B00, 0x1B7F, //Balinese, 128, 121, Balinese
  //0x1B80, 0x1BBF, //Sundanese, 64, 64, Sundanese
  //0x1BC0, 0x1BFF, //Batak, 64, 56, Batak
  //0x1C00, 0x1C4F, //Lepcha, 80, 74, Lepcha
  //0x1C50, 0x1C7F, //Ol Chiki, 48, 48, Ol Chiki
  //0x1C80, 0x1C8F, //Cyrillic Extended-C, 16, 9, Cyrillic
  //0x1CC0, 0x1CCF, //Sundanese Supplement, 16, 8, Sundanese
  //0x1CD0, 0x1CFF, //Vedic Extensions, 48, 42, Common (15 characters), Inherited (27 characters)
  //0x1D00, 0x1D7F, //Phonetic Extensions, 128, 128, Cyrillic (2 characters), Greek (15 characters), Latin (111 characters)
  //0x1D80, 0x1DBF, //Phonetic Extensions Supplement, 64, 64, Greek (1 character), Latin (63 characters)
  //0x1DC0, 0x1DFF, //Combining Diacritical Marks Supplement, 64, 63, Inherited
  //0x1E00, 0x1EFF, //Latin Extended Additional, 256, 256, Latin
  //0x1F00, 0x1FFF, //Greek Extended, 256, 233, Greek
  //0x2000, 0x206F, //General Punctuation, 112, 111, Common (109 characters), Inherited (2 characters)
  //0x2070, 0x209F, //Superscripts and Subscripts, 48, 42, Latin (15 characters), Common (27 characters)
  //0x20A0, 0x20CF, //Currency Symbols, 48, 32, Common
  //0x20D0, 0x20FF, //Combining Diacritical Marks for Symbols, 48, 33, Inherited
  //0x2100, 0x214F, //Letterlike Symbols, 80, 80, Greek (1 character), Latin (4 characters), Common (75 characters)
  //0x2150, 0x218F, //Number Forms, 64, 60, Latin (41 characters), Common (19 characters)
  //0x2190, 0x21FF, //Arrows, 112, 112, Common
  //0x2200, 0x22FF, //Mathematical Operators, 256, 256, Common
  //0x2300, 0x23FF, //Miscellaneous Technical, 256, 256, Common
  //0x2400, 0x243F, //Control Pictures, 64, 39, Common
  //0x2440, 0x245F, //Optical Character Recognition, 32, 11, Common
  //0x2460, 0x24FF, //Enclosed Alphanumerics, 160, 160, Common
  //0x2500, 0x257F, //Box Drawing, 128, 128, Common
  //0x2580, 0x259F, //Block Elements, 32, 32, Common
  //0x25A0, 0x25FF, //Geometric Shapes, 96, 96, Common
  //0x2600, 0x26FF, //Miscellaneous Symbols, 256, 256, Common
  //0x2700, 0x27BF, //Dingbats, 192, 192, Common
  //0x27C0, 0x27EF, //Miscellaneous Mathematical Symbols-A, 48, 48, Common
  //0x27F0, 0x27FF, //Supplemental Arrows-A, 16, 16, Common
  //0x2800, 0x28FF, //Braille Patterns, 256, 256, Braille
  //0x2900, 0x297F, //Supplemental Arrows-B, 128, 128, Common
  //0x2980, 0x29FF, //Miscellaneous Mathematical Symbols-B, 128, 128, Common
  //0x2A00, 0x2AFF, //Supplemental Mathematical Operators, 256, 256, Common
  //0x2B00, 0x2BFF, //Miscellaneous Symbols and Arrows, 256, 207, Common
  //0x2C00, 0x2C5F, //Glagolitic, 96, 94, Glagolitic
  //0x2C60, 0x2C7F, //Latin Extended-C, 32, 32, Latin
  //0x2C80, 0x2CFF, //Coptic, 128, 123, Coptic
  //0x2D00, 0x2D2F, //Georgian Supplement, 48, 40, Georgian
  //0x2D30, 0x2D7F, //Tifinagh, 80, 59, Tifinagh
  //0x2D80, 0x2DDF, //Ethiopic Extended, 96, 79, Ethiopic
  //0x2DE0, 0x2DFF, //Cyrillic Extended-A, 32, 32, Cyrillic
  //0x2E00, 0x2E7F, //Supplemental Punctuation, 128, 74, Common
  //0x2E80, 0x2EFF, //CJK Radicals Supplement, 128, 115, Han
  //0x2F00, 0x2FDF, //Kangxi Radicals, 224, 214, Han
  //0x2FF0, 0x2FFF, //Ideographic Description Characters, 16, 12, Common
  //0x3000, 0x303F, //CJK Symbols and Punctuation, 64, 64, Han (15 characters), Hangul (2 characters), Common (43 characters), Inherited (4 characters)
  //0x3040, 0x309F, //Hiragana, 96, 93, Hiragana (89 characters), Common (2 characters), Inherited (2 characters)
  //0x30A0, 0x30FF, //Katakana, 96, 96, Katakana (93 characters), Common (3 characters)
  //0x3100, 0x312F, //Bopomofo, 48, 42, Bopomofo
  //0x3130, 0x318F, //Hangul Compatibility Jamo, 96, 94, Hangul
  //0x3190, 0x319F, //Kanbun, 16, 16, Common
  //0x31A0, 0x31BF, //Bopomofo Extended, 32, 27, Bopomofo
  //0x31C0, 0x31EF, //CJK Strokes, 48, 36, Common
  //0x31F0, 0x31FF, //Katakana Phonetic Extensions, 16, 16, Katakana
  //0x3200, 0x32FF, //Enclosed CJK Letters and Months, 256, 254, Hangul (62 characters), Katakana (47 characters), Common (145 characters)
  //0x3300, 0x33FF, //CJK Compatibility, 256, 256, Katakana (88 characters), Common (168 characters)
  //0x3400, 0x4DBF, //CJK Unified Ideographs Extension A, 6,592, 6,582, Han
  //0x4DC0, 0x4DFF, //Yijing Hexagram Symbols, 64, 64, Common
  //0x4E00, 0x9FFF, //CJK Unified Ideographs, 20,992, 20,971, Han
  //0xA000, 0xA48F, //Yi Syllables, 1,168, 1,165, Yi
  //0xA490, 0xA4CF, //Yi Radicals, 64, 55, Yi
  //0xA4D0, 0xA4FF, //Lisu, 48, 48, Lisu
  //0xA500, 0xA63F, //Vai, 320, 300, Vai
  //0xA640, 0xA69F, //Cyrillic Extended-B, 96, 96, Cyrillic
  //0xA6A0, 0xA6FF, //Bamum, 96, 88, Bamum
  //0xA700, 0xA71F, //Modifier Tone Letters, 32, 32, Common
  //0xA720, 0xA7FF, //Latin Extended-D, 224, 160, Latin (155 characters), Common (5 characters)
  //0xA800, 0xA82F, //Syloti Nagri, 48, 44, Syloti Nagri
  //0xA830, 0xA83F, //Common Indic Number Forms, 16, 10, Common
  //0xA840, 0xA87F, //Phags-pa, 64, 56, Phags Pa
  //0xA880, 0xA8DF, //Saurashtra, 96, 82, Saurashtra
  //0xA8E0, 0xA8FF, //Devanagari Extended, 32, 30, Devanagari
  //0xA900, 0xA92F, //Kayah Li, 48, 48, Kayah Li (47 characters), Common (1 character)
  //0xA930, 0xA95F, //Rejang, 48, 37, Rejang
  //0xA960, 0xA97F, //Hangul Jamo Extended-A, 32, 29, Hangul
  //0xA980, 0xA9DF, //Javanese, 96, 91, Javanese (90 characters), Common (1 character)
  //0xA9E0, 0xA9FF, //Myanmar Extended-B, 32, 31, Myanmar
  //0xAA00, 0xAA5F, //Cham, 96, 83, Cham
  //0xAA60, 0xAA7F, //Myanmar Extended-A, 32, 32, Myanmar
  //0xAA80, 0xAADF, //Tai Viet, 96, 72, Tai Viet
  //0xAAE0, 0xAAFF, //Meetei Mayek Extensions, 32, 23, Meetei Mayek
  //0xAB00, 0xAB2F, //Ethiopic Extended-A, 48, 32, Ethiopic
  //0xAB30, 0xAB6F, //Latin Extended-E, 64, 54, Latin (52 characters), Greek (1 character), Common (1 character)
  //0xAB70, 0xABBF, //Cherokee Supplement, 80, 80, Cherokee
  //0xABC0, 0xABFF, //Meetei Mayek, 64, 56, Meetei Mayek
  //0xAC00, 0xD7AF, //Hangul Syllables, 11,184, 11,172, Hangul
  //0xD7B0, 0xD7FF, //Hangul Jamo Extended-B, 80, 72, Hangul
  //0xD800, 0xDB7F, //High Surrogates, 896, 0, Unknown
  //0xDB80, 0xDBFF, //High Private Use Surrogates, 128, 0, Unknown
  //0xDC00, 0xDFFF, //Low Surrogates, 1,024, 0, Unknown
  //0xE000, 0xF8FF, //Private Use Area, 6,400, 6,400, Unknown
  //0xF900, 0xFAFF, //CJK Compatibility Ideographs, 512, 472, Han
  //0xFB00, 0xFB4F, //Alphabetic Presentation Forms, 80, 58, Armenian (5 characters), Hebrew (46 characters), Latin (7 characters)
  //0xFB50, 0xFDFF, //Arabic Presentation Forms-A, 688, 611, Arabic (609 characters), Common (2 characters)
  //0xFE00, 0xFE0F, //Variation Selectors, 16, 16, Inherited
  //0xFE10, 0xFE1F, //Vertical Forms, 16, 10, Common
  //0xFE20, 0xFE2F, //Combining Half Marks, 16, 16, Cyrillic (2 characters), Inherited (14 characters)
  //0xFE30, 0xFE4F, //CJK Compatibility Forms, 32, 32, Common
  //0xFE50, 0xFE6F, //Small Form Variants, 32, 26, Common
  //0xFE70, 0xFEFF, //Arabic Presentation Forms-B, 144, 141, Arabic (140 characters), Common (1 character)
  //0xFF00, 0xFFEF, //Halfwidth and Fullwidth Forms, 240, 225, Hangul (52 characters), Katakana (55 characters), Latin (52 characters), Common (66 characters)
  //0xFFF0, 0xFFFF, //Specials, 16, 5, Common

  //0x0030, 0x0039, //Example custom range (numbers 0-9)
  //0x0041, 0x005A, //Example custom range (Upper case A-Z)
  //0x0061, 0x007A, //Example custom range (Lower case a-z)
};

// Here we specify particular individual Unicodes to be included (appended at end of selected range)
static final int[] specificUnicodes = {

  // Commonly used codes, add or remove // in next line
  // 0x00A3, 0x00B0, 0x00B5, 0x03A9, 0x20AC, // £ ° µ Ω €

  // Numbers and characters for showing time, change next line to //* to use
/*
    0x002B, 0x002D, 0x002E, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, // - + . 0 1 2 3 4
    0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x0061, 0x006D, // 5 6 7 8 9 : a m
    0x0070,                                                         // p
 //*/

  // More characters for TFT_eSPI test sketches, change next line to //* to use
  /*
    0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107, 0x010C, 0x010D,
    0x010E, 0x010F, 0x0110, 0x0111, 0x0118, 0x0119, 0x011A, 0x011B,
 
    0x0131, 0x0139, 0x013A, 0x013D, 0x013E, 0x0141, 0x0142, 0x0143,
    0x0144, 0x0147, 0x0148, 0x0150, 0x0151, 0x0152, 0x0153, 0x0154,
    0x0155, 0x0158, 0x0159, 0x015A, 0x015B, 0x015E, 0x015F, 0x0160,
    0x0161, 0x0162, 0x0163, 0x0164, 0x0165, 0x016E, 0x016F, 0x0170,
    0x0171, 0x0178, 0x0179, 0x017A, 0x017B, 0x017C, 0x017D, 0x017E,
    0x0192,
 
    0x02C6, 0x02C7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC, 0x02DD,
    0x03A9, 0x03C0, 0x2013, 0x2014, 0x2018, 0x2019, 0x201A, 0x201C,
    0x201D, 0x201E, 0x2020, 0x2021, 0x2022, 0x2026, 0x2030, 0x2039,
    0x203A, 0x2044, 0x20AC,
 
    0x2122, 0x2202, 0x2206, 0x220F,
 
    0x2211, 0x221A, 0x221E, 0x222B, 0x2248, 0x2260, 0x2264, 0x2265,
    0x25CA,
 
    0xF8FF, 0xFB01, 0xFB02,
  //*/

0x5b89,0x5fbd,0x7701,0x5730,0x7ea7,0x5e02,0xff1a,0x5408,0x80a5,0x3001,0x5bbf,0x5dde,0x6dee,0x5317,0x961c,0x9633,0x868c,0x57e0,0x5357,0x6ec1,0x9a6c,0x978d,0x5c71,0x829c,0x6e56,0x94dc,0x9675,0x5e86,0x9ec4,0x516d,0x6c60,0x5ba3,0x57ce,0x4eb3,0x3002,0x53bf,0x754c,0x9996,0x660e,0x5149,0x5929,0x957f,0x6850,0x5b81,0x56fd,0x5de2,0x798f,0x5efa,0x53a6,0x95e8,0x5e73,0x4e09,0x8386,0x7530,0x6cc9,0x6f33,0x9f99,0x5ca9,0x5fb7,0x6e05,0x90b5,0x6b66,0x5937,0x74ef,0x6c38,0x77f3,0x72ee,0x664b,0x6c5f,0x6d77,0x9f0e,0x7518,0x8083,0x5170,0x6c34,0x5609,0x5cea,0x5173,0x91d1,0x660c,0x767d,0x94f6,0x9152,0x5f20,0x6396,0x5a01,0x51c9,0x5b9a,0x897f,0x9647,0x7389,0x6566,0x714c,0x4e34,0x590f,0x4f5c,0x5e7f,0x4e1c,0x6df1,0x5733,0x8fdc,0x97f6,0x6cb3,0x6e90,0x6885,0x6f6e,0x6c55,0x5934,0x63ed,0x5c3e,0x60e0,0x839e,0x73e0,0x4e2d,0x4f5b,0x8087,0x4e91,0x6d6e,0x8302,0x540d,0x6e5b,0x82f1,0x8fde,0x4e50,0x96c4,0x5174,0x666e,0x9646,0x4e30,0x6069,0x53f0,0x5f00,0x9e64,0x56db,0x4f1a,0x7f57,0x6625,0x5316,0x4fe1,0x5b9c,0x9ad8,0x5434,0x5ddd,0x5ec9,0x96f7,0x8d35,0x76d8,0x9075,0x4e49,0x987a,0x6bd5,0x8282,0x4ec1,0x9547,0x8d64,0x6000,0x51ef,0x91cc,0x90fd,0x5300,0x5bb6,0x5e84,0x90af,0x90f8,0x5510,0x4fdd,0x79e6,0x7687,0x5c9b,0x90a2,0x53e3,0x627f,0x6ca7,0x5eca,0x574a,0x8861,0x8f9b,0x96c6,0x85c1,0x65b0,0x9e7f,0x8fc1,0x9738,0x6dbf,0x7891,0x5e97,0x6cca,0x4efb,0x4e18,0x9a85,0x95f4,0x5180,0x5bab,0x6c99,0x6c49,0x5341,0x5830,0x8944,0x8346,0x5b5d,0x611f,0x5188,0x9102,0x54b8,0x968f,0x76f4,0x8f96,0x4ed9,0x6843,0x6f5c,0x4e39,0x8001,0x67a3,0x949f,0x7965,0x5e94,0x9ebb,0x7a74,0x5927,0x51b6,0x58c1,0x6d2a,0x677e,0x6ecb,0x679d,0x5f53,0x65bd,0x5229,0x5e38,0x76ca,0x5cb3,0x682a,0x6d32,0x6e58,0x6f6d,0x90f4,0x5a04,0x5e95,0x8012,0x6d4f,0x6d25,0x6c85,0x6c68,0x91b4,0x4e61,0x8d44,0x51b7,0x6d9f,0x5409,0x6797,0x539f,0x8fbd,0x901a,0x6986,0x6811,0x78d0,0x86df,0x6866,0x7538,0x8212,0x6d2e,0x53cc,0x516c,0x4e3b,0x5cad,0x5ef6,0x56fe,0x4eec,0x73f2,0x4e95,0x548c,0x6276,0x4f59,0x4e5d,0x8d63,0x666f,0x9e70,0x840d,0x4e0a,0x9976,0x629a,0x6674,0x6717,0x7a7a,0x84dd,0x5c11,0x7166,0x8273,0x98ce,0x6c14,0x723d,0x5a9a,0x6e29,0x6696,0x5bd2,0x591a,0x4e0b,0x96e8,0x51b0,0x96f9,0x971c,0x9732,0x0020,0x9884,0x62a5,0x8bcd,0x8bed,0x6709,0x9634,0xff0c,0x70ed,0x5c0f,0x6700,0x4f4e,0x5ea6,0x5b9e,0x65f6,0x8d28,0x91cf,0x5411,0x4f18,0x826f,0x91cd,0x8f7b,0x7b2c,0x5468,0x5fcc,0x003a,0x2103,0x5eff,0x5345,0x7acb,0x60ca,0x86f0,0x5206,0x8c37,0x6ee1,0x8292,0x79cd,0x81f3,0x6691,0x79cb,0x5904,0x964d,0x51ac,0x96ea,0x7684,0x522b,0x4e3a,0x5e72,0x652f,0x5386,0x5bc5,0x6708,0x536f,0x8fb0,0x5df3,0x5348,0x672a,0x7533,0x9149,0x620c,0x4ea5,0x5b50,0x4e11,0x7b80,0x540e,0x201c,0x7532,0x4e59,0x4e19,0x4e01,0x620a,0x5df1,0x5e9a,0x58ec,0x7678,0x201d,0x79f0,0x4e8c,0x5177,0x4f53,0x60c5,0x51b5,0xff08,0x9f20,0xff09,0x725b,0x864e,0x5154,0x86c7,0x7f8a,0x7334,0x9e21,0x72d7,0x732a,0x5de5,0x65e5,0x4f11,0x606f,0x5143,0x65e6,0x653e,0x5047,0x0031,0x0033,0x0028,0x519c,0x9664,0x5915,0x6b63,0x521d,0x4e00,0x0029,0x52b3,0x52a8,0x7aef,0x0037,0x4e94,0x4e03,0x516b,0x884c,0x653f,0x7c7b,0x8086,0x8d66,0x0032,0x8983,0x518c,0x53d7,0x5c01,0x0034,0x8fdb,0x8868,0x7ae0,0x0035,0x5e03,0x4e8b,0x0036,0x62db,0x8d24,0x0038,0x9881,0x8bcf,0x0039,0x9063,0x4f7f,0x0030,0x8d4f,0x8d3a,0x8d50,0x6064,0x5b64,0x60f8,0x4e3e,0x6574,0x5bb9,0x5e78,0x4eb2,0x6c11,0x8fb9,0x5883,0x6664,0x89c1,0x63a5,0x7ea6,0x5b98,0x8d74,0x81f4,0x4ed5,0x5f52,0x6355,0x6349,0x68c0,0x5211,0x72f1,0x9009,0x5c06,0x8bad,0x5175,0x51fa,0x5e08,0x7b51,0x5824,0x9632,0x7f2e,0x90ed,0x6cbb,0x9053,0x6d82,0x8425,0x5ba4,0x8859,0x6e20,0x4fee,0x7f6e,0x4ea7,0x62c6,0x5378,0x7ecf,0x7edc,0x88c1,0x5236,0x9020,0x8239,0x683d,0x7a7f,0x53d6,0x9c7c,0x7eb3,0x755c,0x7267,0x52b5,0x4f10,0x6728,0x8d22,0x89e3,0x65ad,0x8681,0x626b,0x5c4b,0x820d,0x4ea4,0x6613,0x7ad6,0x67f1,0x6881,0x4eba,0x535a,0x5f69,0x5668,0x751f,0x6d3b,0x6c42,0x4e60,0x827a,0x7ed3,0x513f,0x8033,0x4e73,0x4e66,0x804c,0x5165,0x5b66,0x79fb,0x5f99,0x642c,0x5b85,0x5c45,0x53cb,0x5bb4,0x5243,0x7406,0x53d1,0x624b,0x8db3,0x754b,0x730e,0x6559,0x533b,0x7597,0x75c5,0x670d,0x836f,0x9488,0x7078,0x63a2,0x8bbc,0x95ee,0x54cd,0x535c,0x758f,0x51a4,0x9001,0x5546,0x6302,0x533e,0x6536,0x4ed3,0x503a,0x673a,0x8f66,0x915d,0x917f,0x7b7e,0x7f51,0x5272,0x8702,0x871c,0x4e1a,0x5356,0x76d6,0x907f,0x8d77,0x57fa,0x571f,0x67b6,0x773c,0x810a,0x5395,0x5e93,0x5c4f,0x6247,0x5e8a,0x7893,0x7859,0x9642,0x7834,0x574f,0x57a3,0x8865,0x7076,0x585e,0x8c22,0x9970,0x5899,0x6ce5,0x7a20,0x796d,0x7940,0x6c90,0x6d74,0x7948,0x8bb8,0x613f,0x8fd8,0x916c,0x795e,0x767b,0x55e3,0x5e99,0x5851,0x7ed8,0x658b,0x91ae,0x9f50,0x9999,0x711a,0x706b,0x7f18,0x5a5a,0x5ac1,0x59fb,0x5a36,0x91c7,0x5e01,0x5fb5,0x5e10,0x8863,0x51a0,0x7b04,0x5a7f,0x8ba2,0x76df,0x4e27,0x846c,0x542f,0x6512,0x575f,0x6e21,0x5bff,0x6b93,0x67e9,0x6210,0x5893,0x8bf8,0x4e0d,0x9980,0x52ff
  
};

//                       >>>>>>>>>> USER CONFIGURED PARAMETERS END HERE <<<<<<<<<<

////////////////////////////////////////////////////////////////////////////////////////////////

// Variable to hold the inclusive Unicode range (16 bit values only for this sketch)
int firstUnicode = 0;
int lastUnicode  = 0;

PFont myFont;

PrintWriter logOutput;

void setup() {
  logOutput = createWriter("FontFiles/System_Font_List.txt"); 

  size(1000, 800);

  // Print the available fonts to the console as a list:
  String[] fontList = PFont.list();
  printArray(fontList);

  // Save font list to file
  for (int x = 0; x < fontList.length; x++)
  {
    logOutput.print("[" + x + "] ");
    logOutput.println(fontList[x]);
  }
  logOutput.flush(); // Writes the remaining data to the file
  logOutput.close(); // Finishes the file

  // Set the fontName from the array number or the defined fontName
  if (fontNumber >= 0)
  {
    fontName = fontList[fontNumber];
    fontType = "";
  }

  char[]   charset;
  int  index = 0, count = 0;

  int blockCount = unicodeBlocks.length;

  for (int i = 0; i < blockCount; i+=2) {
    firstUnicode = unicodeBlocks[i];
    lastUnicode  = unicodeBlocks[i+1];
    if (lastUnicode < firstUnicode) {
      delay(100);
      System.err.println("ERROR: Bad Unicode range secified, last < first!");
      System.err.print("first in range = 0x" + hex(firstUnicode, 4));
      System.err.println(", last in range  = 0x" + hex(lastUnicode, 4));
      while (true);
    }
    // calculate the number of characters
    count += (lastUnicode - firstUnicode + 1);
  }

  count += specificUnicodes.length;

  println();
  println("=====================");
  println("Creating font file...");
  println("Unicode blocks included     = " + (blockCount/2));
  println("Specific unicodes included  = " + specificUnicodes.length);
  println("Total number of characters  = " + count);

  if (count == 0) {
    delay(100);
    System.err.println("ERROR: No Unicode range or specific codes have been defined!");
    while (true);
  }

  // allocate memory
  charset = new char[count];

  for (int i = 0; i < blockCount; i+=2) {
    firstUnicode = unicodeBlocks[i];
    lastUnicode  =  unicodeBlocks[i+1];

    // loading the range specified
    for (int code = firstUnicode; code <= lastUnicode; code++) {
      charset[index] = Character.toChars(code)[0];
      index++;
    }
  }

  // loading the specific point codes
  for (int i = 0; i < specificUnicodes.length; i++) {
    charset[index] = Character.toChars(specificUnicodes[i])[0];
    index++;
  }

  // Make font smooth (anti-aliased)
  boolean smooth = true;

  // Create the font in memory
  myFont = createFont(fontName+fontType, displayFontSize, smooth, charset);

  // Print characters to the sketch window
  fill(0, 0, 0);
  textFont(myFont);

  // Set the left and top margin
  int margin = displayFontSize;
  translate(margin/2, margin);

  int gapx = displayFontSize*10/8;
  int gapy = displayFontSize*10/8;
  index = 0;
  fill(0);

  textSize(displayFontSize);

  for (int y = 0; y < height-gapy; y += gapy) {
    int x = 0;
    while (x < width) {

      int unicode = charset[index];
      float cwidth = textWidth((char)unicode) + 2;
      if ( (x + cwidth) > (width - gapx) ) break;

      // Draw the glyph to the screen
      text(new String(Character.toChars(unicode)), x, y);

      // Move cursor
      x += cwidth;
      // Increment the counter
      index++;
      if (index >= count) break;
    }
    if (index >= count) break;
  }


  // creating font to save as a file
  PFont    font;

  font = createFont(fontName+fontType, fontSize, smooth, charset);

  println("Created font " + fontName + str(fontSize) + ".vlw");

  // creating file
  try {
    print("Saving to sketch FontFiles folder... ");

    OutputStream output = createOutput("FontFiles/" + fontName + str(fontSize) + ".vlw");
    font.save(output);
    output.close();

    println("OK!");

    delay(100);

    // Open up the FontFiles folder to access the saved file
    String path = sketchPath();
    Desktop.getDesktop().open(new File(path+"/FontFiles"));

    System.err.println("All done! Note: Rectangles are displayed for non-existant characters.");
  }
  catch(IOException e) {
    println("Doh! Failed to create the file");
  }
}
