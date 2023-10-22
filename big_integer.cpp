#include "big_integer.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <iostream>
#include <stdexcept>

/* Helper methods */

namespace {
bool check_block(uint64_t block) {
  return block <= (std::numeric_limits<uint32_t>::max() - 9) / 10;
}

bool check_pow(uint64_t pow) {
  return pow <= (std::numeric_limits<uint32_t>::max() / 10);
}

const big_integer MINUS_ONE(-1);

static constexpr uint64_t BASE = (1LL << 32);

}; // namespace

/* ============================================= */

big_integer& big_integer::operator=(const big_integer& other) {
  if (this != &other) {
    big_integer(other).swap(*this);
  }
  return *this;
}

/* Constructors section */

big_integer::big_integer(int a) : big_integer(static_cast<long>(a)) {}

big_integer::big_integer(unsigned a) : big_integer(static_cast<unsigned long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long long a) {
  if (a < 0) {
    _is_negative = true;
    if (a == -1) {
      return;
    }
    if (a == std::numeric_limits<long long>::min()) {
      _digits = {0, (1u << 31)};
      return;
    }
    a = -a;
  }
  while (a != 0) {
    _digits.push_back(static_cast<uint32_t>(a % BASE));
    a >>= 32;
  }
  if (_is_negative) {
    inplace_minus();
    _is_negative = true;
  }
}

big_integer::big_integer(unsigned long long a) {
  while (a != 0) {
    _digits.push_back(static_cast<uint32_t>(a));
    a >>= 32;
  }
}

big_integer::big_integer(const std::string& str) {
  if (str.empty() || str == "-") {
    throw std::invalid_argument("Error: empty string");
  }

  bool sign = str[0] == '-';

  uint32_t block = 0;
  uint32_t pow = 1;
  for (size_t i = sign; i < str.size(); i++) {
    char cur = str[i];
    if (!std::isdigit(cur)) {
      throw std::invalid_argument("Error: invalid character at pos " + std::to_string(i) + ": " + cur);
    }
    int32_t digit = cur - '0';
    block = block * 10 + digit;
    pow *= 10;
    if (!check_block(block) || !check_pow(pow)) { // Собираем максимальное число из циферок так, чтобы влезало в инт
      proceed(sign, block, pow);
      block = 0;
      pow = 1;
    }
  }
  if (block || pow > 1) {
    proceed(sign, block, pow);
  }
  clear_zeros();
}

/* ===================================================================== */

/* Boolean operators */

bool operator==(const big_integer& a, const big_integer& b) {

  return a._is_negative == b._is_negative && a._digits == b._digits;
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  if (a._is_negative != b._is_negative) {
    return a._is_negative;
  }

  if (a._digits.size() != b._digits.size()) {
    return a._digits.size() < b._digits.size();
  }

  if (a._digits.empty() && b._digits.empty()) {
    return false;
  }

  for (size_t i = a._digits.size() - 1;; i--) {
    if (a._digits[i] != b._digits[i]) {
      return a._digits[i] < b._digits[i];
    }
    if (i == 0) {
      break;
    }
  }
  return false;
}

bool operator>(const big_integer& a, const big_integer& b) {
  return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return !(a < b);
}

/* ====================================== */

/* Bitwise operators */

big_integer& big_integer::apply_bitwise_operation(uint32_t (*bitwise_operation)(uint32_t, uint32_t),
                                                  const big_integer& rhs) {
  size_t oldSize = _digits.size();
  size_t newSize = std::max(_digits.size(), rhs._digits.size()) + 1;
  _digits.resize(newSize);

  for (size_t i = 0; i < newSize; i++) {
    uint32_t a = (i < oldSize ? _digits[i] : neutral_element());
    uint32_t b = rhs.get_or_default(i, rhs.neutral_element());
    _digits[i] = (*bitwise_operation)(a, b);
  }
  _is_negative = (*bitwise_operation)(_is_negative, rhs._is_negative);
  clear_zeros();
  return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  return apply_bitwise_operation([](uint32_t a, uint32_t b) { return a & b; }, rhs);
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  return apply_bitwise_operation([](uint32_t a, uint32_t b) { return a | b; }, rhs);
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  return apply_bitwise_operation([](uint32_t a, uint32_t b) { return a ^ b; }, rhs);
}

big_integer operator&(big_integer a, const big_integer& b) {
  return a &= b;
}

big_integer operator|(big_integer a, const big_integer& b) {
  return a |= b;
}

big_integer operator^(big_integer a, const big_integer& b) {
  return a ^= b;
}

big_integer& big_integer::operator<<=(int rhs) {
  if (rhs == 0) {
    return *this;
  }

  size_t modulo = rhs % 32;
  size_t zero_count = rhs / 32;

  _digits.push_back(neutral_element());
  _digits.insert(_digits.begin(), zero_count, 0);
  uint32_t remainder = 0;
  for (size_t i = 0; i < _digits.size(); i++) {
    uint64_t a = static_cast<uint64_t>(get_or_default(i, neutral_element())) << modulo;
    uint64_t cur = remainder + a;
    _digits[i] = cur % BASE;
    remainder = cur / BASE;
  }
  clear_zeros();
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  if (rhs == 0) {
    return *this;
  }

  size_t modulo = rhs % 32;
  size_t zero_count = rhs / 32;
  bool sign = _is_negative;

  inplace_abs();
  small_div(1u << modulo);

  // std::vector<uint32_t>::iterator erase_index = std::min(_digits.begin() + zero_count, _digits.end());
  // _digits.erase(_digits.begin(), erase_index);

  if (zero_count >= _digits.size()) {
    _digits.clear();
  } else {
    _digits.erase(_digits.begin(), _digits.begin() + zero_count);
  }

  if (sign) {
    (*this)++;
    inplace_minus();
  }

  clear_zeros();
  return *this;
}

big_integer operator<<(big_integer a, int b) {
  return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
  return a >>= b;
}

/* Arithmetic operators */

big_integer& big_integer::small_mul(uint32_t x) {
  uint64_t carry = 0;
  _digits.resize(_digits.size() + 1);
  bool sign = _is_negative;
  inplace_abs();
  for (size_t i = 0; i < _digits.size() - 1; i++) {
    uint64_t mul = static_cast<uint64_t>(_digits[i]) * x + carry;
    _digits[i] = mul % BASE;
    carry = mul >> 32;
  }
  _digits.back() = static_cast<uint32_t>(carry);
  if (sign) {
    inplace_minus();
  }
  clear_zeros();
  return *this;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  if (this->check_for_zero() || rhs.check_for_zero()) {
    make_zero();
    return *this;
  }

  bool sign = _is_negative ^ rhs._is_negative;
  big_integer left = abs();
  big_integer right = rhs.abs();
  _digits = std::vector<uint32_t>(left._digits.size() + right._digits.size() + 1, 0);
  _is_negative = false;

  for (size_t i = 0; i < left._digits.size(); i++) {
    uint64_t carry = 0;
    for (size_t j = 0; j < right._digits.size() || carry != 0; j++) {
      uint64_t cur = _digits[i + j] + static_cast<uint64_t>(left._digits[i]) * right.get_or_default(j, 0) + carry;
      _digits[i + j] = static_cast<uint32_t>(cur);
      carry = cur >> 32;
    }
  }

  if (sign) {
    inplace_minus();
  }
  clear_zeros();
  return *this;
}

big_integer operator*(big_integer a, int32_t x) {
  return a *= x;
}

big_integer operator*(big_integer a, const big_integer& b) {
  return a *= b;
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  return cool_division(true, rhs);
}

uint32_t big_integer::small_div(uint32_t x) {
  if (x == 0) {
    throw std::invalid_argument("Error: Division by zero");
  }
  uint64_t remainder = 0;
  bool sign = _is_negative;
  inplace_abs();
  for (int i = _digits.size() - 1; i >= 0; i--) {
    uint64_t new_digit = (_digits[i] + BASE * remainder);
    remainder = new_digit % x;
    _digits[i] = new_digit / x;
  }
  if (sign) {
    inplace_minus();
  }
  clear_zeros();
  return remainder;
}

big_integer operator/(big_integer a, const big_integer& b) {
  return a.cool_division(true, b);
}

big_integer& big_integer::operator%=(int64_t rhs) {
  return cool_division(false, rhs);
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  return cool_division(false, rhs);
}

big_integer operator%(big_integer a, const big_integer& b) {
  return a %= b;
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  return subadd(true, rhs);
}

big_integer operator+(big_integer a, uint32_t x) {
  return a += x;
}

big_integer operator+(big_integer a, const big_integer& b) {
  return a += b;
}

big_integer& big_integer::subadd(bool plus, const big_integer& rhs) {
  if (rhs._digits.size() == 1 && !rhs._is_negative) {
    return small_add(plus, rhs._digits[0]);
  }

  size_t new_size = std::max(_digits.size(), rhs._digits.size()) + 2;
  _digits.resize(new_size, neutral_element());
  uint64_t carry = plus ? 0 : 1;
  for (size_t i = 0; i < new_size; i++) {
    uint32_t right_digit =
        !plus ? ~rhs.get_or_default(i, rhs.neutral_element()) : rhs.get_or_default(i, rhs.neutral_element());
    uint64_t sub = _digits[i] + carry + right_digit;
    _digits[i] = static_cast<uint32_t>(sub);
    carry = sub >> 32;
  }

  _is_negative = check_bit(); // В дополнении до двух
  clear_zeros();
  return *this;
}

big_integer& big_integer::operator+=(uint32_t x) {
  return small_add(true, x);
}

big_integer& big_integer::small_add(bool plus, uint32_t x) {
  _digits.resize(_digits.size() + 1, neutral_element());
  uint64_t carry = plus ? 0 : 1;
  for (size_t i = 0; i < _digits.size(); i++) {
    uint32_t digit = (i > 0 ? 0 : x);
    uint64_t add = _digits[i] + carry + (plus ? digit : ~digit);
    _digits[i] = static_cast<uint32_t>(add);
    carry = add >> 32;
  }
  _is_negative = check_bit();
  clear_zeros();
  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  return subadd(false, rhs);
}

big_integer operator-(big_integer a, uint32_t x) {
  return a -= x;
}

big_integer operator-(big_integer a, const big_integer& b) {
  return a -= b;
}

big_integer big_integer::operator-() const {
  if (_digits.empty() && !_is_negative) {
    return *this;
  }
  return ~*this += 1;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator~() const {
  big_integer copy(*this);
  if (copy._digits.empty() && copy._is_negative) {
    return 0;
  }
  copy.inplace_tilda();
  return copy;
}

big_integer big_integer::abs() const {
  if (_is_negative) {
    return -*this;
  }
  return *this;
}

big_integer& big_integer::operator++() {
  small_add(true, 1);
  return *this;
}

big_integer big_integer::operator++(int) {
  big_integer tmp(*this);
  ++*this;
  return tmp;
}

big_integer& big_integer::operator--() {
  small_add(false, 1);
  return *this;
}

big_integer big_integer::operator--(int) {
  big_integer tmp(*this);
  --*this;
  return tmp;
}

/* ============================================== */

std::string to_string(const big_integer& a) {
  if (a._digits.empty()) {
    return a._is_negative ? "-1" : "0";
  }
  std::string res;
  big_integer b = a.abs();
  while (!b.check_for_zero()) {
    uint32_t tmp = b.small_div(1'000'000'000);
    for (int i = 0; i < 9; i++) {
      res += char(tmp % 10 + '0');
      tmp /= 10;
    }
  }
  while (!res.empty() && res.back() == '0') {
    res.pop_back();
  }
  if (a._is_negative) {
    res += '-';
  }
  std::reverse(res.begin(), res.end());
  return res;
}

void big_integer::clear_zeros() {
  while (!_digits.empty() && _digits.back() == neutral_element()) {
    _digits.pop_back();
  }
}

void big_integer::proceed(bool sign, uint32_t cur_block, uint32_t cur_power) {
  small_mul(cur_power);
  small_add(!sign, cur_block);
}

bool big_integer::check_for_zero() const {
  return !_is_negative && (_digits.empty() || (_digits.size() == 1 && _digits[0] == 0));
}

void big_integer::swap(big_integer& other) noexcept {
  using std::swap;
  swap(this->_is_negative, other._is_negative);
  swap(this->_digits, other._digits);
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
  return out << to_string(a);
}

bool big_integer::check_bit() {
  if (_digits.empty()) {
    return false;
  }
  return _digits.back() & (1 << 31);
}

uint32_t big_integer::neutral_element() const {
  return _is_negative ? std::numeric_limits<uint32_t>::max() : 0;
}

big_integer& big_integer::cool_division(bool div, const big_integer& rhs) {
  if (rhs.check_for_zero()) {
    throw std::invalid_argument("Error: division by zero");
  }
  if (check_for_zero()) {
    return *this;
  }

  if (rhs == MINUS_ONE) {
    if (div) {
      inplace_minus();
    } else {
      make_zero();
    }
    return *this;
  }

  if (_digits.size() < rhs._digits.size()) {
    if (div) {
      make_zero();
    }
    return *this;
  }

  bool starting_sign = _is_negative;
  bool sign = (_is_negative ^ rhs._is_negative);
  big_integer b = rhs.abs();
  inplace_abs();

  int32_t base = 0;
  for (size_t i = 32; i-- > 0;) {
    if ((1 << i) & b._digits.back()) {
      break;
    }
    base++;
  }

  *this <<= base;
  b <<= base;
  size_t m = _digits.size() - b._digits.size();
  big_integer result;
  result._digits.resize(m + 1, 0);
  big_integer multiplier = b << (32 * m);
  size_t n = b._digits.size();
  bool is_zero = true;
  for (int64_t i = static_cast<int64_t>(m); i >= 0; i--) {
    auto digit1 = static_cast<uint64_t>(get_or_default(n + i, 0));
    auto digit2 = static_cast<uint64_t>(get_or_default(n + i - 1, 0));
    uint64_t qq = (digit1 * BASE + digit2) / b._digits.back();
    result._digits[i] = std::min(qq, BASE - 1);
    is_zero &= (result._digits[i] == 0);
    if (result._digits[i]) {
      multiplier.small_mul(result._digits[i]);
      *this -= multiplier;
      multiplier.small_div(result._digits[i]);
    }

    while (*this < 0) {
      result._digits[i]--;
      *this += multiplier;
    }
    multiplier >>= 32;
  }

  sign &= !is_zero;

  if (sign) {
    result.inplace_minus();
  }
  result._is_negative = sign;
  result.clear_zeros();
  *this >>= base;
  if (starting_sign) {
    inplace_minus();
  }
  if (div) {
    std::swap(*this, result);
  }
  return *this;
}

uint32_t big_integer::get_or_default(size_t index, uint32_t default_value) const {
  return index >= _digits.size() ? default_value : _digits[index];
}

void big_integer::inplace_abs() {
  _digits.reserve(_digits.size() + 1);
  if (_is_negative) {
    for (uint32_t& _digit : _digits) {
      _digit = ~_digit;
    }
    _is_negative ^= 1;
    small_add(true, 1);
  }
}

void big_integer::inplace_minus() {
  _digits.reserve(_digits.size() + 1);
  inplace_tilda();
  small_add(true, 1);
}

void big_integer::inplace_tilda() {
  for (uint32_t& _digit : _digits) {
    _digit = ~_digit;
  }
  _is_negative ^= 1;
}

void big_integer::make_zero() {
  _digits.clear();
  _is_negative = false;
}
