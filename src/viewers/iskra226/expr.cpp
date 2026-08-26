// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: разбор выражений, общий для текста и токенов

#include "viewers/iskra226/expr.h"

namespace iskra {

ExprParser::ExprParser(TokenSource & src)
    : src_(src), pending_start_(0), has_pending_(false), pending_operand_(true)
{
}

void ExprParser::fail(const std::string & msg)
{
    if (error_.empty()) error_ = msg;
}

bool ExprParser::peek(Tok & t, bool operand_expected)
{
    if (failed()) return false;
    if (has_pending_) {
        if (pending_operand_ != operand_expected && src_.state_sensitive()) {
            fail("внутренняя ошибка разбора: смена состояния при заглядывании");
            return false;
        }
        t = pending_;
        return true;
    }
    pending_start_ = src_.tell();
    if (!src_.next(pending_, operand_expected)) {
        fail("не удалось прочитать лексему");
        return false;
    }
    has_pending_ = true;
    pending_operand_ = operand_expected;
    t = pending_;
    return true;
}

void ExprParser::consume()
{
    has_pending_ = false;
}

bool ExprParser::take(Tok & t, bool operand_expected)
{
    if (!peek(t, operand_expected)) return false;
    consume();
    return true;
}

} // namespace iskra