// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: образ программы в памяти — поток токенов и таблицы переменных

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iskra {

// В памяти машины лежит ровно то же, что в оттранслированном файле на диске:
// таблицы переменных и поток записей строк (docs/format.md, разд. 3 и 6).
// Отдельного промежуточного представления нет — см. docs/DECISIONS.md,
// разд. 12.
//
// Поток:
//     <длина табл. 1: 2 байта BE> <длина табл. 2> <длина табл. 3>
//     <таблицы подряд>
//     <запись строки> FE <запись строки> FE … <запись строки>
//
// Запись строки: <номер: 2 байта BCD> <длина тела + 1> <тело>.
// Тело — операторы `<глагол> <длина операндов> <операнды>` подряд.

// Что известно о переменной. В оттранслированной форме — из таблиц
// (docs/format.md, разд. 6), в текстовой — из имени и операторов DIM/COM.
struct VarInfo {
    VarInfo() : known(false), is_string(false), is_integer(false),
                is_array(false), is_common(false), dim1(0), dim2(0),
                str_len(0) {}

    bool known;
    bool is_string;
    bool is_integer;
    bool is_array;
    bool is_common;      // объявлена оператором COM
    unsigned dim1;       // число элементов; для двумерного — первая размерность
    unsigned dim2;       // 0 у одномерного
    unsigned str_len;
};

// Одна строка программы: номер и байты тела (без номера и без длины).
struct ProgramLine {
    ProgramLine() : number(0) {}
    unsigned number;
    std::vector<uint8_t> body;
};

class ProgramImage
{
public:
    ProgramImage() {}

    // --- загрузка ----------------------------------------------------------

    // Из файла, снятого с дискеты. Оттранслированный грузится как есть;
    // текстовый тут отвергается — его переводит core/tokenize.*.
    bool load_file(const std::vector<uint8_t> & file, std::string & error);
    // Текстовая программа: машина транслирует её при загрузке.
    bool load_text_file(const std::vector<uint8_t> & file, std::string & error);

    // Из уже собранного потока — без заголовочного сектора и служебных байт.
    bool load_stream(const std::vector<uint8_t> & code, std::string & error);

    // --- строки ------------------------------------------------------------

    unsigned line_count() const { return static_cast<unsigned>(lines_.size()); }
    const ProgramLine & line(unsigned i) const { return lines_[i]; }

    // Индекс строки с данным номером; false, если такой нет.
    bool find(unsigned number, unsigned & index) const;

    // Индекс первой строки с номером не меньше данного. Для перехода на
    // строку, которой нет: «программа начинает выполняться со строки с
    // наименьшим номером» (руководство, разд. 19.1).
    unsigned lower_bound(unsigned number) const;

    // Вставить или заменить строку. Строки хранятся по возрастанию номера.
    void put_line(unsigned number, const uint8_t * body, unsigned len);
    void put_line(const ProgramLine & l) { put_line(l.number, l.body.empty() ? 0
                                                   : &l.body[0],
                                                   static_cast<unsigned>(l.body.size())); }

    // Удалить строку. false — такой не было.
    bool erase_line(unsigned number);

    // Удалить диапазон номеров включительно — это CLEAR P оператора LOAD DC.
    // from == 0 значит «с начала», to == 0 — «до конца».
    void erase_range(unsigned from, unsigned to);

    void clear();

    // Собрать таблицы переменных заново — из vars_. Нужно образу, собранному
    // из текста: у него таблиц нет, а SAVE DC пишет на диск именно их.
    void rebuild_tables();

    // --- переменные --------------------------------------------------------

    const std::vector<VarInfo> & vars() const { return vars_; }
    std::vector<VarInfo> & vars() { return vars_; }

    // --- выгрузка ----------------------------------------------------------

    // Поток целиком: таблицы и записи строк. Это же пишет SAVE DC T.
    void save_stream(std::vector<uint8_t> & code) const;

    // Файл для записи на диск: заголовочный сектор и секторы потока.
    void save_file(const std::string & name, std::vector<uint8_t> & file) const;

private:
    std::vector<ProgramLine> lines_;      // по возрастанию номера
    std::vector<VarInfo> vars_;
    std::vector<uint8_t> tables_;         // таблицы 1–3 как есть
    unsigned t1_, t2_, t3_;               // их длины
};

} // namespace iskra