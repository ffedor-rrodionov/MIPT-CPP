#include <type_traits>
#include <utility>

template<typename... Types>
class Tuple;

template <typename... Types>
auto makeTuple(Types&&... args) -> Tuple<std::decay_t<Types>...> { return { std::forward<Types>(args)... }; }

template <typename... Types>
Tuple<Types&...> tie(Types&... args) { return { args... }; }

template <typename... Types>
Tuple<Types&&...> forwardAsTuple(Types&&... args) { return { std::forward<Types>(args)... }; }

template <typename T, typename... Types>
struct CountType;

template <typename T>
struct CountType<T> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename Head, typename... Tail>
struct CountType<T, Head, Tail...> : std::integral_constant<std::size_t,
    (std::is_same_v<T, Head> ? 1 : 0) + CountType<T, Tail...>::value> {};

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>& t) {
    if constexpr (N == 0) {
      return static_cast<decltype(t.head_)&>(t.head_);
    } else {
      return get<N-1>(t.tail_);
    }
}

template <size_t N, typename... Types>
decltype(auto) get(Tuple<Types...>&& t) {
  if constexpr (N == 0) {
    return static_cast<decltype(t.head_)&&>(t.head_);
  } else {
    return get<N-1>(std::move(t.tail_));
  }
}

template <size_t N, typename... Types>
decltype(auto) get(const Tuple<Types...>& t) {
    if constexpr (N == 0) {
      return static_cast<const decltype(t.head_)&>(t.head_);
    } else {
      return get<N-1>(t.tail_);
    }
}

template <size_t N, typename... Types>
decltype(auto) get(const Tuple<Types...>&& t) {
    if constexpr (N == 0) {
      return static_cast<const decltype(t.head_)&&>(t.head_);
    } else {
      return get<N-1>(t.tail_);
    }
}

template <class T, class... Types>
constexpr T& get(Tuple<Types...>& t) {
  static_assert(CountType<T, Types...>::value == 1);
  if constexpr (std::is_same_v<decltype(t.head_), T>) {
    return t.head_;
  } else {
    return get<T>(t.tail_);
  }
}

template <class T, class... Types>
constexpr T&& get(Tuple<Types...>&& t) {
  static_assert(CountType<T, Types...>::value == 1);
  if constexpr (std::is_same_v<decltype(t.head_), T>) {
    return std::move(t.head_);
  } else {
    return get<T>(std::move(t.tail_));
  }
}

template <class T, class... Types>
constexpr const T& get(const Tuple<Types...>& t) {
  static_assert(CountType<T, Types...>::value == 1);
  if constexpr (std::is_same_v<decltype(t.head_), T>) {
    return t.head_;
  } else {
    return get<T>(t.tail_);
  }
}

template <class T, class... Types>
constexpr const T&& get(const Tuple<Types...>&& t) {
  static_assert(CountType<T, Types...>::value == 1);
  if constexpr (std::is_same_v<decltype(t.head_), T>) {
    return std::move(t.head_);
  } else {
    return get<T>(std::move(t.tail_));
  }
}

template <typename... Ti>
concept AllDefaultConstructible = (std::is_default_constructible_v<Ti> && ...);

template <typename... Ti>
concept AllImplicitlyConstructible = (std::__is_implicitly_default_constructible<Ti>::value && ...);

template <typename... Ti>
concept AllCopyConstructible = (std::is_copy_constructible_v<Ti> && ...);

template <typename... Ti>
concept ALLConvertible = (std::is_convertible_v<const Ti&, Ti> && ...);

template <typename T>
struct TupleSize;

template <typename... Types>
struct TupleSize<Tuple<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};

template <typename Tup>
constexpr std::size_t TupleSize_v = TupleSize<Tup>::value;

template <>
class Tuple<> {
public:
    Tuple() = default;
};

template <typename Head, typename... Tail>
class Tuple<Head, Tail...> {
private:
    template<typename...> friend class Tuple;
    Head head_;
    [[no_unique_address]] Tuple<Tail...> tail_;

    template <size_t N, typename... Types>
    friend decltype(auto) get(Tuple<Types...>&);
    template <size_t N, typename... Types>
    friend decltype(auto) get(Tuple<Types...>&&);
    template <size_t N, typename... Types>
    friend decltype(auto) get(const Tuple<Types...>&);
    template <size_t N, typename... Types>
    friend decltype(auto) get(const Tuple<Types...>&&);
    template <class T, class... Types>
    friend constexpr T& get(Tuple<Types...>&);
    template <class T, class... Types>
    friend constexpr T&& get(Tuple<Types...>&&);
    template <class T, class... Types>
    friend constexpr const T& get(const Tuple<Types...>&);
    template <class T, class... Types>
    friend constexpr const T&& get(const Tuple<Types...>&&);

public:
    explicit(!AllImplicitlyConstructible<Head, Tail...>)
    Tuple() requires AllDefaultConstructible<Head, Tail...>
      : head_(), tail_() {}

    explicit(!ALLConvertible<Head, Tail...>)
    Tuple(const Head& head, const Tail&... tail)
    requires AllCopyConstructible<Head, Tail...>
      : head_(head), tail_(tail...) {}

    Tuple(const Tuple&) = default;
    Tuple(Tuple&&) = default;

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<UHead&&, Head> && (std::is_convertible_v<UTail&&, Tail> && ...)))
    Tuple(UHead&& head, UTail&&... tail)
    requires (sizeof...(UTail) == sizeof...(Tail)
              && (std::is_constructible_v<Head, UHead&&> && (std::is_constructible_v<Tail, UTail&&> && ...)))
      : head_(std::forward<UHead>(head)), tail_(std::forward<UTail>(tail)...) {}

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<const UHead&, Head> && (std::is_convertible_v<const UTail&, Tail> && ...)))
    Tuple(const Tuple<UHead, UTail...>& rhs)
    requires (sizeof...(UTail) == sizeof...(Tail)
              && (std::is_constructible_v<Head, const UHead&> && (std::is_constructible_v<Tail, const UTail&> && ...)))
      : head_(rhs.head_), tail_(rhs.tail_) {}

    template <typename UHead, typename... UTail>
    explicit(!(std::is_convertible_v<UHead&&, Head> && (std::is_convertible_v<UTail&&, Tail> && ...)))
    Tuple(Tuple<UHead, UTail...>&& rhs)
    requires (sizeof...(UTail) == sizeof...(Tail)
              && (std::is_constructible_v<Head, UHead&&> && (std::is_constructible_v<Tail, UTail&&> && ...)))
      : head_(std::forward<UHead>(rhs.head_)), tail_(std::move(rhs.tail_)) {}

    template <typename T1, typename T2>
    Tuple(const std::pair<T1, T2>& p) : head_(p.first), tail_(p.second) {}

    template <typename T1, typename T2>
    Tuple(std::pair<T1, T2>&& p) : head_(std::forward<T1>(p.first)), tail_(std::forward<T2>(p.second)) {}

    Tuple& operator=(const Tuple& rhs)
    requires (std::is_assignable_v<Head&, const Head&> && (std::is_assignable_v<Tail&, const Tail&> && ...)) {
        head_ = rhs.head_;
        tail_ = rhs.tail_;
        return *this;
    }

    Tuple& operator=(Tuple&& rhs)
    requires (std::is_assignable_v<Head&, Head> && (std::is_assignable_v<Tail&, Tail> && ...)) {
        head_ = std::move(rhs.head_);
        tail_ = std::move(rhs.tail_);
        return *this;
    }
    template<typename UHead, typename... UTail>
    Tuple& operator=(const Tuple<UHead, UTail...>& rhs)
    requires (sizeof...(UTail) == sizeof...(Tail)
              && (std::is_assignable_v<Head&, const UHead&> && (std::is_assignable_v<Tail&, const UTail&> && ...))) {
        head_ = rhs.head_;
        tail_ = rhs.tail_;
        return *this;
    }

    template<typename UHead, typename... UTail>
    Tuple& operator=(Tuple<UHead, UTail...>&& rhs)
    requires (sizeof...(UTail) == sizeof...(Tail)
              && (std::is_assignable_v<Head&, UHead&&> && (std::is_assignable_v<Tail&, UTail&&> && ...))) {
        head_ = std::forward<UHead>(rhs.head_);
        tail_ = std::move(rhs.tail_);
        return *this;
    }
};

template <typename... Types>
bool operator==(const Tuple<Types...>& l, const Tuple<Types...>& r) {
    if constexpr (sizeof...(Types) == 0) {
      return true;
    }
    if (get<0>(l) != get<0>(r)) {
      return false;
    }
    return operator==(l.tail_, r.tail_);
}

template <typename... Types>
bool operator!=(const Tuple<Types...>& l, const Tuple<Types...>& r) { return !(l == r); }

template <typename... Types>
bool operator<(const Tuple<Types...>& l, const Tuple<Types...>& r) {
    constexpr size_t N = TupleSize_v<Tuple<Types...>>;
    for (size_t i = 0; i < N; ++i) {
        if (get<i>(l) < get<i>(r)) {
          return true;
        }
        if (get<i>(l) > get<i>(r)) {
          return false;
        }
    }
    return false;
}

template <typename... Types>
bool operator>(const Tuple<Types...>& a, const Tuple<Types...>& b) { return b < a; }

template <typename... Types>
bool operator<=(const Tuple<Types...>& a, const Tuple<Types...>& b) { return !(b < a); }

template <typename... Types>
bool operator>=(const Tuple<Types...>& a, const Tuple<Types...>& b) { return !(a < b); }

template<typename... Ts>
struct ConcatTuples;

template<>
struct ConcatTuples<> { using type = Tuple<>; };

template<typename... Us>
struct ConcatTuples<Tuple<Us...>> { using type = Tuple<std::decay_t<Us>...>; };

template<typename... Us, typename... Vs, typename... Rest>
struct ConcatTuples<Tuple<Us...>, Tuple<Vs...>, Rest...> {
  using type = typename ConcatTuples<
      Tuple<std::decay_t<Us>..., std::decay_t<Vs>...>, std::decay_t<Rest>...>::type;
};

template<typename... Ts>
using ConcatResult_t = typename ConcatTuples<std::decay_t<Ts>...>::type;

struct TupleCat_Impl {
  template <typename FIndexSeq, typename TupleIndexSeq>
  struct ConcatToFTuple;

  template <size_t... FIndexSeq, size_t... TupleIndexSeq>
  struct ConcatToFTuple<std::index_sequence<FIndexSeq...>, std::index_sequence<TupleIndexSeq...>> {
    template <typename FTuple, typename Tuple>
    static constexpr auto Concat(FTuple&& ft, Tuple&& t) {
      return forwardAsTuple(get<FIndexSeq>(std::forward<FTuple>(ft))..., get<TupleIndexSeq>(std::forward<Tuple>(t))...);
    }
  };

  template <typename IndexSeq>
  struct Create_Tuple_From_Ftuple;

  template <size_t ... indexes>
  struct Create_Tuple_From_Ftuple<std::index_sequence<indexes...>> {
    template <typename FTuple>
    static constexpr auto f(FTuple&& ft) {
      using Result = ConcatResult_t<FTuple>;
      return Result{get<indexes>(std::forward<FTuple>(ft))...};
    }
  };

  template <typename FTuple, typename Tuple, typename... Tuples>
  static constexpr auto f(FTuple&& ft, Tuple&& t, Tuples&&... ts) {
      return f(ConcatToFTuple<std::make_index_sequence<TupleSize_v<std::decay_t<FTuple>>>,
                            std::make_index_sequence<TupleSize_v<std::decay_t<Tuple>>>>::Concat(
                            std::forward<FTuple>(ft),
                            std::forward<Tuple>(t)),
                            std::forward<Tuples>(ts)...);
  }

  template <typename FTuple>
  static constexpr auto f(FTuple&& rt) {
    return Create_Tuple_From_Ftuple<std::make_index_sequence<TupleSize_v<FTuple>>>::f(std::forward<FTuple>(rt));
  }
};

template <typename... Tuples>
constexpr auto tupleCat(Tuples&&... tuples) { return TupleCat_Impl::f(std::forward<Tuples>(tuples)...); }

template <typename T1, typename T2>
Tuple(const std::pair<T1, T2>&) -> Tuple<T1, T2>;

template <typename T1, typename T2>
Tuple(std::pair<T1, T2>&&) -> Tuple<T1, T2>;
