#ifndef UTILS_ADT_STRINGMAP
#define UTILS_ADT_STRINGMAP

#include "StringRef.hpp"
#include "utils/likehood.hpp"
#include "utils/macro.hpp"
#include "utils/memory.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

namespace {
/// TODO: when sizeof(ValueTy) is small but amount is huge, considering use
/// consist memory instead of malloc
/// maybe try bump pointer
template <typename ValueTy> class StringMapEntry {
public:
  using size_ty = std::size_t;

private:
  size_ty KeyLength;
  char* KeyPtr;
  ValueTy* ValuePtr;

public:
  StringMapEntry(const StringMapEntry& Entry) {
    this->KeyLength = Entry.KeyLength;
    this->KeyPtr = (char*)utils::malloc(this->KeyLength);

    utils_assert(Entry.KeyPtr, "Expect Data to be valid");

    std::memcpy(this->KeyPtr, Entry.KeyPtr, this->KeyLength);

    ValuePtr = (ValueTy*)utils::malloc(sizeof(ValueTy));

    ::new ((void*)ValuePtr) ValueTy(*(Entry.ValuePtr));
  }

  StringMapEntry(StringMapEntry&& Entry) {
    this->KeyLength = Entry.KeyLength;
    this->KeyPtr = Entry.KeyPtr;
    Entry.KeyPtr = nullptr;
    this->ValuePtr = Entry.ValuePtr;
    Entry.ValuePtr = nullptr;
  }

  StringMapEntry(utils::ADT::StringRef Str, const ValueTy& Val) {
    this->KeyLength = Str.size();
    this->KeyPtr = (char*)utils::malloc(this->KeyLength);

    utils_assert(Str.data(), "Expect Data to be valid");

    std::memcpy(this->KeyPtr, Str.data(), this->KeyLength);

    ValuePtr = (ValueTy*)utils::malloc(sizeof(ValueTy));

    ::new ((void*)ValuePtr) ValueTy(Val);
  }

  StringMapEntry(utils::ADT::StringRef Str, ValueTy&& Val) {
    this->KeyLength = Str.size();
    this->KeyPtr = (char*)utils::malloc(this->KeyLength);

    utils_assert(Str.data(), "Expect Data to be valid");

    std::memcpy(this->KeyPtr, Str.data(), this->KeyLength);

    ValuePtr = (ValueTy*)utils::malloc(sizeof(ValueTy));

    ::new ((void*)ValuePtr) ValueTy(std::forward<ValueTy>(Val));
  }

  [[nodiscard]] size_ty getSize() const { return KeyLength; }

  [[nodiscard]] utils::ADT::StringRef getEntry() const {
    return utils::ADT::StringRef(KeyPtr, KeyLength);
  }

  [[nodiscard]] const ValueTy* getValue() const { return ValuePtr; }

  [[nodiscard]] ValueTy* getValue() { return ValuePtr; }

  ~StringMapEntry() {
    free(KeyPtr);
    ValuePtr->~ValueTy();
    free(ValuePtr);
  }
};

template <typename ValueTy, std::size_t SmallLength = 8,
          std::size_t SmallBucket = 4>
class StringMapImpl {
public:
  using size_ty = StringMapEntry<ValueTy>::size_ty;
  using StringTyTable =
      utils::ADT::SmallVector<StringMapEntry<ValueTy>, SmallBucket>;

protected:
  /// keys quick access
  std::vector<std::string> Keys;

  /// it is easy to notice that the key String is often short
  /// the StringTableEntry of length between 1 ~ 8 will stored here
  std::array<StringTyTable, SmallLength> SmallSizeTyTable;

  /// otherwise, using map as fallback
  std::map<size_ty, StringTyTable> LargeSizeTyTable;

  StringMapImpl() = default;
  StringMapImpl(const StringMapImpl& map) {
    for (size_ty i = 0; i < SmallLength; i++) {
      this->SmallSizeTyTable[i].clear();
      for (const auto& Entry : map.SmallSizeTyTable[i]) {
        this->SmallSizeTyTable[i].push_back(Entry);
      }
    }

    this->LargeSizeTyTable.clear();
    for (const auto& [Key, Val] : map.LargeSizeTyTable) {
      for (const auto& Entry : Val) {
        this->LargeSizeTyTable[Key].push_back(Entry);
      }
    }

    this->Keys.clear();
    for (const auto& Key : map.Keys) {
      this->Keys.emplace_back(Key);
    }
  }
  StringMapImpl(StringMapImpl&& map) {
    SmallSizeTyTable = std::move(map.SmallSizeTyTable);
    LargeSizeTyTable = std::move(map.LargeSizeTyTable);
    Keys = std::move(map.Keys);
  }

  StringMapImpl& operator=(StringMapImpl&& map) {
    if (utils::is_likely(this != &map)) {
      SmallSizeTyTable = std::move(map.SmallSizeTyTable);
      LargeSizeTyTable = std::move(map.LargeSizeTyTable);
      Keys = std::move(map.Keys);
    }
    return *this;
  }

  StringMapImpl& operator=(const StringMapImpl& map) {
    if (utils::is_likely(this != &map)) {
      for (size_ty i = 0; i < SmallLength; i++) {
        SmallSizeTyTable[i].clear();
        for (const auto& Entry : map.SmallSizeTyTable[i]) {
          this->SmallSizeTyTable[i].push_back(Entry);
        }
      }

      LargeSizeTyTable.clear();
      for (const auto& [Key, Val] : map.LargeSizeTyTable) {
        for (const auto& Entry : Val) {
          this->LargeSizeTyTable[Key].push_back(Entry);
        }
      }

      this->Keys.clear();
      for (const auto& Key : map.Keys) {
        this->Keys.emplace_back(Key);
      }
    }
    return *this;
  }

  ~StringMapImpl() {
    for (unsigned i = 0; i < SmallLength; ++i) {
      SmallSizeTyTable[i].clear();
    }
    for (auto& [Key, Table] : LargeSizeTyTable) {
      Table.clear();
    }
  }
};
} // namespace

namespace utils {

namespace ADT {
template <typename V, std::size_t SL = 8, std::size_t SB = 4>
class StringMap : StringMapImpl<V, SL, SB> {
public:
  using size_ty = StringMapEntry<V>::size_ty;
  using StringTyTable = utils::ADT::SmallVector<StringMapEntry<V>, SB>;
  using Impl = StringMapImpl<V, SL, SB>;

private:
  size_ty Size = 0;

public:
  StringMap() = default;
  StringMap(const StringMap& map) : Impl(map), Size(map.Size) {}
  StringMap(StringMap&& map) : Impl(std::move(map)), Size(map.Size) {}

  StringMap(std::initializer_list<std::pair<StringRef, V>>&& list)
      : StringMap() {

    for (auto [Str, Val] : list) {
      utils_assert(this->insert(Str, Val), "StringRef Key is conflicted");
    }
  }

  StringMap& operator=(StringMap&& map) {
    if (utils::is_likely(this != &map)) {
      Impl::operator=(std::move(map));
      Size = map.Size;
    }
    return *this;
  }

  StringMap& operator=(const StringMap& map) {
    if (utils::is_likely(this != &map)) {
      Impl::operator=(map);
      Size = map.Size;
    }
    return *this;
  }

  ~StringMap() = default;

  bool insert(StringRef Key, V&& Val) {
    size_ty KeyLength = Key.size();
    if (is_likely(KeyLength < SL)) {
      for (const auto& Entry : this->SmallSizeTyTable[KeyLength]) {
        if (is_unlikely(utils::ADT::operator==(Entry.getEntry(), Key))) {
          return false;
        }
      }
      this->SmallSizeTyTable[KeyLength].emplace_back(Key, std::forward<V>(Val));
      this->Keys.emplace_back(Key.str());
      Size++;
      return true;
    } else {
      auto& Table = this->LargeSizeTyTable[KeyLength];
      for (const auto& Entry : Table) {
        if (is_unlikely(utils::ADT::operator==(Entry.getEntry(), Key))) {
          return false;
        }
      }
      Table.emplace_back(Key, std::forward<V>(Val));
      this->Keys.emplace_back(Key.str());
      Size++;
      return true;
    }
  }

  bool insert(StringRef Key, const V& Val) {
    size_ty KeyLength = Key.size();
    if (is_likely(KeyLength < SL)) {
      for (const auto& Entry : this->SmallSizeTyTable[KeyLength]) {
        if (is_unlikely(utils::ADT::operator==(Entry.getEntry(), Key))) {
          return false;
        }
      }
      this->SmallSizeTyTable[KeyLength].emplace_back(
          std::forward<StringRef>(Key), Val);
      this->Keys.emplace_back(Key.str());
      Size++;
      return true;
    } else {
      auto& Table = this->LargeSizeTyTable[KeyLength];
      for (const auto& Entry : Table) {
        if (is_unlikely(utils::ADT::operator==(Entry.getEntry(), Key))) {
          return false;
        }
      }
      Table.emplace_back(Key.str(), Val);
      this->Keys.emplace_back(Key.str());
      Size++;
      return true;
    }
  }

  /// you can trait this as some kind of iter
  V* find(StringRef Key) {
    size_ty KeyLength = Key.size();
    if (is_likely(KeyLength < SL)) {
      for (auto& Entry : this->SmallSizeTyTable[KeyLength]) {
        if (utils::ADT::operator==(Entry.getEntry(), Key)) {
          return Entry.getValue();
        }
      }
      return nullptr;
    } else {
      auto It = this->LargeSizeTyTable.find(KeyLength);
      if (It == this->LargeSizeTyTable.end()) {
        return nullptr;
      }
      for (auto& Entry : It->second) {
        if (utils::ADT::operator==(Entry.getEntry(), Key)) {
          return Entry.getValue();
        }
      }
      return nullptr;
    }
  }

  const V* find(StringRef Key) const {
    size_ty KeyLength = Key.size();
    if (is_likely(KeyLength < SL)) {
      for (auto& Entry : this->SmallSizeTyTable[KeyLength]) {
        if (utils::ADT::operator==(Entry.getEntry(), Key)) {
          return Entry.getValue();
        }
      }
      return nullptr;
    } else {
      auto It = this->LargeSizeTyTable.find(KeyLength);
      if (It == this->LargeSizeTyTable.end()) {
        return nullptr;
      }
      for (auto& Entry : It->second) {
        if (utils::ADT::operator==(Entry.getEntry(), Key)) {
          return Entry.getValue();
        }
      }
      return nullptr;
    }
  }

  const std::vector<std::string>& keys() const { return this->Keys; }

  std::optional<size_ty> position(StringRef Key) const {
    auto it = std::find_if(this->Keys.begin(), this->Keys.end(),
                           [&](const std::string& Str) { return Key == Str; });

    if (it == this->Keys.end()) {
      return std::nullopt;
    } else {
      return std::distance(this->Keys.begin(), it);
    }
  }
};

} // namespace ADT
} // namespace utils

#endif