// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: обратная трансляция — токены в текст

#include "viewers/iskra226/detokenize.h"

#include <cstdio>

#include "viewers/iskra226/byte_source.h"

namespace iskra {

namespace {

std::string dec(unsigned v)
{
    char b[16];
    std::sprintf(b, "%u", v);
    return b;
}

const char * HEXD = "0123456789ABCDEF";

std::string hex_bytes(const std::string & s)
{
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const unsigned char b = static_cast<unsigned char>(s[i]);
        r += HEXD[b >> 4];
        r += HEXD[b & 15];
    }
    return r;
}

// Номер строки парой BCD, как он лежит в потоке.
bool line_number(ByteSource & src, std::string & out)
{
    uint8_t a = 0, b = 0;
    if (!src.take_raw_byte(a) || !src.take_raw_byte(b)) return false;
    out += dec(bcd2(a) * 100 + bcd2(b));
    return true;
}

// ---------------------------------------------------------------------------
// Выражения
// ---------------------------------------------------------------------------

// Обратная сторона Encoder из tokenize.cpp. Скобки в потоке расставлены
// явно (`EB` … `D0`), поэтому старшинство пересчитывать не нужно: лексемы
// выписываются подряд. Отдельно обрабатываются только те конструкции, где
// текст и поток расходятся, — их же список в docs/format.md, разд. 5.
class Decoder
{
public:
    Decoder(ByteSource & src, const NameTable & names, std::string & out)
        : ex_(src), src_(src), names_(names), out_(out) {}

    // Выражение до разделителя уровня оператора. stop_at_gt — остановиться
    // ещё и на `D4`: внутри группы `PLOT` это закрывающая скобка группы, а
    // не знак «больше» (docs/format.md, разд. 5).
    bool expr(bool stop_at_gt = false);
    // Приёмник: переменная, элемент массива либо STR(. by_table — решать
    // «скаляр или массив» строго по таблицам, без заглядывания вперёд:
    // нужно там, где за приёмником сразу идёт значение (CLAUDE.md).
    // indices_ok — есть ли вообще где стоять списку индексов; ложь там, где
    // за именем заведомо идут сырые байты.
    bool lvalue(bool by_table = false, bool indices_ok = true);

    ExprParser & parser() { return ex_; }
    ByteSource & source() { return src_; }
    void emit(const std::string & s) { out_ += s; }

    const std::string & error() const
    {
        return src_.error().empty() ? error_ : src_.error();
    }
    bool fail(const std::string & m)
    {
        if (error_.empty()) error_ = m;
        return false;
    }

    // Имя переменной по индексу; у массива — без скобки.
    bool name(unsigned index, std::string & out) const;
    bool operand();                  // один операнд со всеми его хвостами

private:
    bool token(const Tok & t, bool operand_expected, bool & stop);
    bool indices();                  // список индексов до D0
    bool call(const char * word, unsigned args_min, unsigned args_max,
              bool closed);
    bool substr();
    bool implicit(const char * word, bool with_count, bool with_rel);

    ExprParser ex_;
    ByteSource & src_;
    const NameTable & names_;
    std::string & out_;
    std::string error_;
};

bool Decoder::name(unsigned index, std::string & out) const
{
    if (index >= names_.count()) return false;
    out = names_.name(index);
    // Ключ массива кончается скобкой — в тексте её ставит вызывающий.
    if (!out.empty() && out[out.size() - 1] == '(') out.resize(out.size() - 1);
    return true;
}

bool Decoder::indices()
{
    emit("(");
    for (;;) {
        if (!expr()) return false;
        Tok t;
        if (!ex_.take(t, false)) return fail(ex_.error());
        if (t.t == Tok::COMMA) { emit(","); continue; }
        if (t.t != Tok::RPAR) return fail("список индексов не закрыт");
        emit(")");
        return true;
    }
}

// Функция со скобкой в тексте. closed = закрывается ли она `D0` в потоке;
// у AT( закрывающей скобки нет вовсе.
bool Decoder::call(const char * word, unsigned args_min, unsigned args_max,
                   bool closed)
{
    emit(word);
    emit("(");
    unsigned args = 0;
    for (;;) {
        if (!expr()) return false;
        ++args;
        if (closed) {
            Tok t;
            if (!ex_.take(t, false)) return fail(ex_.error());
            if (t.t == Tok::COMMA) { emit(","); continue; }
            if (t.t != Tok::RPAR) return fail(std::string(word) + "( не закрыт");
            break;
        }
        if (args >= args_max) break;
        Tok t;
        if (!ex_.peek(t, false)) return fail(ex_.error());
        if (t.t != Tok::COMMA) break;
        ex_.consume();
        emit(",");
    }
    if (args < args_min) return fail(std::string(word) + "( без аргументов");
    emit(")");
    return true;
}

// STR(что, начало [, длина]) — первая запятая в потоке не кодируется,
// вторая кодируется DE, закрывается D0.
bool Decoder::substr()
{
    emit("STR(");
    Tok t;
    if (!ex_.take(t, true)) return fail(ex_.error());
    std::string nm;
    if (t.t == Tok::ARRAY) {
        if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
        emit(nm + "()");
    } else if (t.t == Tok::VAR) {
        if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
        emit(nm);
        // Индексация первого аргумента решается строго по таблицам: за
        // именем тут идёт не индекс, а начало подстроки, и заглядывание
        // приняло бы STR(Z¤,67) за Z¤(67) (CLAUDE.md, ловушка 2).
        if (t.table_array && !indices()) return false;
    } else if (t.t == Tok::STR) {
        emit("\"" + t.s + "\"");
    } else {
        return fail("STR( ждёт символьную переменную");
    }
    emit(",");                        // первая запятая — только в тексте

    for (;;) {
        if (!expr()) return false;
        if (!ex_.take(t, false)) return fail(ex_.error());
        if (t.t == Tok::COMMA) { emit(","); continue; }
        if (t.t != Tok::RPAR) return fail("STR( не закрыт");
        break;
    }
    emit(")");
    return true;
}

// Неявные функции: скобок в потоке нет вовсе, в тексте они нужны.
bool Decoder::implicit(const char * word, bool with_count, bool with_rel)
{
    emit(word);
    emit("(");
    if (!operand()) return false;

    Tok t;
    if (!ex_.peek(t, false)) return fail(ex_.error());
    if (with_rel) {
        const char * rel = 0;
        switch (t.t) {
            case Tok::EQ: rel = "="; break;
            case Tok::NE: rel = "<>"; break;
            case Tok::LT: rel = "<"; break;
            case Tok::LE: rel = "<="; break;
            case Tok::GT: rel = ">"; break;
            case Tok::GE: rel = ">="; break;
            default: break;
        }
        if (rel) {
            ex_.consume();
            emit(rel);
            // Справа бывает и код знака (`POS(Q¤=20)`, EDITOR 2630), и
            // переменная (EDITOR 6325). Различает их сам источник: код знака
            // приезжает лексемой с шестнадцатеричной записью.
            if (!operand()) return false;
        }
    } else if (with_count && t.t == Tok::COMMA) {
        ex_.consume();
        Tok two;
        if (!ex_.take(two, true)) return fail(ex_.error());
        if (two.t != Tok::HASH) return fail("у VAL( второй аргумент не DB");
        emit(",2");
    }
    emit(")");
    return true;
}

// Один операнд: константа, переменная, скобка, функция.
bool Decoder::operand()
{
    Tok t;
    if (!ex_.take(t, true)) return fail(ex_.error());
    bool stop = false;
    return token(t, true, stop);
}

bool Decoder::token(const Tok & t, bool operand_expected, bool & stop)
{
    stop = false;
    std::string nm;
    switch (t.t) {
        case Tok::NUM:
            // У константы своя исходная запись: по значению её не восстановить.
            emit(t.s.empty() ? t.num.to_display() : t.s);
            return true;
        case Tok::STR: emit("\"" + t.s + "\""); return true;
        case Tok::PI:  emit("#PI"); return true;
        case Tok::FN_HEX: emit("HEX(" + hex_bytes(t.s) + ")"); return true;

        case Tok::VAR:
            if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
            emit(nm);
            if (t.indexed) return indices();
            return true;
        case Tok::ARRAY:
            if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
            emit(nm + "()");
            return true;

        case Tok::LPAR: {
            emit("(");
            if (!expr()) return false;
            Tok c;
            if (!ex_.take(c, false) || c.t != Tok::RPAR)
                return fail("скобка не закрыта");
            emit(")");
            return true;
        }

        case Tok::FN_ABS: return call("ABS", 1, 1, true);
        case Tok::FN_INT: return call("INT", 1, 1, true);
        case Tok::FN_SGN: return call("SGN", 1, 1, true);
        case Tok::FN_SQR: return call("SQR", 1, 1, true);
        case Tok::FN_LOG: return call("LOG", 1, 1, true);
        case Tok::FN_EXP: return call("EXP", 1, 1, true);
        case Tok::FN_SIN:  return call("SIN", 1, 1, true);
        case Tok::FN_COS:  return call("COS", 1, 1, true);
        case Tok::FN_TAN:  return call("TAN", 1, 1, true);
        case Tok::FN_ASIN: return call("ARCSIN", 1, 1, true);
        case Tok::FN_ACOS: return call("ARCCOS", 1, 1, true);
        case Tok::FN_ATAN: return call("ARCTAN", 1, 1, true);
        case Tok::FN_RND: return call("RND", 1, 1, true);
        case Tok::FN_ROUND: return call("ROUND", 2, 2, true);
        case Tok::FN_TAB: return call("TAB", 1, 1, true);
        // У AT( закрывающей скобки в потоке нет.
        case Tok::FN_AT:  return call("AT", 2, 3, false);

        // FN<имя>( — функция пользователя: имя лежит сырым кодом символа,
        // и по таблице имён его искать не надо.
        case Tok::FN_USER: {
            emit(std::string("FN") + static_cast<char>(t.var) + "(");
            if (!expr()) return false;
            Tok c;
            if (!ex_.take(c, false)) return fail(ex_.error());
            if (c.t != Tok::RPAR) return fail("FN( не закрыт");
            emit(")");
            return true;
        }

        case Tok::FN_STR: return substr();
        case Tok::FN_LEN: return implicit("LEN", false, false);
        case Tok::FN_NUM: return implicit("NUM", false, false);
        case Tok::FN_VAL: return implicit("VAL", true,  false);
        case Tok::FN_POS: return implicit("POS", false, true);

        // `#<а.в.>` — номер строки таблицы устройств.
        case Tok::HASH: emit("#"); return operand();

        case Tok::MINUS: emit("-"); return operand_expected ? operand() : true;
        case Tok::PLUS:  emit("+"); return true;
        case Tok::STAR:  emit("*"); return true;
        case Tok::CARET: emit("^"); return true;
        case Tok::EQ: emit("="); return true;
        case Tok::NE: emit("<>"); return true;
        case Tok::LT: emit("<"); return true;
        case Tok::LE: emit("<="); return true;
        case Tok::GT: emit(">"); return true;
        case Tok::GE: emit(">="); return true;
        case Tok::AND: emit("AND"); return true;
        case Tok::OR:  emit("OR"); return true;
        case Tok::XOR: emit("XOR"); return true;

        case Tok::SLASH:
            // В позиции операнда `/` — адрес устройства, а не деление.
            if (operand_expected) {
                Tok a;
                if (!ex_.take(a, false) || a.t != Tok::COMMA)
                    return fail("после DC нет DE");
                uint8_t code = 0;
                if (!src_.peek_raw_byte(code)) return fail("после DC DE нет адреса");
                src_.skip(1);
                emit("/");
                emit(std::string(1, HEXD[code >> 4]) + HEXD[code & 15]);
                return true;
            }
            emit("/");
            return true;

        default: break;
    }
    stop = true;
    return true;
}

bool Decoder::expr(bool stop_at_gt)
{
    bool first = true;
    for (;;) {
        Tok t;
        if (!ex_.peek(t, first)) return fail(ex_.error());
        if (t.t == Tok::END) return !first || fail("выражение пусто");

        bool stop = false;
        // Заглядывать можно только в том состоянии, в каком лексема потом
        // будет прочитана (CLAUDE.md): поэтому состояние считаем заранее.
        const bool as_operand = first;
        if (!ex_.peek(t, as_operand)) return fail(ex_.error());
        if (t.t == Tok::COMMA || t.t == Tok::SEMI || t.t == Tok::RPAR
            || t.t == Tok::KW_TO || t.t == Tok::KW_STEP || t.t == Tok::KW_THEN
            || t.t == Tok::KW_GOTO || t.t == Tok::KW_GOSUB
            || (stop_at_gt && t.t == Tok::GT))
            return !first || fail("выражение пусто");

        ex_.consume();
        if (!token(t, as_operand, stop)) return false;
        if (stop) return fail("лексема не кодируется в тексте: " + t.s);
        first = !first;
    }
}

// Встречается ли байт в остатке операндов. Звать только там, где у
// разборщика нет заглянутой лексемы, — иначе позиция уже уехала.
bool rest_has(ByteSource & src, uint8_t what)
{
    const unsigned save = src.pos();
    bool found = false;
    uint8_t b = 0;
    while (src.take_raw_byte(b))
        if (b == what) { found = true; break; }
    src.seek(save);
    return found;
}

bool Decoder::lvalue(bool by_table, bool indices_ok)
{
    Tok t;
    if (!ex_.take(t, true)) return fail(ex_.error());
    if (t.t == Tok::FN_STR) return substr();
    std::string nm;
    if (t.t == Tok::ARRAY) {
        if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
        emit(nm + "()");
        return true;
    }
    if (t.t != Tok::VAR) return fail("приёмником ожидалась переменная");
    if (!name(t.var, nm)) return fail("нет имени для индекса переменной");
    emit(nm);
    bool indexed = indices_ok && (by_table ? t.table_array : t.indexed);
    // У символьной переменной «скаляр или массив» по таблицам не
    // различается, и догадка ошибается в сторону массива (docs/format.md,
    // разд. 6). Но если сразу за приёмником стоит , списка индексов там
    // нет наверняка — этого хватает, чтобы не сломать присваивание.
    uint8_t next = 0;
    if (indexed && src_.peek_raw_byte(next) && next == 0xD9) indexed = false;
    if (indexed) return indices();
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Имена переменных
// ---------------------------------------------------------------------------

namespace {

// Имя по номеру: A, A0…A9, B, B0…B9, … — 286 штук на 26 букв. Индексов в
// потоке не больше 202 (00…C9), так что хватает с запасом.
std::string base_name(unsigned i)
{
    std::string s;
    s += static_cast<char>('A' + i / 11);
    const unsigned d = i % 11;
    if (d) s += static_cast<char>('0' + (d - 1));
    return s;
}

} // namespace


// ---------------------------------------------------------------------------
// Операторы
// ---------------------------------------------------------------------------

namespace {

// Приставка дисковых операторов: буква устройства, `¤`, `/адрес`, `#строка`.
// Обратное к StmtEncoder::disk_prefix из tokenize.cpp.
bool disk_prefix(Decoder & d, ByteSource & src, bool with_device)
{
    uint8_t b = 0;
    if (with_device && src.peek_raw_byte(b) && b <= 2) {
        src.skip(1);
        d.emit(b == 0 ? "F" : (b == 1 ? "R" : "T"));
    }
    if (src.peek_raw_byte(b) && b == 0xD6) { src.skip(1); d.emit("$"); }
    if (src.peek_raw_byte(b) && b == 0xDC) {
        src.skip(1);
        d.emit("/");
        // За `DC` идёт выражение. Однобайтовый литерал `DE hh` пишется
        // двумя шестнадцатеричными цифрами — так адреса и записывают; всё
        // прочее (в корпусе это переменная) выписывается как выражение.
        uint8_t addr = 0;
        if (src.peek_raw_byte(b) && b == 0xDE) {
            src.skip(1);
            if (!src.take_raw_byte(addr)) return false;
            d.emit(std::string(1, HEXD[addr >> 4]) + HEXD[addr & 15]);
            if (src.peek_raw_byte(b) && b == 0xDE) { src.skip(1); d.emit(","); }
        } else {
            if (!d.expr()) return false;
            Tok t;
            if (!d.parser().peek(t, false)) return false;
            if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
            else d.parser().unpeek();
        }
    }
    if (src.peek_raw_byte(b) && b == 0xDB) {
        src.skip(1);
        d.emit("#");
        if (!d.expr()) return false;
        Tok t;
        if (!d.parser().peek(t, false)) return false;
        if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
        // Иначе источник надо вернуть: дальше его читают сырыми байтами, а
        // заглянутая лексема из него уже вынута (CLAUDE.md).
        else d.parser().unpeek();
    }
    return true;
}

// Хвост из номеров строк у SAVE DC и LOAD DC: сырые пары BCD через `DE`.
// Читать его можно только после того, как разборщику вернули заглянутую
// лексему: она уже вынута из источника (CLAUDE.md).
bool line_tail(Decoder & d, ByteSource & src)
{
    d.parser().unpeek();
    bool first = true;
    for (;;) {
        uint8_t b = 0;
        if (!src.peek_raw_byte(b)) return true;
        if (b == 0xDE) { src.skip(1); d.emit(","); first = false; continue; }
        if (src.left() < 2) return true;
        std::string n;
        if (!line_number(src, n)) return false;
        (void)first;
        d.emit(n);
        first = false;
    }
}

// Список выражений через DE до конца операндов.
bool expr_list(Decoder & d, ByteSource & src, std::string & error)
{
    for (;;) {
        if (!d.expr()) { error = d.error(); return false; }
        Tok t;
        if (!d.parser().peek(t, false)) { error = d.error(); return false; }
        if (t.t != Tok::COMMA) return true;
        d.parser().consume();
        d.emit(",");
        if (!d.parser().peek(t, true)) { error = d.error(); return false; }
        if (t.t == Tok::END) return true;
    }
}

// Приёмники идут вплотную, разделителей в потоке нет. Конец спрашиваем у
// разборщика, а не у источника: заглянутая лексема уже прочитана из него,
// и at_end() соврёт (CLAUDE.md).
bool lvalue_list(Decoder & d, ByteSource & src, std::string & error)
{
    (void)src;
    for (bool first = true;; first = false) {
        Tok t;
        if (!d.parser().peek(t, true)) { error = d.error(); return false; }
        if (t.t == Tok::END) return true;
        if (!first) d.emit(",");
        if (!d.lvalue(true)) { error = d.error(); return false; }
    }
}

// Один оператор: глагол уже прочитан, дальше только его операнды.
bool decode_stmt(unsigned verb, const uint8_t * ops, unsigned len,
                 const NameTable & names, std::string & out, std::string & error)
{
    // REM и % забирают операнды сырым текстом.
    if (verb == 0x56 || verb == 0x3F) {
        out += (verb == 0x56) ? "REM" : "%";
        out.append(reinterpret_cast<const char *>(ops), len);
        return true;
    }

    // Плоский DATA: значения идут вплотную, последние два байта — адрес
    // следующего оператора DATA, машина заполняет его сама. Читаем поток
    // короче на эти два байта — тогда они просто не видны.
    if (verb == 0x29) {
        if (len < 2) { error = "DATA без цепочки"; return false; }
        ByteSource dsrc(ops, len - 2, &names.vars());
        Decoder dd(dsrc, names, out);
        dd.emit("DATA ");
        // Берётся ровно один операнд, а не выражение: разделителей между
        // значениями нет, и полный разбор прочитал бы `E7` следующей
        // константы как `AND` — в позиции операции это она и есть.
        for (bool first = true; !dsrc.at_end(); first = false) {
            if (!first) dd.emit(",");
            if (!dd.operand()) { error = dd.error(); return false; }
        }
        return true;
    }

    ByteSource src(ops, len, &names.vars());
    Decoder d(src, names, out);

    switch (verb) {
        case 0x59: d.emit("END"); return true;
        case 0x5E: d.emit("RETURN"); return true;
        case 0x21: case 0x22: {
            d.emit(verb == 0x21 ? "GOTO " : "GOSUB ");
            std::string n;
            if (!line_number(src, n)) { error = "переход без номера строки"; return false; }
            d.emit(n);
            return true;
        }
        case 0x2C: {                                   // CLEAR
            d.emit("CLEAR");
            uint8_t code = 0;
            if (!src.peek_raw_byte(code)) return true;
            src.skip(1);
            if (code == 0x11) { d.emit(" V"); return true; }
            if (code == 0x12) { d.emit(" N"); return true; }
            if (code != 0x14) { error = "CLEAR: неизвестный вид"; return false; }
            d.emit(" P");
            if (src.at_end()) return true;
            std::string n1;
            if (!line_number(src, n1)) { error = "CLEAR P без номера строки"; return false; }
            d.emit(n1);
            uint8_t b = 0;
            if (!src.peek_raw_byte(b) || b != 0xDE) return true;
            src.skip(1);
            d.emit(",");
            std::string n2;
            if (!line_number(src, n2)) { error = "CLEAR P без номера строки"; return false; }
            d.emit(n2);
            return true;
        }

        case 0x33:                                     // LIST S
        case 0x2E: {                                   // LIST
            d.emit(verb == 0x33 ? "LIST S" : "LIST");
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xDC) {
                src.skip(1);
                uint8_t de = 0, addr = 0;
                if (!src.take_raw_byte(de) || de != 0xDE) {
                    error = "LIST: после / нет DE";
                    return false;
                }
                if (!src.take_raw_byte(addr)) { error = "LIST без адреса"; return false; }
                d.emit(" /" + std::string(1, HEXD[addr >> 4]) + HEXD[addr & 15]);
                if (src.peek_raw_byte(b) && b == 0xDE) { src.skip(1); d.emit(","); }
            }
            if (src.at_end()) return true;
            std::string n1;
            if (!line_number(src, n1)) { error = "LIST без номера строки"; return false; }
            d.emit(" " + n1);
            if (!src.peek_raw_byte(b) || b != 0xDE) return true;
            src.skip(1);
            d.emit(",");
            std::string n2;
            if (!line_number(src, n2)) { error = "LIST без номера строки"; return false; }
            d.emit(n2);
            return true;
        }

        case 0x2F: {
            d.emit("RUN");
            if (len) {
                std::string n;
                if (!line_number(src, n)) { error = "RUN без номера строки"; return false; }
                d.emit(" " + n);
            }
            return true;
        }
        case 0x52:                                     // NEXT
            d.emit("NEXT ");
            if (!d.lvalue()) { error = d.error(); return false; }
            return true;

        case 0x42:                                     // STOP, с сообщением или без
            d.emit("STOP");
            if (!len) return true;
            d.emit(" ");
            if (!d.expr()) { error = d.error(); return false; }
            return true;

        case 0x36: {                                   // присваивание
            for (;;) {
                if (!d.lvalue(true)) { error = d.error(); return false; }
                Tok t;
                // Заглядывать надо в том же состоянии, в каком лексема потом
                // будет прочитана: дальше либо `=`, либо очередная цель, а
                // это позиция операнда (CLAUDE.md).
                if (!d.parser().peek(t, true)) { error = d.error(); return false; }
                if (t.t == Tok::EQ) { d.parser().consume(); break; }
                d.emit(",");
            }
            d.emit("=");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x4C: {                                   // PRINT
            d.emit("PRINT ");
            if (!len) return true;
            for (;;) {
                Tok t;
                // Разделитель читается в позиции операции: `DE` там запятая,
                // а в позиции операнда — однобайтовый литерал, и `PRINT ,F5%`
                // (`4C 03 DE 3D DD`, SMAL2 262) разбирался как литерал 3D.
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::END) break;
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); continue; }
                if (t.t == Tok::SEMI) { d.parser().consume(); d.emit(";"); continue; }
                d.parser().unpeek();
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
                else if (t.t == Tok::SEMI) { d.parser().consume(); d.emit(";"); }
                else break;
            }
            return true;
        }

        case 0x41: {                                   // INPUT
            d.emit("INPUT ");
            Tok t;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            if (t.t == Tok::STR) {
                d.parser().consume();
                d.emit("\"" + t.s + "\",");
            }
            // Приёмники идут вплотную, разделителей в потоке нет
            // (docs/format.md, разд. 4).
            return lvalue_list(d, src, error);
        }

        case 0x1E: {                                   // IF END THEN <строка>
            d.emit("IF END THEN ");
            std::string n;
            if (!line_number(src, n)) { error = "IF END THEN без номера строки"; return false; }
            d.emit(n);
            return true;
        }

        case 0x24: {                                   // IF … THEN <строка>
            d.emit("IF ");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::KW_THEN) {
                error = "IF без THEN";
                return false;
            }
            long n = 0;
            t.num.to_int(n);
            d.emit("THEN" + dec(static_cast<unsigned>(n)));
            return true;
        }

        case 0x57: {                                   // FOR
            d.emit("FOR ");
            if (!d.lvalue()) { error = d.error(); return false; }
            d.emit("=");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::KW_TO) {
                error = "FOR без TO";
                return false;
            }
            d.emit("TO");
            if (!d.expr()) { error = d.error(); return false; }
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::KW_STEP) {
                d.parser().consume();
                d.emit("STEP");
                if (!d.expr()) { error = d.error(); return false; }
            }
            return true;
        }

        case 0x26: {                                   // ON … GOTO/GOSUB
            d.emit("ON ");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::KW_GOTO) d.emit("GOTO");
            else if (t.t == Tok::KW_GOSUB) d.emit("GOSUB");
            else { error = "ON без GOTO или GOSUB"; return false; }
            for (bool first = true; !src.at_end(); first = false) {
                if (!first) d.emit(",");
                std::string n;
                if (!line_number(src, n)) { error = "ON без номеров строк"; return false; }
                d.emit(n);
            }
            return true;
        }

        case 0x46: case 0x4E: {                        // DIM и COM
            // Размеры лежат только в таблицах переменных — оттуда их и берём.
            d.emit(verb == 0x46 ? "DIM " : "COM ");
            for (unsigned i = 0; i < len; ++i) {
                if (i) d.emit(",");
                const unsigned v = ops[i];
                std::string nm;
                if (!d.name(v, nm)) { error = "нет имени для индекса переменной"; return false; }
                d.emit(nm);
                if (v >= names.vars().size()) { error = "индекс вне таблиц"; return false; }
                const VarInfo & vi = names.vars()[v];
                if (vi.is_array) {
                    d.emit("(" + dec(vi.dim1));
                    if (vi.dim2) d.emit("," + dec(vi.dim2));
                    d.emit(")");
                }
                if (vi.is_string && vi.str_len) d.emit(dec(vi.str_len));
            }
            return true;
        }


        case 0x30:                                     // RETURN CLEAR
            d.emit("RETURN CLEAR");
            if (len) {
                uint8_t b = 0;
                if (!src.take_raw_byte(b) || b != 0xCB) { error = "RETURN CLEAR: не ALL"; return false; }
                d.emit(" ALL");
            }
            return true;

        case 0x44:                                     // READ
            d.emit("READ ");
            for (bool first = true; !src.at_end(); first = false) {
                if (!first) d.emit(",");
                if (!d.lvalue()) { error = d.error(); return false; }
            }
            return true;

        case 0x51: {                                   // RESTORE
            // Все четыре формы разд. 4.9: без параметров, с номером
            // константы, с номером строки и с обоими.
            d.emit("RESTORE");
            if (src.at_end()) return true;
            d.emit(" ");
            Tok t;
            bool comma = false;
            uint8_t b = 0;
            // Запятая в начале операндов — сырой `DE`: в позиции операнда
            // разбор принял бы его за однобайтовый литерал и съел старший
            // байт номера строки (VICT 2190 = `51 03 DE 20 40`).
            if (src.peek_raw_byte(b) && b == 0xDE) {
                src.skip(1);
                comma = true;
            } else {
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.parser().consume(); comma = true; }
                else d.parser().unpeek();
            }
            if (!comma) return true;
            d.emit(",");
            std::string n;
            if (!line_number(src, n)) { error = "RESTORE без номера строки"; return false; }
            d.emit(n);
            return true;
        }

        case 0x25: {                                   // KEYIN
            d.emit("KEYIN ");
            // За приёмником сразу идут два номера строк парами BCD, и
            // заглядывание приняло бы их за список индексов (EDITOR 2505).
            // Пять байт операндов — это голый индекс и четыре байта номеров.
            if (len == 5) {
                uint8_t v = 0;
                std::string nm;
                if (!src.take_raw_byte(v) || !d.name(v, nm)) {
                    error = "KEYIN: нет приёмника";
                    return false;
                }
                d.emit(nm);
            } else if (!d.lvalue(true)) { error = d.error(); return false; }
            for (unsigned k = 0; k < 2; ++k) {
                std::string n;
                d.emit(",");
                if (!line_number(src, n)) { error = "KEYIN без номеров строк"; return false; }
                d.emit(n);
            }
            return true;
        }

        case 0x23: {                                   // GOSUB'
            uint8_t label = 0;
            if (!src.take_raw_byte(label)) { error = "GOSUB' без метки"; return false; }
            d.emit("GOSUB '" + dec(label));
            if (src.at_end()) return true;
            d.emit("(");
            for (;;) {
                if (!d.expr()) { error = d.error(); return false; }
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) break;
                d.parser().consume();
                d.emit(",");
            }
            d.emit(")");
            return true;
        }

        case 0x5A: {                                   // DEFFN — своя функция
            uint8_t nm = 0;
            if (!src.take_raw_byte(nm)) { error = "DEFFN без имени"; return false; }
            // Два байта рабочего поля машина заполняет при исполнении.
            uint8_t skip = 0;
            for (unsigned k = 0; k < 2; ++k)
                if (!src.take_raw_byte(skip)) { error = "DEFFN оборван"; return false; }
            uint8_t formal = 0;
            if (!src.take_raw_byte(formal))
                { error = "DEFFN без формальной переменной"; return false; }
            std::string nv;
            if (!d.name(formal, nv)) { error = "нет имени для индекса переменной"; return false; }
            d.emit(std::string("DEFFN ") + static_cast<char>(nm) + "(" + nv + ")=");
            return d.expr();
        }

        case 0x27: case 0x3A: {                        // DEFFN' и DEFFN' с текстом
            uint8_t label = 0;
            if (!src.take_raw_byte(label)) { error = "DEFFN' без метки"; return false; }
            // Четыре байта адреса возврата машина заполняет при исполнении;
            // в тексте их нет и восстановить их нечем.
            uint8_t skip = 0;
            for (unsigned k = 0; k < 4; ++k)
                if (!src.take_raw_byte(skip)) { error = "DEFFN' без адреса"; return false; }
            d.emit("DEFFN '" + dec(label));
            if (src.at_end()) return true;

            if (verb == 0x3A) {
                Tok t;
                if (!d.parser().take(t, true) || t.t != Tok::STR) {
                    error = "DEFFN': нет текста клавиши";
                    return false;
                }
                d.emit("\"" + t.s + "\"");
                return true;
            }
            // Формальные параметры идут вплотную, разделителей в потоке нет.
            d.emit("(");
            for (bool first = true; !src.at_end(); first = false) {
                if (!first) d.emit(",");
                Tok t;
                if (!d.parser().take(t, true) || t.t != Tok::VAR) {
                    error = "DEFFN': ждали формальный параметр";
                    return false;
                }
                std::string nm;
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm);
            }
            d.emit(")");
            return true;
        }

        case 0x64: {                                   // INIT(
            Tok t;
            if (!d.parser().take(t, true)) { error = d.error(); return false; }
            // Код задаётся числом или знаком в кавычках (EDITOR 454).
            if (t.t == Tok::NUM) d.emit("INIT (" + t.s + ")");
            else if (t.t == Tok::STR) d.emit("INIT (\"" + t.s + "\")");
            else { error = "INIT без кода"; return false; }
            // Разделителей между приёмниками нет вовсе, поэтому «скаляр или
            // массив» решается строго по таблицам: заглядывание приняло бы
            // индекс следующего приёмника за список индексов (ловушка 3).
            for (bool first = true; !src.at_end(); first = false) {
                if (!first) d.emit(",");
                if (!d.lvalue(true)) { error = d.error(); return false; }
            }
            return true;
        }

        case 0x37: {                                   // COM CLEAR
            d.emit("COM CLEAR");
            if (src.at_end()) return true;
            d.emit(" ");
            Tok t;
            if (!d.parser().take(t, true)) { error = d.error(); return false; }
            std::string nm;
            if (t.t == Tok::ARRAY) {
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm + "()");
                return true;
            }
            if (t.t == Tok::VAR) {
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm);
                return true;
            }
            error = "COM CLEAR: ждали переменную либо массив";
            return false;
        }

        case 0x34: {                                   // ON ERROR
            d.emit("ON ERROR ");
            if (!len) return true;
            Tok t;
            // Приёмники кода и номера строки есть не всегда, а `CC`/`CD`/`D3`
            // двузначны: в позиции операнда они значат не то. Смотрим сырой
            // байт — это единственное состояние, в котором он однозначен.
            uint8_t first_byte = 0;
            const bool with_targets =
                src.peek_raw_byte(first_byte) &&
                first_byte != 0xCC && first_byte != 0xCD && first_byte != 0xD3;
            if (with_targets) {
                for (unsigned k = 0; k < 2; ++k) {
                    if (k) d.emit(",");
                    if (!d.parser().take(t, true) || t.t != Tok::VAR) {
                        error = "ON ERROR: ждали переменную";
                        return false;
                    }
                    std::string nm;
                    if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                    d.emit(nm);
                }
            }
            if (!d.parser().take(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::KW_THEN) {
                long n = 0;
                t.num.to_int(n);
                d.emit("THEN" + dec(static_cast<unsigned>(n)));
                return true;
            }
            if (t.t == Tok::KW_GOTO) d.emit("GOTO");
            else if (t.t == Tok::KW_GOSUB) d.emit("GOSUB");
            else { error = "ON ERROR без GOTO, THEN или GOSUB"; return false; }
            std::string n;
            if (!line_number(src, n)) { error = "ON ERROR без номера строки"; return false; }
            d.emit(n);
            return true;
        }

        case 0x50: {                                   // HEXPRINT
            d.emit("HEXPRINT ");
            if (src.at_end()) return true;
            for (;;) {
                if (!d.expr()) { error = d.error(); return false; }
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
                else if (t.t == Tok::SEMI) { d.parser().consume(); d.emit(";"); }
                else { d.parser().unpeek(); break; }
                if (src.at_end()) break;
            }
            return true;
        }

        case 0x28: {                                   // PRINTUSING
            d.emit("PRINTUSING ");
            if (!d.expr()) { error = d.error(); return false; }
            for (;;) {
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
                else if (t.t == Tok::SEMI) { d.parser().consume(); d.emit(";"); }
                else break;
                if (src.at_end()) break;
                if (!d.expr()) { error = d.error(); return false; }
            }
            return true;
        }

        case 0x4D: {                                   // ROTATE
            d.emit("ROTATE ");
            Tok t;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            if (t.t == Tok::GT) { d.parser().consume(); d.emit("C"); }
            if (!d.parser().take(t, true) || t.t != Tok::LPAR) {
                error = "ROTATE без скобки";
                return false;
            }
            d.emit("(");
            for (;;) {
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().take(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.emit(","); continue; }
                if (t.t != Tok::RPAR) { error = "ROTATE: скобка не закрыта"; return false; }
                break;
            }
            d.emit(")");
            return true;
        }

        case 0x48: case 0x5D: {                        // PACK( и UNPACK(
            Tok t;
            if (!d.parser().take(t, true) || t.t != Tok::STR) {
                error = "PACK без образа";
                return false;
            }
            const bool un = (verb == 0x5D);
            d.emit(un ? "UNPACK(" : "PACK(");
            d.emit(t.s + ")");
            if (un) {
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().take(t, false) || t.t != Tok::KW_TO) {
                    error = "UNPACK без TO";
                    return false;
                }
                d.emit("TO");
                for (bool first = true; !src.at_end(); first = false) {
                    if (!first) d.emit(",");
                    if (!d.lvalue()) { error = d.error(); return false; }
                    if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                    if (t.t != Tok::COMMA) break;
                    d.parser().consume();
                }
                return true;
            }
            if (!d.lvalue()) { error = d.error(); return false; }
            uint8_t from = 0;
            if (!src.take_raw_byte(from) || from != 0xCA) {
                error = "PACK без FROM";
                return false;
            }
            d.emit("FROM");
            for (;;) {
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) break;
                d.parser().consume();
                d.emit(",");
            }
            return true;
        }

        case 0x0624: {                                 // LINPUT
            d.emit("LINPUT ");
            Tok t;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            if (t.t == Tok::STR) {
                d.parser().consume();
                d.emit("\"" + t.s + "\",");
                if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            }
            if (t.t == Tok::MINUS) { d.parser().consume(); d.emit("-"); }
            if (!d.lvalue()) { error = d.error(); return false; }
            return true;
        }

        case 0x0626: {                                 // REPLACE
            d.emit("REPLACE ");
            if (!d.lvalue()) { error = d.error(); return false; }
            for (;;) {
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) break;
                d.parser().consume();
                d.emit(",");
                if (!d.expr()) { error = d.error(); return false; }
            }
            return true;
        }

        case 0x0600: {                                 // PLOT
            // Группы `D7` … `D4`, элементы и группы через `DE`; любой
            // элемент может быть пуст. Третий — перо: байты `E5`…`E9` это
            // буквы `U`, `D`, `R`, `S`, `C` (пара SLIDE/SL2, строки
            // 5650–5690 против 5660–5700).
            d.emit("PLOT ");
            for (bool firstg = true; ; firstg = false) {
                uint8_t b = 0;
                if (!firstg) d.emit(",");
                if (!src.take_raw_byte(b) || b != 0xD7) {
                    error = "PLOT: группа не открыта";
                    return false;
                }
                d.emit("<");
                unsigned k = 0;
                for (;;) {
                    if (!src.peek_raw_byte(b)) { error = "PLOT: группа не закрыта"; return false; }
                    if (b == 0xD4) { src.skip(1); break; }
                    if (b == 0xDE) { src.skip(1); d.emit(","); ++k; continue; }

                    bool pen = false;
                    if (k == 2 && b >= 0xE5 && b <= 0xE9) {
                        const unsigned save = src.pos();
                        src.skip(1);
                        uint8_t nx = 0;
                        if (src.peek_raw_byte(nx) && (nx == 0xD4 || nx == 0xDE)) {
                            static const char PEN[] = "UDRSC";
                            d.emit(std::string(1, PEN[b - 0xE5]));
                            pen = true;
                        } else {
                            src.set_pos(save);
                        }
                    }
                    if (!pen) {
                        if (!d.expr(true)) { error = d.error(); return false; }
                        d.parser().unpeek();
                    }
                }
                d.emit(">");
                if (!src.peek_raw_byte(b) || b != 0xDE) return true;
                src.skip(1);
            }
        }

        case 0x0605: {                                 // MAT PRINT
            d.emit("MAT PRINT ");
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xDC) {
                src.skip(1);
                uint8_t de = 0, a = 0;
                if (!src.take_raw_byte(de) || de != 0xDE || !src.take_raw_byte(a)) {
                    error = "MAT PRINT: нет адреса устройства";
                    return false;
                }
                d.emit(std::string("/") + HEXD[a >> 4] + HEXD[a & 15]);
                if (src.peek_raw_byte(b) && b == 0xDE) { src.skip(1); d.emit(","); }
            }
            for (;;) {
                Tok t;
                if (!d.parser().take(t, true)) { error = d.error(); return false; }
                if (t.t != Tok::ARRAY && t.t != Tok::VAR) {
                    error = "MAT PRINT: ждали массив";
                    return false;
                }
                std::string nm;
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm);
                if (!src.peek_raw_byte(b)) return true;
                if (b == 0xDD) { src.skip(1); d.emit(";"); }
                else if (b == 0xDE) { src.skip(1); d.emit(","); }
                else return true;
                if (src.at_end()) return true;
            }
        }

        case 0x0603:                                   // MAT READ
        case 0x0604: {                                 // MAT INPUT
            d.emit(verb == 0x0603 ? "MAT READ " : "MAT INPUT ");
            for (bool first = true; ; first = false) {
                if (!first) d.emit(",");
                Tok t;
                if (!d.parser().take(t, true)) { error = d.error(); return false; }
                if (t.t != Tok::ARRAY && t.t != Tok::VAR) {
                    error = "MAT READ/INPUT: ждали массив";
                    return false;
                }
                std::string nm;
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm);
                // Новые размерности необязательны: в потоке они видны
                // открывающей скобкой `EB`.
                uint8_t b = 0;
                if (src.peek_raw_byte(b) && b == 0xEB) {
                    src.skip(1);
                    d.emit("(");
                    if (!expr_list(d, src, error)) return false;
                    if (!d.parser().take(t, false) || t.t != Tok::RPAR) {
                        error = "MAT READ/INPUT: скобка не закрыта";
                        return false;
                    }
                    d.emit(")");
                    // Длина элемента необязательна, а за ней может сразу
                    // стоять запятая между записями. Заглядывать надо в
                    // позиции операции: `DE` в позиции операнда — не
                    // запятая, а однобайтовый литерал (CLAUDE.md, ловушка 2).
                    if (!src.at_end()) {
                        if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                        d.parser().unpeek();
                        if (t.t != Tok::COMMA && t.t != Tok::END) {
                            if (!d.expr()) { error = d.error(); return false; }
                        }
                    }
                }
                if (src.at_end()) return true;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) { d.parser().unpeek(); return true; }
                d.parser().consume();
            }
        }

        case 0x0602: {                                 // MAT REDIM
            d.emit("MAT REDIM ");
            for (bool first = true;; first = false) {
                if (!first) d.emit(",");
                Tok t;
                if (!d.parser().take(t, true) || t.t != Tok::ARRAY) {
                    error = "MAT REDIM без массива";
                    return false;
                }
                std::string nm;
                if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
                d.emit(nm);
                if (!d.parser().take(t, true) || t.t != Tok::LPAR) {
                    error = "MAT REDIM без размерностей";
                    return false;
                }
                d.emit("(");
                for (;;) {
                    if (!d.expr()) { error = d.error(); return false; }
                    if (!d.parser().take(t, false)) { error = d.error(); return false; }
                    if (t.t == Tok::COMMA) { d.emit(","); continue; }
                    if (t.t != Tok::RPAR) { error = "MAT REDIM: скобка не закрыта"; return false; }
                    break;
                }
                d.emit(")");
                if (src.at_end()) break;
                // За скобкой может стоять длина элемента, а может — запятая.
                if (!d.parser().peek(t, true)) { error = d.error(); return false; }
                if (t.t == Tok::END) break;
                if (t.t != Tok::COMMA) {
                    if (!d.expr()) { error = d.error(); return false; }
                    if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                    if (t.t == Tok::END) break;
                }
                if (t.t != Tok::COMMA) { error = "MAT REDIM: нет запятой"; return false; }
                d.parser().consume();
            }
            return true;
        }


        case 0x47: {                                   // CONVERT
            d.emit("CONVERT ");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::KW_TO) {
                error = "CONVERT без TO";
                return false;
            }
            d.emit("TO");
            if (!d.lvalue()) { error = d.error(); return false; }
            if (src.at_end()) return true;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            if (t.t == Tok::STR) {
                d.parser().consume();
                d.emit(",(" + t.s + ")");
                return true;
            }
            d.emit(",(");
            if (!d.expr()) { error = d.error(); return false; }
            d.emit(")");
            return true;
        }

        case 0x4B: {                                   // BIN(
            d.emit("BIN(");
            if (!d.lvalue(true)) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::COMMA) {
                d.parser().consume();
                Tok two;
                if (!d.parser().take(two, true) || two.t != Tok::HASH) {
                    error = "BIN(: второй аргумент не DB";
                    return false;
                }
                d.emit(",2");
            } else {
                // За приёмником сразу идёт значение, и заглянули мы в позиции
                // операции: источник надо вернуть (CLAUDE.md, ловушка 2).
                d.parser().unpeek();
            }
            d.emit(")=");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x43: case 0x61: case 0x62:               // AND( OR( XOR(
        case 0x45: case 0x4A: case 0x63: {             // BOOL, ADD и ADD C
            if (verb == 0x45) {
                // Впереди цифра операции (LКОПДИСК 4243).
                uint8_t x = 0;
                if (!src.take_raw_byte(x)) { error = "BOOL без кода операции"; return false; }
                d.emit("BOOL" + std::string(1, HEXD[x & 15]) + "(");
            } else if (verb == 0x4A) {
                d.emit("ADD(");
            } else if (verb == 0x63) {
                // Сложение с переносом между байтами — свой глагол, а не
                // признак у ADD (docs/format.md, разд. 5).
                d.emit("ADD C(");
            } else {
                d.emit(verb == 0x43 ? "AND(" : (verb == 0x61 ? "OR(" : "XOR("));
            }
            // Приёмник индексируется только по таблицам: за ним сразу идёт
            // второй аргумент, и заглядывание приняло бы его индекс за
            // список индексов приёмника (CLAUDE.md, ловушка 3 — та же, что
            // у `BIN(` и `INIT`).
            if (!d.lvalue(true)) { error = d.error(); return false; }
            d.emit(",");
            // Второй аргумент — либо однобайтовый литерал `DE hh`, либо
            // вторая переменная; разделителя между ними в потоке нет.
            if (!d.operand()) { error = d.error(); return false; }
            d.emit(")");
            return true;
        }

        case 0x54: {                                   // SELECT
            d.emit("SELECT ");
            for (bool first = true;; first = false) {
                if (!first) d.emit(",");
                uint8_t code = 0;
                if (!src.take_raw_byte(code)) { error = "SELECT без группы"; return false; }
                bool disk = false;
                if (code == 0x00) {                    // #n
                    uint8_t row = 0, addr = 0;
                    if (!src.take_raw_byte(row) || !src.take_raw_byte(addr)) {
                        error = "SELECT #: нет адреса";
                        return false;
                    }
                    d.emit("#" + dec(row)
                           + std::string(1, HEXD[addr >> 4]) + HEXD[addr & 15]);
                    disk = true;
                } else if (code == 0x01 || code == 0x02 || code == 0x03) {
                    // Единицы измерения углов (разд. 4.6): адреса за ними
                    // не идёт.
                    d.emit(code == 0x01 ? "D" : code == 0x02 ? "R" : "G");
                } else if (code == 0x05) {             // P<цифра>
                    d.emit("P");
                    uint8_t p = 0;
                    if (src.peek_raw_byte(p) && p <= 9) { src.skip(1); d.emit(dec(p)); }
                } else {
                    const char * word = 0;
                    switch (code) {
                        case 0x06: word = "LIST"; break;
                        case 0x07: word = "PRINT"; break;
                        case 0x08: word = "PLOT"; break;
                        case 0x09: word = "TAPE"; break;
                        case 0x0A: word = "DISK"; disk = true; break;
                        case 0x0C: word = "CO"; break;
                        default: break;
                    }
                    if (!word) {
                        error = "SELECT: неизвестный код группы "
                              + std::string(1, HEXD[code >> 4]) + HEXD[code & 15];
                        return false;
                    }
                    uint8_t addr = 0;
                    if (!src.take_raw_byte(addr)) { error = "SELECT: нет адреса"; return false; }
                    d.emit(word);
                    d.emit(std::string(1, HEXD[addr >> 4]) + HEXD[addr & 15]);
                }
                uint8_t b = 0;
                if (disk) {
                    if (src.peek_raw_byte(b) && (b == 0x00 || b == 0x01)) {
                        src.skip(1);
                        d.emit(b ? "R" : "F");
                    }
                } else if (src.peek_raw_byte(b) && b == 0xEB) {
                    src.skip(1);
                    uint8_t hi = 0, lo = 0;
                    if (!src.take_raw_byte(hi) || !src.take_raw_byte(lo)) {
                        error = "SELECT: нет ширины строки";
                        return false;
                    }
                    d.emit("(" + dec((static_cast<unsigned>(hi) << 8) | lo) + ")");
                }
                if (src.at_end()) break;
                if (!src.peek_raw_byte(b) || b != 0xDE) break;
                src.skip(1);
            }
            return true;
        }

        // Дисковые операторы. Раскладка приставки и списков — та же, что у
        // StmtEncoder в tokenize.cpp.
        case 0x75: case 0x78: {                        // DATA LOAD/SAVE DC OPEN
            d.emit(verb == 0x75 ? "DATA LOAD DC OPEN " : "DATA SAVE DC OPEN ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            if (verb == 0x78) {
                Tok t;
                if (!d.parser().take(t, true) || t.t != Tok::LPAR) {
                    error = "DATA SAVE DC OPEN без размера";
                    return false;
                }
                d.emit("(");
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().take(t, false) || t.t != Tok::RPAR) {
                    error = "DATA SAVE DC OPEN: скобка не закрыта";
                    return false;
                }
                d.emit(")");
            }
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x74: {                                   // DATA LOAD DC
            d.emit("DATA LOAD DC ");
            if (!disk_prefix(d, src, false)) { error = "приставка устройства"; return false; }
            return lvalue_list(d, src, error);
        }

        case 0x76: case 0x77: {                        // DATA SAVE DC [CLOSE]
            d.emit("DATA SAVE DC ");
            if (verb == 0x77) d.emit("CLOSE ");
            if (!disk_prefix(d, src, false)) { error = "приставка устройства"; return false; }
            if (verb == 0x77) {
                uint8_t a = 0;
                if (src.peek_raw_byte(a) && a == 0xCB) { src.skip(1); d.emit("ALL"); }
                return true;
            }
            if (src.at_end()) return true;
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xD7) { src.skip(1); d.emit("END"); return true; }
            return expr_list(d, src, error);
        }

        case 0x66: case 0x68: case 0x6E: case 0x6F:    // BT, BA, DA
        case 0x70: case 0x71: {
            const bool load = (verb == 0x66 || verb == 0x70 || verb == 0x71);
            const char * how = (verb == 0x66 || verb == 0x68) ? "BT"
                             : ((verb == 0x6E || verb == 0x70) ? "BA" : "DA");
            d.emit(std::string("DATA ") + (load ? "LOAD " : "SAVE ") + how + " ");
            // У `BT` приставка это только `/адрес` или `#строка`: буквы
            // устройства там не бывает, и байт `00`…`02` в начале — уже
            // значение, а не `F`/`R`/`T`.
            const bool block = (verb == 0x66 || verb == 0x68);
            if (!disk_prefix(d, src, !block)) { error = "приставка устройства"; return false; }
            Tok t;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            if (t.t == Tok::END) return true;
            if (t.t == Tok::LPAR) {
                d.parser().consume();
                d.emit("(");
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) {
                    // Приёмник адреса за последним занятым сектором.
                    d.parser().consume();
                    d.emit(",");
                    if (!d.lvalue()) { error = d.error(); return false; }
                }
                if (!d.parser().take(t, false) || t.t != Tok::RPAR) {
                    error = "обмен по адресу: скобка не закрыта";
                    return false;
                }
                d.emit(")");
            } else {
                // Заглянутую лексему надо вернуть: конец операндов
                // спрашивают у разборщика, а не у источника (CLAUDE.md).
                d.parser().unpeek();
            }
            if (src.at_end()) return true;
            uint8_t e = 0;
            if (!load && src.peek_raw_byte(e) && e == 0xD7) {
                src.skip(1);
                d.emit("END");
                return true;
            }
            return load ? lvalue_list(d, src, error) : expr_list(d, src, error);
        }

        case 0x79: case 0x7A: {                        // DBACKSPACE и DSKIP
            d.emit(verb == 0x79 ? "DBACKSPACE " : "DSKIP ");
            if (!disk_prefix(d, src, false)) { error = "приставка устройства"; return false; }
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xD6) { src.skip(1); d.emit("BEG"); return true; }
            if (src.peek_raw_byte(b) && b == 0xD7) { src.skip(1); d.emit("END"); return true; }
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x7B: {                                   // LIMITS
            d.emit("LIMITS ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            // Форма с именем файла отличается тем, что первый операнд
            // символьный (руководство, разд. 18.8.3).
            Tok t;
            if (!d.parser().peek(t, true)) { error = d.error(); return false; }
            const bool named = (t.t == Tok::STR)
                || ((t.t == Tok::VAR || t.t == Tok::ARRAY)
                    && t.var < names.vars().size() && names.vars()[t.var].is_string);
            if (named) {
                // Имя — один операнд, и **индексируется он строго по
                // таблицам**: за ним идут приёмники вплотную, без
                // разделителей, и заглядывание вперёд примет индекс первого
                // приёмника за список индексов имени. Литерал такой беды не
                // знает, а переменную приходится брать приёмником.
                if (t.t == Tok::STR) {
                    if (!d.operand()) { error = d.error(); return false; }
                } else {
                    if (!d.lvalue(true)) { error = d.error(); return false; }
                }
                d.emit(",");
            }
            return lvalue_list(d, src, error);
        }

        case 0x81: {                                   // SCRATCH
            d.emit("SCRATCH ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            return expr_list(d, src, error);
        }

        case 0x82: {                                   // SCRATCH DISK
            d.emit("SCRATCH DISK ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0x06) {
                src.skip(1);
                uint8_t eq = 0;
                if (!src.take_raw_byte(eq) || eq != 0xD9) { error = "SCRATCH DISK: LS без ="; return false; }
                d.emit("LS=");
                if (!d.expr()) { error = d.error(); return false; }
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); }
            }
            if (!src.take_raw_byte(b) || b != 0xD7) { error = "SCRATCH DISK без END"; return false; }
            uint8_t eq = 0;
            if (!src.take_raw_byte(eq) || eq != 0xD9) { error = "SCRATCH DISK: END без ="; return false; }
            d.emit("END=");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x6D: {                                   // COPY … TO …
            d.emit("COPY ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            Tok t;
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t != Tok::KW_TO) {
                d.parser().unpeek();
                d.emit("(");
                if (!expr_list(d, src, error)) return false;
                d.emit(")");
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            }
            if (t.t != Tok::KW_TO) { error = "COPY без TO"; return false; }
            d.parser().consume();
            d.emit(" TO ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            if (src.at_end()) return true;
            d.emit("(");
            if (!d.expr()) { error = d.error(); return false; }
            d.emit(")");
            return true;
        }

        case 0x83: {                                   // VERIFY
            d.emit("VERIFY ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            if (src.at_end()) return true;
            d.emit("(");
            if (!expr_list(d, src, error)) return false;
            d.emit(")");
            return true;
        }

        case 0x7C: {                                   // LIST DC
            d.emit("LIST DC ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            if (src.at_end()) return true;
            // За приставкой бывает имя файла: `LIST DC F"CHANAL"` (CHANAL 1).
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x73: {                                   // SAVE DA
            d.emit("SAVE DA ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            Tok t;
            if (!d.parser().take(t, true) || t.t != Tok::LPAR) {
                error = "SAVE DA без адреса сектора";
                return false;
            }
            d.emit("(");
            if (!d.expr()) { error = d.error(); return false; }
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::COMMA) {
                d.parser().consume();
                d.emit(",");
                if (!d.lvalue()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            }
            if (t.t != Tok::RPAR) { error = "SAVE DA: скобка не закрыта"; return false; }
            d.parser().consume();
            d.emit(")");
            return true;
        }

        case 0x72: {                                   // LOAD DA
            d.emit("LOAD DA ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            Tok t;
            if (!d.parser().take(t, true) || t.t != Tok::LPAR) {
                error = "LOAD DA без адреса сектора";
                return false;
            }
            d.emit("(");
            if (!d.expr()) { error = d.error(); return false; }
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::COMMA) {
                d.parser().consume();
                d.emit(",");
                if (!d.lvalue()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            }
            if (t.t != Tok::RPAR) { error = "LOAD DA: скобка не закрыта"; return false; }
            d.parser().consume();
            d.emit(")");
            return line_tail(d, src);
        }

        case 0x7D: {                                   // LOAD DC
            d.emit("LOAD DC ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            if (src.at_end()) return true;
            // Имя — один операнд: за ним сразу идут сырые пары BCD, и полное
            // выражение приняло бы их за продолжение.
            if (!d.operand()) { error = d.error(); return false; }
            return line_tail(d, src);
        }

        case 0x80: {                                   // SAVE DC
            d.emit("SAVE DC ");
            if (!disk_prefix(d, src, true)) { error = "приставка устройства"; return false; }
            uint8_t b = 0;
            // `T` и скобка друг от друга не зависят: `CHANAL` 8000 =
            // `00 D6 EB …` — есть скобка, а `T` нет.
            if (src.peek_raw_byte(b) && b == 0xD2) { src.skip(1); d.emit("T"); }
            if (src.peek_raw_byte(b) && b == 0xEB) {
                src.skip(1);
                d.emit("(");
                if (!expr_list(d, src, error)) return false;
                Tok t;
                if (!d.parser().take(t, false) || t.t != Tok::RPAR) {
                    error = "SAVE DC: скобка не закрыта";
                    return false;
                }
                d.emit(")");
            }
            if (src.at_end()) return true;
            // Имя — один операнд: за ним сразу идут сырые пары BCD, и полное
            // выражение приняло бы их за продолжение.
            if (!d.operand()) { error = d.error(); return false; }
            return line_tail(d, src);
        }

        case 0x2A: case 0x2D: {                        // SAVE и LOAD через буфер
            d.emit(verb == 0x2A ? "SAVE " : "LOAD ");
            uint8_t b = 0;
            if (!src.take_raw_byte(b) || b != 0xDD) { error = "SAVE/LOAD без DD"; return false; }
            // За именем буфера идут сырые пары BCD — номера строк, — и
            // заглядывание приняло бы их за список индексов. Таблицы про
            // символьную переменную «скаляр или массив» не говорят вовсе и
            // ошибаются в сторону массива (docs/format.md, разд. 6), поэтому
            // спрашиваем поток: список индексов кончается байтом `D0`, а
            // невалидный BCD `D0` в номере строки стоять не может. Нет `D0` —
            // нет и списка (`EDITOR` 5195 = `2A 07 DD 10 52 15 DE 52 15`).
            if (!d.lvalue(true, rest_has(src, 0xD0))) { error = d.error(); return false; }
            for (bool first = true; !src.at_end(); first = false) {
                if (!first) {
                    uint8_t de = 0;
                    if (!src.take_raw_byte(de) || de != 0xDE) break;
                    d.emit(",");
                }
                std::string n;
                if (!line_number(src, n)) { error = "SAVE/LOAD: неверный номер строки"; return false; }
                d.emit(n);
            }
            return true;
        }

        case 0x40: {                                   // $GIO
            d.emit("$GIO ");
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xDC) {
                if (!disk_prefix(d, src, false)) { error = "приставка устройства"; return false; }
            }
            if (src.peek_raw_byte(b) && b == 0xD5) { src.skip(1); d.emit("'"); }
            Tok t;
            if (!d.parser().take(t, true) || t.t != Tok::FN_HEX) {
                error = "$GIO без микропрограммы канала";
                return false;
            }
            d.emit("HEX(" + hex_bytes(t.s) + ")");
            if (src.at_end()) return true;
            d.emit(",");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        // Двухбайтовые глаголы с однородным списком через DE.
        case 0x0601: {                                 // MAT <массив>=<что>
            d.emit("MAT ");
            Tok t;
            if (!d.parser().take(t, true) || t.t != Tok::ARRAY) {
                error = "MAT без массива";
                return false;
            }
            std::string nm;
            if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
            d.emit(nm + "=");
            uint8_t b = 0;
            if (src.peek_raw_byte(b) && b == 0xD9) src.skip(1);
            if (src.peek_raw_byte(b) && b == 0xEF) { src.skip(1); d.emit("ZER"); return true; }
            if (src.peek_raw_byte(b) && b == 0xF0) { src.skip(1); d.emit("CON"); return true; }
            if (src.peek_raw_byte(b) && b == 0xEE) { src.skip(1); d.emit("IDN"); return true; }
            if (!d.parser().take(t, true) || t.t != Tok::ARRAY) {
                error = "MAT: ждали массив";
                return false;
            }
            if (!d.name(t.var, nm)) { error = "нет имени для индекса"; return false; }
            d.emit(nm);
            return true;
        }

        case 0x0606: {                                 // MAT COPY
            d.emit("MAT COPY ");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::KW_TO) {
                error = "MAT COPY без TO";
                return false;
            }
            d.emit("TO");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x060A: {                                 // MAT SEARCH
            d.emit("MAT SEARCH ");
            if (!d.expr()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::COMMA) {
                error = "MAT SEARCH без запятой";
                return false;
            }
            d.emit(",");
            uint8_t rel = 0;
            if (!src.take_raw_byte(rel)) { error = "MAT SEARCH без знака"; return false; }
            switch (rel) {
                case 0xD9: d.emit("="); break;
                case 0xD5: d.emit("<>"); break;
                case 0xD6: d.emit("<="); break;
                case 0xD7: d.emit("<"); break;
                case 0xD8: d.emit(">="); break;
                case 0xD4: d.emit(">"); break;
                default: error = "MAT SEARCH: непонятный знак"; return false;
            }
            if (!d.expr()) { error = d.error(); return false; }
            if (!d.parser().take(t, false) || t.t != Tok::KW_TO) {
                error = "MAT SEARCH без TO";
                return false;
            }
            d.emit("TO");
            if (!d.expr()) { error = d.error(); return false; }
            if (src.at_end()) return true;
            if (!d.parser().peek(t, false)) { error = d.error(); return false; }
            if (t.t == Tok::KW_STEP) {
                d.parser().consume();
                d.emit("STEP");
                if (!d.expr()) { error = d.error(); return false; }
            }
            return true;
        }

        case 0x060C: {                                 // $TRAN(
            d.emit("$TRAN(");
            if (!expr_list(d, src, error)) return false;
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::RPAR) {
                error = "$TRAN: скобка не закрыта";
                return false;
            }
            d.emit(")");
            uint8_t b = 0;
            if (src.take_raw_byte(b)) {
                uint8_t mode = 0;
                if (b != 0xDE || !src.take_raw_byte(mode)) {
                    error = "$TRAN: непонятный хвост";
                    return false;
                }
                d.emit("R");
            }
            return true;
        }

        case 0x060F: {                                 // $OPEN
            // Буферов бывает несколько — `¤OPEN A¤(),B¤()` (SIG 7580), — а
            // первый бывает пропущен: `¤OPEN ,B¤()` (SLIDE 220), и операнды
            // тогда начинаются прямо с `DE`. Буфер берётся приёмником, а не
            // выражением: тот не заглядывает вперёд, и сразу за ним можно
            // смотреть сырой байт.
            d.emit("$OPEN ");
            for (;;) {
                uint8_t b = 0;
                if (src.peek_raw_byte(b) && b == 0xDE) {
                    src.skip(1);
                    d.emit(",");
                    continue;
                }
                if (!d.lvalue()) { error = d.error(); return false; }
                if (!src.peek_raw_byte(b) || b != 0xDE) return true;
                src.skip(1);
                d.emit(",");
            }
        }

        case 0x0622: {                                 // $LET
            d.emit("$LET ");
            if (!d.lvalue()) { error = d.error(); return false; }
            Tok t;
            if (!d.parser().take(t, false) || t.t != Tok::EQ) { error = "$LET без ="; return false; }
            d.emit("=");
            if (!d.expr()) { error = d.error(); return false; }
            return true;
        }

        case 0x061F:                                   // $COPY
            d.emit("$COPY ");
            if (!disk_prefix(d, src, false)) { error = "приставка устройства"; return false; }
            if (!d.expr()) { error = d.error(); return false; }
            return true;

        case 0x0613: case 0x0614: case 0x0615: case 0x0619:
        case 0x061A: case 0x061B: case 0x061C:
        case 0x061D: case 0x0623: {                    // графика
            const char * word = (verb == 0x0613) ? "DOT"
                              : (verb == 0x0614) ? "DDRAW"
                              : (verb == 0x0615) ? "DRAW"
                              : (verb == 0x0619) ? "NPLOT"
                              : (verb == 0x061A) ? "$MOVE"
                              : (verb == 0x061B) ? "TURN"
                              : (verb == 0x061C) ? "STRETCH"
                              : (verb == 0x061D) ? "FRAME" : "WINDOW";
            d.emit(std::string(word) + " ");
            return expr_list(d, src, error);
        }

        // `LABEL` разбирается отдельно: у него **пропущенные аргументы** —
        // `LABEL B¤(),,,"Y"` (VICT 6150), — а множитель размера пишется
        // вплотную к имени буфера, без разделителя (`LABEL B¤()3,,,"Q"`).
        // Общий список выражений тут не годится ни тем, ни другим.
        case 0x061E: {
            d.emit("LABEL ");
            // Буфер берётся приёмником, а не выражением: тот не заглядывает
            // вперёд, и сразу за ним можно смотреть сырой байт.
            if (!d.lvalue()) { error = d.error(); return false; }

            uint8_t b = 0;
            if (!src.peek_raw_byte(b)) return true;
            if (b != 0xDE) {                      // множитель размера
                if (!d.expr()) { error = d.error(); return false; }
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) return true;
                d.parser().consume();
            } else {
                src.skip(1);
            }
            d.emit(",");

            for (;;) {
                if (!src.peek_raw_byte(b)) return true;
                if (b == 0xDE) { src.skip(1); d.emit(","); continue; }
                if (!d.expr()) { error = d.error(); return false; }
                Tok t;
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) return true;
                d.parser().consume();
                d.emit(",");
            }
        }

        case 0x0625: {                                 // ASMB
            d.emit("ASMB ");
            for (;;) {
                if (src.at_end()) break;
                Tok t;
                if (!d.parser().peek(t, true)) { error = d.error(); return false; }
                if (t.t == Tok::END) break;
                if (t.t == Tok::COMMA) { d.parser().consume(); d.emit(","); continue; }
                if (t.t == Tok::STAR) { d.parser().consume(); d.emit("*"); continue; }
                if (!d.expr()) { error = d.error(); return false; }
                if (!d.parser().peek(t, false)) { error = d.error(); return false; }
                if (t.t != Tok::COMMA) break;
                d.parser().consume();
                d.emit(",");
            }
            return true;
        }

        default: break;
    }

    char b[16];
    std::sprintf(b, "%02X", verb & 0xFF);
    error = std::string("глагол ") + ((verb > 0xFF) ? "06 " : "") + b
          + " ещё не детокенизируется";
    return false;
}

} // namespace

bool detokenize_line(const ProgramLine & line, const NameTable & names,
                     std::string & koi8, std::string & error)
{
    koi8 = dec(line.number);
    koi8 += ' ';

    const std::vector<uint8_t> & b = line.body;
    unsigned p = 0;
    bool first = true;
    while (p < b.size()) {
        unsigned verb = b[p++];
        if (verb == 0x06) {
            if (p >= b.size()) { error = "двухбайтовый глагол оборвался"; return false; }
            verb = 0x0600 | b[p++];
        }
        if (p >= b.size()) { error = "оператор без длины"; return false; }
        const unsigned len = b[p++];
        if (p + len > b.size()) { error = "оператор выходит за строку"; return false; }

        if (!first) koi8 += ':';
        first = false;
        if (!decode_stmt(verb, len ? &b[p] : 0, len, names, koi8, error))
            return false;
        p += len;
    }
    return true;
}


bool detokenize(const ProgramImage & img, NameTable & names,
                std::string & koi8, std::string & error)
{
    koi8.clear();

    // Имена раздаются по индексам, тип берётся из таблиц переменных. Порядок
    // важен: NameTable раздаёт индексы подряд, и обратная трансляция найдёт
    // те же имена на тех же местах.
    const std::vector<VarInfo> & vars = img.vars();
    if (vars.size() > 286) { error = "переменных больше, чем имён"; return false; }

    // Признак массива входит в ключ: `A¤` и `A¤()` — разные переменные с
    // разными индексами, и обратная трансляция обязана попасть в ту же.
    //
    // Здесь и вылезает известная неопределённость: «строка-скаляр или массив
    // строк» по таблицам не различается, и build_vars ошибается в сторону
    // массива (docs/format.md, разд. 6). Если такая переменная употреблена в
    // программе без индекса, обратная трансляция заведёт для неё новую
    // запись. На корпусе так расходятся три строки STAT03 — там `B2¤`
    // помечена массивом, а используется скаляром.
    for (unsigned i = 0; i < vars.size(); ++i) {
        std::string nm = base_name(i);
        if (vars[i].is_string) nm += '$';
        else if (vars[i].is_integer) nm += '%';
        if (vars[i].is_array) nm += '(';
        if (names.index(nm) != i) { error = "имена разъехались с индексами"; return false; }
    }

    for (unsigned i = 0; i < img.line_count(); ++i) {
        std::string text;
        if (!detokenize_line(img.line(i), names, text, error)) {
            error = "строка " + dec(img.line(i).number) + ": " + error;
            return false;
        }
        koi8 += text;
        koi8 += '\n';
    }
    return true;
}

} // namespace iskra
