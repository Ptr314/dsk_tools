// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: таблица имён переменных текстовой формы

#include "viewers/iskra226/names.h"

namespace iskra {

unsigned NameTable::index(const std::string & name)
{
    for (unsigned i = 0; i < names_.size(); ++i)
        if (names_[i] == name) return i;

    names_.push_back(name);

    VarInfo v;
    v.known = true;
    // У массива ключ кончается скобкой — тип берём из знака перед ней.
    std::size_t tail = name.size() - 1;
    if (name[tail] == '(') { v.is_array = true; if (tail) --tail; }
    const char last = name[tail];
    v.is_string = (last == '$');
    v.is_integer = (last == '%');
    if (v.is_string) v.str_len = 16;      // длина по умолчанию
    vars_.push_back(v);

    return static_cast<unsigned>(names_.size() - 1);
}

} // namespace iskra
