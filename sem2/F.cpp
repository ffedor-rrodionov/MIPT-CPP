#include <memory>
#include <iterator>

template <size_t N>
struct StackStorage {
  std::byte memory[N];
  std::byte* current = memory;

  StackStorage() = default;
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;
};

template <typename T, size_t N>
struct StackAllocator {

  using value_type = T;

  StackStorage<N>* ptr;

  StackAllocator(StackStorage<N>& storage)
  : ptr(&storage) {}

  template <typename U>
  StackAllocator(StackAllocator<U, N> other) : ptr(other.ptr)
  {}

  template <typename U>
  StackAllocator operator=(StackAllocator<U, N> other) {
    ptr = other.ptr;
    return *this;
  }

  template <typename U>
  bool operator==(const StackAllocator<U, N>& other) const {
    return ptr == other.ptr;
  }

  template <typename U>
  bool operator!=(const StackAllocator<U, N>& other) const {
    return !(*this == other);
  }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  T* allocate(size_t n) const {
    size_t alignment = alignof(T);
    if (reinterpret_cast<uintptr_t>(ptr->current) % alignment != 0) {
      ptr->current += (alignment - reinterpret_cast<uintptr_t>(ptr->current) % alignment);
    }
    T* result = reinterpret_cast<T*>(ptr->current);
    ptr->current += n * sizeof(T);
    return result;
  }

  void deallocate(T*, size_t) {}
};

template <typename T, typename Allocator=std::allocator<T>>
class List {
private:
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
  };

  struct Node : public BaseNode {
    T value;
    Node(const T& value, BaseNode* prev, BaseNode* next) : value(value) {
      this->prev = prev;
      this->next = next;
    }

    Node(BaseNode* prev, BaseNode* next) : value() {
      this->prev = prev;
      this->next = next;
    }
  };

  template <bool IsConst>
  class Iterator {
    public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using difference_type = std::ptrdiff_t;

    Iterator(const Iterator& it) {
      current_ = it.current_;
    }

    template<bool RConst, typename = std::enable_if_t<RConst==false>>
    Iterator(const Iterator<RConst>& it) {
      current_ = it.current_;
    }

    Iterator& operator=(const Iterator&) = default;

    operator Iterator<true>() {
      return {current_};
    }

    reference operator*() const {
      return static_cast<Node*>(current_)->value;
    }

    pointer operator->() const {
      return &(current_->value);
    }

    Iterator& operator++() {
      current_ = current_->next;
      return *this;
    }

    Iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    Iterator& operator--() {
      current_ = current_->prev;
      return *this;
    }

    Iterator operator--(int) {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }

    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }

  private:
    Iterator(BaseNode* ptr) : current_(ptr) {}
    BaseNode* current_;
    friend class List;

  };

public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List(const Allocator& alloc=Allocator()) : alloc_(alloc) {
    size_ = 0;
    fakenode.next = &fakenode;
    fakenode.prev = &fakenode;
  }

  List(size_t size, const T& value, const Allocator& alloc=Allocator()) : List(alloc) {
    for (size_t i = 0; i < size; i++) {
      push_back(value);
    }
  }

  List(size_t size, const Allocator& alloc=Allocator()) : List(alloc) {
    for (size_t i = 0; i < size; i++) {
      auto ptr = alloc_.allocate(1);
      AllocTraits::construct(alloc_, ptr, fakenode.prev, &fakenode);
      fakenode.prev->next = ptr;
      fakenode.prev = ptr;
      ++size_;
    }
  }

  List(const List& other) : List(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator())) {
    for (auto it = other.begin(); it != other.end(); ++it) {
      push_back(*it);
    }
  }

  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }
    List copy(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value ? other.get_allocator() : get_allocator());
    for (auto it = other.begin(); it != other.end(); ++it) {
      copy.push_back(*it);
    }
    swap(copy);
    return *this;
  }

  void push_back(const T& value) {
    insert(end(), value);
  }

  void pop_back() {
    erase(--end());
  }

  void clear() {
    while (!empty()) {
      pop_back();
    }
  }

  void pop_front() {
    erase(begin());
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  Allocator get_allocator() const {
    using ReboundAlloc = typename AllocTraits::template rebind_alloc<T>;
    return ReboundAlloc(alloc_);
  }

  iterator begin() noexcept {
    return {fakenode.next};
  }

  const_iterator begin() const noexcept {
    return {fakenode.next};
  }

  iterator end() noexcept {
    return {fakenode.prev->next};
  }

  const_iterator end() const noexcept {
    return {fakenode.prev->next};
  }

  const_iterator cbegin() const noexcept {
    return begin();
  }

  const_iterator cend() const noexcept {
    return end();
  }

  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  size_t size() const noexcept {
    return size_;
  }

  size_t empty() const noexcept {
    return size_ == 0;
  }

  void insert(const_iterator pos, const T& value) {
    auto ptr = AllocTraits::allocate(alloc_, 1);
    try {
      AllocTraits::construct(alloc_, ptr, value, pos.current_->prev, pos.current_);
      pos.current_->prev->next = ptr;
      pos.current_->prev = ptr;
      ++size_;
    } catch(...) {
      AllocTraits::deallocate(alloc_, ptr, 1);
      throw;
    }
  }

  void erase(const_iterator pos) noexcept {
    auto ptr = static_cast<Node*>(pos.current_);
    pos.current_->next->prev = pos.current_->prev;
    pos.current_->prev->next = pos.current_->next;
    AllocTraits::destroy(alloc_, ptr);
    AllocTraits::deallocate(alloc_, ptr, 1);
    --size_;
  }

  ~List() {
    clear();
  }
private:
  BaseNode fakenode;
  size_t size_;
  using NodeAlloc = typename std::allocator_traits<Allocator>:: template rebind_alloc<Node>;
  using AllocTraits = std::allocator_traits<NodeAlloc>;
  NodeAlloc alloc_;

  void swap(List& other) noexcept {
    std::swap(alloc_, other.alloc_);
    std::swap(fakenode.prev->next, other.fakenode.prev->next);
    std::swap(fakenode.next->prev, other.fakenode.next->prev);
    std::swap(fakenode, other.fakenode);
    std::swap(size_, other.size_);
  }
};