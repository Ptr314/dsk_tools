#ifndef BAS_TOKENS_H
#define BAS_TOKENS_H

#include <array>

namespace dsk_tools {

    constexpr std::array<const char*, 128> ABS_tokens = {
        "END", "FOR", "NEXT", "DATA", "INPUT", "DEL", "DIM", "READ", "GR", "TEXT",
        "PR#", "IN#", "CALL", "PLOT", "HLIN", "VLIN", "HGR2", "HGR", "HCOLOR=",
        "HPLOT", "DRAW", "XDRAW", "HTAB", "HOME", "ROT=", "SCALE=", "SHLOAD",
        "TRACE", "NOTRACE", "NORMAL", "INVERSE", "FLASH", "COLOR=", "POP", "VTAB",
        "HIMEM:", "LOMEM:", "ONERR", "RESUME", "RECALL", "STORE", "SPEED=", "LET",
        "GOTO", "RUN", "IF", "RESTORE", "&", "GOSUB", "RETURN", "REM", "STOP", "ON",
        "WAIT", "LOAD", "SAVE", "DEF", "POKE", "PRINT", "CONT", "LIST", "CLEAR",
        "GET", "NEW", "TAB(", "TO", "FN", "SPC(", "THEN", "AT", "NOT", "STEP", "+",
        "-", "*", "/", "^", "AND", "OR", ">", "=", "<", "SGN", "INT", "ABS", "USR",
        "FRE", "SCRN(", "PDL", "POS", "SQR", "RND", "LOG", "EXP", "COS", "SIN",
        "TAN", "ATN", "PEEK", "LEN", "STR$", "VAL", "ASC", "CHR$", "LEFT$",
        "RIGHT$", "MID$",
        "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
        "?", "?", "?", "?", "?", "?"
    };

    constexpr std::array<const char*, 128> Agat_tokens = {
        "END", "FOR", "NEXT", "DATA", "INPUT", "DEL", "DIM", "READ", "SCREEN 1, ", "SCREEN 0, ",
        "PR#", "IN#", "CALL", "LINE", "!", "&1", "SCREEN 1, ", "SCREEN 1, ", "COLOR", "LINE(",
        "DRAW", "XDRAW", "LOCATE ,", "CLS", "ROT=", "SCALE=", "REM SHLOAD", "TRACE", "NOTRACE", "COLOR 7,0",
        "COLOR 0,7", "COLOR 31,0", "COLOR", "POP", "LOCATE", "REM HIMEM:", "REM LOMEM:", "ON ERROR", "RESUME", "RECALL",
        "STORE", "REM SPEED=", "LET", "GOTO", "RUN", "IF", "RESTORE", "&3", "GOSUB", "RETURN",
        "REM", "STOP", "ON", "WAIT", "LOAD \"CON:", "SAVE \"CON:", "DEF", "REM POKE", "PRINT", "CONT",
        "LIST", "CLEAR", "INPUT$(1)", "NEW", "TAB(", "TO", "FN", "SPC(", "THEN", "AT",
        "NOT", "STEP", "+", "-", "*", "/", "^", "AND", "OR", ">",
        "=", "<", "SGN", "INT", "ABS", "USR", "FRE", "POINT(", "PDL", "POS",
        "SQR", "RND", "LOG", "EXP", "COS", "SIN", "TAN", "ATN", "PEEK", "LEN",
        "STR$", "VAL", "ASC", "CHR$", "LEFT$", "RIGHT$", "MID$", " NO FOR", "SYNTAX", "NO GOSUB",
        "NO DATA", "ILLEGAL VALUE", "OVERFLOW", "OUT OF MEMORY", "UNDEF STATEMENT", "SUBSCRIPT", "REDIM ARRAY", "DIVISION BY ZERO", "ILLEGAL DIRECT", "TYPE",
        "LONG STRING", "BIG EXPR", "CONTINUE", "UNDEF NAME", "BYTE UNCOMPL", "LABEL", "OPCODE", "DOUBLE DEF NAME"
    };

    constexpr std::array<const char*, 128> MBASIC_main_tokens = {
        "", "END", "FOR", "NEXT", "DATA", "INPUT", "DIM", "READ", "LET", "GOTO",
        "RUN", "IF", "RESTORE", "GOSUB", "RETURN", "REM", "STOP", "PRINT", "CLEAR", "LIST",
        "NEW", "ON", "DEF", "POKE", "CONT", "", "", "LPRINT", "LLIST", "WIDTH",
        "ELSE", "TRACE", "NOTRACE", "SWAP", "ERASE", "EDIT", "ERROR", "RESUME", "DEL", "AUTO",
        "RENUM", "DEFSTR", "DEFINT", "DEFSNG", "DEFDBL", "LINE", "POP", "WHILE", "WEND", "CALL",
        "WRITE", "COMMON", "CHAIN", "OPTION", "RANDOMIZE", "SYSTEM", "OPEN", "FIELD", "GET", "PUT",
        "CLOSE", "LOAD", "MERGE", "FILES", "NAME", "KILL", "LSET", "RSET", "SAVE", "RESET",
        "TEXT", "HOME", "VTAB", "HTAB", "INVERSE", "NORMAL", "GR", "COLOR", "HLIN", "VLIN",
        "PLOT", "HGR", "HPLOT", "HCOLOR", "BEEP", "WAIT",  "", "", "", "",
        "", "", "", "TO", "THEN", "TAB(", "STEP", "USR", "FN", "SPC(",
        "NOT", "ERL", "ERR", "STRING$", "USING", "INSTR", "'", "VARPTR", "SCRN", "HSCRN",
        "INKEY$", ">", "=", "<", "+", "-", "*", "/", "^", "AND",
        "OR", "XOR", "EQV", "IMP", "MOD", "<FD>", "<FE>", "<FF>"
    };
    constexpr std::array<const char*, 54> MBASIC_extended_tokens = {
        "", "LEFT$", "RIGHT$", "MID$", "SGN", "INT", "", "SQR", "RND", "SIN",
        "LOG", "EXP", "COS", "TAN", "ATN", "FRE", "POS", "LEN", "STR$", "VAL",
        "ASC", "CHR$", "PEEK", "SPACE$", "OCT$", "HEX$", "LPOS", "CINT", "CSNG", "CDBL",
        "FIX", "", "", "", "", "", "", "", "", "",
        "", "", "CVI", "CVS", "CVD", "EOF", "LOC", "LOF", "MKI$", "MKS$",
        "MKD$", "VPOS", "PDL", "BUTTON"
    };

}

#endif // BAS_TOKENS_H
