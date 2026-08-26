// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: таблица имён переменных текстовой формы

#pragma once

#include <string>
#include <vector>

#include "viewers/iskra226/program.h"

namespace iskra {

// Имена переменных живут только в текстовой форме; в оттранслированной от
// переменной остаётся индекс. Транслятор раздаёт индексы в порядке первого
// появления имени — правило из docs/format.md, разд. 6.
class NameTable
{
public:
    // Ключ — имя вместе с признаком массива: `B¤` и `B¤(` — разные
    // переменные с разными индексами (VICT 32 и 36, docs/format.md, разд. 6).
    unsigned index(const std::string & name);
    unsigned count() const { return static_cast<unsigned>(names_.size()); }

    // Откат после неудачной строки: имена, набранные при разборе того, что
    // разобрать не удалось, — выдумка, и они сдвинули бы все дальнейшие
    // индексы.
    void truncate(unsigned n)
    {
        if (n < names_.size()) { names_.resize(n); vars_.resize(n); }
    }
    const std::string & name(unsigned i) const { return names_[i]; }

    // Тип переменной виден прямо из имени: ¤ — символьная, % — целая.
    // Заполняется при первом появлении имени, дальше уточняется из DIM.
    const std::vector<VarInfo> & vars() const { return vars_; }
    std::vector<VarInfo> & vars() { return vars_; }

private:
    std::vector<std::string> names_;
    std::vector<VarInfo> vars_;
};

} // namespace iskra
