// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: образ программы в памяти — поток токенов и таблицы переменных

#include "viewers/iskra226/program.h"

#include "viewers/iskra226/names.h"


#include <cstdio>
#include <cstring>

namespace iskra {

namespace {

// ---------------------------------------------------------------------------

// Таблицы 2 и 3 — один непрерывный массив дескрипторов по 4 байта, идущих
// в порядке убывания индекса переменной. Таблица 1 хранит размеры тех
// переменных, у которых установлен бит 0 флага, и сопоставляется с ними
// порядково.
void build_vars(const std::vector<uint8_t> & code, unsigned L1, unsigned L2,
                unsigned L3, std::vector<VarInfo> & vars)
{
    const unsigned N = L2 / 4 + L3 / 4;
    vars.assign(N, VarInfo());
    if (!N) return;

    const unsigned t1 = 6;
    const unsigned t23 = 6 + L1;
    if (t23 + N * 4 > code.size()) return;

    for (unsigned pos = 0; pos < N; ++pos) {
        const uint8_t flag = code[t23 + pos * 4 + 2];
        VarInfo & v = vars[N - 1 - pos];
        v.known = true;
        v.is_string = (flag & 0x20) != 0;
        v.is_integer = (flag & 0x30) == 0;
        // Таблица 3 — хвост того же массива, и приходится он на самые
        // младшие индексы: это и есть область COM (разд. 6).
        v.is_common = (N - 1 - pos) < L3 / 4;
    }

    // Порядковое соответствие «запись таблицы 1 → переменная с битом 0».
    unsigned k = 0;
    for (unsigned pos = 0; pos < N; ++pos) {
        if (!(code[t23 + pos * 4 + 2] & 1)) continue;

        const unsigned off = t1 + k * 8;
        if (off + 8 > t23) break;                  // таблица 1 кончилась
        ++k;

        VarInfo & v = vars[N - 1 - pos];
        const unsigned field = code[off + 2] | (code[off + 3] << 8);
        const unsigned nelem = code[off + 4] | (code[off + 5] << 8);
        const unsigned sizecode = code[off + 6] | (code[off + 7] << 8);

        if ((field >> 8) == 0x08) {
            // Одномерный массив либо строка с явной длиной.
            v.dim1 = nelem;
            v.dim2 = 0;
        } else if (nelem == 10 && (field == 295 || field == 1881)) {
            // Неявно объявленный массив: размерность по умолчанию.
            v.dim1 = 10;
            v.dim2 = 0;
        } else {
            v.dim1 = nelem;
            v.dim2 = field;
        }

        if (v.is_string) {
            // Размерный код = 2 x размер элемента, младший бит — «длина
            // задана явно».
            const unsigned elem = sizecode >> 1;
            v.str_len = elem ? elem : 16;

            // Строку-скаляр от массива строк по числу элементов не отличить:
            // у скаляра поле 4–5 числом элементов не является вовсе — у A¤
            // в EDITOR там 129 при длине 1. Разбору это важно (у массива за
            // именем идёт список индексов), исполнению тоже: иначе поле
            // заводится в 129 раз длиннее нужного.
            //
            // Лестница правил по убыванию надёжности — docs/format.md,
            // разд. 6 «Различение скаляр/массив для строк».
            if (!(sizecode & 1)) {
                // Длина не задана явно. Скалярная строка длины по умолчанию
                // дескриптора не получает вовсе, значит это массив.
                v.is_array = true;
            } else if (v.dim2 != 0) {
                v.is_array = true;
            } else {
                // Записи таблицы 1 идут в порядке убывания индекса переменной,
                // адреса при этом растут; за последней считается FFFE.
                const unsigned addr = code[off] | (code[off + 1] << 8);
                const unsigned next = (off + 16 <= t23)
                                    ? (code[off + 8] | (code[off + 9] << 8))
                                    : 0xFFFE;

                if (addr && next && next > addr) {
                    // Сравнивать только точными равенствами: у L1¤(1)2
                    // разность 8 = 1*2+6, а как скаляр было бы 2.
                    const unsigned delta = next - addr;
                    const unsigned long as_array =
                        static_cast<unsigned long>(nelem) * elem + 6;
                    const unsigned as_scalar = elem + (elem & 1);

                    if (delta == as_array) v.is_array = true;
                    else if (delta == as_scalar) v.is_array = false;
                    // Разность — верхняя граница: между дескрипторами могут
                    // лежать скаляры. Не поместился — значит не массив.
                    else v.is_array = (as_array <= delta);
                } else {
                    // Адресов нет — так бывает в невыполнявшихся файлах
                    // (STAT05, STAT01A). Различить нечем: остаётся догадка по
                    // числу элементов. У массива там настоящий счётчик, у
                    // скаляра — постороннее значение, так что ошибается она
                    // только в сторону «массив».
                    v.is_array = (v.dim1 > 1);
                }
                if (!v.is_array) v.dim1 = 1;
            }
        } else {
            // Числовые скаляры дескриптора не получают: раз он есть — массив.
            v.is_array = true;
        }
    }
}

const unsigned SECTOR = 256;
const unsigned SECTOR_DATA = 254;      // байт полезных данных в секторе
const uint8_t REC_SEP = 0xFE;

std::string hex2(unsigned v)
{
    char b[8];
    std::sprintf(b, "%02X", v & 0xFF);
    return b;
}

bool bcd_ok(uint8_t b) { return (b >> 4) <= 9 && (b & 0x0F) <= 9; }
unsigned bcd2(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }

uint8_t to_bcd(unsigned v) { return static_cast<uint8_t>(((v / 10) % 10) * 16 + v % 10); }

// Переход к следующему сектору выполняется только если весь остаток
// текущего 254-байтового куска нулевой (docs/format.md, разд. 3).
void skip_padding(const std::vector<uint8_t> & code, unsigned & p)
{
    const unsigned b = ((p / SECTOR_DATA) + 1) * SECTOR_DATA;
    if (b <= p || b > code.size()) return;
    for (unsigned i = p; i < b; ++i)
        if (code[i] != 0) return;
    p = b;
}

bool is_record_start(const std::vector<uint8_t> & code, unsigned p)
{
    skip_padding(code, p);
    if (p + 3 > code.size()) return false;
    if (!bcd_ok(code[p]) || !bcd_ok(code[p + 1])) return false;
    const unsigned len = code[p + 2];
    if (len < 1) return false;
    const unsigned e = p + 2 + len;
    return e < code.size() && code[e] == REC_SEP;
}

unsigned be16(const uint8_t * p) { return (static_cast<unsigned>(p[0]) << 8) | p[1]; }

} // namespace

// Обратное к build_vars(): собрать таблицы переменных из их описаний.
// Раскладка — docs/format.md, разд. 6. Адреса машина назначает при
// исполнении, но записывать их приходится: по разностям соседних адресов
// читатель отличает строку-скаляр от массива строк, и без них описание
// становится неоднозначным.
void ProgramImage::rebuild_tables()
{
    tables_.clear();
    t1_ = t2_ = t3_ = 0;

    const unsigned n = static_cast<unsigned>(vars_.size());
    if (!n) return;

    // Область COM занимает самые младшие индексы и лежит в таблице 3.
    unsigned common = 0;
    while (common < n && vars_[common].is_common) ++common;

    // Дескриптор размера получают массивы и символьные переменные с явной
    // длиной; числовые скаляры и строки длины по умолчанию — нет.
    std::vector<uint8_t> t1;
    std::vector<uint8_t> t23;
    unsigned addr = 0x0100;

    // Записи идут в порядке убывания индекса; у таблицы 1 адреса при этом
    // растут, поэтому обе строятся одним проходом.
    for (unsigned pos = 0; pos < n; ++pos) {
        const unsigned i = n - 1 - pos;
        const VarInfo & v = vars_[i];

        const bool explicit_len = v.is_string && v.str_len && v.str_len != 16;
        const bool descr = v.is_array || explicit_len;

        uint8_t flag = v.is_string ? 0x20 : (v.is_integer ? 0x00 : 0x10);
        if (descr) flag |= 0x01;

        if (descr) {
            const unsigned d1 = v.dim1 ? v.dim1 : (v.is_array ? 10 : 1);
            const unsigned d2 = v.dim2;
            const unsigned elem = v.is_string ? (v.str_len ? v.str_len : 16)
                                              : (v.is_integer ? 2 : 8);
            // Размерный код = удвоенный размер элемента, младший бит —
            // «длина задана явно».
            unsigned sizecode = elem * 2;
            if (v.is_string && explicit_len) sizecode |= 1;

            const unsigned field = d2 ? d2 : (v.is_string ? 0x0800 : 0x082D);

            t1.push_back(static_cast<uint8_t>(addr & 0xFF));
            t1.push_back(static_cast<uint8_t>(addr >> 8));
            t1.push_back(static_cast<uint8_t>(field & 0xFF));
            t1.push_back(static_cast<uint8_t>(field >> 8));
            t1.push_back(static_cast<uint8_t>(d1 & 0xFF));
            t1.push_back(static_cast<uint8_t>(d1 >> 8));
            t1.push_back(static_cast<uint8_t>(sizecode & 0xFF));
            t1.push_back(static_cast<uint8_t>(sizecode >> 8));

            // Размер выделяемой памяти: у скалярной строки — длина, округлённая
            // вверх до чётной; у массива — элементы плюс шесть байт дескриптора.
            addr += v.is_array
                        ? d1 * (d2 ? d2 : 1) * elem + 6
                        : elem + (elem & 1);
        }

        t23.push_back(0);                    // адрес назначает исполнение
        t23.push_back(0);
        t23.push_back(flag);
        t23.push_back(0);
    }

    t1_ = static_cast<unsigned>(t1.size());
    t3_ = common * 4;
    t2_ = static_cast<unsigned>(t23.size()) - t3_;

    tables_.insert(tables_.end(), t1.begin(), t1.end());
    tables_.insert(tables_.end(), t23.begin(), t23.end());
}

void ProgramImage::clear()
{
    lines_.clear();
    vars_.clear();
    tables_.clear();
    t1_ = t2_ = t3_ = 0;
}

bool ProgramImage::load_file(const std::vector<uint8_t> & file, std::string & error)
{
    if (file.size() < 512) { error = "файл слишком короток"; return false; }
    if (file[0] != 1) { error = "не программа BASIC"; return false; }

    const uint8_t attr = file[9];
    if (!(attr == 0x20 || attr == 0x21 || attr == 0x24 || attr == 0x25)) {
        error = "не программа BASIC (признак " + hex2(attr) + ")";
        return false;
    }
    // Младший бит признака различает два представления программы. Текстовое
    // машина **транслирует при загрузке** — промежуточного вида у неё нет
    // вовсе (docs/DECISIONS.md, разд. 12), — и на дисках корпуса половина
    // программ лежит именно так (`М1`, `FAN01`, `STAT01.`, `STAT06`).
    if ((attr & 1) == 0) return load_text_file(file, error);

    std::vector<uint8_t> code;
    unsigned p = SECTOR;
    while (p + SECTOR <= file.size()) {
        if (file[p] == 0x1C) break;             // control record
        if (file[p + 1] != 0x80) break;
        code.insert(code.end(), file.begin() + p + 2, file.begin() + p + SECTOR);
        p += SECTOR;
    }
    return load_stream(code, error);
}

// Текстовая программа с дискеты: сектора устроены так же, как у
// оттранслированной, а в них лежит листинг в КОИ-8, где строки разделены
// байтом `85` — тем же, что разделяет их в символьном буфере `SAVE`
// (docs/format.md, разд. 10).
//
// Имена переменных здесь не сохраняются: индексы раздаются по первому
// появлению имени, как и у машины, а для `LIST` детокенизатор придумает их
// заново. Диалог для этого зовёт `Console::refresh_names()`.
// Текстовая форма здесь оглушена: она тянет за собой транслятор
// (core/tokenize.* и core/text_lexer.*), а обратной трансляции он не нужен
// вовсе. В самом эмуляторе этот метод собирает листинг из секторов —
// строки там разделены байтом 85 — и зовёт tokenize().
bool ProgramImage::load_text_file(const std::vector<uint8_t> &,
                                  std::string & error)
{
    error = "текстовая форма программы здесь не поддерживается";
    return false;
}

bool ProgramImage::load_stream(const std::vector<uint8_t> & code, std::string & error)
{
    clear();
    if (code.size() < 6) { error = "поток слишком короток"; return false; }

    t1_ = be16(&code[0]);
    t2_ = be16(&code[2]);
    t3_ = be16(&code[4]);
    unsigned start = 6 + t1_ + t2_ + t3_;
    if (start > code.size()) { error = "таблицы переменных выходят за поток"; return false; }
    tables_.assign(code.begin() + 6, code.begin() + start);

    // У части файлов между таблицами и первой записью лежат четыре лишних
    // байта (docs/format.md, разд. 3). Отличаем по тому, стоит ли по
    // вычисленному смещению корректная запись строки.
    if (!is_record_start(code, start) && is_record_start(code, start + 4)) start += 4;

    unsigned p = start;
    while (p + 3 <= code.size()) {
        skip_padding(code, p);
        if (p + 3 > code.size()) break;
        if (!bcd_ok(code[p]) || !bcd_ok(code[p + 1])) {
            error = "номер строки не BCD";
            return false;
        }
        const unsigned len = code[p + 2];
        if (len < 1) break;
        const unsigned end = p + 2 + len;
        if (end > code.size()) { error = "длина строки выходит за поток"; return false; }

        ProgramLine l;
        l.number = bcd2(code[p]) * 100 + bcd2(code[p + 1]);
        l.body.assign(code.begin() + p + 3, code.begin() + end);
        lines_.push_back(l);

        // За последней записью FE стоит тоже, но у рукотворных потоков его
        // может не быть, а за ним идут нули до конца сектора. Байт, не
        // равный FE, значит «дальше строк нет» — так же читал и прежний
        // разбор оттранслированной формы.
        if (end >= code.size() || code[end] != REC_SEP) break;
        p = end + 1;
    }

    // Размеры массивов и типы переменных известны только из таблиц: в самих
    // операторах DIM лежат одни индексы (docs/format.md, разд. 6).
    build_vars(code, t1_, t2_, t3_, vars_);
    return true;
}

bool ProgramImage::find(unsigned number, unsigned & index) const
{
    for (unsigned i = 0; i < lines_.size(); ++i)
        if (lines_[i].number == number) { index = i; return true; }
    return false;
}

unsigned ProgramImage::lower_bound(unsigned number) const
{
    for (unsigned i = 0; i < lines_.size(); ++i)
        if (lines_[i].number >= number) return i;
    return static_cast<unsigned>(lines_.size());
}

void ProgramImage::put_line(unsigned number, const uint8_t * body, unsigned len)
{
    ProgramLine l;
    l.number = number;
    if (len) l.body.assign(body, body + len);

    const unsigned i = lower_bound(number);
    if (i < lines_.size() && lines_[i].number == number) lines_[i] = l;
    else lines_.insert(lines_.begin() + i, l);
}

bool ProgramImage::erase_line(unsigned number)
{
    unsigned i = 0;
    if (!find(number, i)) return false;
    lines_.erase(lines_.begin() + i);
    return true;
}

void ProgramImage::erase_range(unsigned from, unsigned to)
{
    std::vector<ProgramLine> kept;
    for (unsigned i = 0; i < lines_.size(); ++i) {
        const unsigned n = lines_[i].number;
        if ((from == 0 || n >= from) && (to == 0 || n <= to)) continue;
        kept.push_back(lines_[i]);
    }
    lines_.swap(kept);
}

void ProgramImage::save_stream(std::vector<uint8_t> & code) const
{
    code.clear();
    code.push_back(static_cast<uint8_t>((t1_ >> 8) & 0xFF));
    code.push_back(static_cast<uint8_t>(t1_ & 0xFF));
    code.push_back(static_cast<uint8_t>((t2_ >> 8) & 0xFF));
    code.push_back(static_cast<uint8_t>(t2_ & 0xFF));
    code.push_back(static_cast<uint8_t>((t3_ >> 8) & 0xFF));
    code.push_back(static_cast<uint8_t>(t3_ & 0xFF));
    code.insert(code.end(), tables_.begin(), tables_.end());

    for (unsigned i = 0; i < lines_.size(); ++i) {
        if (i) code.push_back(REC_SEP);
        const ProgramLine & l = lines_[i];
        code.push_back(to_bcd(l.number / 100));
        code.push_back(to_bcd(l.number % 100));
        code.push_back(static_cast<uint8_t>(l.body.size() + 1));
        code.insert(code.end(), l.body.begin(), l.body.end());
    }
    // FE стоит и после последней записи: он и разделитель, и признак конца.
    // Проверено на STAT04, STAT09, VICT — за ним идут нули до конца сектора.
    if (!lines_.empty()) code.push_back(REC_SEP);
}

void ProgramImage::save_file(const std::string & name,
                             std::vector<uint8_t> & file) const
{
    std::vector<uint8_t> code;
    save_stream(code);

    file.assign(SECTOR, 0);
    file[0] = 0x01;
    for (unsigned i = 0; i < 8; ++i)
        file[1 + i] = (i < name.size()) ? static_cast<uint8_t>(name[i])
                                        : static_cast<uint8_t>(' ');
    file[9] = 0x21;                     // оттранслирована, не защищена

    for (std::size_t p = 0; p < code.size(); p += SECTOR_DATA) {
        file.push_back(0x02);           // все секторы наблюдались с 02
        file.push_back(0x80);
        for (unsigned i = 0; i < SECTOR_DATA; ++i)
            file.push_back(p + i < code.size() ? code[p + i] : 0);
    }

    // Концевая запись `1C <позиция сектора от начала файла, 2 байта BE>`.
    // Она есть у всех программных файлов корпуса: у `LСОЗСК` на klerk.dsk
    // файл занимает секторы 140…147, и сектор 147 начинается с `1C 00 08`.
    // Без неё LIST DC и LIMITS показывают «использовано 0» — по ней они и
    // считают. Загрузчик на `1C` останавливается, так что круг сходится.
    const unsigned pos = static_cast<unsigned>(file.size() / SECTOR) + 1;
    file.push_back(0x1C);
    file.push_back(static_cast<uint8_t>(pos >> 8));
    file.push_back(static_cast<uint8_t>(pos & 0xFF));
    file.resize(file.size() + SECTOR - 3, 0);
}

} // namespace iskra