#include "lgfx/v1/misc/SpriteBuffer.hpp"
/*
    Screen.h
    R.A.Lincoln   2026

    Layer to make the display reminiscent of oldschool text-matrix, Pet or C64 ish

    scr.fg          Set foreground colour (rgb565)
    scr.bg          Set background colour
    scr.fb          Set foreground and background colour

    scr.cls         Clear screen to background or a colour
    scr.at          Text or {char list} at col,row
    scr.at4x4       4x4 text made from block glyphs


    255 possible chars from Petscii/Ascii style 8x8 pixel glyphs
    Foreground and background colours selectable on each char

    Row sized sprite used for at commands,
    4x4 sized sprite used for 4x4

    To convert: https://petscii.krissz.hu
    xxd -i font.64c > font.h
*/

#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

#define MAX(a, b) ((a) > (b) ? (a) : (b))


//  Pins
#define TFT_SCLK    10
#define TFT_MOSI    9
#define TFT_MISO    -1
#define TFT_RST     8
#define TFT_DC      7
#define TFT_CS      6
#define TFT_BLK     5


// Configuration for a 15x20 grid using 16x16 double-size characters on a 240x320 portrait screen
#define PANEL_W     240
#define PANEL_H     320
#define CHAR_W      16
#define CHAR_H      16
#define COLS        (PANEL_W / CHAR_W)  // 15
#define ROWS        (PANEL_H / CHAR_H)  // 20


//  C64 colours
//  RGB 565 format
enum class c64 : uint32_t {
  Black       = 0x0000,
  White       = 0xFFFF,
  Red         = 0x8900,
  Cyan        = 0x6E95,
  Purple      = 0xA9D4,
  Green       = 0x55C3,
  Blue        = 0x1892,
  Yellow      = 0xF74B,
  Orange      = 0xD300,  // Brightened from 0xA240
  Brown       = 0x69C0,  // Elevated from 0x4143
  Light_red   = 0xCBCE,
  Dark_grey   = 0x4A49,
  Grey        = 0x8410,
  Light_green = 0x9FF3,
  Light_blue  = 0x549A,
  Light_grey  = 0xBDD7 
};

#define SCANLINE_COLOUR 0x1082


//  Glyphs
#include "petscii_16x16_font.h"         // Generated with scalefont.py from https://petscii.krissz.hu .64c export
#include "petscii_chunky.h"             //  Chunky matrix lookups
#include "c64_balloon.h"                //  C64 user guide balloon sprite


// --- LOVYANGFX HARDWARE DEFINITION ---
//
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

public:
  LGFX() {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;     // FSPI on ESP32-C3
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;      // 40 MHz SPI clock
      cfg.freq_read   = 16000000;
      cfg.pin_sclk    = TFT_SCLK;
      cfg.pin_mosi    = TFT_MOSI;
      cfg.pin_miso    = TFT_MISO;
      cfg.pin_dc      = TFT_DC;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST;
      cfg.pin_busy         = -1;
      cfg.panel_width      = PANEL_W;
      cfg.panel_height     = PANEL_H;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      _panel_instance.config(cfg);
    }

    {
      auto l_cfg = _light_instance.config();
      l_cfg.pin_bl = TFT_BLK;          // Backlight
      _light_instance.config(l_cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
} inline display;


//  Mimic operating on a text display  
//
class Screen {
private:

    // Single shared buffer large enough for a maximum-length row (COLS characters wide)
    //  and the chunky sprites
    uint16_t _spriteBuffer[MAX(COLS * CHAR_W * CHAR_H, 
                               4*4 * CHAR_W * CHAR_H)];
    
    LGFX_Sprite* _rowSprite[COLS + 1];              // Index 0 unused, 1 to COLS for character lengths
    LGFX_Sprite* _chunkySprite[4];                  // 2x3, 2x4, 3x4, 4x4 chars from petscii blocks  


    //  Scanlines on/off
    bool enableScanlines = true;


    // Last used colurs
    c64 currentFg = c64::Light_blue;            
    c64 currentBg = c64::Blue;            
    

    // Draw a char into a sprite
    //
    void drawChar(LGFX_Sprite* sprite, char c, int x, int y, c64 fg, c64 bg) {
        const uint8_t* glyph = &petscii_16x16[(unsigned char)c *32];                // the glyph
        sprite->drawBitmap(x, y, glyph, CHAR_W, CHAR_H, (uint16_t) fg, (uint16_t) bg);
    }

    // Dim every other line to simulate scanlines
    // Support sprite and display objects    
    template <typename T>
    void drawScanlines(T* gfx) {
        if(!enableScanlines) return;                                                // -->
        
        for(int i = 1; i < gfx->height(); i += 2) {
            gfx->drawFastHLine(0, i, gfx->width(), (uint16_t)SCANLINE_COLOUR);
        }
    }

  
    // Renders a 16x16 glyph into a 4x4 grid of PETSCII block characters (0xF0-0xFF)
    // Read the pixels and replace with the quarter blocks
    //
    void generate4x4(unsigned char c, uint8_t chunky[16]) {

        for(int ci=0; ci<16; ci++) chunky[ci] = 0xf0;                       // base block char 0xf0-0xff

        /*
        glyph   b0  b1          b0>c0/1 b1>c2/3
                b2  b3          ignore
                b4  b5          b4>c0/1 b5>c2/3
                b6  b7          ignore
                etc...

        chunky  c0  c1  c2  c3  
                c4  c5  c6  c7
                c8  c9  c10 c11
                c12 c13 c14 c15

        */
        uint16_t* glyph = (uint16_t*)&petscii_16x16[c *32];             // find the base glyph to scale up
        int ci = 0;                                                     // chunky index
        for(int r = 0; r < CHAR_H; r += 4) {                            // iterate glyph rows
            uint16_t l = glyph[r];                                      // top row little endian, low byte = left of char
            if(l & 0b1100000000000000) chunky[ci +2] |= 1;    
            if(l & 0b0011000000000000) chunky[ci +2] |= 2;          
            if(l & 0b0000110000000000) chunky[ci +3] |= 1;       
            if(l & 0b0000001100000000) chunky[ci +3] |= 2;      

            if(l & 0b0000000011000000) chunky[ci +0] |= 1;       
            if(l & 0b0000000000110000) chunky[ci +0] |= 2;          
            if(l & 0b0000000000001100) chunky[ci +1] |= 1;  
            if(l & 0b0000000000000011) chunky[ci +1] |= 2;

            l = glyph[r +2];                                            // bottom row
            if(l & 0b1100000000000000) chunky[ci +2] |= 4;    
            if(l & 0b0011000000000000) chunky[ci +2] |= 8;          
            if(l & 0b0000110000000000) chunky[ci +3] |= 4;       
            if(l & 0b0000001100000000) chunky[ci +3] |= 8;      

            if(l & 0b0000000011000000) chunky[ci +0] |= 4;       
            if(l & 0b0000000000110000) chunky[ci +0] |= 8;          
            if(l & 0b0000000000001100) chunky[ci +1] |= 4;  
            if(l & 0b0000000000000011) chunky[ci +1] |= 8;

            ci += 4;
        }
    }


public:
    //  Initialise
    //
    void init() {
        //  Hardware setup
        display.init();
        display.setRotation(0);

        //  One sprite per length of string output
        _rowSprite[0] = nullptr;
        for (int i = 1; i <= COLS; i++) {
            _rowSprite[i] = new LGFX_Sprite(&display);
            _rowSprite[i]->setColorDepth(16);
            _rowSprite[i]->setBuffer(_spriteBuffer, i * CHAR_W, CHAR_H);
        }

        // Chunky sprites to build out large numbers 2x3, 2x4 & 4x4
        _chunkySprite[0] = new LGFX_Sprite(&display);               // 2x3
        _chunkySprite[0]->setColorDepth(16);
        _chunkySprite[0]->setBuffer(_spriteBuffer, CHAR_W *2, CHAR_H *3);

        _chunkySprite[1] = new LGFX_Sprite(&display);               // 2x4
        _chunkySprite[1]->setColorDepth(16);
        _chunkySprite[1]->setBuffer(_spriteBuffer, CHAR_W *2, CHAR_H *4);

        _chunkySprite[2] = new LGFX_Sprite(&display);               // 3x4
        _chunkySprite[2]->setColorDepth(16);
        _chunkySprite[2]->setBuffer(_spriteBuffer, CHAR_W *3, CHAR_H *4);

        _chunkySprite[3] = new LGFX_Sprite(&display);               // 4x4
        _chunkySprite[3]->setColorDepth(16);
        _chunkySprite[3]->setBuffer(_spriteBuffer, CHAR_W *4, CHAR_H *4);
    }


    //  Clear screen, optionally to a colour
    //
    void cls() {
      display.fillScreen((uint16_t)currentBg);
      drawScanlines(&display);
    }

    void cls(c64 bg) {
        currentBg = bg;
        cls();
    }

    void cls(c64 fg, c64 bg) {
        currentFg = fg;
        currentBg = bg;
        cls();
    }

    //  Standard layout - reserve the top and bottom lines
    //  Bottom line for any message will get overwritten by the next caller
    //
    void cls(char* header) { 
        cls(header, "");
    }

    void cls(char* header, char* footer) {
        char buffer[COLS +1];

        cls(currentFg, currentBg);        
        snprintf(buffer, sizeof(buffer), "%-*.*s", COLS, COLS, header);
        at(0, 0, buffer, currentBg, currentFg);             // Reverse header

        status(footer);
    };

    void status(char* footer) {
        char buffer[COLS +1];

        if (footer == nullptr || footer[0] == '\0') {
            at(0, 19, "...HELLORLD!..", currentBg, currentFg);
        } else {
            snprintf(buffer, sizeof(buffer), "%-*.*s", COLS, COLS, footer);
            at(0, 19, buffer, currentBg, currentFg);        // Reverse footer
        }
        at(14, 19, "\x8e", c64::Black, currentFg);          // Easter - Sandra Bullock The Net Pi
    }


    //  Enable/disable scanlines
    //  Follow with cls to update the screen
    //
    void lines(bool lines) {
        enableScanlines = lines;
    }

    //  Set colours for anything that doesn't specify
    //
    c64 fg() {
        return currentFg;
    }

    void fg(c64 colour) {
        currentFg = colour;
    }

    c64 bg() {
        return currentBg;
    }

    void bg(c64 colour) {
        currentBg = colour;
    }

    void fb(c64 fg, c64 bg) {
      currentFg = fg;
      currentBg = bg;      
    }

    //  Swap foreground/background
    //
    void reverse() {
        c64 oldFg = currentFg;
        currentFg = currentBg;
        currentBg = oldFg;
    }


    // Inline list of byte/char values on the fly (e.g., printat(x, y, {0xE0, 0xE1, 0x00, 'A'}))
    //  Optional colours
    //
    inline void at(int col, int row, std::initializer_list<uint8_t> codes) {
        at(col, row, codes, currentFg, currentBg);
    }

    inline void at(int col, int row, std::initializer_list<uint8_t> codes, c64 fg) {
        at(col, row, codes, fg, currentBg);
    }

    inline void at(int col, int row, std::initializer_list<uint8_t> codes, c64 fg, c64 bg) {
        int len = codes.size();
        if (len == 0) return;
        if (col + len > COLS) len = COLS - col; 

        int currentX = 0;
        for (uint8_t code : codes) {
            drawChar(_rowSprite[len], code, currentX, 0, fg, bg);       // To the sprite
            currentX += CHAR_W;
        }

        drawScanlines(_rowSprite[len]);                                 // Scanlines overlay

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _rowSprite[len]->pushSprite(screenX, screenY);                  // Sprite to the screen
    }


    //  Text string
    //  Optional colours
    //
    void at(int col, int row, const char* text) {
        at(col, row, text, currentFg, currentBg);
    }

    void at(int col, int row, const char* text, c64 fg) {
        at(col, row, text, fg, currentBg);
    }

    void at(int col, int row, const char* text, c64 fg, c64 bg) {
        int len = strlen(text);
        if (len == 0) return;
        if (col + len > COLS) len = COLS - col; 

        for (int i = 0; i < len; i++) {
            int charX = i * CHAR_W; 
            drawChar(_rowSprite[len], text[i], charX, 0, fg, bg);           // To the sprite
        }
      
        drawScanlines(_rowSprite[len]);                                     // Scanlines overlay

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _rowSprite[len]->pushSprite(screenX, screenY);                      // Sprite to the screen
    }


    //  4x4 chunky char
    //
    void at4x4(int col, int row, char c) {                              // use current colours
        at4x4(col, row, c, currentFg, currentBg);
    }

    void at4x4(int col, int row, char c, c64 fg) {                      // specify fg use current bg
        at4x4(col, row, c, fg, currentBg);
    }

    void at4x4(int col, int row, char c, c64 fg, c64 bg) {
        uint8_t chunky[16];
        generate4x4(c, chunky);                                        // convert 16x16 to 4x4 $f0-$ff chars

        int i = 0;                                                      // draw into the 4x4 sprite
        for(int y=0; y<4; y++) {
            for(int x=0; x<4; x++) {
                drawChar(_chunkySprite[3], chunky[i++], x *CHAR_W, y *CHAR_H, fg, bg);
            }
        }

        drawScanlines(_chunkySprite[3]);

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _chunkySprite[3]->pushSprite(screenX, screenY);                 // Sprite to the screen
    }


    //  3x4 chunky char
    //  0-9 & space
    //
    void at3x4(int col, int row, int n) {
        if(n <0 or n>9) return;                                         // -->

        at3x4(col, row, (char)('0' + n), currentFg, currentBg);
    }

    void at3x4(int col, int row, char c, c64 fg, c64 bg) {
        if((c<'0' || c>'9') && c != ' ') return;                        // -->

        if(c == ' ') c = ':';                                           // space encoded after 9
        int i = (c -'0') *12;

        for(int y=0; y<4; y++) {
            for(int x=0; x<3; x++) {
                drawChar(_chunkySprite[2], petscii_3x4[i++], x *CHAR_W, y *CHAR_H, fg, bg);
            }
        }

        drawScanlines(_chunkySprite[2]);

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _chunkySprite[2]->pushSprite(screenX, screenY);                 // Sprite to the screen
    }


    //  2x4 chunky char
    //  0-9 & space
    //
    void at2x4(int col, int row, int n) {
        if(n <0 or n>9) return;                                         // -->

        at2x4(col, row, (char)('0' + n), currentFg, currentBg);
    }

    void at2x4(int col, int row, char c, c64 fg, c64 bg) {
        if((c<'0' || c>'9') && c != ' ') return;                        // -->

        if(c == ' ') c = ':';                                           // space encoded after 9
        int i = (c -'0') *8;

        for(int y=0; y<4; y++) {
            for(int x=0; x<2; x++) {
                drawChar(_chunkySprite[1], petscii_2x4[i++], x *CHAR_W, y *CHAR_H, fg, bg);
            }
        }

        drawScanlines(_chunkySprite[1]);

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _chunkySprite[1]->pushSprite(screenX, screenY);                 // Sprite to the screen
    }


    //  2x3 chunky char
    //  0-9 & space
    //
    void at2x3(int col, int row, int n) {
        if(n <0 or n >9) return;                                        // -->

        at2x3(col, row, (char)('0' + n));
    }

    void at2x3(int col, int row, char c) {
        if((c<'0' || c>'9') && c != ' ') return;                        // -->

        if(c == ' ') c = ':';                                           // space encoded after 9
        int i = (c -'0') *6;

        for(int y=0; y<4; y++) {
            for(int x=0; x<2; x++) {
                drawChar(_chunkySprite[0], petscii_2x3[i++], x *CHAR_W, y *CHAR_H, currentFg, currentBg);
            }
        }

        drawScanlines(_chunkySprite[0]);

        int screenX = col * CHAR_W;
        int screenY = row * CHAR_H;      
        _chunkySprite[0]->pushSprite(screenX, screenY);                // Sprite to the screen
    }

} inline scr;
