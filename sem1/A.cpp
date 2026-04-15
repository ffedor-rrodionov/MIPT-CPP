#include <cstring>
#include <iostream>

class String {
private:
  char *buffer_ = nullptr;
  size_t size_;
  size_t capacity_;

  void swap(String& other) {
    std::swap(buffer_, other.buffer_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  size_t GeneralFind(const String& substr, int delta) const {
    long long pos = delta > 0 ? 0 : static_cast<long long>(size_) -
      static_cast<long long>(substr.size_);
    while ((pos + substr.size_) <= size_ &&
          ((delta == -1 && pos > -1) || (delta == 1 && pos < static_cast<long long>(size_)))) {
      if (strncmp(buffer_ + pos, substr.buffer_, substr.size_) == 0) {
        return pos;
      }
      pos += delta;
          }
    return length();
  }

  void relocate(size_t new_capacity) {
    char* new_buffer = new char[new_capacity + 1];
    memcpy(new_buffer, buffer_, size_);
    delete[] buffer_;
    buffer_ = new_buffer;
    capacity_ = new_capacity;
  }

public:
  String() // default constructor
    : buffer_(new char[1])
    , size_(0)
    , capacity_(0) {
    buffer_[0] = '\0';
  }

   String(const char* str) // c-style constructor
    : buffer_(new char[strlen(str) + 1])
    , size_(strlen(str))
    , capacity_(strlen(str))
  {
    memcpy(buffer_, str, size_);
    buffer_[size_] = '\0';
  }

  String(size_t size, char c)
    : buffer_(new char[size + 1])
    , size_(size)
    , capacity_(size) {
    memset(buffer_, c , size_);
    buffer_[size] = '\0';
  }

  String(const String& other) // copy constructor
    : buffer_(new char[other.capacity_ + 1])
    , size_(other.size_)
    , capacity_(other.capacity_)
  {
    memcpy(buffer_, other.buffer_, size_);
    buffer_[size_] = '\0';
  }

  String& operator=(String other) {
    if (this == &other) {
      return *this;
    }
    swap(other);
    return *this;
  }

  char& operator[](size_t index) {
    return buffer_[index];
  }

  const char& operator[](size_t index) const {
    return buffer_[index];
  }

  size_t length() const {
    return size_;
  }

  size_t size() const {
    return size_;
  }

  size_t capacity() const {
    return capacity_;
  }

  char& front() {
    return buffer_[0];
  }

  const char& front() const {
    return buffer_[0];
  }

  char& back() {
    int index = static_cast<int>(size_) - 1;
    return buffer_[index];
  }

  const char& back() const {
    int index = static_cast<int>(size_) - 1;
    return buffer_[index];
  }

  void push_back(char c) {
    if (capacity_ == 0) {
      relocate(1);
    } else if (size_ == capacity_) {
      relocate(capacity_ * 2);
    }
    buffer_[size_++] = c;
    buffer_[size_] = '\0';
  }

  void pop_back() {
    buffer_[--size_] = '\0';
  }

  String& operator+=(char c) {
    push_back(c);
    return *this;
  }

  String& operator+=(const String& right) {
    if (size_ + right.size_ > capacity_) {
      relocate(size_ + right.size_);
      memcpy(buffer_ + size_, right.buffer_, right.size_);
      size_ += right.size_;
      buffer_[size_] = '\0';
      return *this;
    }
    memcpy(buffer_ + size_, right.buffer_, right.size_);
    size_ += right.size_;
    buffer_[size_] = '\0';
    return *this;
  }

  bool empty() { return size_ == 0; }

  void clear() {
    size_ = 0;
    buffer_[0] = '\0';
  }

  void shrink_to_fit() {
    if (size_ < capacity_) {
      relocate(size_);
      buffer_[size_] = '\0';
    }
  }

  char *data() {
    return buffer_;
  }

  const char *data() const {
    return buffer_;
  }

  friend std::ostream &operator<<(std::ostream &os, const String &str);

  size_t find(const String& substr) const {
    return GeneralFind(substr, 1);
  }

  size_t rfind(const String& substr) const {
    return GeneralFind(substr, -1);
  }

  String substr(size_t start, size_t count) const {
    String str = String(count + 1, '\0');
    memcpy(str.buffer_, buffer_ + start, count);
    return str;
  }

  ~String() {
    delete[] buffer_;
  }
};

bool operator<(const String& left, const String& right) {
  return std::strcmp(left.data(), right.data()) < 0;
}

bool operator>(const String& left, const String& right) {
  return right < left;
}

bool operator<=(const String& left, const String& right) {
  return !(left > right);
}

bool operator>=(const String& left, const String& right) {
  return !(left < right);
}

bool operator==(const String& left, const String &right) {
  return std::strcmp(left.data(), right.data()) == 0;
}

bool operator!=(const String& left, const String& right) {
  return !(left == right);
}

std::ostream &operator<<(std::ostream &out, const String &right) {
  out << right.buffer_;
  return out;
}

std::istream &operator>>(std::istream &is, String &str)
{
  str.clear();
  char c;
  while (is.get(c) && std::isspace(c)) {}
  str.push_back(c);
  if (is.peek() == EOF) {
    return is;
  }
  while (is.get(c) && !std::isspace(c)) {
    str.push_back(c);
  }
  return is;
}

String operator+(const char left, const String& right) {
  String result;
  result += left;
  result += right;
  return result;
}

String operator+(const String& left, const String& right) {
  String result = left;
  result += right;
  return result;
}

String operator+(const String& left, const char right) {
  String result;
  result += left;
  result += right;
  return result;
}
