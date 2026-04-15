#include <memory>
#include <iterator>
#include <vector>

bool IsPrime(u_int64_t n) {
  if (n == 1) return false;
  u_int64_t i = 2;
  while (i * i <= n) {
    if (n % i == 0) return false;
    ++i;
  }
  return true;
}

u_int64_t GetNextPrime(uint64_t n) {
  u_int64_t curr = n;
  while (!IsPrime(curr)) {
    ++curr;
  }
  return curr;
}

const u_int64_t default_bucket_count = GetNextPrime(2);
template <typename Key,
          typename Value,
          typename Hash=std::hash<Key>,
          typename Equal=std::equal_to<Key>,
          typename Alloc=std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
  using NodeType = std::pair<const Key, Value>;

private:
  struct BaseNode {
    BaseNode* prev = nullptr;
    BaseNode* next = nullptr;
  };

  struct MapNode : BaseNode {
    NodeType kv;
    uint64_t hash;

    template <typename... Args>
    MapNode(Args&&... args) : kv{std::forward<Args>(args)...} {}
  };

public:
  using NodeAlloc = typename std::allocator_traits<Alloc>:: template rebind_alloc<MapNode>;
  using AllocTraits = std::allocator_traits<NodeAlloc>;

  template <bool IsConst>
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::conditional_t<IsConst, const NodeType, NodeType>;
    using pointer = std::conditional_t<IsConst, const NodeType*, NodeType*>;
    using reference = std::conditional_t<IsConst, const NodeType&, NodeType&>;
    using difference_type = std::ptrdiff_t;

    Iterator(const Iterator& it) { current_ = it.current_; }
    Iterator() : current_(nullptr) {}

    template<bool RConst, typename = std::enable_if_t<RConst==false>>
    Iterator(const Iterator<RConst>& it) { current_ = it.current_; }

    Iterator& operator=(const Iterator&) = default;

    operator Iterator<true>() { return {current_}; }

    reference operator*() const { return static_cast<MapNode*>(current_)->kv; }

    pointer operator->() const { return &(static_cast<MapNode*>(current_)->kv); }

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

    bool operator==(const Iterator& other) const { return current_ == other.current_; }

    bool operator!=(const Iterator& other) const { return current_ != other.current_; }

  private:
    Iterator(BaseNode* ptr) : current_(ptr) {}
    BaseNode* current_;
    friend class UnorderedMap;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  explicit UnorderedMap(u_int64_t bucket_count, const Hash& hash = Hash(),
               const Equal& key_equal=Equal(), const Alloc& alloc = Alloc()) :
    fakenode(),
    equal_(key_equal),
    hasher_(hash),
    alloc_(alloc),
    size_(0),
    buckets_(bucket_count, nullptr) {
    fakenode.prev = &fakenode;
    fakenode.next = &fakenode;
  }

  UnorderedMap(uint64_t bucket_count, const Alloc& alloc) :
  UnorderedMap(bucket_count, Hash(), Equal(), alloc) {}

  UnorderedMap(uint64_t bucket_count, const Hash& hash, const Alloc& alloc) :
  UnorderedMap(bucket_count, hash, Equal(), alloc) {}

  explicit UnorderedMap(const Alloc& alloc) :
  UnorderedMap(default_bucket_count, Hash(), Equal(), alloc) {}

  UnorderedMap () : UnorderedMap(default_bucket_count, Hash(), Equal(), Alloc()) {}

  UnorderedMap(const UnorderedMap& other) :
  UnorderedMap(other.buckets_.size(), other.hasher_, other.equal_,
    std::allocator_traits<Alloc>::select_on_container_copy_construction(other.get_allocator())) {
    for (auto it = other.begin(); it != other.end(); ++it) {
      emplace(*it);
    }
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this == &other) {
      return *this;
    }
    UnorderedMap copy(other);
    swap(copy);
    return *this;
  }

  UnorderedMap(UnorderedMap&& other) noexcept :
  fakenode(),
  equal_(std::move(other.equal_)),
  hasher_(std::move(other.hasher_)),
  alloc_(std::move(other.alloc_)),
  size_(std::move(other.size_)),
  buckets_(std::move(other.buckets_))  {
    other.size_ = 0;
    fakenode.prev = other.fakenode.prev;
    fakenode.next = other.fakenode.next;
    other.fakenode.prev->next = &fakenode;
    other.fakenode.next->prev = &fakenode;
    other.fakenode.prev = &other.fakenode;
    other.fakenode.next = &other.fakenode;
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    UnorderedMap copy(std::move(other));
    swap(copy);
    return *this;
  }

  iterator begin() noexcept { return {fakenode.next}; }
  const_iterator begin() const noexcept { return {fakenode.next}; }
  iterator end() noexcept { return {fakenode.prev->next}; }
  const_iterator end() const noexcept { return {fakenode.prev->next}; }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  uint64_t size() { return size_; }
  bool empty() { return size() == 0; }
  double load_factor() {return size() / static_cast<double>(buckets_.size()); }
  double max_load_factor() {return max_load_factor_; }

  iterator find(const Key& key) {
    uint64_t hash = hasher_(key) % buckets_.size();
    auto curr = buckets_[hash];
    while (curr != nullptr && curr != &fakenode &&
           static_cast<MapNode*>(curr)->hash % buckets_.size() == hash) {
      if (equal_(key, static_cast<MapNode*>(curr)->kv.first)) {
        return {curr};
      }
      curr = curr->next;
    }
    return end();
  }

private:
  iterator get_bucket_end(uint64_t hash) {
    uint64_t idx = hash % buckets_.size();
    auto curr = buckets_[idx];
    if (curr == nullptr) {
      return end();
    }
    while (curr != nullptr && curr != &fakenode &&
           static_cast<MapNode*>(curr)->hash % buckets_.size() == idx) {
      curr = curr->next;
    }
    if (curr == &fakenode) {
      return end();
    }
    return {curr};
  }

public:
  template <typename P>
  std::pair<iterator, bool> insert(P&& kv) {
    NodeAlloc node_alloc(alloc_);
    auto ptr = AllocTraits::allocate(node_alloc, 1);
    try {
      AllocTraits::construct(node_alloc, ptr, std::forward<P>(kv));
    } catch (...) {
      AllocTraits::deallocate(node_alloc, ptr, 1);
      throw;
    }
    return insert_logic(ptr);
  }

  std::pair<iterator, bool> insert(const NodeType& kv) {
    NodeAlloc node_alloc(alloc_);
    auto ptr = AllocTraits::allocate(node_alloc, 1);
    try {
      AllocTraits::construct(node_alloc, ptr, std::forward<const NodeType&>(kv));
    } catch (...) {
      AllocTraits::deallocate(node_alloc, ptr, 1);
      throw;
    }
    return insert_logic(ptr);
  }

  template<typename  InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  template <typename ... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    NodeAlloc node_alloc(alloc_);
    auto ptr = AllocTraits::allocate(node_alloc, 1);
    try {
      std::allocator_traits<Alloc>::construct(alloc_, &(ptr->kv), std::forward<Args>(args)...);
    } catch (...) {
      AllocTraits::deallocate(node_alloc, ptr, 1);
      throw;
    }
    return insert_logic(ptr);
  }

  iterator erase(const_iterator it) {
    auto ptr = static_cast<MapNode*>(it.current_);
    size_t idx = ptr->hash % buckets_.size();
    BaseNode* next_ptr = ptr->next;
    iterator next_it(next_ptr);
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
    if (buckets_[idx] == ptr) {
      if (next_ptr != &fakenode &&
          static_cast<MapNode*>(next_ptr)->hash % buckets_.size() == idx)
      {
        buckets_[idx] = next_ptr;
      } else {
        buckets_[idx] = nullptr;
      }
    }
    NodeAlloc node_alloc(alloc_);
    AllocTraits::destroy(node_alloc, ptr);
    AllocTraits::deallocate(node_alloc, ptr, 1);
    --size_;
    return next_it;
  }


  void erase(const_iterator first, const_iterator last) {
    for (; first != last; first = erase(first)) {
    }
  }

  template <typename NodeKey>
  Value& operator[](NodeKey&& key) {
    auto pair = emplace(std::forward<NodeKey>(key), std::forward<Value>(Value()));
    return pair.first->second;
  }

  Value& operator[](const Key& key) {
    auto pair = emplace(key, std::forward<Value>(Value()));
    return pair.first->second;
  }

  Value& at(const Key& key) {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Key not found");
    }
    return it->second;
  }

  const Value& at(const Key& key) const {
    auto it = find(key);
    if (it == end()) {
      throw std::out_of_range("Key not found");
    }
    return it->second;
  }

  void reserve(uint64_t new_bucket_count) {
    if (new_bucket_count <= buckets_.size()) {
      return;
    }
    uint64_t next_prime = GetNextPrime(new_bucket_count);
    rehash(next_prime);
  }

  Alloc get_allocator() const { return alloc_; }

  void swap(UnorderedMap& other) {
    if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
    auto fake_ptr = &fakenode;
    auto other_ptr = &other.fakenode;
    auto start = fake_ptr->next;
    auto finish = fake_ptr->prev;
    auto other_start = other_ptr->next;
    auto other_finish = other_ptr->prev;
    if (size_ > 0 && other.size_ > 0) {
      fake_ptr->next = other_start;
      fake_ptr->prev = other_finish;
      other_start->prev = fake_ptr;
      other_finish->next = fake_ptr;
      other_ptr->next = start;
      other_ptr->prev = finish;
      start->prev = other_ptr;
      finish->next = other_ptr;
    } else if (size_ == 0) {
      fake_ptr->next = other_start;
      fake_ptr->prev = other_finish;
      other_start->prev = fake_ptr;
      other_finish->next = fake_ptr;
      other_ptr->next = other_ptr;
      other_ptr->prev = other_ptr;
    } else {
      other_ptr->next = start;
      other_ptr->prev = finish;
      start->prev = other_ptr;
      finish->next = other_ptr;
      fake_ptr->next = fake_ptr;
      fake_ptr->prev = fake_ptr;
    }
    std::swap(size_, other.size_);
    std::swap(equal_, other.equal_);
    std::swap(hasher_, other.hasher_);
    std::swap(buckets_, other.buckets_);
  }

  ~UnorderedMap() {
    clear();
  }

private:
  std::pair<iterator, bool> insert_logic(MapNode* ptr) {
    NodeAlloc node_alloc(alloc_);
    uint64_t new_hash = hasher_(ptr->kv.first);
    ptr->hash = new_hash;
    auto it = find(ptr->kv.first);
    if (it != end()) {
      AllocTraits::destroy(node_alloc, ptr);
      AllocTraits::deallocate(node_alloc, ptr, 1);
      return {it, false};
    }
    auto b_end = get_bucket_end(new_hash);
    ptr->prev = b_end.current_->prev;
    ptr->next = b_end.current_;
    b_end.current_->prev->next = ptr;
    b_end.current_->prev = ptr;
    if (buckets_[new_hash % buckets_.size()] == nullptr) {
      buckets_[new_hash % buckets_.size()] = ptr;
    }
    ++size_;
    rehash_if_nedeed();
    return {iterator(ptr), true};
  }

  void rehash_if_nedeed() {
    if (max_load_factor() <= load_factor()) {
      uint64_t new_bucket_count = GetNextPrime(buckets_.size() * 2 + 1);
      rehash(new_bucket_count);
    }
  }

  void rehash(uint64_t new_bucket_count) {
    using MapPtrAlloc = typename std::allocator_traits<Alloc>:: template rebind_alloc<MapNode*>;
    std::vector<MapNode*, MapPtrAlloc> all_ptrs;
    all_ptrs.reserve(size_);
    for (BaseNode* cur = fakenode.next; cur != &fakenode; cur = cur->next) {
      all_ptrs.push_back(static_cast<MapNode*>(cur));
    }
    fakenode.next = &fakenode;
    fakenode.prev = &fakenode;
    std::vector<BaseNode*, VectorAlloc> new_buckets(new_bucket_count, nullptr);

    for (MapNode* node : all_ptrs) {
      uint64_t idx = node->hash % new_bucket_count;
      if (new_buckets[idx] == nullptr) {
        new_buckets[idx] = node;
        node->next = &fakenode;
        node->prev = fakenode.prev;
        fakenode.prev->next = node;
        fakenode.prev = node;
      } else {
        node->prev = new_buckets[idx]->prev;
        node->next = new_buckets[idx];
        new_buckets[idx]->prev->next = node;
        new_buckets[idx]->prev = node;
        new_buckets[idx] = node;
      }
    }
    buckets_ = std::move(new_buckets);
  }


  void clear() {
    if (size() > 0) {
      erase(begin(), end());
    }
  }

  using VectorAlloc = typename std::allocator_traits<Alloc>:: template rebind_alloc<BaseNode*>;

  BaseNode fakenode;
  [[no_unique_address]] Equal equal_;
  [[no_unique_address]] Hash hasher_;
  [[no_unique_address]] Alloc alloc_;
  uint64_t size_;
  std::vector<BaseNode*, VectorAlloc> buckets_;
  const double max_load_factor_ = 1.0;
};
