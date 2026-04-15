#include <memory>

struct BaseControlBlock {
  size_t shared_count = 0;
  size_t weak_count = 0;
  virtual ~BaseControlBlock() = default;
  virtual void destroy_value() = 0;
  virtual void deallocate_memory() = 0;
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr;

template<typename T>
class EnableSharedFromThis {
public:

  SharedPtr<T> shared_from_this() const noexcept { return weak_ptr_.lock(); }

private:
  friend class WeakPtr<T>;
  friend class SharedPtr<T>;

  WeakPtr<T> weak_ptr_;
};

template<typename T>
class SharedPtr {
  T* ptr_;
  BaseControlBlock* cb_;
  using element_type = T;

  template<typename U, typename Deleter, typename Alloc>
  struct ControlBlockRegular : BaseControlBlock {
    U* ptr_;
    Deleter del;
    Alloc alloc;

    ControlBlockRegular(U* ptr, const Deleter& d, const Alloc& a) : ptr_(ptr),  del(d), alloc(a) {}

    void destroy_value() override { del(ptr_); }
    void deallocate_memory() override {
      using CBAlloc = typename std::allocator_traits<Alloc>::
      template rebind_alloc<ControlBlockRegular<U, Deleter, Alloc>>;
      CBAlloc cb_alloc(alloc);
      std::allocator_traits<CBAlloc>::deallocate(cb_alloc, this, 1);
    }
  };

  template<typename U, typename Alloc=std::allocator<U>>
  struct ControlBlockMakeShared : BaseControlBlock {
    Alloc alloc;
    U value;
    using CBAlloc = typename std::allocator_traits<Alloc>
    ::template rebind_alloc<ControlBlockMakeShared<U, Alloc>>;

    template <typename... Args>
    explicit ControlBlockMakeShared(const Alloc& allocator, Args&& ... args) :
    alloc(allocator), value(std::forward<Args>(args)...) {
      ++shared_count;
    }

    void destroy_value() override {
      std::allocator_traits<Alloc>::destroy(alloc, &value);
    }
    void deallocate_memory() override {
      CBAlloc cb_alloc(alloc);
      std::allocator_traits<CBAlloc>::deallocate(cb_alloc, this, 1);
    }
  };

  template<typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&...);

  template<typename SomeAlloc>
  explicit SharedPtr(ControlBlockMakeShared<T, SomeAlloc>* cbp) : ptr_(&(cbp->value)), cb_(cbp) {
    cb_->shared_count = 1;
    if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
      ptr_->weak_ptr_ = *this;
    }
  }

  public:

  explicit SharedPtr() : ptr_(nullptr), cb_(nullptr) {}

  SharedPtr(const SharedPtr& other ) : ptr_(other.ptr_), cb_(other.cb_) {
    if (ptr_ != nullptr) {
      ++cb_->shared_count;
    }
  }

  template <typename Y>
  SharedPtr(const SharedPtr<Y>& r ) noexcept :
    ptr_(r.ptr_),
    cb_(r.cb_) {
    if (ptr_ != nullptr) {
      ++cb_->shared_count;
    }
  }



  template<typename Y, typename Deleter, typename Alloc=std::allocator<Y>>
  SharedPtr(Y* ptr, const Deleter& del, const Alloc& alloc=Alloc()) :
    ptr_(ptr) {
    using CBAlloc = typename std::allocator_traits<Alloc>:: template rebind_alloc<ControlBlockRegular<T, Deleter, Alloc>>;
    CBAlloc cb_alloc(alloc);
    auto* memory_ptr = std::allocator_traits<CBAlloc>::allocate(cb_alloc, 1);
    try {
      new(memory_ptr) ControlBlockRegular<T, Deleter, Alloc>(ptr, del, alloc);
    } catch (...) {
      std::allocator_traits<CBAlloc>::deallocate(cb_alloc, memory_ptr, 1);
      throw;
    }
    cb_ = memory_ptr;
    if (ptr_ != nullptr) {
      ++cb_->shared_count;
    }
  }

  template<typename Y>
  explicit SharedPtr(Y* ptr) : SharedPtr(ptr, std::default_delete<Y>()) {}

  template<class Y>
  SharedPtr(const SharedPtr<Y>& other, element_type* ptr) noexcept : ptr_(ptr), cb_(other.cb_) {
    if (ptr_ != nullptr) {
      ++cb_->shared_count;
    }
  }

  SharedPtr(SharedPtr&& r ) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    r.ptr_ = nullptr;
    r.cb_ = nullptr;
  }

  template<typename  Y>
  SharedPtr(SharedPtr<Y>&& r ) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    r.ptr_ = nullptr;
    r.cb_ = nullptr;
  }

  template<typename Y>
  explicit SharedPtr(const WeakPtr<Y>& r ) {
    if (r.expired()) {
      throw std::bad_weak_ptr();
    }
    ptr_ = r.ptr_;
    cb_ = r.cb_;
    ++cb_->shared_count;
  }

  template<typename Y>
  SharedPtr& operator=(const SharedPtr<Y>& r ) noexcept {
    SharedPtr<T> tmp(r);
    swap(tmp);
    return *this;
  }

  template<typename  Y>
  SharedPtr& operator=(SharedPtr<Y>&& r ) noexcept {
    SharedPtr<T> tmp(std::move(r));
    swap(tmp);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& r) noexcept {
    if (this != &r) {
      SharedPtr tmp(r);
      swap(tmp);
    }
    return *this;
  }

  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }
  size_t use_count() const { return cb_->shared_count; }

  T* get() const { return ptr_; }
  explicit operator bool() const { return ptr_ != nullptr; }

  void reset() {
    SharedPtr copy;
    swap(copy);
  }

  template<typename Y, typename Deleter = std::default_delete<Y>, typename Alloc = std::allocator<Y>>
  void reset(Y* ptr, Deleter del = Deleter(), Alloc alloc=Alloc()) {
    reset();
    SharedPtr<Y> tmp(ptr, del, alloc);
    swap(tmp);
  }

  void swap(SharedPtr& r) noexcept {
    std::swap(cb_, r.cb_);
    std::swap(ptr_, r.ptr_);
  }

  ~SharedPtr() {
    if (cb_ == nullptr) {
      return;
    }
    --cb_->shared_count;
    if (cb_->shared_count == 0) {
      cb_->destroy_value();
    }
    if constexpr (!std::is_base_of_v<EnableSharedFromThis<T>, T>) {
      if (cb_->shared_count == 0 && cb_->weak_count ==0) {
        cb_->deallocate_memory();
      }
    }
  }

  template <typename U>
  friend class WeakPtr;

  template<typename U>
  friend class SharedPtr;

};

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& allocator, Args&& ... args) {
  using CBAlloc = typename std::allocator_traits<Alloc>::
  template rebind_alloc<typename SharedPtr<T>::template ControlBlockMakeShared<T, Alloc>>;
  CBAlloc cb_alloc(allocator);
  auto* ptr = cb_alloc.allocate(1);
  try {
    std::allocator_traits<CBAlloc>::construct(cb_alloc, ptr, allocator, std::forward<Args>(args)...);
  } catch (...) {
    std::allocator_traits<CBAlloc>::deallocate(cb_alloc, ptr, 1);
    throw;
  }
  return SharedPtr<T>(ptr);
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&& ... args) { return allocateShared<T, std::allocator<T>, Args...>(std::allocator<T>(), std::forward<Args>(args)...); }

template<typename T>
class WeakPtr {
  T* ptr_;
  BaseControlBlock* cb_;

  template <typename U>
  friend class WeakPtr;

  template<typename U>
  friend class SharedPtr;

public:
  WeakPtr() :ptr_(nullptr), cb_(nullptr) {}


  WeakPtr(const WeakPtr& r) noexcept
    : ptr_(r.ptr_), cb_(r.cb_)
  {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  template <typename Y>
  WeakPtr(const WeakPtr<Y>& r ) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  template<typename Y>
  WeakPtr(const SharedPtr<Y>& r) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    if (cb_ != nullptr) {
      ++cb_->weak_count;
    }
  }

  WeakPtr(WeakPtr&& r) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    r.ptr_ = nullptr;
    r.cb_ = nullptr;
  }

  template<typename  Y>
  WeakPtr(WeakPtr<Y>&& r) noexcept : ptr_(r.ptr_), cb_(r.cb_) {
    r.ptr_ = nullptr;
    r.cb_ = nullptr;
  }

  WeakPtr& operator=(const WeakPtr& r) {
    swap(WeakPtr<T>(r));
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=( const WeakPtr<Y>& r ) noexcept {
    swap(WeakPtr<T>(r));
    return *this;
  }

  template< typename  Y>
  WeakPtr& operator=(const SharedPtr<Y>& r ) noexcept {
    WeakPtr<T> tmp(r);
    swap(tmp);
    return *this;
  }

  WeakPtr& operator=( WeakPtr&& r) noexcept {
    swap(WeakPtr<T>(std::move(r)));
    return *this;
  }

  template<typename  Y>
  WeakPtr& operator=(WeakPtr<Y>&& r ) noexcept {
    swap(WeakPtr<T>(std::move(r)));
    return *this;
  }

  void swap(WeakPtr& r) noexcept {
    std::swap(ptr_, r.ptr_);
    std::swap(cb_, r.cb_);
  }

  size_t use_count() const {
    if (cb_ == nullptr) {
      return 0;
    }
    return cb_->shared_count;
  }

  bool expired() const { return use_count() == 0; }

  SharedPtr<T> lock() const { return SharedPtr<T>(*this); }

  ~WeakPtr() {
    if (cb_ == nullptr) {
      return;
    }
    --cb_->weak_count;
    if (cb_->weak_count == 0 && cb_->shared_count == 0) {
      cb_->deallocate_memory();
    }
  }
};
