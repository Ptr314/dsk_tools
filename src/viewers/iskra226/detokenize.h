// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: обратная трансляция — токены в текст

#pragma once

#include <string>

#include "viewers/iskra226/names.h"
#include "viewers/iskra226/program.h"

namespace iskra {

// Обратное к tokenize(): образ программы в текстовый листинг КОИ-8.
// Это `LIST` диалогового режима и вторая половина круговой проверки —
// «текст → токены → текст» обязано давать те же байты (docs/DECISIONS.md,
// разд. 12).
//
// Имён переменных в потоке нет вовсе, есть только индексы, поэтому имена
// приходится придумывать. Та же таблица нужна и обратной трансляции: без
// неё tokenize() раздаст индексы заново, по первому появлению в тексте, и
// они разъедутся. Поэтому names — выход детокенизации и вход трансляции.
bool detokenize(const ProgramImage & img, NameTable & names,
                std::string & koi8, std::string & error);

// Одна строка: «<номер> <операторы>», без перевода строки на конце.
bool detokenize_line(const ProgramLine & line, const NameTable & names,
                     std::string & koi8, std::string & error);

} // namespace iskra
