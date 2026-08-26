// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: источник лексем поверх байтов операндов одного оператора

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "viewers/iskra226/expr.h"
#include "viewers/iskra226/program.h"

namespace iskra {

// Двоично-десятичный байт: и номера строк, и константы хранятся так.
inline bool bcd_ok(uint8_t b) { return (b >> 4) <= 9 && (b & 0x0F) <= 9; }
inline unsigned bcd2(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }

// Индексы переменных занимают 00…C9; выше начинаются токены.
const uint8_t VAR_MAX = 0xC9;

// Байт в шестнадцатеричном виде — для сообщений об ошибке.
std::string hex2(unsigned v);

class ByteSource : public TokenSource
{
public:
    ByteSource(const uint8_t * p, unsigned len, const std::vector<VarInfo> * vars)
        : p_(p), n_(len), i_(0), vars_(vars), list_context_(false) {}

    bool next(Tok & t, bool operand_expected);
    bool state_sensitive() const { return true; }

    // В операторах, операнды которых — список приёмников (INPUT, READ …),
    // DE всегда запятая, а не однобайтовый литерал.
    void set_list_context() { list_context_ = true; }

    unsigned pos() const { return i_; }
    unsigned tell() const { return i_; }
    void seek(unsigned p) { i_ = (p < n_) ? p : n_; }
    bool at_end() const { return i_ >= n_; }
    // Сколько байт осталось. Нужно там, где читают сырые пары BCD и должны
    // остановиться на неполной паре.
    unsigned left() const { return (i_ < n_) ? n_ - i_ : 0u; }

    // Перескочить «шапку» оператора — байты, которые не лексемы выражения:
    // метку GOSUB'/DEFFN', адрес возврата и тому подобное.
    void set_pos(unsigned p) { i_ = (p < n_) ? p : n_; }

    // Сырые байты: номера строк, коды знаков, адреса устройств — они не
    // лексемы выражения, и читать их надо мимо разбора. Звать только там,
    // где у ExprParser нет заглянутой лексемы.
    bool peek_raw_byte(uint8_t & out) const
    {
        if (i_ >= n_) return false;
        out = p_[i_];
        return true;
    }
    bool take_raw_byte(uint8_t & out)
    {
        if (i_ >= n_) return false;
        out = p_[i_++];
        return true;
    }
    void skip(unsigned k) { i_ = (i_ + k < n_) ? i_ + k : n_; }

    const std::string & error() const { return error_; }
    const std::string & source_error() const { return error_; }

private:
    bool number_e5(Tok & t, bool with_exponent);
    bool fail(const std::string & m) { if (error_.empty()) error_ = m; return false; }

    const uint8_t * p_;
    unsigned n_;
    unsigned i_;
    const std::vector<VarInfo> * vars_;
    bool list_context_;
    std::string error_;
};

} // namespace iskra
