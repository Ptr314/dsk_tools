// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: разбор выражений, общий для текста и токенов

#pragma once

#include <string>

#include "viewers/iskra226/number.h"

namespace iskra {

// Лексема выражения. Оба представления сводятся к одному потоку лексем,
// и дальше выражение разбирает общий код: приоритеты операций в тексте и
// в токенах одни и те же.
struct Tok {
    enum Type {
        END,            // конец операндов оператора
        NUM, STR, VAR, PI,
        LPAR, RPAR, COMMA, SEMI,
        PLUS, MINUS, STAR, SLASH, CARET,
        EQ, NE, LT, LE, GT, GE,
        AND, OR, XOR,   // логические связки условий
        FN_ABS, FN_INT, FN_SGN, FN_SQR, FN_LOG, FN_EXP, FN_ROUND, FN_RND,
        // Тригонометрия, токены `F9`…`FE` подряд. Угол — в единицах,
        // заданных `SELECT D/R/G` (руководство, разд. 4.6).
        FN_SIN, FN_COS, FN_TAN, FN_ASIN, FN_ACOS, FN_ATAN,
        FN_HEX,         // строка байт уже разобрана в s
        FN_AT, FN_TAB,
        FN_STR,                 // STR( — первая запятая в потоке не кодируется
        FN_LEN, FN_NUM, FN_VAL, FN_POS,   // неявные: закрывающей скобки нет
        FN_USER,                // FN<имя>( — функция пользователя, имя в var
        ARRAY,                  // ссылка на массив целиком
        HASH,                   // DB — второй аргумент VAL(
        KW_TO, KW_STEP, KW_THEN, KW_GOTO, KW_GOSUB,
        UNKNOWN         // распознано, но не поддержано — текст в s
    };

    Tok() : t(END), var(0), indexed(false), table_array(false) {}

    Type t;
    Number num;
    std::string s;
    // У VAR — индекс переменной, у FN_USER — имя функции кодом символа.
    // Пространства имён разные: `DEFFN A(H)` уживается с переменной `A`
    // (руководство, пример 4.20), и в `L2` индекс `52` — это и переменная
    // `R¤`, и функция `R`.
    unsigned var;

    // У VAR: за именем идёт список индексов. В тексте признак — открывающая
    // скобка, в токенах — то, что переменная объявлена массивом: там скобки
    // у индекса нет вовсе (docs/format.md, разд. 7).
    bool indexed;

    // То же, но строго по таблицам переменных, без заглядывания вперёд.
    // Нужно первому аргументу STR(: там за именем идёт не индекс, а начало
    // подстроки, потому что первая запятая STR( не кодируется, — и
    // заглядывание принимает STR(Z¤,67) за обращение к Z¤(67).
    bool table_array;
};

// Источник лексем. Токенизированная форма двузначна: один и тот же байт
// значит разное в позиции операнда и в позиции операции, — поэтому источник
// должен знать, чего от него ждут. Текстовому источнику это безразлично.
class TokenSource
{
public:
    virtual ~TokenSource() {}
    virtual bool next(Tok & t, bool operand_expected) = 0;

    // Причина отказа у самого источника: у разборщика она беднее, а бывает
    // и пустой — тогда наружу уходило сообщение ни о чём.
    // Позиция в источнике — чтобы можно было вернуться к началу заглянутой
    // лексемы. Двузначный токен в другом состоянии значит другое, и
    // перечитывать его приходится заново.
    virtual unsigned tell() const { return 0; }
    virtual void seek(unsigned) {}

    virtual const std::string & source_error() const
    {
        static const std::string none;
        return none;
    }

    // Зависит ли разбор лексемы от ожидаемого состояния. У токенов да —
    // и тогда заглядывать вперёд можно только в том состоянии, в каком
    // лексема потом будет прочитана. Текстовому лексеру состояние
    // безразлично, и это ограничение к нему не применяется.
    virtual bool state_sensitive() const { return false; }
};

// Разбор выражения по приоритетам: сравнения, затем + -, затем * /,
// затем унарный минус, затем ^.
class ExprParser
{
public:
    explicit ExprParser(TokenSource & src);

    // Заглянуть в следующую лексему, не потребляя её.
    bool peek(Tok & t, bool operand_expected);
    // Потребить лексему, на которую смотрели.
    void consume();
    bool take(Tok & t, bool operand_expected);

    const std::string & error() const { return error_; }
    // Вернуть источник к началу заглянутой лексемы и забыть её. Нужно
    // там, где заглядывали в позиции операции, разделителя не оказалось,
    // и ту же лексему надо прочесть как операнд.
    void unpeek()
    {
        if (has_pending_) { src_.seek(pending_start_); has_pending_ = false; }
    }

    // Сбросить заглянутую лексему. Обязательно после TokenSource::set_pos():
    // иначе разбор продолжится с лексемы, прочитанной со старого места.
    void reset() { has_pending_ = false; }

    void fail(const std::string & msg);
    bool failed() const { return !error_.empty(); }

private:
    TokenSource & src_;
    Tok pending_;
    unsigned pending_start_;
    bool has_pending_;
    bool pending_operand_;
    std::string error_;
};

} // namespace iskra