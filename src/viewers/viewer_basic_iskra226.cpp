// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025-2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the dsk_tools project: https://github.com/Ptr314/dsk_tools
// Description: Viewer for Iskra-226 BASIC


#include "viewer_basic.h"
#include "viewer_basic_iskra226.h"

#include <cstdint>
#include <string>

#include "utils.h"

// The detokenizer itself comes from the Iskra-226 emulator, where the same code
// serves as the LIST of its interactive mode: https://github.com/Ptr314/Iskra-226
// Do not edit src/viewers/iskra226/, re-extract it with tools/detok_min.py there.
#include "viewers/iskra226/detokenize.h"

namespace dsk_tools {

    namespace {

        // The detokenizer returns a flat line of KOI-8 text while the viewer
        // paints it by CSS classes, so the classes are recovered from the text.
        //
        // The rules below are deliberately crude, and they are allowed to be:
        // a span only wraps characters, it never changes them. Names invented
        // by the detokenizer are a single letter with an optional digit ("A",
        // "A0"), and no keyword is that short, which is what tells the two
        // apart. Single letter keywords used as flags (SELECT D, ADD C) do get
        // painted as variables.

        bool is_letter(const uint8_t c) { return c >= 'A' && c <= 'Z'; }
        bool is_digit(const uint8_t c)  { return c >= '0' && c <= '9'; }

        bool is_operator(const uint8_t c)
        {
            return c=='+' || c=='-' || c=='*' || c=='/' || c=='^'
                || c=='=' || c=='<' || c=='>';
        }

        uint8_t at(const std::string & s, const unsigned i)
        {
            return static_cast<uint8_t>(s[i]);
        }

        std::string to_text(const std::string & koi8, const unsigned from, const unsigned to, const CharmapInfo & cm)
        {
            std::string out;
            for (unsigned i = from; i < to; i++) {
                const uint8_t c = at(koi8, i);
                // The machine has no dollar sign: code 24 is the currency one.
                // The detokenizer writes it as '$', both as the string variable
                // suffix and in the verbs spelled with it, like $GIO.
                if (c == 0x24) out += "¤";
                else out += (*cm.charmap)[c];
            }
            return out;
        }

        std::string span(const EntityType t, const std::string & text, const bool nbsp = false)
        {
            if (t == EntityType::NONE) return escapeHtml(text, nbsp);
            return "<span class=\"" + entityTypeToString(t) + "\">" + escapeHtml(text, nbsp) + "</span>";
        }

        // A number: the leading digits, then an optional fraction and exponent.
        unsigned number_end(const std::string & koi8, unsigned p)
        {
            const unsigned n = koi8.size();
            while (p < n && is_digit(at(koi8, p))) p++;
            if (p < n && koi8[p] == '.') {
                p++;
                while (p < n && is_digit(at(koi8, p))) p++;
            }
            if (p + 1 < n && koi8[p] == 'E') {
                unsigned e = p + 1;
                if (koi8[e] == '+' || koi8[e] == '-') e++;
                if (e < n && is_digit(at(koi8, e))) {
                    while (e < n && is_digit(at(koi8, e))) e++;
                    p = e;
                }
            }
            return p;
        }

        std::string highlight(const std::string & koi8, const CharmapInfo & cm)
        {
            const unsigned n = koi8.size();
            std::string out;
            unsigned p = 0;

            // The line number opens the line and is the only place where digits
            // are not a part of an expression.
            while (p < n && is_digit(at(koi8, p))) p++;
            if (p > 0) out += span(EntityType::LINE_NUMBER, to_text(koi8, 0, p, cm), true);
            const unsigned first = p;

            while (p < n) {
                const uint8_t c = at(koi8, p);

                // '%' is the short REM, but only where a statement begins:
                // anywhere else it is the integer suffix of a name.
                if (c == '%') {
                    unsigned q = p;
                    while (q > first && koi8[q-1] == ' ') q--;
                    if (q == first || koi8[q-1] == ':') {
                        out += span(EntityType::TOKEN, "%");
                        p++;
                        // An empty comment is followed by the next statement,
                        // as in EDITOR 9600: "%:SAVE S¤()2915,2940".
                        if (p < n && koi8[p] != ':') {
                            out += span(EntityType::REM, to_text(koi8, p, n, cm));
                            return out;
                        }
                        continue;
                    }
                }

                if (c == '"') {                                 // quotes included
                    unsigned e = p + 1;
                    while (e < n && koi8[e] != '"') e++;
                    if (e < n) e++;
                    out += span(EntityType::STRING, to_text(koi8, p, e, cm));
                    p = e;
                    continue;
                }

                // A word: a verb, a function or a variable. '$' opens the verbs
                // spelled with the currency sign.
                if (is_letter(c) || (c == '$' && p + 1 < n && is_letter(at(koi8, p+1)))) {
                    const bool currency = (c == '$');
                    unsigned e = p + (currency ? 1 : 0);
                    while (e < n && is_letter(at(koi8, e))) e++;

                    if (currency || e - p > 1) {                 // a keyword
                        const std::string word = koi8.substr(p, e - p);
                        out += span(EntityType::TOKEN, to_text(koi8, p, e, cm));
                        p = e;

                        // The comment runs to the end of the line as it is.
                        // Its short form, '%', is handled above.
                        if (word == "REM") {
                            out += span(EntityType::REM, to_text(koi8, p, n, cm));
                            return out;
                        }
                        // HEX( holds digits and letters alike, and all of them
                        // are a number, not names.
                        if (word == "HEX" && p < n && koi8[p] == '(') {
                            unsigned h = p + 1;
                            while (h < n && koi8[h] != ')') h++;
                            out += span(EntityType::NONE, "(");
                            out += span(EntityType::NUMBER, to_text(koi8, p + 1, h, cm));
                            p = h;
                        }
                        continue;
                    }

                    // A name: one letter, one digit, then the type suffix.
                    if (e < n && is_digit(at(koi8, e))) e++;
                    if (e < n && (koi8[e] == '$' || koi8[e] == '%')) e++;
                    out += span(EntityType::VAR, to_text(koi8, p, e, cm));
                    p = e;
                    continue;
                }

                if (is_digit(c)) {
                    const unsigned e = number_end(koi8, p);
                    out += span(EntityType::NUMBER, to_text(koi8, p, e, cm));
                    p = e;
                    continue;
                }

                if (is_operator(c)) {
                    unsigned e = p;
                    while (e < n && is_operator(at(koi8, e))) e++;
                    out += span(EntityType::TOKEN, to_text(koi8, p, e, cm));
                    p = e;
                    continue;
                }

                out += span(EntityType::NONE, to_text(koi8, p, p + 1, cm));
                p++;
            }

            return out;
        }

    } // namespace

    std::string ViewerBASIC_Iskra226::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        std::string out;
        cm = init_charmap(cm_name);

        if (data.size() < 256*2) return "NOT A BASIC";
        const uint8_t attr = data[9];

        if (data[0] != 1 || !(attr==0x20 || attr==0x21 || attr==0x24 || attr==0x25)) return "NOT A BASIC";

        if ((attr & 1) == 0) {
            // out += "TEXT BASIC\n\n";
            // The listing is stored as it was typed, lines separated by 85, and
            // it is painted by the same rules as the detokenized one: names of
            // the machine are as short as the invented ones.
            std::string line;
            const unsigned char * part = &data[256];
            const unsigned char * end = data.data() + data.size();
            while (part < end) {
                const uint8_t b1 = part[0];
                const uint8_t b2 = part[1];
                if (b2==0x80 && (b1==0x02 || b1==0x03 || b1==0x8F)) {
                    // We take this part as a code
                    // 02 80 usually is the first part, 8F 80 middle and 03 80 the last
                    // We should skip the first two bytes and trailing zeroes
                    constexpr unsigned from_byte = 2;
                    unsigned to_byte = 255;
                    while (to_byte>from_byte && part[to_byte]==0) to_byte--;
                    for (unsigned i=from_byte; i<=to_byte; i++) {
                        if (part[i]==0x85) {
                            out += highlight(line, cm);
                            out += '\n';
                            line.clear();
                        } else {
                            line += static_cast<char>(part[i]);
                        }
                    }
                    part += 256;
                    continue;
                }
                // A closing part can appear not only at the end, so we check for it too
                // It happens when a shorter file was written over a longer
                if (b1 == 0x1C) break;
                if (!line.empty()) { out += highlight(line, cm); line.clear(); }
                out += "\nUNKNOWN BLOCK SIGNATURE AT " + int_to_hex(part-data.data()) + '\n';
                part += 256;
            }
            // The last line of a listing has no 85 after it.
            if (!line.empty()) out += highlight(line, cm);
        } else {
            // out += "TOKENIZED BASIC\n\n";

            // Sector assembly, the variable tables and the listing itself are
            // all done by the emulator's code, so that the two always agree.
            iskra::ProgramImage img;
            std::string error;
            if (!img.load_file(data, error)) return "NOT A BASIC: " + error;

            // Variable names are not kept in the stream, only their indices, so
            // the detokenizer invents them. The program is walked once to fill
            // the table, and every line is then rendered against it.
            iskra::NameTable names;
            std::string whole;
            iskra::detokenize(img, names, whole, error);

            out += '\n';

            for (unsigned i = 0; i < img.line_count(); i++) {
                std::string koi8;
                std::string err;
                if (iskra::detokenize_line(img.line(i), names, koi8, err)) {
                    out += highlight(koi8, cm);
                } else {
                    // A line the detokenizer cannot read must not eat the whole
                    // listing: its reason is shown in place of that line.
                    out += span(EntityType::LINE_NUMBER, std::to_string(img.line(i).number), true);
                    // The reason names verbs the way the listing does, with the
                    // currency sign rather than a dollar the machine has not got.
                    for (std::size_t k = 0; k < err.size(); k++)
                        if (err[k] == '$') err.replace(k, 1, "¤");
                    out += span(EntityType::NONE, " ??? " + err);
                }
                out += '\n';
            }
        }


        // return escapeHtml(out);
        return out;
    }

}
