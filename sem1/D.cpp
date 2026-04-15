#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>

const int cMod = 1'000'000'000;

void EraseZeroes(std::vector<int> &v) {
  while (v.size() > 1 && v.back() == 0) {
    v.pop_back();
  }
}

std::vector<int> AbsPlus(const std::vector<int>& left,
                        const std::vector<int>& right) {
  std::vector<int> result(left.size() > right.size() ? left.size() + 1 : right.size() + 1, 0);
  for (size_t i = 0; i < result.size() - 1; ++i) {
    if (i < left.size() && i < right.size()) {
      result[i] += left[i] + right[i];
      result[i + 1] += result[i] / cMod;
      result[i] %= cMod;
    } else if (i < left.size()) {
      result[i] += left[i];
      result[i + 1] += result[i] / cMod;
      result[i] %= cMod;
    } else {
      result[i] += right[i];
      result[i + 1] += result[i] / cMod;
      result[i] %= cMod;
    }
  }
  EraseZeroes(result);
  return result;
}

std::vector<int> AbsMinus(const std::vector<int>& left,
                          const std::vector<int>& right) {
  std::vector<int> result;
  bool is_left = false;
  if (left.size() > right.size()) {
    result = left;
    is_left = true;
  } else if (left.size() < right.size()) {
    result = right;
  } else {
    is_left = true;
    result = left;
    for (int i = static_cast<int>(left.size()) - 1; i > -1; --i) {
      if (left[i] < right[i]) {
        result = right;
        is_left = false;
        break;
      } else if (left[i] > right[i]) {
        break;
      }
    }
  }
  if (is_left) {
    for(size_t i = 0; i < right.size(); ++i) {
      result[i] -= right[i];
      if (result[i] < 0) {
        result[i] += cMod;
        result[i + 1] -= 1;
      }
    }
  } else {
    for(size_t i = 0; i < left.size(); ++i) {
      result[i] -= left[i];
      if (result[i] < 0) {
        result[i] += cMod;
        result[i + 1] -= 1;
      }
    }
  }
  for (size_t i = 0; i < result.size(); ++i) {
    if (result[i] < 0) {
      result[i] += cMod;
      result[i + 1] -= 1;
    }
  }
  EraseZeroes(result);
  return result;
}

int CompareAbs(const std::vector<int>& left, const std::vector<int>& right) {
  if (left.size() > right.size()) {
    return -1;
  }
  if (left.size() < right.size()) {
    return 1;
  }
  if (left == right) {
    return 0;
  }
  for (int i = static_cast<int>(left.size()) - 1; i > -1; --i) {
    if (left[i] < right[i]) {
      return 1;
    } else if (left[i] > right[i]) {
      return -1;
    }
  }
  return 0;
}

std::vector<int> AbsMultiply(const std::vector<int>& left, const std::vector<int>& right) {
  std::vector<int> result(left.size() + right.size() + 1);
  for (size_t i = 0; i < left.size(); ++i) {
    long long to_next = 0;
    for (size_t j = 0; j < right.size(); ++j) {
      long long curr = static_cast<long long>(left[i]) * right[j] + to_next + result[i + j];
      result[i + j] = static_cast<int>(curr % cMod);
      to_next = curr / cMod;
    }
    result[i + right.size()] += static_cast<int>(to_next);
  }
  EraseZeroes(result);
  return result;
}

int FindNum(const std::vector<int>& dividend, const std::vector<int>& divisor) {
  int l = 0;
  int r = cMod;
  while (r - l > 1) {
    int m = (l + r) / 2;
    std::vector<int> tmp = AbsMultiply(divisor, {m});
    EraseZeroes(tmp);
    if (CompareAbs(dividend, tmp) == 1) {
      r = m;
    } else {
      l = m;
    }
  }
  return l;
}

std::vector<int> AbsDivide(const std::vector<int>& dividend, const std::vector<int>& divisor) {
  std::vector<int> dividend2 = dividend;
  std::vector<int> current(0);
  std::vector<int> result;
  std::vector<int> step = {0, 1};
  for (int i = static_cast<int>(dividend2.size()) - 1; i >= 0; --i) {
    current = AbsPlus(AbsMultiply(current, step), {dividend2[i]});
    EraseZeroes(current);
    if (CompareAbs(current, divisor) == 1) {
      result.push_back(0);
      continue;
    }
    int l = FindNum(current, divisor);
    std::vector<int> subtrahend = AbsMultiply(divisor, {l});
    current = AbsMinus(current, subtrahend);
    result.push_back(l);
  }
  std::reverse(result.begin(), result.end());
  EraseZeroes(result);
  return result;
}

class BigInteger {
private:
  bool isNegative;
  std::vector<int> digits;
  void CheckZeroes();

public:
  BigInteger(int number);
  BigInteger(unsigned long long number, char);
  BigInteger() :BigInteger(0) {}
  BigInteger(std::string str);
  BigInteger(const BigInteger& other) = default;
  BigInteger& operator=(const BigInteger& other);
  friend bool operator<(const BigInteger& left, const BigInteger& right);
  friend bool operator==(const BigInteger& left, const BigInteger& right);
  friend BigInteger operator-(const BigInteger& rhs);
  BigInteger& operator+=(const BigInteger& other);
  BigInteger& operator-=(const BigInteger& other);
  BigInteger& operator*=(const BigInteger& other);
  BigInteger& operator/=(const BigInteger& other);
  BigInteger& operator%=(const BigInteger& other);
  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);
  explicit operator bool() const {
    return *this != BigInteger(0);
  }
  std::string toString() const;
};

BigInteger& BigInteger::operator=(const BigInteger& other) {
  if (this == &other) {
    return *this;
  }
  digits = other.digits;
  isNegative = other.isNegative;
  return *this;
}

bool operator<(const BigInteger& left, const BigInteger& right) {
  if (left.isNegative != right.isNegative) {
    return left.isNegative;
  }
  if (left.digits.size() != right.digits.size()) {
    return (left.isNegative && left.digits.size() > right.digits.size()) ||
           (!left.isNegative && left.digits.size() < right.digits.size());
  }
  int abs_comp = CompareAbs(left.digits, right.digits); // 1 left < right, -1 left > right
  return (abs_comp == 1 && !left.isNegative) || (abs_comp == -1 && left.isNegative);
}

bool operator>(const BigInteger& left, const BigInteger& right) {
  return right < left;
}

bool operator<=(const BigInteger& left, const BigInteger& right) {
  return !(left > right);
}

bool operator>=(const BigInteger& left, const BigInteger& right) {
  return !(left < right);
}

bool operator==(const BigInteger& left, const BigInteger& right) {
  return (left.isNegative == right.isNegative) && (left.digits == right.digits);
}

bool operator!=(const BigInteger& left, const BigInteger& right) {
  return !(left == right);
}

void BigInteger::CheckZeroes() {
  EraseZeroes(digits);
  if (digits == std::vector<int>(1,0)) {
    isNegative = false;
  }
}

BigInteger::BigInteger(int number)
    : isNegative(number < 0) {
  if (isNegative) {
    number = -number;
  }
  digits.push_back(number % cMod);
  if (number >= cMod) {
    digits.push_back(number / cMod);
  }
  CheckZeroes();
}

BigInteger::BigInteger(unsigned long long number, char) : isNegative(false) {
  digits.push_back(number % cMod);
  number /= cMod;
  digits.push_back(number % cMod);
  number /= cMod;
  digits.push_back(number % cMod);
  CheckZeroes();
}

BigInteger::BigInteger(std::string str) {
  int n = static_cast<int>(str.size());
  int end = str[0] == '-' ? 1 : 0;
  isNegative = str[0] == '-';
  int count = 0;
  int curr = 0;
  int pow = 1;
  for (int i = n - 1; i >= end; --i) {
    if (count == 9) {
      digits.push_back(curr);
      pow = 1;
      count = 0;
      curr = 0;
    }
    curr = (str[i] - '0') * pow + curr;
    ++count;
    pow *= 10;
  }
  digits.push_back(curr);
  CheckZeroes();
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (isNegative == other.isNegative) {
    digits = AbsPlus(digits, other.digits);
    isNegative = other.isNegative;
    return *this;
  }
  if (other.isNegative) {
    isNegative = CompareAbs(digits, other.digits) == 1;
  } else {
    isNegative = CompareAbs(digits, other.digits) == -1;
  }
  digits = AbsMinus(digits, other.digits);
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  if (isNegative != other.isNegative) {
    digits = AbsPlus(digits, other.digits);
    isNegative = !other.isNegative;
    return *this;
  }
  if (other.isNegative) {
    isNegative = CompareAbs(digits, other.digits) == -1;
  } else {
    isNegative = CompareAbs(digits, other.digits) == 1;
  }
  digits = AbsMinus(digits, other.digits);
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  if (*this == 0 || other == 0) {
    *this = 0;
    return *this;
  }
  isNegative = isNegative != other.isNegative;
  digits = AbsMultiply(digits, other.digits);
  return *this;
}

BigInteger operator*(const BigInteger& left, const BigInteger& right) {
  BigInteger result = left;
  result *= right;
  return result;
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
  digits = AbsDivide(digits, other.digits);
  isNegative = isNegative != other.isNegative;
  CheckZeroes();
  return *this;
}

BigInteger operator/(const BigInteger& left, const BigInteger& right) {
  BigInteger result = left;
  result /= right;
  return result;
}

BigInteger& BigInteger::operator%=(const BigInteger &other) {
  *this -= (*this / other) * other;
  CheckZeroes();
  return *this;
}

BigInteger operator%(const BigInteger& left, const BigInteger& right) {
  BigInteger result = left;
  result %= right;
  return result;
}

BigInteger operator+(const BigInteger& left, const BigInteger& right) {
  BigInteger result = left;
  result += right;
  return result;
}

BigInteger operator-(const BigInteger& rhs) {
  BigInteger result = rhs;
  result.isNegative = !rhs.isNegative;
  result.CheckZeroes();
  return result;
}

BigInteger operator-(const BigInteger& left, const BigInteger& right) {
  BigInteger result = left;
  result -= right;
  return result;
}

std::string BigInteger::toString() const {
  std::string str;
  std::string tmp;
  if (isNegative) {
    str += "-";
  }
  str += std::to_string(digits.back());
  for (int i = digits.size() - 2; i > -1; --i) {
    tmp = std::to_string(digits[i]);
    if (tmp.size() < 9) {
      std::string zeroes(9 - tmp.size(), '0');
      tmp = zeroes + tmp;
    }
    str += tmp;
  }
  return str;
}

BigInteger& BigInteger::operator++() {
  *this += 1;
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger temp(*this);
  *this += 1;
  return temp;
}

BigInteger& BigInteger::operator--() {
  *this -= 1;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger temp(*this);
  *this -= 1;
  return temp;
}

BigInteger operator""_bi(const char* str, size_t) {
  return BigInteger(std::string(str));
}

BigInteger operator""_bi(unsigned long long num) {
  return BigInteger(num, 'u');
}

std::ostream& operator<<(std::ostream& os, const BigInteger& number) {
  os << number.toString();
  return os;
}

std::istream& operator>>(std::istream& is, BigInteger& number) {
  std::string tmp;
  is >> tmp;
  number = BigInteger(tmp);
  return is;
}

BigInteger gcd(BigInteger a, BigInteger b) {
  if ((a == 0) || (b == 0)) {
    return a + b;
  }
  if (a > b) {
    return gcd(a % b, b);
  }
  return gcd(a, b % a);
}

class Rational {
private:
  BigInteger numerator;
  BigInteger denominator;
  void Reduce();

public:
  Rational(const BigInteger& numerator, const BigInteger& denominator);
  Rational(const BigInteger& numerator);
  Rational();
  Rational(int numerator, int denominator);
  Rational(int numerator);
  Rational(const Rational& other) = default;
  Rational& operator=(const Rational& other);
  friend bool operator<(const Rational& left, const Rational& right);
  friend bool operator==(const Rational& left, const Rational& right);
  friend Rational operator-(const Rational& left);
  Rational& operator+=(const Rational& other);
  Rational& operator-=(const Rational& other);
  Rational& operator*=(const Rational& other);
  Rational& operator/=(const Rational& other);
  std::string toString() const;
  std::string asDecimal(size_t precision) const;
  explicit operator double() const;
};

Rational::Rational(const BigInteger& numerator, const BigInteger& denominator)
  : numerator(numerator)
  , denominator(denominator) {
  if (denominator < 0) {
    this->numerator = -this->numerator;
    this->denominator = -this->denominator;
  }
  Reduce();
}


Rational::Rational() :Rational(0, 1) {}

void Rational::Reduce() {
  BigInteger Greatest = gcd(numerator < 0 ? -numerator : numerator,
                            denominator);
  numerator /= Greatest;
  denominator /= Greatest;
}

Rational::Rational(const BigInteger& numerator)
  :Rational(numerator, 1)
{}

Rational::Rational(int numerator, int denominator)
  :Rational(BigInteger(numerator), BigInteger(denominator))
{}

Rational::Rational(int numerator)
  : Rational(numerator, 1)
{}

Rational& Rational::operator=(const Rational& other) {
  if (this == &other) {
    return *this;
  }
  numerator = other.numerator;
  denominator = other.denominator;
  return *this;
}

bool operator<(const Rational& left, const Rational& right) {
  if (left.denominator == right.denominator) {
    return left.numerator < right.numerator;
  }
  return left.numerator * right.denominator < right.numerator * left.denominator;
}

bool operator>(const Rational& left, const Rational& right) {
  return right < left;
}

bool operator>=(const Rational& left, const Rational& right) {
  return !(left < right);
}

bool operator<=(const Rational& left, const Rational& right) {
  return !(left > right);
}

bool operator==(const Rational& left, const Rational& right) {
  return left.denominator == right.denominator && left.numerator == right.numerator;
}

bool operator!=(const Rational& left, const Rational& right) {
  return !(left == right);
}

Rational operator-(const Rational& left) {
  Rational temp(-left.numerator, left.denominator);
  return temp;
}

Rational& Rational::operator+=(const Rational& other) {
  *this = Rational(numerator * other.denominator +
                   other.numerator * denominator,
                   denominator * other.denominator);
  return *this;
}

Rational operator+(const Rational& left, const Rational& right) {
  Rational temp = left;
  temp += right;
  return temp;
}

Rational& Rational::operator-=(const Rational& other) {
  *this = Rational(numerator * other.denominator -
                   other.numerator * denominator,
                   denominator * other.denominator);
  return *this;
}

Rational operator-(const Rational& left, const Rational& right) {
  Rational temp = left;
  temp -= right;
  return temp;
}

Rational& Rational::operator*=(const Rational& other) {
  *this = Rational(numerator * other.numerator,
    denominator * other.denominator);
  return *this;
}

Rational operator*(const Rational& left, const Rational& right) {
  Rational temp = left;
  temp *= right;
  return temp;
}

Rational& Rational::operator/=(const Rational& other) {
  *this = Rational(numerator * other.denominator,
                  denominator * other.numerator);
  return *this;
}

Rational operator/(const Rational& left, const Rational& right) {
  Rational temp = left;
  temp /= right;
  return temp;
}

std::string Rational::toString() const {
  if (denominator == 1) {
    return numerator.toString();
  }
  std::string result = numerator.toString();
  result += '/';
  result += denominator.toString();
  return result;
}

std::string Rational::asDecimal(size_t precision) const {
  std::string result = numerator < 0 ? "-" : "";
  BigInteger temp = numerator < 0 ? -numerator : numerator;
  result += (temp / denominator).toString();
  result += '.';
  temp %= denominator;
  for (size_t i = 0; i < precision; i++) {
    temp *= 10;
    result += (temp / denominator).toString();
    temp %= denominator;
  }
  return result;
}

Rational::operator double() const {
  std::string str = asDecimal(18);
  double result = std::stod(str);
  return result;
}

std::istream &operator>>(std::istream &is, Rational &rational)
{
  int x;
  is >> x;
  rational = Rational(x);
  return is;
}

std::ostream &operator<<(std::ostream &os, const Rational &rational) {
  os << rational.toString();
  return os;
}

long long MyPow(long long a, long long pow, long long p) {
  if (pow == 0) {
    return 1;
  }
  long long result = a;
  long long current_pow = 1;
  while (current_pow < pow) {
    if (current_pow * 2 <= pow) {
      result = result * result % p;
      current_pow *= 2;
    } else {
      result = result * a % p;
      ++current_pow;
    }
  }
  return result;
}


constexpr bool IsPrimeHelper(size_t n, size_t curr_d) {
  if (n == 1) {
    return false;
  }
  if (curr_d * curr_d > n) {
    return true;
  }
  if (n % curr_d == 0) {
    return false;
  }
  return IsPrimeHelper(n, curr_d + 1);
}

template<size_t N>
struct IsPrime {
  static const bool value = IsPrimeHelper(N, 2);
};

template <size_t N>
class Residue {
 private:
  long long value;

 public:
  Residue(int value) {
    while (value < 0) {
      value += N;
    }
    this->value = value % N;
  }
  Residue() : value(0) {}
  Residue(const Residue &other) = default;
  Residue &operator=(const Residue &other) = default;

  explicit operator int () const { return value; }

  Residue& operator+=(const Residue& other) {
    value = (value + other.value) % N;
    return *this;
  }

  Residue& operator-=(const Residue& other) {
    value = (value + N - other.value) % N;
    return *this;
  }

  Residue operator-() {
    return Residue(N - value);
  }

  Residue& operator*=(const Residue& other) {
    value = (value * other.value) % N;
    return *this;
  }

  Residue& operator/=(const Residue& other) {
    static_assert(IsPrime<N>::value, "Not prime");
    value = value * (MyPow(other.value, N - 2, N)) % N;
    return *this;
  }

  template <size_t T>
  friend bool operator==(const Residue<T>& a, const Residue<T>& b);
  template <size_t T>
  friend std::ostream& operator<< (std::ostream& os, const Residue<T>& residue);
};

template <size_t N>
std::ostream& operator<< (std::ostream& os, const Residue<N>& residue) {
  os << residue.value;
  return os;
}

template <size_t N>
Residue<N> operator+(const Residue<N>& a, const Residue<N>& b) {
  Residue<N> res(a);
  res += b;
  return res;
}

template <size_t N>
Residue<N> operator-(const Residue<N>& a, const Residue<N>& b) {
  Residue<N> res(a);
  res -= b;
  return res;
}

template <size_t N>
Residue<N> operator*(const Residue<N>& a, const Residue<N>& b) {
  Residue<N> res(a);
  res *= b;
  return res;
}

template <size_t N>
Residue<N> operator/(const Residue<N>& a, const Residue<N>& b) {
  Residue<N> res(a);
  res /= b;
  return res;
}

template <size_t N>
bool operator==(const Residue<N>& a, const Residue<N>& b) {
  return a.value == b.value;
}

template <size_t M, size_t N, typename Field = Rational>
class Matrix {
 private:
  std::array<std::array<Field, N>, M> table;

  static size_t find_main_row(size_t i, const Matrix<M, N, Field>& copy) {
    size_t main_row = i;
    for (size_t row = i + 1; row < M; ++row) {
      if (copy[row][i] != Field(0)) {
        main_row = row;
      }
    }
    return main_row;
  }

  static void substract(const Field& temp, size_t row, size_t i, Matrix<M, N, Field>& copy) {
    for (size_t col = 0; col < M; ++col) {
      copy.table[row][col] -= temp * copy.table[i][col];
    }
  }

  static void divide_row(const Field& z, Matrix<M, N, Field>& res, size_t row) {
    for (size_t j = 0; j < N; ++j) {
      res[row][j] /= z;
    }
  }

 public:
  Matrix() = default;
  Matrix(const Matrix<M, N, Field>& other) = default;
  template <typename T>
  Matrix(const std::initializer_list<std::initializer_list<T>>& list) {
    for (auto it = list.begin(); it != list.end(); ++it) {
      std::copy((*it).begin(), (*it).end(), table[it - list.begin()].begin());
    }
  }
  Matrix<M, N, Field>& operator=(const Matrix<M, N, Field>& other) = default;

  Matrix<M, N, Field>& operator+=(const Matrix<M, N, Field>& other) {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        table[i][j] += other.table[i][j];
      }
    }
    return *this;
  }

  Matrix<M, N, Field>& operator-=(const Matrix<M, N, Field>& other) {
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        table[i][j] -= other.table[i][j];
      }
    }
    return *this;
  }

  Matrix<N, M, Field>& operator*=(const Matrix<N, M, Field>& other) {
    static_assert(N == M, "N != M");
    Matrix<M, N, Field> res = (*this) * other;
    *this = res;
    return *this;
  }

  std::array<Field, M> getColumn(size_t j) const {
    std::array<Field, M> res;
    for (size_t i = 0; i < M; ++i) {
      res[i] = table[i][j];
    }
    return res;
  }

  std::array<Field, N> getRow(size_t i) const {
    std::array<Field, N> res;
    for (size_t j = 0; j < N; ++j) {
      res[j] = table[i][j];
    }
    return res;
  }

  std::array<Field, N>& operator[](size_t i) {
    return table[i];
  }

  const std::array<Field, N>& operator[](size_t i) const {
    return table[i];
  }

  Field trace() const {
    static_assert(N == M, "M != N");
    Field res = Field(0);
    for (size_t i = 0; i < M; ++i) {
      res += table[i][i];
    }
    return res;
  }

  Field det() const {
    static_assert(N == M, "M != N");
    Matrix<M, N, Field> copy = *this;
    if (N == 1) {
      return copy.table[0][0];
    }
    Field res = Field(1);
    for (size_t i = 0; i < M; ++i) {
      size_t main_row = find_main_row(i, copy);
      if (copy.table[main_row][i] == Field(0)) {
        return Field(0);
      }

      if (main_row != i) {
        std::swap(copy.table[i], copy.table[main_row]);
        res = -res;
      }

      res *= copy.table[i][i];

      for (size_t row = i + 1; row < M; ++row) {
        Field temp = copy.table[row][i] / copy.table[i][i];
        substract(temp, row, i, copy);
      }
    }
    return res;
  }

  size_t rank() const {
    Matrix<M, N, Field> copy = *this;
    size_t rank = 0;
    size_t row = 0;
    size_t col = 0;
    while (row < M && col < N) {
      size_t main_row = find_main_row(row, copy);
      if (copy.table[main_row][col] == Field(0)) {
        ++col;
        continue;
      }
      std::swap(copy.table[row], copy.table[main_row]);
      Field z = copy.table[row][col];
      divide_row(z, copy, row);
      for (size_t i = 0; i < M; ++i) {
        if (i != row) {
          Field temp = copy.table[i][col];
          substract(temp, i, row, copy);
        }
      }
      ++row;
      ++rank;
    }
    return rank;
  }

  Matrix<N, M, Field> transposed() const {
    Matrix<N, M, Field> res;
    for (size_t i = 0; i < M; ++i) {
      for (size_t j = 0; j < N; ++j) {
        res[j][i] = table[i][j];
      }
    }
    return res;
  }

  static Matrix<N, M, Field> unityMatrix() {
    static_assert(N == M, "M != N");
    Matrix<N, M, Field> res;
    for (size_t i = 0; i < M; ++i) {
      res[i][i] = Field(1);
    }
    return res;
  }

  Matrix<N, M, Field> inverted() const {
    static_assert(N == M, "M != N");
    Matrix<N, M, Field> copy =*this;
    Matrix<N, M, Field> result = unityMatrix();
    size_t row = 0;
    size_t col = 0;
    while (row < M && col < N) {
      size_t main_row = find_main_row(row, copy);
      std::swap(copy.table[row], copy.table[main_row]);
      std::swap(result.table[row], result.table[main_row]);
      Field z = copy.table[row][col];
      divide_row(z, copy, row);
      divide_row(z, result, row);
      for (size_t i = 0; i < M; ++i) {
        if (i != row) {
          Field temp = copy.table[i][col];
          substract(temp, i, row, copy);
          substract(temp, i, row, result);
        }
      }
      ++col;
      ++row;
    }
    return result;
  }

  void invert() {
    *this = inverted();
  }
  template <size_t T, size_t U, typename Field2>
  friend bool operator==(const Matrix<T, U, Field2>& lhs, const Matrix<T, U, Field2>& rhs);

};

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator+(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
  Matrix<M, N, Field> res = lhs;
  res += rhs;
  return res;
}


template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator-(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
  Matrix<M, N, Field> res = lhs;
  res -= rhs;
  return res;
}

template <size_t M, size_t N, typename Field>
Matrix<M, N, Field> operator*(const Field& lambda, const Matrix<M, N, Field>& rhs) {
  Matrix<M, N, Field> res = rhs;
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      res[i][j] = rhs[i][j] * lambda;
    }
  }
  return res;
}

template <size_t M, size_t N, typename Field>
bool operator==(const Matrix<M, N, Field>& lhs, const Matrix<M, N, Field>& rhs) {
  return lhs.table == rhs.table;
}

template <size_t M, size_t N, size_t K, typename Field>
Matrix<M, K, Field> operator*(const Matrix<M, N, Field>& lhs, const Matrix<N, K, Field>& rhs) {
  Matrix<M, K, Field> res;
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < K; ++j) {
      for (size_t k = 0; k < N; ++k) {
        res[i][j] += rhs[k][j] * lhs[i][k];
      }
    }
  }
  return res;
}

template <size_t N, typename Field=Rational>
using SquareMatrix = Matrix<N, N, Field>;