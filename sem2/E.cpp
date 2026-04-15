#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <stdexcept>

constexpr std::size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

template <typename T, size_t Capacity>
struct IteratorFields {
  T* ptr;
  size_t start;
  size_t pos;
  IteratorFields() = default;
  IteratorFields(const IteratorFields<T, Capacity>&) = default;
  size_t capacity() const { return Capacity; }
};

template <typename T>
struct IteratorFields<T, DYNAMIC_CAPACITY> {
  T* ptr;
  size_t start;
  size_t pos;
  size_t cap;
  IteratorFields() = default;
  IteratorFields(const IteratorFields<T, DYNAMIC_CAPACITY>&) = default;
  size_t capacity() const { return cap; }
};


template <typename T, size_t Capacity>
struct alignas(T) Buffer {
  std::byte data_[Capacity * sizeof(T)];

  Buffer(const Buffer<T, Capacity>&) {}

  Buffer(size_t capacity) {
    if (capacity != Capacity) {
      throw std::invalid_argument("capacity must be equal to Capacity");
    }
  }

  size_t capacity() const { return Capacity; }

  T* data() { return reinterpret_cast<T*>(data_); }

  const T* data() const { return reinterpret_cast<const T*>(data_); }
};

template <typename T>
struct Buffer<T, DYNAMIC_CAPACITY> {
  size_t capacity_ = DYNAMIC_CAPACITY;
  T* data_ = nullptr;

  Buffer(size_t capacity) : capacity_(capacity) {
    data_ = reinterpret_cast<T*>(new std::byte[capacity * sizeof(T)]);
  }

  Buffer(const Buffer<T, DYNAMIC_CAPACITY>& rhs) : Buffer(rhs.capacity()) {}

  size_t capacity() const { return capacity_; }

  T* data() { return reinterpret_cast<T*>(data_); }
  const T* data() const { return reinterpret_cast<const T*>(data_); }


  ~Buffer() { delete[] reinterpret_cast<std::byte*>(data_); }
};

template<typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
  template<bool IsConst>
  class Iterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using reference = std::conditional_t<IsConst, const T& , T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using difference_type = std::ptrdiff_t;

    Iterator(const Iterator&) = default;
    Iterator& operator=(const Iterator &) = default;

    reference operator*() const {return *(fields.ptr + get_index()); }
    pointer operator->() const { return fields.ptr + get_index(); }
    difference_type operator-(const Iterator& rhs) const { return fields.pos - rhs.fields.pos; }

    Iterator &operator++() {
      ++fields.pos;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++fields.pos;
      return tmp;
    }

    Iterator &operator--() {
      --fields.pos;
      return *this;
    }

    Iterator operator--(int) {
      Iterator tmp = *this;
      --fields.pos;
      return tmp;
    }

    Iterator &operator+=(difference_type rhs) {
      fields.pos += rhs;
      return *this;
    }

    Iterator &operator-=(difference_type rhs) {
      fields.pos -= rhs;
      return *this;
    }

    operator Iterator<true>() const { return {fields.ptr, fields.start, fields.pos, fields.capacity()}; }

    bool operator==(const Iterator &rhs) const { return fields.pos == rhs.fields.pos; }
    bool operator!=(const Iterator &rhs) const { return fields.pos != rhs.fields.pos; }
    bool operator<(const Iterator &rhs) const { return fields.pos < rhs.fields.pos; }
    bool operator>(const Iterator &rhs) const { return fields.pos > rhs.fields.pos; }
    bool operator<=(const Iterator &rhs) const { return fields.pos <= rhs.fields.pos; }
    bool operator>=(const Iterator &rhs) const { return fields.pos >= rhs.fields.pos; }

    friend Iterator operator+(const Iterator &lhs, difference_type rhs) {
      Iterator temp(lhs);
      temp += rhs;
      return temp;
    }

    friend Iterator operator+(difference_type lhs, const Iterator& it) { return it + lhs; }

    friend Iterator operator-(const Iterator &lhs, difference_type rhs) {
      Iterator temp(lhs);
      temp -= rhs;
      return temp;
    }

  private:
    IteratorFields<T, Capacity> fields;

    Iterator(T* ptr, size_t start, size_t pos, size_t capacity) {
      fields.ptr = ptr;
      fields.start = start;
      fields.pos = pos;
      if constexpr (Capacity == DYNAMIC_CAPACITY) {
        fields.cap = capacity;
      }
    }

    size_t capacity() const { return fields.capacity(); }
    size_t get_index() const {return (fields.start + fields.pos) % capacity(); }
    T* data() { return fields.ptr + get_index(); }
    friend class CircularBuffer<T, Capacity>;
  };

public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<Iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

  std::size_t size() const noexcept { return size_; }
  std::size_t capacity() const noexcept { return buffer_.capacity(); }
  bool full() const noexcept { return size() == capacity(); }
  bool empty() const noexcept { return size() == 0; }


  CircularBuffer() :start_(0), size_(0), buffer_(Capacity) {}

  explicit CircularBuffer(std::size_t capacity) :
    start_(0), size_(0), buffer_(capacity) {}

  CircularBuffer(const CircularBuffer& rhs) :
    start_(rhs.start_),
    size_(rhs.size_),
    buffer_(rhs.buffer_)
  {
    size_t count = 0;
    const size_t cMod = rhs.capacity();
    size_t end = start_ + size_;
    auto ptr = buffer_.data();
    auto ptr_rhs = rhs.buffer_.data();
    try {
      for (size_t i = start_; i < end; ++i) {
        new (ptr + (i % cMod)) T(ptr_rhs[i % cMod]);
        ++count;
      }
    } catch (...) {
      size_t end = start_ + count;
      for (size_t i = start_; i < end; ++i) {
        (ptr + i % cMod)->~T();
      }
      throw;
    }
  }

  void swap(CircularBuffer<T, Capacity>& rhs) {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      std::swap(start_, rhs.start_);
      std::swap(size_, rhs.size_);
      std::swap(buffer_.capacity_, rhs.buffer_.capacity_);
      std::swap(buffer_.data_, rhs.buffer_.data_);
    }
  }

  CircularBuffer& operator=(const CircularBuffer& rhs) {
    if (this == &rhs) {
      return *this;
    }
    if constexpr (Capacity != DYNAMIC_CAPACITY) {
      clear();
      for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        push_back(*it);
      }
    } else {
      CircularBuffer copy(rhs);
      swap(copy);
    }
    return *this;
  }

  iterator begin() { return {buffer_.data(), start_, 0, capacity()}; }
  const_iterator begin() const { return {const_cast<T*>(buffer_.data()), start_, 0, capacity()}; }
  iterator end() noexcept { return {buffer_.data(), start_, size(), capacity()}; }
  const_iterator end() const { return {const_cast<T*>(buffer_.data()), start_, size(), capacity()}; }
  const_iterator cbegin() const {return begin(); }
  const_iterator cend() const { return end(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return rbegin(); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return rend(); }
  const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
  const_reverse_iterator crend() const {return const_reverse_iterator(cbegin()); }

  void push_back(const T& item) {
    if (size() == capacity()) {
      size_t index = start_;
      buffer_.data()[index] = item;
      start_ = (index + 1) % capacity();
    } else {
      size_t index = (start_ + size()) % capacity();
      new (buffer_.data() + index) T(item);
      ++size_;
    }
  }

  void pop_back() {
    size_t index = (start_ + size() - 1) % capacity();
    (buffer_.data() + index)->~T();
    --size_;
  }

  void push_front(const T& item) {
    if (size() == capacity()) {
      size_t index = (start_ + size() - 1) % capacity();
      buffer_.data()[index] = item;
      start_ = index;
    } else {
      size_t index = (start_ + capacity() - 1) % capacity();
      new (buffer_.data() + index) T(item);
      start_ = (start_ + capacity() - 1) % capacity();
      ++size_;
    }
  }

  void pop_front() {
    (buffer_.data() + start_)->~T();
    --size_;
    start_ = (start_+ 1) % capacity();
  }

  T& operator[](size_t index) { return buffer_.data()[(start_ + index) % capacity()]; }
  const T& operator[](size_t index) const { return buffer_.data()[(start_ + index) % capacity()]; }

  T& at(size_t index) {
    if (index >= size()) {
      throw std::out_of_range("Circular buffer index out of range");
    }
    return operator[](index);
  }

  const T& at(size_t index) const {
    if (index >= size()) {
      throw std::out_of_range("Circular buffer index out of range");
    }
    return operator[](index);
  }

  void insert(iterator it, const T& item) {
    if (it == begin() && full()) {
      return;
    }
    size_t pos = (it - begin()) % capacity();
    size_t cMod = capacity();
    if (full()) {
      pop_front();
      pos = (pos - 1 + cMod) % cMod;
    }
    for (size_t i = size() ; i > pos; --i) {
      size_t to_ind = (start_ + i) % cMod;
      size_t from_ind = (start_ + i - 1) % cMod;
      T temp(buffer_.data()[from_ind]);
      (buffer_.data() + from_ind)->~T();
      new (buffer_.data() + to_ind) T(temp);
    }
    size_t insert_ind = (start_ + pos) % cMod;
    new (buffer_.data() + insert_ind) T(item);
    ++size_;
  }

  void erase(iterator it) {
    for (auto iter = it; iter != end() - 1; ++iter) {
      (*iter.data()).~T();
      new (iter.data()) T(*(iter+1));
    }
    auto iter = end() - 1;
    (*iter.data()).~T();
    --size_;
  }

  void clear() { while(!empty()) pop_back(); }

  ~CircularBuffer() { clear(); }

private:
  size_t start_;
  size_t size_;
  Buffer<T, Capacity> buffer_;
};
