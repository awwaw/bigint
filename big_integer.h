#pragma once

#include <limits>
#include <string>
#include <vector>

struct big_integer {
  big_integer() = default;
  big_integer(const big_integer& other) = default;
  big_integer(int a);
  explicit big_integer(const std::string& str);

  big_integer(long a);

  big_integer(unsigned long a);

  big_integer(long long int a);

  big_integer(unsigned long long int a);

  ~big_integer() = default;

  big_integer(unsigned int a);

  void swap(big_integer& other) noexcept;

  big_integer& operator=(const big_integer& other);

  big_integer& operator+=(const big_integer& rhs);
  big_integer& operator-=(const big_integer& rhs);
  big_integer& operator*=(const big_integer& rhs);
  big_integer& operator/=(const big_integer& rhs);
  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator%=(int64_t rhs);

  big_integer& operator+=(uint32_t x);

  big_integer& operator&=(const big_integer& rhs);
  big_integer& operator|=(const big_integer& rhs);
  big_integer& operator^=(const big_integer& rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  big_integer abs() const;

  friend bool operator==(const big_integer& a, const big_integer& b);
  friend bool operator!=(const big_integer& a, const big_integer& b);
  friend bool operator<(const big_integer& a, const big_integer& b);
  friend bool operator>(const big_integer& a, const big_integer& b);
  friend bool operator<=(const big_integer& a, const big_integer& b);
  friend bool operator>=(const big_integer& a, const big_integer& b);

  friend std::string to_string(const big_integer& a);

  bool check_for_zero() const;

  big_integer& cool_division(bool div, const big_integer& rhs);

private:
  big_integer& subadd(bool, const big_integer&);

  void make_zero();

  big_integer& small_add(bool plus, uint32_t x);
  uint32_t small_div(uint32_t x);

  big_integer& small_mul(uint32_t);

  uint32_t get_or_default(size_t index, uint32_t default_value) const;

  void inplace_abs();

  void inplace_minus();
  void inplace_tilda();

  std::vector<uint32_t> _digits;
  bool _is_negative{};

  bool check_bit();

  void clear_zeros();

  void proceed(bool sign, uint32_t cur_block, uint32_t cur_power);

  uint32_t neutral_element() const;

  big_integer& apply_bitwise_operation(uint32_t (*bitwise_operation)(uint32_t, uint32_t), const big_integer& rhs);
};

big_integer operator+(big_integer a, const big_integer& b);
big_integer operator-(big_integer a, const big_integer& b);
big_integer operator*(big_integer a, const big_integer& b);
big_integer operator/(big_integer a, const big_integer& b);
big_integer operator%(big_integer a, const big_integer& b);

big_integer operator*(big_integer a, int32_t x);

big_integer operator&(big_integer a, const big_integer& b);
big_integer operator|(big_integer a, const big_integer& b);
big_integer operator^(big_integer a, const big_integer& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(const big_integer& a);
std::ostream& operator<<(std::ostream& out, const big_integer& a);
