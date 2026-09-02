// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the DISK Commander project: https://github.com/Ptr314/dsk_commander
// Description: Viewer for tokenized BBC BASIC, as used by the Onix OS of the Agat

#include <algorithm>

#include "viewer_basic_bbc.h"
#include "bas_tokens.h"
#include "utils.h"

namespace dsk_tools {

    namespace {

        // A program is a run of records, each one <CR><line hi><line lo><record length>
        // followed by the tokenized text, and it ends with <CR><FF>
        constexpr uint8_t BBC_LINE_START   = 0x0D;
        constexpr uint8_t BBC_PROGRAM_END  = 0xFF;
        constexpr unsigned BBC_HEADER_LEN  = 4;

        constexpr uint8_t BBC_TOKEN_LINE   = 0x8D;      // an encoded line number follows
        constexpr uint8_t BBC_TOKEN_REM    = 0xF4;

        // GOTO 100 keeps its target as three bytes so that RENUMBER can find it without
        // parsing the line. The top two bits of each half are packed into the first byte.
        uint16_t decode_line_number(const uint8_t * p)
        {
            // The tokenizer folds the top two bits of each half into the first byte
            // and flips the result with $54 so that it can never look like a token
            const unsigned packed = p[0] ^ 0x54;
            const unsigned lo = (p[1] & 0x3F) | ((packed << 2) & 0xC0);
            const unsigned hi = (p[2] & 0x3F) | ((packed << 4) & 0xC0);
            return static_cast<uint16_t>((hi << 8) | lo);
        }

        bool is_digit(uint8_t c)
        {
            return c >= '0' && c <= '9';
        }

        bool is_hex_letter(uint8_t c)
        {
            return c >= 'A' && c <= 'F';
        }

        bool is_name_char(uint8_t c)
        {
            return is_digit(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '%' || c == '$';
        }

    }

    bool ViewerBASIC_BBC::fits(const BYTES & data, const std::string & file_name)
    {
        if (data.size() < 2) return false;

        size_t a = 0;
        int lines = 0;

        while (a + 1 < data.size()) {
            if (data[a] != BBC_LINE_START) return false;
            if (data[a + 1] == BBC_PROGRAM_END) return lines > 0;
            if (a + BBC_HEADER_LEN > data.size()) return false;
            const unsigned len = data[a + 3];
            if (len < BBC_HEADER_LEN) return false;
            a += len;
            lines++;
        }

        return false;
    }

    // Unlike the Applesoft dialects, BBC BASIC keeps the spacing of the source as it was
    // typed in, so the text only has to be classified, never laid out again.
    std::string ViewerBASIC_BBC::process_as_text(const BYTES & data, const std::string & cm_name)
    {
        cm = init_charmap(cm_name);

        std::string out;

        EntityType run_type = EntityType::NONE;
        std::string run_text;

        auto flush = [&]() {
            if (run_text.empty()) return;
            out += "<span class=\"" + entityTypeToString(run_type) + "\">" + escapeHtml(run_text) + "</span>";
            run_text.clear();
        };

        auto push = [&](EntityType type, const std::string & text) {
            if (type != run_type) {
                flush();
                run_type = type;
            }
            run_text += text;
        };

        size_t a = 0;

        while (a + 1 < data.size()) {
            if (data[a] != BBC_LINE_START) break;
            if (data[a + 1] == BBC_PROGRAM_END) break;              // the end of the program
            if (a + BBC_HEADER_LEN > data.size()) break;

            const unsigned line_num = (static_cast<unsigned>(data[a + 1]) << 8) | data[a + 2];
            const unsigned len = data[a + 3];
            if (len < BBC_HEADER_LEN) break;

            const size_t body = a + BBC_HEADER_LEN;
            const size_t end = std::min(a + len, data.size());

            out += "<div class=\"line\">";
            out += "<span class=\"" + entityTypeToString(EntityType::LINE_NUMBER) + "\">"
                 + escapeHtml(pad_number(static_cast<int>(line_num), 5), true) + "</span>";

            run_type = EntityType::NONE;
            run_text.clear();

            bool in_string = false;
            bool in_rem = false;
            uint8_t prev = 0;

            for (size_t p = body; p < end; p++) {
                const uint8_t c = data[p];

                if (in_rem) {
                    push(EntityType::REM, (*cm.charmap)[c]);
                    continue;
                }

                if (in_string) {
                    push(EntityType::STRING, (*cm.charmap)[c]);
                    if (c == '"') in_string = false;
                    prev = c;
                    continue;
                }

                if (c == BBC_TOKEN_LINE) {
                    // The three bytes that follow are the target of a GOTO or a GOSUB
                    if (p + 3 < end) {
                        push(EntityType::NUMBER, std::to_string(decode_line_number(&data[p + 1])));
                        p += 3;
                    } else {
                        push(EntityType::TOKEN, "?$" + int_to_hex(c) + "?");
                    }
                    prev = 0;
                    continue;
                }

                if (c >= 0x80) {
                    const char * token = BBC_tokens[c & 0x7F];
                    if (token) {
                        push(EntityType::TOKEN, std::string(token));
                        if (c == BBC_TOKEN_REM) in_rem = true;
                    } else {
                        push(EntityType::TOKEN, "?$" + int_to_hex(c) + "?");
                    }
                    prev = 0;
                    continue;
                }

                if (c == '"') {
                    push(EntityType::STRING, "\"");
                    in_string = true;
                    prev = c;
                    continue;
                }

                // A digit right after a letter belongs to a name, elsewhere it opens a
                // number; &1F00 and 1.5 then carry on the number that is already open
                if (c == '&' || (is_digit(c) && !is_name_char(prev)))
                    push(EntityType::NUMBER, (*cm.charmap)[c]);
                else
                if (run_type == EntityType::NUMBER && (is_digit(c) || is_hex_letter(c) || c == '.'))
                    push(EntityType::NUMBER, (*cm.charmap)[c]);
                else
                    push(EntityType::CHAR, (*cm.charmap)[c]);

                prev = c;
            }

            flush();
            out += "</div>\n";

            a += len;
        }

        return out;
    }

}
