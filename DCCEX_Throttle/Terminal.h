/*
    Terminal.h
    R.A.Lincoln       2026

    Dcc-ex terminal
    
*/

class terminalClass {

private:

    void drawUI() {
        scr.fb(c64::Yellow, c64::Black);
        scr.cls("Terminal");                                 // Header/footer
    }

public:

    void init() {

    }

    void switchTo() {
        drawUI();
    }

    void handleEncoder(int step) {

    }

    bool handleEncoderButton() {

    return false;                                       // --> stay on this screen
    }

} inline Terminal;
