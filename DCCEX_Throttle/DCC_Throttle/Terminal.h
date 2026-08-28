/*
    Terminal.h
    R.A.Lincoln       2026

    More of a log of traffic to/from the DCC-Ex CS
    
*/

#pragma once

#include "Screen.h"

class terminalClass {
    static const int HISTORY_ROWS = 50;                     // Deep history buffer size
    static const int DISPLAY_ROWS = 18;                     // Visible rows on screen (rows 1 to 18)


private:
    char    history[HISTORY_ROWS][COLS + 1] = {{0}};
    int     lastRow = 0;
    int     topRow = 0;                                     // The index currently at the top of the screen
    bool    isLatest = true;                                // True = locked to latest, False = inspecting history
    bool    isActive = false;


    //  Static and initial draw
    //
    void drawUI() {
        scr.fb(c64::Yellow, c64::Black);
        scr.cls("Terminal","");                             // Header/footer

        drawUIHistory();
    }

    //  The part that changes
    //
    void drawUIHistory() { 
        if (!isActive) return;                              // --> Can be called async while not on screen

        // Scroll arrows
        //
        int beforeTop = topRow == 0 ? HISTORY_ROWS : topRow -1;     // Row before top, including wrap
        bool canScrollUp = history[beforeTop][0] != '\0'
                        && beforeTop != lastRow;                    // Empty or back at the last row 
        bool canScrollDown = true;                                  // Posit can scroll down

        // Row entries
        //
        for (int i = 0; i < DISPLAY_ROWS; i++) {
            int y = i +1;                                   // Start at row 1
            int idx = (topRow + i) % HISTORY_ROWS;          // Simple rolling forward step

            if (idx == lastRow) {                           // Highlight the last entered row
                scr.inverse();
                scr.at(0, y, history[idx]);  
                scr.normal();          
                canScrollDown = false;                      // Admit, Last lastRow showing cant scroll down

            } else {
                scr.at(0, y, history[idx]);
            }
        }

        // Scroll arrows
        //
        if(canScrollUp)     scr.at(14, 1, "\xd0");           // Up
        if(canScrollDown)   scr.at(14, 18, "\xe0");          // Down
    }

    // Position topRow so that the newest entry (lastRow) lands at the very bottom of the screen
    //
    void toLatest() {
        isLatest = true;
        topRow = (lastRow - DISPLAY_ROWS +1) % HISTORY_ROWS;
        if (topRow < 0) topRow += HISTORY_ROWS;
    }

public:
    void init() {
        toLatest();                                           // Calculate topRow
    }

    void switchTo() {
        isActive = true;
        toLatest();                                           // Jump to end when entering
        drawUI();
    }

    void switchAway() {
        isActive = false;
    }


    // Turning encoder scrolls the history list
    //
    void handleEncoder(int step) {
        int oldTop = topRow;                                // save the current state 
        isLatest = false;                                   // Posit not following the latest any more

        //  Apply the step
        topRow = (topRow + step) % HISTORY_ROWS;            // Apply the step and wrap
        if (topRow < 0) topRow += HISTORY_ROWS;

        // Calculate distance from topRow to lastRow around the history ring
        int distance;
        if (lastRow >= topRow) {
            distance = lastRow - topRow;                    // Scenario 1: No wrap (topRow is behind or equal to lastRow)
        } else {
            distance = HISTORY_ROWS - (topRow - lastRow);   // Scenario 2: Wrapped (lastRow has wrapped around to the front, topRow is near the end)
        }

        //  Check scrolling down to within a screen of the last row
        if (distance < DISPLAY_ROWS -1) {
            toLatest();                                     // Lock onto the end
        }

        //  Check scrolling up past the top of the buffer or to empty rows
        if(distance >= HISTORY_ROWS -1
        || history[topRow][0] == '\0') {
            topRow = oldTop;                                // Reject the move
        }

        drawUIHistory();
    }


    //  Click snap back to latest view
    //
    bool handleEncoderButton() {
        toLatest();
        drawUIHistory();
        return false;                                       // Stay on screen
    }


    //  Log comms traffic (newest row at the bottom)
    //  Note - this can be called when the terminal screen is not active
    //
    void log(const char *text, bool sent) {                 // Explicit sent/received
        lastRow = (lastRow +1) % HISTORY_ROWS;              // Advance buffer pointer circularly

        char arrow = sent ? '\xbf' : '\xbe';
        snprintf(history[lastRow], sizeof(history[0]), "%c%-*.*s", arrow, COLS -1, COLS -1, text);

        if(isLatest) toLatest();                            // Move to the latest entry if following latest
        drawUIHistory();                                    // Update the changed part of the screen
    }

    void log(const char *text) {                            // No direction - internal logging call
        lastRow = (lastRow +1) % HISTORY_ROWS;              // Advance buffer pointer circularly

        snprintf(history[lastRow], sizeof(history[0]), " %-*.*s", COLS -1, COLS -1, text);

        if(isLatest) toLatest();                            // Move to the latest entry if following latest
        drawUIHistory();                                    // Update the changed part of the screen
    }

} inline Terminal;
