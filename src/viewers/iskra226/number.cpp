// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
// Part of the Iskra-226 project: https://github.com/Ptr314/Iskra-226
// Description: число BASIC 02 — десятичное, 13 значащих разрядов

#include "viewers/iskra226/number.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace iskra {

namespace {
    const unsigned D = Number::DIGITS;
    // Запас под выравнивание при сложении и под произведение.
    const unsigned WIDE = 2 * Number::DIGITS + 4;
}

Number::Number()
{
    set_zero();
}

void Number::set_zero()
{
    neg_ = false;
    exp_ = 0;
    for (unsigned i = 0; i < D; ++i) d_[i] = 0;
}

// Значение = buf[0].buf[1]… * 10^e. Разряды приводятся к 0..9 переносами,
// затем результат нормализуется и округляется до DIGITS разрядов.
bool Number::set_from(const int * src, unsigned n, int e, bool neg)
{
    int buf[WIDE + 2];
    if (n > WIDE) n = WIDE;
    for (unsigned i = 0; i < n; ++i) buf[i + 1] = src[i];
    buf[0] = 0;
    ++n;

    // Переносы справа налево.
    int carry = 0;
    for (unsigned i = n; i-- > 0; ) {
        int v = buf[i] + carry;
        carry = v / 10;
        buf[i] = v % 10;
    }
    // buf[0] появился ради переноса: он сдвигает точку на разряд влево.
    int lead = e + 1;

    // Отбрасываем ведущие нули.
    unsigned first = 0;
    while (first < n && buf[first] == 0) { ++first; --lead; }
    if (first == n) { set_zero(); return true; }

    // Округление до DIGITS разрядов, полуцелое вверх.
    unsigned avail = n - first;
    if (avail > D) {
        if (buf[first + D] >= 5) {
            unsigned i = first + D;
            int c = 1;
            while (i-- > first && c) {
                int v = buf[i] + c;
                buf[i] = v % 10;
                c = v / 10;
            }
            if (c) {
                // Перенос вышел за старший разряд: 999… -> 1000…
                // Запасной разряд buf[0] бывает уже занят переносом из
                // первого прохода — тогда сдвигать некуда, и это
                // переполнение, а не повод залезть за границу массива.
                if (!first) return false;
                --first;
                buf[first] = 1;
                ++lead;
                ++avail;
            }
        }
        avail = D;
    }

    // lead — порядок первого уцелевшего разряда, он же exp_.
    if (lead > EXP_MAX) return false;
    if (lead < EXP_MIN) { set_zero(); return true; }       // исчезновение порядка

    neg_ = neg;
    exp_ = lead;
    for (unsigned i = 0; i < D; ++i)
        d_[i] = (i < avail) ? static_cast<uint8_t>(buf[first + i]) : 0;
    return true;
}

Number Number::from_int(long v)
{
    Number r;
    bool neg = false;
    if (v < 0) { neg = true; }

    char tmp[32];
    std::sprintf(tmp, "%ld", v < 0 ? -v : v);
    const unsigned n = static_cast<unsigned>(std::strlen(tmp));

    int buf[32];
    for (unsigned i = 0; i < n; ++i) buf[i] = tmp[i] - '0';
    r.set_from(buf, n, static_cast<int>(n) - 1, neg);
    return r;
}

Number Number::from_double(double v)
{
    Number r;
    if (!(v == v) || v > 1e300 || v < -1e300) return r;   // NaN и бесконечности — нуль
    char tmp[64];
    std::sprintf(tmp, "%.*e", static_cast<int>(D) - 1, v);
    parse(tmp, r);
    return r;
}

bool Number::parse(const std::string & s, Number & out)
{
    unsigned p = 0;
    const unsigned n = static_cast<unsigned>(s.size());
    while (p < n && s[p] == ' ') ++p;

    bool neg = false;
    if (p < n && (s[p] == '+' || s[p] == '-')) { neg = (s[p] == '-'); ++p; }

    int buf[WIDE];
    unsigned len = 0;
    int point = -1;                 // сколько разрядов до точки
    bool any = false;

    while (p < n) {
        if (s[p] >= '0' && s[p] <= '9') {
            if (len < WIDE) buf[len++] = s[p] - '0';
            else if (point < 0) return false;   // слишком длинная целая часть
            any = true;
            ++p;
        } else if (s[p] == '.' && point < 0) {
            point = static_cast<int>(len);
            ++p;
        } else {
            break;
        }
    }
    if (!any) return false;
    if (point < 0) point = static_cast<int>(len);

    int e10 = 0;
    if (p < n && (s[p] == 'E' || s[p] == 'e')) {
        ++p;
        bool eneg = false;
        if (p < n && (s[p] == '+' || s[p] == '-')) { eneg = (s[p] == '-'); ++p; }
        if (p >= n || s[p] < '0' || s[p] > '9') return false;
        int v = 0;
        while (p < n && s[p] >= '0' && s[p] <= '9') {
            v = v * 10 + (s[p] - '0');
            if (v > 9999) return false;
            ++p;
        }
        e10 = eneg ? -v : v;
    }

    while (p < n && s[p] == ' ') ++p;
    if (p != n) return false;

    // Порядок старшего разряда буфера.
    return out.set_from(buf, len, point - 1 + e10, neg);
}

int Number::cmp_mag(const Number & a, const Number & b)
{
    if (a.is_zero() && b.is_zero()) return 0;
    if (a.is_zero()) return -1;
    if (b.is_zero()) return 1;
    if (a.exp_ != b.exp_) return a.exp_ < b.exp_ ? -1 : 1;
    for (unsigned i = 0; i < D; ++i)
        if (a.d_[i] != b.d_[i]) return a.d_[i] < b.d_[i] ? -1 : 1;
    return 0;
}

int Number::compare(const Number & b) const
{
    const bool an = is_negative(), bn = b.is_negative();
    if (an != bn) return an ? -1 : 1;
    const int m = cmp_mag(*this, b);
    return an ? -m : m;
}

Number Number::negated() const
{
    Number r = *this;
    if (!r.is_zero()) r.neg_ = !r.neg_;
    return r;
}

// Сложение модулей: |a| + |b|, знак задаётся отдельно. Требует |a| >= |b|.
bool Number::add_mag(const Number & a, const Number & b, bool neg, Number & r)
{
    if (b.is_zero()) { r = a; if (!r.is_zero()) r.neg_ = neg; return true; }

    const unsigned shift = static_cast<unsigned>(a.exp_ - b.exp_);
    if (shift > D + 1) { r = a; r.neg_ = neg; return true; }

    int buf[WIDE];
    const unsigned len = shift + D;
    for (unsigned i = 0; i < len; ++i) {
        int v = (i < D) ? a.d_[i] : 0;
        if (i >= shift && i - shift < D) v += b.d_[i - shift];
        buf[i] = v;
    }
    return r.set_from(buf, len, a.exp_, neg);
}

// Вычитание модулей: |a| - |b| при |a| >= |b|.
bool Number::sub_mag(const Number & a, const Number & b, bool neg, Number & r)
{
    if (b.is_zero()) { r = a; if (!r.is_zero()) r.neg_ = neg; return true; }

    const unsigned shift = static_cast<unsigned>(a.exp_ - b.exp_);
    if (shift > D + 1) { r = a; r.neg_ = neg; return true; }

    const unsigned len = shift + D;
    int buf[WIDE];
    for (unsigned i = 0; i < len; ++i) buf[i] = (i < D) ? a.d_[i] : 0;

    for (unsigned i = 0; i < D; ++i) {
        unsigned k = shift + i;
        if (k >= len) break;
        buf[k] -= b.d_[i];
    }
    // Заёмы.
    for (unsigned i = len; i-- > 0; ) {
        while (buf[i] < 0) {
            buf[i] += 10;
            if (i == 0) return false;    // не должно случаться при |a| >= |b|
            --buf[i - 1];
        }
    }
    return r.set_from(buf, len, a.exp_, neg);
}

bool Number::add(const Number & a, const Number & b, Number & r)
{
    if (a.is_zero()) { r = b; return true; }
    if (b.is_zero()) { r = a; return true; }

    if (a.neg_ == b.neg_) {
        return cmp_mag(a, b) >= 0 ? add_mag(a, b, a.neg_, r)
                                  : add_mag(b, a, a.neg_, r);
    }
    const int m = cmp_mag(a, b);
    if (m == 0) { r = Number(); return true; }
    return m > 0 ? sub_mag(a, b, a.neg_, r)
                 : sub_mag(b, a, b.neg_, r);
}

bool Number::sub(const Number & a, const Number & b, Number & r)
{
    return add(a, b.negated(), r);
}

bool Number::mul(const Number & a, const Number & b, Number & r)
{
    if (a.is_zero() || b.is_zero()) { r = Number(); return true; }

    long long acc[2 * Number::DIGITS];
    for (unsigned i = 0; i < 2 * D; ++i) acc[i] = 0;
    for (unsigned i = 0; i < D; ++i) {
        if (!a.d_[i]) continue;
        for (unsigned j = 0; j < D; ++j)
            acc[i + j + 1] += static_cast<long long>(a.d_[i]) * b.d_[j];
    }
    // Переносы: acc[k] соответствует разряду 10^-(k-1) от произведения мантисс.
    for (unsigned k = 2 * D; k-- > 1; ) {
        acc[k - 1] += acc[k] / 10;
        acc[k] %= 10;
    }

    int buf[2 * Number::DIGITS];
    for (unsigned i = 0; i < 2 * D; ++i) buf[i] = static_cast<int>(acc[i]);

    // Мантиссы в [1,10), произведение в [1,100): buf[0] — разряд десятков.
    return r.set_from(buf, 2 * D, a.exp_ + b.exp_ + 1, a.neg_ != b.neg_);
}

bool Number::div(const Number & a, const Number & b, Number & r)
{
    if (b.is_zero()) return false;
    if (a.is_zero()) { r = Number(); return true; }

    // Деление мантисс столбиком. Обе в [1,10), частное в (0.1, 10), поэтому
    // первая же цифра частного имеет тот же порядок, что и exp_ разность.
    //
    // Остаток держим на разряд шире делителя: делитель стоит в rem[1..D],
    // а rem[0] принимает перенос после умножения остатка на десять. Без
    // этого запаса сдвиг влево терял бы старший разряд.
    const unsigned L = D + 2;
    int rem[D + 2];
    rem[0] = 0;
    for (unsigned i = 0; i < D; ++i) rem[i + 1] = a.d_[i];
    rem[D + 1] = 0;

    const unsigned OUT = D + 2;
    int q[D + 2];

    for (unsigned k = 0; k < OUT; ++k) {
        int digit = 0;
        while (digit < 9) {
            int t[D + 2];
            for (unsigned i = 0; i < L; ++i) t[i] = rem[i];
            for (unsigned i = 0; i < D; ++i) t[i + 1] -= b.d_[i];

            bool ok = true;
            for (unsigned i = L; i-- > 0; ) {
                if (t[i] >= 0) continue;
                if (i == 0) { ok = false; break; }
                const int borrow = (-t[i] + 9) / 10;
                t[i] += borrow * 10;
                t[i - 1] -= borrow;
            }
            if (!ok) break;

            for (unsigned i = 0; i < L; ++i) rem[i] = t[i];
            ++digit;
        }
        q[k] = digit;

        // Остаток умножается на десять — сдвиг разрядов влево.
        for (unsigned i = 0; i + 1 < L; ++i) rem[i] = rem[i + 1];
        rem[L - 1] = 0;
    }

    return r.set_from(q, OUT, a.exp_ - b.exp_, a.neg_ != b.neg_);
}

bool Number::to_int(long & out) const
{
    if (is_zero()) { out = 0; return true; }
    if (exp_ < 0 || exp_ > 17) return false;
    const unsigned n = static_cast<unsigned>(exp_) + 1;
    if (n > D) {
        // Целое, но старших разрядов больше, чем хранится: хвост — нули.
        for (unsigned i = D; i < n; ++i) { /* нули */ }
    } else {
        for (unsigned i = n; i < D; ++i)
            if (d_[i]) return false;                     // есть дробная часть
    }
    long v = 0;
    for (unsigned i = 0; i < n; ++i) {
        const int dig = (i < D) ? d_[i] : 0;
        if (v > (0x7FFFFFFFL - dig) / 10) return false;
        v = v * 10 + dig;
    }
    out = neg_ ? -v : v;
    return true;
}

Number Number::floor() const
{
    if (is_zero()) return *this;

    // |значение| < 1: целой части нет, а вниз от отрицательного — минус один.
    if (exp_ < 0) return neg_ ? from_int(-1) : Number();

    const unsigned n = static_cast<unsigned>(exp_) + 1;
    if (n >= DIGITS) return *this;              // дробных разрядов нет вовсе

    Number r = *this;
    bool frac = false;
    for (unsigned i = n; i < DIGITS; ++i) {
        if (r.d_[i]) frac = true;
        r.d_[i] = 0;
    }
    // Старший разряд не обнуляется никогда (n >= 1), поэтому r нормализовано.
    if (!frac || !neg_) return r;

    Number out;
    if (!sub(r, from_int(1), out)) return r;    // порядок тут переполниться не может
    return out;
}

bool Number::floor_to_int(long & out) const
{
    return floor().to_int(out);
}

void Number::to_disk8(uint8_t out[8]) const
{
    for (unsigned i = 0; i < 8; ++i) out[i] = 0;
    if (is_zero()) return;                      // нуль — восемь нулевых байт

    // Внутри значение равно D0.D1…D12 * 10^exp_, на диске — мантисса с
    // точкой после третьей цифры и порядок степенями 1000. Значит
    // порядок = floor(exp_ / 3), а старшая цифра встаёт на позицию
    // 3 - (exp_ - 3*порядок), то есть 1, 2 или 3.
    int e3 = exp_ / 3;
    if (exp_ % 3 < 0) --e3;                     // деление с округлением вниз
    const unsigned lead = 3 - static_cast<unsigned>(exp_ - 3 * e3);

    uint8_t dig[DIGITS];
    for (unsigned i = 0; i < DIGITS; ++i) dig[i] = 0;
    for (unsigned i = 0; i + lead <= DIGITS; ++i) dig[lead - 1 + i] = d_[i];

    for (unsigned i = 0; i < 6; ++i)
        out[i] = static_cast<uint8_t>((dig[2 * i] << 4) | dig[2 * i + 1]);
    out[6] = static_cast<uint8_t>(e3 & 0xFF);
    out[7] = static_cast<uint8_t>((dig[12] << 4) | (neg_ ? 1 : 0));
}

bool Number::from_disk8(const uint8_t in[8], Number & out)
{
    int dig[DIGITS];
    for (unsigned i = 0; i < 6; ++i) {
        const unsigned hi = in[i] >> 4, lo = in[i] & 0x0F;
        if (hi > 9 || lo > 9) return false;
        dig[2 * i]     = static_cast<int>(hi);
        dig[2 * i + 1] = static_cast<int>(lo);
    }
    const unsigned last = in[7] >> 4;
    const unsigned sign = in[7] & 0x0F;
    // Младшая тетрада байта 7 во всём корпусе только 0 или 1.
    if (last > 9 || sign > 1) return false;
    dig[DIGITS - 1] = static_cast<int>(last);

    const int e3 = static_cast<int>(static_cast<int8_t>(in[6]));
    // d1.d2…d13 * 10^(2 + 3*порядок) — та же величина, что d1d2d3.d4…d13
    // умноженная на 1000^порядок.
    out = Number();
    return out.set_from(dig, DIGITS, 2 + 3 * e3, sign != 0);
}

double Number::to_double() const
{
    if (is_zero()) return 0.0;
    char buf[32];
    unsigned p = 0;
    if (neg_) buf[p++] = '-';
    buf[p++] = static_cast<char>('0' + d_[0]);
    buf[p++] = '.';
    for (unsigned i = 1; i < D; ++i) buf[p++] = static_cast<char>('0' + d_[i]);
    buf[p] = 0;
    return std::strtod(buf, 0) * std::pow(10.0, static_cast<double>(exp_));
}

bool Number::pow(const Number & a, const Number & b, Number & r)
{
    long e;
    if (b.to_int(e) && e >= -4096 && e <= 4096) {
        // Целую степень возводим точно, повторным умножением.
        if (a.is_zero()) {
            if (e > 0) { r = Number(); return true; }
            if (e == 0) { r = from_int(1); return true; }
            return false;                                // 0 в отрицательной степени
        }
        Number base = a;
        Number acc = from_int(1);
        long k = e < 0 ? -e : e;
        while (k) {
            if (k & 1) { if (!mul(acc, base, acc)) return false; }
            k >>= 1;
            if (k) { if (!mul(base, base, base)) return false; }
        }
        if (e < 0) return div(from_int(1), acc, r);
        r = acc;
        return true;
    }

    // Дробная степень — через double: своего алгоритма ПЗУ у нас нет.
    if (a.is_negative()) return false;                   // корень из отрицательного
    if (a.is_zero()) { r = Number(); return true; }
    const double v = std::pow(a.to_double(), b.to_double());
    if (!(v == v)) return false;
    r = from_double(v);
    return true;
}

bool Number::sqrt(const Number & a, Number & r)
{
    if (a.is_negative()) return false;
    if (a.is_zero()) { r = Number(); return true; }
    r = from_double(std::sqrt(a.to_double()));
    return true;
}

const Number & Number::pi()
{
    // 13 значащих разрядов, как и всё остальное.
    static Number p;
    static bool ready = false;
    if (!ready) { parse("3.141592653590", p); ready = true; }
    return p;
}

void Number::to_digits(std::string & digits, int & point) const
{
    digits.assign(D, '0');
    if (is_zero()) { point = 1; return; }
    for (unsigned i = 0; i < D; ++i)
        digits[i] = static_cast<char>('0' + d_[i]);
    // Внутри значение равно d0.d1…d12 * 10^exp — на разряд больше слева.
    point = exp_ + 1;
}

std::string Number::to_display() const
{
    std::string s;
    s += is_negative() ? '-' : ' ';

    if (is_zero()) { s += '0'; return s; }

    // Значащие разряды без хвостовых нулей.
    unsigned last = D;
    while (last > 1 && d_[last - 1] == 0) --last;

    // Плотная запись, пока порядок это позволяет; иначе показатель степени.
    // Границы подобраны по виду чисел в листингах корпуса и книге; точные
    // пороги «Искры» не проверены — см. docs/DECISIONS.md.
    if (exp_ >= -12 && exp_ <= 12) {
        if (exp_ >= 0) {
            const unsigned ip = static_cast<unsigned>(exp_) + 1;
            for (unsigned i = 0; i < ip; ++i)
                s += static_cast<char>('0' + (i < last ? d_[i] : 0));
            if (last > ip) {
                s += '.';
                for (unsigned i = ip; i < last; ++i)
                    s += static_cast<char>('0' + d_[i]);
            }
        } else {
            s += '.';
            for (int i = 0; i < -exp_ - 1; ++i) s += '0';
            for (unsigned i = 0; i < last; ++i)
                s += static_cast<char>('0' + d_[i]);
        }
        return s;
    }

    // Мантисса печатается с восемью знаками после точки. Основание —
    // единственный пример свободного формата в руководстве (разд. 13.6):
    // CONVERT "-123.05E+14" TO X, PRINT X даёт -1.23050000E+16.
    const unsigned FRACTION = 8;
    s += static_cast<char>('0' + d_[0]);
    s += '.';
    for (unsigned i = 1; i <= FRACTION; ++i)
        s += static_cast<char>('0' + (i < D ? d_[i] : 0));
    s += 'E';
    char tmp[16];
    std::sprintf(tmp, "%+03d", exp_);
    s += tmp;
    return s;
}

} // namespace iskra