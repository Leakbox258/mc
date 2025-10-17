#ifndef UTILS_ADT_SMALLVECTOR
#define UTILS_ADT_SMALLVECTOR

#include "utils/likehood.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace utils {
namespace ADT {
template <typename T, unsigned N> class SmallVector;

template <typename T, unsigned N> struct SmallVectorStorage {
  alignas(T) char InlineElts[N * sizeof(T)];
};

template <typename T> struct alignas(T) SmallVectorStorage<T, 0> {};

template <typename T> class CalculatePredferInlinedElems {
  static constexpr std::size_t kPreferSVSizeof = 64;
  static_assert(
      sizeof(T) <= 256,
      "You are trying to use a default number of inlined elements for "
      "`SmallVector<T>` but `sizeof(T)` is really big! Please use an "
      "explicit number of inlined elements with `SmallVector<T, N>` to make "
      "sure you really want that much inline storage.");
  static constexpr std::size_t PreferInlineBytes =
      kPreferSVSizeof - sizeof(utils::ADT::SmallVector<T, 0>);
  static constexpr std::size_t PreferNumElems = PreferInlineBytes / sizeof(T);
  static constexpr std::size_t value =
      PreferNumElems ? PreferNumElems : 1; // at least one elem
};

template <class Iterator>
concept IfConvertibleToInputIterator = std::is_convertible_v<
    typename std::iterator_traits<Iterator>::iterator_category,
    std::input_iterator_tag>;

class SmallVectorBase {
protected:
  void* BeginWith;
  std::size_t Size = 0, Capacity;

public:
  SmallVectorBase() = delete;
  SmallVectorBase(void* _BeginWith, std::size_t _Capacity)
      : BeginWith(_BeginWith), Capacity(_Capacity) {}

  std::size_t size() const { return Size; }
  std::size_t capacity() const { return Capacity; }
  bool empty() const { return Size == 0; }

protected:
  static std::size_t getNewCapacity(std::size_t MinSize,
                                    std::size_t OldCapacity) {
    constexpr std::size_t MaxSize = std::numeric_limits<std::size_t>::max();

    assert(MinSize < MaxSize && OldCapacity < MaxSize);

    std::size_t NewCapacity = 2 * OldCapacity + 1; // Always grow.
    return std::clamp(NewCapacity, MinSize, MaxSize);
  }
  static void* replaceAllocation(void* NewElems, std::size_t TSize,
                                 std::size_t NewCapacity,
                                 std::size_t VSize = 0);

  /// helper when use `grow()`
  void* malloc4grow(void* FirstEl, std::size_t MinSize, std::size_t TSize,
                    std::size_t& NewCapacity);

  /// only use when T is POD type
  void grow_pod(void* FirstEl, std::size_t MinSize, std::size_t TSize);

  /// modify size
  template <typename Numeric> void set_size(Numeric N) {
    assert(N <= this->Capacity && N >= 0);
    this->Size = static_cast<std::size_t>(N);
  }

  /// only to reset memory ranges on stack
  /// never construct of destruct elements
  template <typename Numeric>
  void set_allocation_range(void* Begin, Numeric N) {
    assert(N <= std::numeric_limits<std::size_t>::max());
    this->BeginWith = Begin;
    this->Capacity = static_cast<std::size_t>(N);
  }
};

/// view of the layout of the instantce vec
template <class T, typename = void> struct SmallVectorAlignmentAndSize {
  alignas(SmallVectorBase) char Base[sizeof(SmallVectorBase)];
  alignas(T) char FirstElement[sizeof(T)];
};

template <typename T, typename = void>
class SmallVectorTemplateCommon : public SmallVectorBase {
protected:
  /// skip the offset of the metadata of vec
  /// so this method always get the ptr to the first elem on stack,
  /// no matter it exists or doesnt
  constexpr void* getFirstEl() const {
    return const_cast<void*>(reinterpret_cast<const void*>(
        reinterpret_cast<const char*>(this) +
        offsetof(SmallVectorAlignmentAndSize<T>, FirstElement)));
  }

  SmallVectorTemplateCommon(std::size_t _Size)
      : SmallVectorBase(this->getFirstEl(), _Size) {}

  void grow_pod(std::size_t MinSize, std::size_t TSize) {
    SmallVectorBase::grow_pod(this->getFirstEl(), MinSize, TSize);
  }

  /// check if BeginWith eq getFirtstEl()
  /// if obj(BeginWith) is migrate to heap, the result is false
  bool isSmall() const { return this->getFirstEl() == this->BeginWith; }

  /// reset as small
  void resetToSmall() {
    this->BeginWith = this->getFirstEl();
    this->Size = this->Capacity = 0;
  }

  /// check a memref in range or not
  bool isRefOfRange(const void* V, const void* Fir, const void* Last) const {
    std::less<const void*> LessThan;
    return !LessThan(Fir, V) && LessThan(Fir, Last); // Last serve as sentinel
  }
  /// check a memref as vec or not
  bool isRefOfStorage(const void* V) const {
    return this->isRefOfRange(V, this->begin(), this->end());
  }

  bool isRangeInStorage(const void* S, const void* E) const {
    std::less<const void*> LessThan;
    return !LessThan(S, this->begin()) && !LessThan(E, S) &&
           !LessThan(this->end(), E);
  }

  /// if V is going to be move when resize
  bool isSafeToRefAfterResize(const void* V, std::size_t NewSize) {
    if (utils::is_likely(!this->isRefOfStorage(V))) {
      return true;
    }

    /// shrink
    if (NewSize <= this->size()) {
      return V < this->begin() + NewSize;
    }

    /// if migrate
    return NewSize <= this->capacity();
  }
  /// check whether V will be invalidated by resizing the vector to NewSize.
  void assertSafeToRefAfterResize(const void* V, std::size_t NewSize) {
    assert(this->isSafeToRefAfterResize(V, NewSize) &&
           "Attempting to reference an element of the vector in an operation "
           "that invalidates it");
  }
  /// check if V is safe after add new element
  void assertSafeToRefAfterAdd(const void* V, std::size_t N = 1) {
    this->assertSafeToRefAfterResize(V, this->size() + N);
  }
  /// check if From ~ To will be invalidated when clearing
  template <class ItTy> void assertSafeToRefAfterClear(ItTy From, ItTy To) {
    if constexpr (std::is_pointer_v<ItTy> &&
                  std::is_same_v<
                      std::remove_const_t<T>,
                      std::remove_const_t<std::remove_pointer_t<ItTy>>>) {
      if (From == To) {
        return; // no elem
      }

      this->assertSafeToRefAfterResize(From, 0);
      // To serve as sentinel
      this->assertSafeToRefAfterResize(To - 1, 0);
    }
  }
  /// check ...
  template <class ItTy> void assertSafeToAddRange(ItTy From, ItTy To) {
    if constexpr (std::is_pointer_v<ItTy> &&
                  std::is_same_v<
                      std::remove_const_t<T>,
                      std::remove_const_t<std::remove_pointer_t<ItTy>>>) {
      if (From == To) {
        return; // no elem
      }
      this->assertSafeToRefAfterResize(From, To - From);
      this->assertSafeToRefAfterResize(To - 1, To - From);
    }
    (void)From;
    (void)To;
  }

  template <class U>
  static const T* reserveForParamAndGetAddressImpl(U* This, const T& Elt,
                                                   std::size_t N) {
    std::size_t NewSize = This->size() + N;
    if (utils::is_likely(NewSize <= This->capacity())) {
      return &Elt; // no growth
    }

    bool ReferencesStorage = false; // if Elt if a ref on This
    int64_t Index = -1;
    if (!U::TakesParamByValue) {
      if (utils::is_unlikely(This->isRefOfStorage(&Elt))) {
        // if true, re-generate an idx for Elt
        ReferencesStorage = true;
        Index = &Elt - This->begin();
      }
    }
    This->grow(NewSize);

    // check is Elt in the vec
    return ReferencesStorage ? This->begin() + Index : &Elt;
  }

public:
  using size_ty = std::size_t;
  using difference_ty = std::ptrdiff_t;
  using value_ty = T;

  using iter = T*;
  using const_iter = const T*;
  using const_rev_iter = std::reverse_iterator<const_iter>;
  using rev_iter = std::reverse_iterator<iter>;

  using ref = T&;
  using const_ref = const T&;
  using ptr = T*;
  using const_ptr = const T*;

  using Base = SmallVectorBase;
  using Base::capacity;
  using Base::empty;
  using Base::size;

  /// iterator
  iter begin() { return (iter)this->BeginWith; }
  const_iter begin() const { return (const_iter)this->BeginWith; }
  iter end() { return (iter)(this->BeginWith) + size(); }
  const_iter end() const { return (const_iter)(this->BeginWith) + size(); }

  /// reverse iterator
  rev_iter rbegin() { return rev_iter(end()); }
  const_rev_iter rbegin() const { return const_rev_iter(end()); }
  rev_iter rend() { return rev_iter(begin()); }
  const_rev_iter rend() const { return const_rev_iter(begin()); }

  /// sizes
  size_ty size_in_bytes() const { return size() * sizeof(value_ty); }
  size_ty capacity_in_bytes() const { return capacity() * sizeof(value_ty); }
  size_ty max_size() const { return size_ty(-1) / sizeof(value_ty); }

  /// raw ptr
  ptr data() { return ptr(begin()); }
  const_ptr data() const { return const_ptr(begin()); }

  /// ref
  ref operator[](size_ty idx) {
    assert(idx < size());
    return begin()[idx];
  }
  const_ref operator[](size_ty idx) const {
    assert(idx < size());
    return begin()[idx];
  }

  ref front() {
    assert(!empty());
    return begin()[0];
  }
  const_ref front() const {
    assert(!empty());
    return begin()[0];
  }

  ref back() {
    assert(!empty());
    return end()[-1];
  }
  const_ref back() const {
    assert(!empty());
    return end()[-1];
  }
};

template <typename T>
concept IsPOD = std::is_trivially_copy_constructible_v<T> &&
                std::is_trivially_move_constructible_v<T> &&
                std::is_trivially_destructible_v<T>;

template <typename T> class SmallVEctorTemplateBase;

/// template base to deal with no-trivial type
template <typename T>
class SmallVectorTemplateBase : public SmallVectorTemplateCommon<T> {
  friend SmallVectorTemplateCommon<T>;

protected:
  static constexpr bool TakesParamByValue = false; // non-trivial
  using ValueParamT = const T&;

  SmallVectorTemplateBase(std::size_t _Size)
      : SmallVectorTemplateCommon<T>(_Size) {}

  /// non-trivial need explicity destructions
  static void destroy_range(T* S, T* E) {
    while (S != E) {
      --E;
      E->~T();
    }
  }

  template <typename It1, typename It2>
  static void uninitialized_move(It1 I, It1 E, It2 Dest) {
    std::uninitialized_move(I, E, Dest);
  }

  template <typename It1, typename It2>
  static void uninitialized_copy(It1 I, It2 E, It2 Dest) {
    std::uninitialized_copy(I, E, Dest);
  }

  void grow(std::size_t MinSize = 0) {
    std::size_t NewCapacity;
    T* NewElems = malloc4grow(MinSize, NewCapacity);
    this->moveElements4Grow(NewElems);
    this->takeAllocation4Grow(NewElems, NewCapacity);
  }

  T* malloc4grow(std::size_t MinSize, std::size_t& NewCapacity) {
    return static_cast<T*>(SmallVectorBase::malloc4grow(
        this->getFirstEl(), MinSize, sizeof(T), NewCapacity));
  }

  void moveElements4Grow(T* NewElems) {
    this->uninitialized_move(this->begin(), this->end(), NewElems);

    this->destroy_range(this->begin(), this->end());
  }

  /// transfer the ownership of the allocation
  void takeAllocation4Grow(T* NewElems, std::size_t NewCapacity) {
    if (!this->isSmall()) {
      free(this->begin());
    }

    this->set_allocation_range(NewElems, NewCapacity);
  }

  const T* reserveForParamAndGetAddress(const T& Elem, std::size_t N = 1) {
    return this->reserveForParamAndGetAddressImpl(this, Elem, N);
  }

  T* reserveForParamAndGetAddress(T& Elem, std::size_t N = 1) {
    return const_cast<T*>(
        this->reserveForParamAndGetAddressImpl(this, Elem, N));
  }

  static T&& forward_value_param(T&& V) { return std::move(V); }
  static const T& forward_value_param(const T& V) { return V; }

  void growAndAssign(std::size_t Num, const T& Elem) {
    std::size_t NewCapacity;
    T* NewElems = this->malloc4grow(Num, NewCapacity);
    std::uninitialized_fill_n(NewElems, Num, Elem);

    // clean-up old range
    this->destroy_range(this->begin(), this->end());
    takeAllocation4Grow(NewElems, NewCapacity);
    this->set_size(Num);
  }

  template <typename... ArgTys> T& growAndEmplaceBack(ArgTys&&... Args) {
    std::size_t NewCapacity;
    T* NewElems = this->malloc4grow(0, NewCapacity);

    ::new ((void*)(NewElems + this->size())) T(std::forward<ArgTys>(Args)...);

    this->moveElements4Grow(NewElems);
    this->takeAllocation4Grow(NewElems, NewCapacity);
    this->set_size(this->size() + 1);
    return this->back();
  }

public:
  void push_back(const T& Elem) {
    const T* ElemPtr = this->reserveForParamAndGetAddress(Elem);
    ::new ((void*)this->end()) T(*ElemPtr);
    this->set_size(this->size() + 1);
  }
  void push_back(T&& Elem) {
    const T* ElemPtr = this->reserveForParamAndGetAddress(Elem);
    ::new ((void*)this->end()) T(std::move(*ElemPtr));
    this->set_size(this->size() + 1);
  }

  void pop_back() {
    this->set_size(this->size() - 1);
    this->end()->~T();
  }
};

/// template base to deal with trivial type
template <typename T>
  requires IsPOD<T>
class SmallVectorTemplateBase<T> : public SmallVectorTemplateCommon<T> {
  friend SmallVectorTemplateCommon<T>;

protected:
  /// whether it is cheap enough to take by values
  static constexpr bool TakesParamByValue = sizeof(T) <= 2 * sizeof(void*);

  using ValueParamT = std::conditional_t<TakesParamByValue, T, const T&>;

  SmallVectorTemplateBase(std::size_t _Size)
      : SmallVectorTemplateCommon<T>(_Size) {}

  static void destroy_range(T*, T*) {}

  template <typename It1, typename It2>
  static void uninitialized_move(It1 I, It1 E, It2 Dest) {
    std::uninitialized_copy(I, E, Dest);
  }

  template <typename It1, typename It2>
  static void uninitialized_copy(It1 I, It1 E, It2 Dest) {
    std::uninitialized_copy(I, E, Dest);
  }

  template <typename Tx, typename Ty>
  static void uninitialized_copy(
      Tx* I, Tx* E, Ty* Dest,
      std::enable_if_t<std::is_same<std::remove_const_t<Tx>, Ty>::value>* =
          nullptr) {

    if (I != E) {
      std::memcpy(reinterpret_cast<void*>(Dest), I, (E - I) * sizeof(T));
    }
  }

  void grow(std::size_t MinSize = 0) { this->grow_pod(MinSize, sizeof(T)); }

  const T* reserveForParamAndGetAddress(const T& Elem, std::size_t N = 1) {
    return this->reserveForParamAndGetAddressImpl(this, Elem, N);
  }

  T* reserveForParamAndGetAddress(T& Elem, std::size_t N = 1) {
    return const_cast<T*>(
        this->reserveForParamAndGetAddressImpl(this, Elem, N));
  }

  const T* reserveParamAndGetAddress(std::size_t N = 1) {
    return this->reserveParamAndGetAddressImpl(this, N);
  }

  static ValueParamT forward_value_param(ValueParamT V) { return V; }

  void growAndAssign(std::size_t Num, T Elem) {
    this->set_size(0);
    this->grow(Num);
    std::uninitialized_fill_n(this->begin(), Num, Elem);
    this->set_size(Num);
  }

  template <typename... ArgTys> T& growAndEmplaceBack(ArgTys&&... Args) {
    this->push_back(T(std::forward<ArgTys>(Args)...));
    return this->back();
  }

public:
  void push_back(ValueParamT Elem) {
    const T* ElemPtr = reserveForParamAndGetAddress(Elem);
    std::memcpy(reinterpret_cast<void*>(const_cast<T*>(this->end())), ElemPtr,
                sizeof(T));
    this->set_size(this->size() + 1);
  }

  void pop_back() { this->set_size(this->size() - 1); }
};

template <typename T>
struct SmallVectorImpl : public SmallVectorTemplateBase<T> {
  using TemplateBase = SmallVectorTemplateBase<T>;

public:
  using iter = typename TemplateBase::iter;
  using const_iter = typename TemplateBase::const_iter;
  using rev_iter = typename TemplateBase::rev_iter;
  using const_rev_iter = typename TemplateBase::const_rev_iter;
  using ref = typename TemplateBase::ref;
  using size_ty = typename TemplateBase::size_ty;

protected:
  using TemplateBase::TakesParamByValue;
  using ValueParamT = typename TemplateBase::ValueParamT;

  explicit SmallVectorImpl(unsigned N) : SmallVectorTemplateBase<T>(N) {}

  void assignRemote(SmallVectorImpl&& RHS) {
    // which destroy_range depands on T
    this->destroy_range(this->begin(), this->end());
    if (!this->isSmall()) {
      free(this->begin());
    }
    this->BeginWith = RHS.BeginWith;
    this->Size = RHS.Size;
    this->Capacity = RHS.Capacity;
  }

  ~SmallVectorImpl() {
    if (!this->isSmall()) {
      free(this->begin());
    }
  }

public:
  SmallVectorImpl(const SmallVectorImpl&) = delete;

  void clear() {
    // remain capacity
    this->destroy_range(this->begin(), this->end());
    this->Size = 0;
  }

private:
  using SmallVectorTemplateBase<T>::set_size;

  template <bool ForOverwrite> void resizeImpl(size_ty N) {
    if (N == this->size()) {
      return;
    }

    if (N < this->size()) {
      this->truncate(N);
      return;
    }

    this->reserve(N);
    for (auto I = this->end(), E = this->begin() + N; I != E; ++I) {
      if (ForOverwrite) {
        new (&*I) T;
      } else {
        new (&*I) T();
      }
    }
    this->set_size(N);
  }

public:
  void resize(size_ty N) { this->resizeImpl<false>(N); }
  void resize_for_overwrite(size_ty N) { this->resizeImpl<true>(N); }

  void truncate(size_ty N) {
    assert(N <= this->size());
    this->destroy_range(this->begin() + N, this->end());
    this->set_size(N);
  }

  void resize(size_ty N, ValueParamT NV) {
    if (N == this->size())
      return;

    if (N < this->size()) {
      this->truncate(N);
      return;
    }

    this->append(N - this->size(), NV);
  }

  void reserve(size_ty N) {
    if (this->capacity() < N) {
      this->grow(N);
    }
  }

  void pop_back_n(size_ty Num) {
    assert(Num <= this->size());
    this->truncate(this->size() - Num);
  }

  [[nodiscard]] T pop_back_val() {
    T Res = ::std::move(this->back());
    this->pop_back(); // empty obj
    return Res;
  }

  template <IfConvertibleToInputIterator ItTy>
  void append(ItTy in_start, ItTy in_end) {
    this->assertSafeToAddRange(in_start, in_end);
    size_ty Num = std::distance(in_start, in_end);
    this->reserve(Num + this->size());
    this->uninitialized_copy(in_start, in_end, this->end());
    this->set_size(this->size() + Num);
  }

  void append(size_ty Num, ValueParamT Elem) {
    const T* ElemPtr = this->reserveForParamAndGetAddress(Elem, Num);
    std::uninitialized_fill_n(this->end(), Num, *ElemPtr);
    this->set_size(this->size() + Num);
  }

  void append(std::initializer_list<T> List) {
    this->append(List.begin(), List.end());
  }

  void append(const SmallVectorImpl& RHS) { append(RHS.begin(), RHS.end()); }

  /// explictly override
  void assign(size_ty Num, ValueParamT Elem) {
    if (Num > this->capacity()) {
      return this->growAndAssign(Num, Elem);
    }

    std::fill_n(this->begin(), std::min(Num, this->size()), Elem);

    if (Num > this->size()) {
      std::uninitialized_fill_n(this->end(), Num - this->size(), Elem);
    } else if (Num < this->size()) {
      this->destroy_range(this->begin() + Num, this->end());
    }

    this->set_size(Num);
  }

  template <IfConvertibleToInputIterator ItTy>
  void assign(ItTy in_start, ItTy in_end) {
    this->assertSafeToRefAfterClear(in_start, in_end);
    this->clear();
    this->append(in_start, in_end);
  }

  void assign(std::initializer_list<T> List) {
    this->clear();
    this->append(List);
  }

  void assign(const SmallVectorImpl& RHS) { assign(RHS.begin(), RHS.end()); }

  iter erase(const_iter Citer) {
    iter I = const_cast<iter>(Citer);
    assert(this->isRefOfStorage(I));

    iter N = I;
    std::move(I + 1, this->end(), I);

    this->pop_back();
    return N;
  }

  iter erase(const_iter CS, const_iter CE) {
    iter S = const_cast<iter>(CS);
    iter E = const_cast<iter>(CE);

    assert(this->isRangeInStorage(S, E));

    iter N = S;
    iter I = std::move(E, this->end(), S);

    this->destroy_range(I, this->end());
    this->set_size(I - this->begin());
    return N;
  }

  void swap(SmallVectorImpl& RHS);

private:
  template <class ArgTy> iter insert_one_impl(iter I, ArgTy&& Elem) {
    static_assert(
        std::is_same<std::remove_const_t<std::remove_reference_t<ArgTy>>,
                     T>::value,
        "ArgType must be derived from T!");

    if (I == this->end()) {
      this->push_back(::std::forward<ArgTy>(Elem));
      return this->end() - 1;
    }

    assert(this->isRefOfStorage(I));

    std::size_t Idx = I - this->begin();
    std::remove_reference<ArgTy>* ElemPtr =
        this->reserveForParamAndGetAddress(Elem);
    I = this->begin() + Idx; // recalculate

    ::new ((void*)this->end()) T(::std::move(this->back()));

    std::move_backward(I, this->end() - 1, this->end());

    this->set_size(this->size() + 1);

    // if T is not small POD, then ArgTy must passed by value
    static_assert(!TakesParamByValue || std::is_same<ArgTy, T>::value,
                  "ArgType must be 'T' when taking by value!");

    if (!TakesParamByValue && this->isRefOfRange(ElemPtr, I, this->end())) {
      ++ElemPtr;
    }

    *I = ::std::forward<ArgTy>(*ElemPtr);
    return I;
  }

public:
  iter insert(iter I, T&& Elem) {
    return insert_one_impl(I, this->forward_value_param(std::move(Elem)));
  }
  iter insert(iter I, const T& Elem) {
    return insert_one_impl(I, this->forward_value_param(Elem));
  }
  iter insert(iter I, size_ty Num, ValueParamT Elem) {

    std::size_t Idx = I - this->begin();
    if (I == this->end()) {
      this->append(Num, Elem);
      return this->begin() + Idx;
    }

    const T* ElemPtr = this->reserveForParamAndGetAddress(Elem, Num);

    if (std::size_t(this->end() - I) >= Num) {
      iter OldEnd = this->end();
      this->append(std::move_iterator<iter>(this->end() - Num),
                   std::move_iterator<iter>(this->end()));

      std::move_backward(I, OldEnd - Num, OldEnd);

      if (!TakesParamByValue && I <= ElemPtr && ElemPtr < this->end()) {
        ElemPtr += Num;
      }

      std::fill_n(I, Num, *ElemPtr);
      return I;
    }

    T* OldEnd = this->end();
    this->set_size(this->size() + Num);
    std::size_t NumOverWrite = OldEnd - I;
    this->uninitialized_move(I, OldEnd, this->end() - NumOverWrite);

    if (!TakesParamByValue && I <= ElemPtr && ElemPtr < this->end()) {
      ElemPtr += Num;
    }

    // operator=
    std::fill_n(I, NumOverWrite, *ElemPtr);

    // T::T()
    std::uninitialized_fill_n(OldEnd, Num - NumOverWrite, *ElemPtr);
    return I;
  }

  template <IfConvertibleToInputIterator ItTy>
  iter insert(iter I, ItTy From, ItTy To) {
    size_t Idx = I - this->begin();

    if (I == this->end()) {
      append(From, To);
      return this->begin() + Idx;
    }

    assert(this->isReferenceToStorage(I) &&
           "Insertion iterator is out of bounds.");

    this->assertSafeToAddRange(From, To);

    size_t Num = std::distance(From, To);

    reserve(this->size() + Num);

    I = this->begin() + Idx;

    if (std::size_t(this->end() - I) >= Num) {
      T* OldEnd = this->end();
      append(std::move_iterator<iter>(this->end() - Num),
             std::move_iterator<iter>(this->end()));

      std::move_backward(I, OldEnd - Num, OldEnd);

      std::copy(From, To, I);
      return I;
    }

    T* OldEnd = this->end();
    this->set_size(this->size() + Num);
    size_t NumOverwritten = OldEnd - I;
    this->uninitialized_move(I, OldEnd, this->end() - NumOverwritten);

    // operator=
    for (T* J = I; NumOverwritten > 0; --NumOverwritten) {
      *J = *From;
      ++J;
      ++From;
    }

    // T::T()
    this->uninitialized_copy(From, To, OldEnd);
    return I;
  };

  void insert(iter I, std::initializer_list<T> IL) {
    insert(I, IL.begin(), IL.end());
  }

  template <typename... ArgTys> ref emplace_back(ArgTys&&... Args) {
    if (utils::is_unlikely(this->size() >= this->capacity())) {
      return this->growAndEmplaceBack(std::forward<ArgTys>(Args)...);
    }

    ::new ((void*)this->end()) T(std::forward<ArgTys>(Args)...);
    this->set_size(this->size() + 1);
    return this->back();
  }

  SmallVectorImpl& operator=(const SmallVectorImpl& RHS);

  SmallVectorImpl& operator=(SmallVectorImpl&& RHS);

  bool operator==(const SmallVectorImpl& RHS) const {
    if (this->size() != RHS.size())
      return false;
    return std::equal(this->begin(), this->end(), RHS.begin());
  }

  bool operator!=(const SmallVectorImpl& RHS) const { return !(*this == RHS); }

  bool operator<(const SmallVectorImpl& RHS) const {
    return std::lexicographical_compare(this->begin(), this->end(), RHS.begin(),
                                        RHS.end());
  }
  bool operator>(const SmallVectorImpl& RHS) const { return RHS < *this; }
  bool operator<=(const SmallVectorImpl& RHS) const { return !(*this > RHS); }
  bool operator>=(const SmallVectorImpl& RHS) const { return !(*this < RHS); }
};

/// no inline
template <typename T> void SmallVectorImpl<T>::swap(SmallVectorImpl<T>& RHS) {
  if (this == &RHS)
    return;

  // We can only avoid copying elements if neither vector is small.
  if (!this->isSmall() && !RHS.isSmall()) {
    std::swap(this->BeginWith, RHS.BeginWith);
    std::swap(this->Size, RHS.Size);
    std::swap(this->Capacity, RHS.Capacity);
    return;
  }
  this->reserve(RHS.size());
  RHS.reserve(this->size());

  // Swap the shared elements.
  size_t NumShared = this->size();
  if (NumShared > RHS.size())
    NumShared = RHS.size();
  for (size_ty i = 0; i != NumShared; ++i)
    std::swap((*this)[i], RHS[i]);

  // Copy over the extra elts.
  if (this->size() > RHS.size()) {
    size_t ElemDiff = this->size() - RHS.size();
    this->uninitialized_copy(this->begin() + NumShared, this->end(), RHS.end());
    RHS.set_size(RHS.size() + ElemDiff);
    this->destroy_range(this->begin() + NumShared, this->end());
    this->set_size(NumShared);
  } else if (RHS.size() > this->size()) {
    size_t ElemDiff = RHS.size() - this->size();
    this->uninitialized_copy(RHS.begin() + NumShared, RHS.end(), this->end());
    this->set_size(this->size() + ElemDiff);
    this->destroy_range(RHS.begin() + NumShared, RHS.end());
    RHS.set_size(NumShared);
  }
}

template <typename T>
SmallVectorImpl<T>&
SmallVectorImpl<T>::operator=(const SmallVectorImpl<T>& RHS) {
  if (&RHS == this) {
    return *this;
  }

  std::size_t RHSSize = RHS.size();
  std::size_t CurSize = this->size();
  if (CurSize >= RHSSize) {
    iter NewEnd;
    if (RHSSize)
      NewEnd = std::copy(RHS.begin(), RHS.begin() + RHSSize, this->begin());
    else
      NewEnd = this->begin();

    // Destroy excess elements.
    this->destroy_range(NewEnd, this->end());

    // Trim.
    this->set_size(RHSSize);
    return *this;
  }

  if (this->capacity() < RHSSize) {
    // Destroy current elements.
    this->clear();
    CurSize = 0;
    this->grow(RHSSize);
  } else if (CurSize) {
    // Otherwise, use assignment for the already-constructed elements.
    std::copy(RHS.begin(), RHS.begin() + CurSize, this->begin());
  }

  // Copy construct the new elements in place.
  this->uninitialized_copy(RHS.begin() + CurSize, RHS.end(),
                           this->begin() + CurSize);

  // Set end.
  this->set_size(RHSSize);
  return *this;
}

template <typename T>
SmallVectorImpl<T>& SmallVectorImpl<T>::operator=(SmallVectorImpl<T>&& RHS) {
  if (this == &RHS) {
    return *this;
  }

  if (!RHS.isSmall()) {
    this->assignRemote(std::move(RHS));
    return *this;
  }

  std::size_t RHSSize = RHS.size();
  std::size_t CurSize = this->size();
  if (CurSize >= RHSSize) {
    iter NewEnd = this->begin();
    if (RHSSize)
      NewEnd = std::move(RHS.begin(), RHS.end(), NewEnd);

    this->destroy_range(NewEnd, this->end());
    this->set_size(RHSSize);

    RHS.clear();

    return *this;
  }

  if (this->capacity() < RHSSize) {
    this->clear();
    CurSize = 0;
    this->grow(RHSSize);
  } else if (CurSize) {
    std::move(RHS.begin(), RHS.begin() + CurSize, this->begin());
  }

  this->uninitialized_move(RHS.begin() + CurSize, RHS.end(),
                           this->begin() + CurSize);

  this->set_size(RHSSize);

  RHS.clear();
  return *this;
}

template <typename T> class ArrayRef;
template <typename IterT> class iterator_range;

template <typename T, unsigned N = CalculatePredferInlinedElems<T>::value>
class SmallVector : public SmallVectorImpl<T>, public SmallVectorStorage<T, N> {
public:
  SmallVector() : SmallVectorImpl<T>(N) {}
  ~SmallVector() { this->destroy_range(this->begin(), this->end()); }

  explicit SmallVector(std::size_t Size) : SmallVectorImpl<T>(N) {
    this->resize(Size);
  }

  SmallVector(std::size_t Size, const T& Value) : SmallVectorImpl<T>(N) {
    this->assign(Size, Value);
  }

  template <IfConvertibleToInputIterator ItTy>
  SmallVector(ItTy S, ItTy E) : SmallVectorImpl<T>(N) {
    this->append(S, E);
  }

  template <typename RangeTy>
  explicit SmallVector(const iterator_range<RangeTy>& R)
      : SmallVectorImpl<T>(N) {
    this->append(R.begin(), R.end());
  }

  SmallVector(std::initializer_list<T> IL) : SmallVectorImpl<T>(N) {
    this->append(IL);
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  explicit SmallVector(ArrayRef<U> A) : SmallVectorImpl<T>(N) {
    this->append(A.begin(), A.end());
  }

  SmallVector(const SmallVector& RHS) : SmallVectorImpl<T>(N) {
    if (!RHS.empty())
      SmallVectorImpl<T>::operator=(RHS);
  }

  SmallVector& operator=(const SmallVector& RHS) {
    SmallVectorImpl<T>::operator=(RHS);
    return *this;
  }

  SmallVector(SmallVector&& RHS) : SmallVectorImpl<T>(N) {
    if (!RHS.empty())
      SmallVectorImpl<T>::operator=(::std::move(RHS));
  }

  SmallVector(SmallVectorImpl<T>&& RHS) : SmallVectorImpl<T>(N) {
    if (!RHS.empty())
      SmallVectorImpl<T>::operator=(::std::move(RHS));
  }

  SmallVector& operator=(SmallVector&& RHS) {
    if (N) {
      SmallVectorImpl<T>::operator=(::std::move(RHS));
      return *this;
    }
    // SmallVectorImpl<T>::operator= does not leverage N==0. Optimize the
    // case.
    if (this == &RHS)
      return *this;
    if (RHS.empty()) {
      this->destroy_range(this->begin(), this->end());
      this->Size = 0;
    } else {
      this->assignRemote(std::move(RHS));
    }
    return *this;
  }

  SmallVector& operator=(SmallVectorImpl<T>&& RHS) {
    SmallVectorImpl<T>::operator=(::std::move(RHS));
    return *this;
  }

  SmallVector& operator=(std::initializer_list<T> IL) {
    this->assign(IL);
    return *this;
  }
};

/// SmallVector Builder

} // namespace ADT
} // namespace utils

#endif