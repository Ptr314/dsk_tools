// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: источник лексем поверх байтов операндов одного оператора

#include "viewers/iskra226/byte_source.h"

#include <cstdio>

namespace iskra {

namespace {

std::string dec_text(unsigned v)
{
    char b[16];
    std::sprintf(b, "%u", v);
    return b;
}

} // namespace

std::string hex2(unsigned v)
{
    char b[8];
    std::sprintf(b, "%02X", v & 0xFF);
    return b;
}

// E5: описатель + BCD. Старшая тетрада описателя — цифр до запятой,
// младшая — всего цифр. E6 добавляет байт порядка.
bool ByteSource::number_e5(Tok & t, bool with_exponent)
{
    if (i_ >= n_) return fail("константа оборвалась");
    const uint8_t desc = p_[i_++];
    const unsigned ip = desc >> 4;
    const unsigned total = desc & 0x0F;
    const unsigned bytes = (total + 1) / 2;
    if (i_ + bytes > n_) return fail("константа оборвалась");

    std::string digits;
    for (unsigned k = 0; k < bytes; ++k) {
        const uint8_t b = p_[i_ + k];
        digits += static_cast<char>('0' + (b >> 4));
        digits += static_cast<char>('0' + (b & 0x0F));
    }
    i_ += bytes;
    digits.resize(total);

    std::string s;
    if (ip == 0) {
        s = "." + digits;
    } else if (ip >= total) {
        s = digits;
        for (unsigned k = total; k < ip; ++k) s += '0';
    } else {
        s = digits.substr(0, ip) + "." + digits.substr(ip);
    }

    if (with_exponent) {
        if (i_ >= n_) return fail("константа оборвалась");
        const uint8_t e = p_[i_++];
        if (!bcd_ok(e)) return fail("порядок константы не BCD");
        s += "E";
        char b[8];
        std::sprintf(b, "%u", bcd2(e));
        s += b;
    }

    if (!Number::parse(s, t.num)) return fail("не разобралась константа " + s);
    t.t = Tok::NUM;
    // Исходная запись едет с лексемой: 1E6 и 1000000 — одно число, но байты
    // разные, и обратной трансляции значения мало (docs/format.md, разд. 5).
    t.s = s;
    return true;
}

// Может ли байт начинать операнд. Нужно там, где «массив или скаляр»
// по таблицам не разрешается, — у символьных переменных (docs/format.md,
// разд. 7): за именем идёт либо список индексов, либо операция.
bool looks_like_operand(uint8_t b)
{
    if (b <= VAR_MAX) return true;                 // индекс переменной
    switch (b) {
        // `E1` STR( сюда не входит нарочно: индекс — число, а вырезка
        // всегда символьная, и списка индексов она начать не может. Так
        // разбирается образ `CONVERT … TO <приёмник>,(STR(…))`, где за
        // приёмником сразу идёт образ (`EDITOR` 5137, 5475).
        case 0xE5: case 0xE6: case 0xE7: case 0xE8:   // константы
        case 0xEB:                                 // (
        case 0xF0:                                 // FN<имя>( — своя функция
        case 0xF1:                                 // #PI
        case 0xF2: case 0xF3: case 0xF4: case 0xF5:
        case 0xF6: case 0xF7: case 0xF8:
        case 0xF9: case 0xFA: case 0xFB:
        case 0xFC: case 0xFD: case 0xFE:           // функции
            return true;
        default:
            return false;
    }
}

bool ByteSource::next(Tok & t, bool operand_expected)
{
    t = Tok();
    if (i_ >= n_) { t.t = Tok::END; return true; }

    const uint8_t c = p_[i_++];

    if (c <= VAR_MAX) {
        if (!operand_expected) {
            // Индекс переменной там, где ждали операцию, — это не ошибка:
            // за выражением может сразу стоять следующий приёмник, списка
            // разделителей в потоке нет. Разбор выражения просто кончается
            // здесь, а вызывающий возвращает источник назад через unpeek().
            t.t = Tok::UNKNOWN;
            t.s = "индекс переменной";
            --i_;
            return true;
        }
        t.t = Tok::VAR;
        t.var = c;

        // Скобки у индекса в потоке нет. У числовых переменных массив
        // виден по таблицам: числовой скаляр дескриптора не получает.
        // У символьных так нельзя — отличить строку-скаляр от массива строк
        // можно только по разностям адресов, а те бывают нулевыми. Поэтому
        // здесь работает правило из разд. 7: смотрим, операнд ли дальше.
        if (vars_ && c < vars_->size()) {
            const VarInfo & v = (*vars_)[c];
            t.table_array = v.is_array;
            t.indexed = v.is_string ? (i_ < n_ && looks_like_operand(p_[i_]))
                                    : v.is_array;
        }
        return true;
    }

    // Двузначные токены: значение зависит от того, чего ждёт разбор.
    if (operand_expected) {
        switch (c) {
            case 0xD5: t.t = Tok::FN_AT; return true;
            case 0xDF: t.t = Tok::FN_TAB; return true;
            case 0xD8: t.t = Tok::FN_ROUND; return true;   // EDITOR 4132
            // `/адрес` в позиции операнда — адрес устройства, а не деление
            // (docs/format.md, разд. 5).
            case 0xDC: t.t = Tok::SLASH; return true;
            case 0xE0: {                                // ссылка на массив целиком
                if (i_ >= n_) return fail("ссылка на массив оборвалась");
                const uint8_t v = p_[i_++];
                if (v > VAR_MAX) return fail("после E0 не индекс переменной");
                t.t = Tok::ARRAY;
                t.var = v;
                return true;
            }
            case 0xE9: t.t = Tok::MINUS; return true;   // унарный минус
            case 0xE5: return number_e5(t, false);
            case 0xE6: return number_e5(t, true);
            case 0xE7: {
                if (i_ + 2 > n_) return fail("константа оборвалась");
                const unsigned v = bcd2(p_[i_]) * 100 + bcd2(p_[i_ + 1]);
                i_ += 2;
                t.t = Tok::NUM;
                t.num = Number::from_int(static_cast<long>(v));
                t.s = dec_text(v);
                return true;
            }
            case 0xE8: {
                if (i_ >= n_) return fail("константа оборвалась");
                const uint8_t b = p_[i_++];
                if (!bcd_ok(b)) return fail("константа не BCD: " + hex2(b));
                t.t = Tok::NUM;
                t.num = Number::from_int(static_cast<long>(bcd2(b)));
                t.s = dec_text(bcd2(b));
                return true;
            }
            case 0xDE: {
                if (list_context_) { t.t = Tok::COMMA; return true; }
                // Сырой байт: байтовые константы и адреса устройств.
                if (i_ >= n_) return fail("литерал оборвался");
                const uint8_t raw = p_[i_++];
                t.t = Tok::NUM;
                t.num = Number::from_int(static_cast<long>(raw));
                // Записывается он двумя шестнадцатеричными цифрами, как в
                // INIT( и AND( — по значению этого не восстановить.
                t.s = hex2(raw);
                return true;
            }
            default: break;
        }
    } else {
        switch (c) {
            case 0xD5: t.t = Tok::NE; return true;
            case 0xD6: t.t = Tok::LE; return true;
            case 0xD7: t.t = Tok::LT; return true;
            case 0xD8: t.t = Tok::GE; return true;
            // Связки условий. Оба байта двузначны: в позиции
            // операнда это числовые константы. Сверено на паре
            // «текст + токены» (EDITOR 360, 1360).
            case 0xE5: t.t = Tok::XOR; return true;
            case 0xE6: t.t = Tok::OR; return true;
            case 0xE7: t.t = Tok::AND; return true;
            case 0xDE: t.t = Tok::COMMA; return true;
            case 0xE9: t.t = Tok::MINUS; return true;   // бинарный минус
            case 0xE0: t.t = Tok::CARET; return true;
            case 0xDC: t.t = Tok::SLASH; return true;
            case 0xDF: t.t = Tok::STAR; return true;
            case 0xCC: t.t = Tok::KW_GOSUB; return true;   // внутри ON
            case 0xCD: t.t = Tok::KW_GOTO;  return true;   // внутри ON
            case 0xD3: {
                if (i_ + 2 > n_) return fail("THEN без номера строки");
                const unsigned ln = bcd2(p_[i_]) * 100 + bcd2(p_[i_ + 1]);
                i_ += 2;
                t.t = Tok::KW_THEN;
                t.num = Number::from_int(static_cast<long>(ln));
                return true;
            }
            default: break;
        }
    }

    // Однозначные.
    switch (c) {
        case 0xD0: t.t = Tok::RPAR; return true;
        case 0xD1: t.t = Tok::KW_TO; return true;
        case 0xD2: t.t = Tok::KW_STEP; return true;
        case 0xD4: t.t = Tok::GT; return true;
        case 0xD9: t.t = Tok::EQ; return true;
        case 0xDD: t.t = Tok::SEMI; return true;
        case 0xEA: t.t = Tok::PLUS; return true;
        case 0xEB: t.t = Tok::LPAR; return true;
        case 0xDB: t.t = Tok::HASH; return true;
        case 0xE1: t.t = Tok::FN_STR; return true;

        // FN<имя>( — функция пользователя. Имя лежит следующим байтом сырым
        // кодом символа, а не индексом переменной: пространства имён разные
        // (руководство, разд. 4.8). Закрывается скобкой `D0`.
        case 0xF0: {
            if (!operand_expected) break;
            if (i_ >= n_) return fail("FN без имени функции");
            t.t = Tok::FN_USER;
            t.var = p_[i_++];
            return true;
        }
        case 0xEC: t.t = Tok::FN_POS; return true;
        case 0xED: t.t = Tok::FN_LEN; return true;
        case 0xEE: t.t = Tok::FN_NUM; return true;
        case 0xEF: t.t = Tok::FN_VAL; return true;
        case 0xF1: t.t = Tok::PI; return true;
        case 0xF2: t.t = Tok::FN_ABS; return true;
        case 0xF4: t.t = Tok::FN_RND; return true;
        case 0xF3: t.t = Tok::FN_INT; return true;
        case 0xF5: t.t = Tok::FN_SGN; return true;
        case 0xF6: t.t = Tok::FN_SQR; return true;
        case 0xF7: t.t = Tok::FN_LOG; return true;
        case 0xF8: t.t = Tok::FN_EXP; return true;
        case 0xF9: t.t = Tok::FN_SIN;  return true;
        case 0xFA: t.t = Tok::FN_COS;  return true;
        case 0xFB: t.t = Tok::FN_TAN;  return true;
        case 0xFC: t.t = Tok::FN_ASIN; return true;
        case 0xFD: t.t = Tok::FN_ACOS; return true;
        case 0xFE: t.t = Tok::FN_ATAN; return true;

        case 0xE2: {                                   // HEX( — длина и данные
            if (i_ >= n_) return fail("HEX( оборвался");
            const unsigned len = p_[i_++];
            if (i_ + len > n_) return fail("HEX( оборвался");
            t.t = Tok::FN_HEX;
            t.s.assign(reinterpret_cast<const char *>(p_ + i_), len);
            i_ += len;
            return true;
        }
        case 0xE3:
        case 0xE4: {                                   // литерал в кавычках / апострофах
            if (i_ >= n_) return fail("литерал оборвался");
            const unsigned len = p_[i_++];
            if (i_ + len > n_) return fail("литерал оборвался");
            t.t = Tok::STR;
            t.s.assign(reinterpret_cast<const char *>(p_ + i_), len);
            i_ += len;
            return true;
        }
        default: break;
    }

    t.t = Tok::UNKNOWN;
    t.s = "токен " + hex2(c) + (operand_expected ? " в позиции операнда"
                                                 : " в позиции операции");
    return true;
}

} // namespace iskra
