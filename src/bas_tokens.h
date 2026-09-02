// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Numeric tokens for different tokenized BASIC files
#pragma once


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
        "END", 		"FOR", 		"NEXT", 	"DATA", "INPUT",	"DEL", 		"DIM", 		"READ", 	"GR=", 		"TEXT=",
        "PR#", 		"IN#", 		"CALL", 	"PLOT", "!", 		"&", 		"MGR=", 	"HGR=", 	"RIBBON=", 	"&",
        "DRAW", 	"XDRAW", 	"HTAB", 	"HOME", "ROT=", 	"SCALE=",	"SHLOAD", 	"TRACE", 	"NOTRACE",	"NORMAL",
        "INVERSE", 	"FLASH", 	"COLOR=",	"POP", 	"VTAB", 	"HIMEM:", 	"LOMEM:", 	"ONERR",	"RESUME", 	"RECALL",
        "STORE", 	"SPEED=",	"LET", 		"GOTO",	"RUN", 		"IF", 		"RESTORE",	"&", 		"GOSUB", 	"RETURN",
        "REM", 		"STOP", 	"ON", 		"WAIT", "LOAD", 	"SAVE", 	"DEF", 		"POKE", 	"PRINT", 	"CONT",
        "LIST", 	"CLEAR", 	"GET", 		"NEW", 	"TAB(", 	"TO", 		"FN", 		"SPC(", 	"THEN", 	"AT",
        "NOT", 		"STEP", 	"+", 		"-", 	"*", 		"/", 		"^", 		"AND", 		"OR", 		">",
        "=", 		"<", 		"SGN", 		"INT", 	"ABS", 		"USR", 		"FRE", 		"SCRN(", 	"PDL", 		"POS",
        "SQR", 		"RND", 		"LOG", 		"EXP", 	"COS", 		"SIN", 		"TAN", 		"ATN", 		"PEEK", 	"LEN",
        "STR¤", 	"VAL", 		"ASC", 		"CHR¤", "LEFT¤", 	"RIGHT¤", 	"MID¤", 	"NO FOR", 	"SYNTAX", 	"NO GOSUB",
        "NO DATA",		"ILLEGAL VALUE",	"OVERFLOW",	"OUT OF MEMORY",	"UNDEF STATEMENT",	"SUBSCRIPT",	"REDIM ARRAY",	"DIVISION BY ZERO", "ILLEGAL DIRECT", "TYPE",
        "LONG STRING",	"BIG EXPR", 		"CONTINUE",	"UNDEF NAME", 		"BYTE UNCOMPL",		"LABEL", 		"OPCODE", 		"DOUBLE DEF NAME"
    };

    constexpr std::array<const char*, 128> Vector_tokens = {
        "CLS","FOR","NEXT","DATA","INPUT","DIM","READ","CUR","GOTO",
        "RUN","IF","RESTORE","GOSUB","RETURN","REM","STOP","OUT","ON",
        "PLOT","LINE","POKE","PRINT","DEF","CONT","LIST","CLEAR",
        "CLOAD","CSAVE","NEW","TAB(","TO","SPC(","FN","THEN","NOT",
        "STEP","+","-","*","/","^","AND","OR",">","=","<","SGN","INT",
        "ABS","USR","FRE","INP","POS","SQR","RND","LOG","EXP","COS",
        "SIN","TAN","ATN","PEEK","LEN","STR$","VAL","ASC","CHR$",
        "LEFT$","RIGHT$","MID$","POINT","INKEY$","AT","&","BEEP",
        "PAUSE","VERIFY","HOME","EDIT","DELETE","MERGE","AUTO","HIMEM",
        "@","ASN","ADDR","PI","RENUM","ACS","LG","LPRINT","LLIST",
        "SCREEN","COLOR","GET","PUT","BSAVE","BLOAD","PLAY","PAINT",
        "CIRCLE"
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


    // Acorn BASIC II, indexed by token - $80. Verified against the keyword table of the
    // Onix 1.20 system area, which carries the standard set unchanged. $8D is not a
    // keyword but the marker of an encoded line number and $CE is unused, hence nullptr.
    constexpr std::array<const char*, 128> BBC_tokens = {
        "AND", "DIV", "EOR", "MOD", "OR", "ERROR", "LINE", "OFF",
        "STEP", "SPC", "TAB(", "ELSE", "THEN", nullptr, "OPENIN", "PTR",
        "PAGE", "TIME", "LOMEM", "HIMEM", "ABS", "ACS", "ADVAL", "ASC",
        "ASN", "ATN", "BGET", "COS", "COUNT", "DEG", "ERL", "ERR",
        "EVAL", "EXP", "EXT", "FALSE", "FN", "GET", "INKEY", "INSTR(",
        "INT", "LEN", "LN", "LOG", "NOT", "OPENUP", "OPENOUT", "PI",
        "POINT(", "POS", "RAD", "RND", "SGN", "SIN", "SQR", "TAN",
        "TO", "TRUE", "USR", "VAL", "VPOS", "CHR$", "GET$", "INKEY$",
        "LEFT$(", "MID$(", "RIGHT$(", "STR$", "STRING$(", "EOF", "AUTO", "DELETE",
        "LOAD", "LIST", "NEW", "OLD", "RENUMBER", "SAVE", nullptr, "PTR",
        "PAGE", "TIME", "LOMEM", "HIMEM", "SOUND", "BPUT", "CALL", "CHAIN",
        "CLEAR", "CLOSE", "CLG", "CLS", "DATA", "DEF", "DIM", "DRAW",
        "END", "ENDPROC", "ENVELOPE", "FOR", "GOSUB", "GOTO", "GCOL", "IF",
        "INPUT", "LET", "LOCAL", "MODE", "MOVE", "NEXT", "ON", "VDU",
        "PLOT", "PRINT", "PROC", "READ", "REM", "REPEAT", "REPORT", "RESTORE",
        "RETURN", "RUN", "STOP", "COLOUR", "TRACE", "UNTIL", "WIDTH", "OSCLI"
    };
}
