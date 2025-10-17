#ifndef UTILS_ADT_STRINGSET
#define UTILS_ADT_STRINGSET

#include "StringRef.hpp"
#include "utils/likehood.hpp"
#include "utils/macro.hpp"
#include "utils/memory.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <map>

namespace {

class StringSetEntry {
public:
  using size_ty = std::size_t;

private:
  size_ty KeyLength;
  char* KeyPtr;

public:
  StringSetEntry(utils::ADT::StringRef Str) {
    KeyLength = Str.size();
    utils_assert(Str.data(), "Expect valid StringRef data");
    KeyPtr = (char*)utils::malloc(KeyLength);
    std::memcpy(KeyPtr, Str.data(), KeyLength);
  }

  StringSetEntry(const StringSetEntry& E) {
    KeyLength = E.KeyLength;
    KeyPtr = (char*)utils::malloc(KeyLength);
    std::memcpy(KeyPtr, E.KeyPtr, KeyLength);
  }

  StringSetEntry(StringSetEntry&& E) noexcept {
    KeyLength = E.KeyLength;
    KeyPtr = E.KeyPtr;
    E.KeyPtr = nullptr;
  }

  StringSetEntry& operator=(const StringSetEntry& E) {
    if (utils::is_likely(this != &E)) {
      if (KeyPtr)
        free(KeyPtr);
      KeyLength = E.KeyLength;
      KeyPtr = (char*)utils::malloc(KeyLength);
      std::memcpy(KeyPtr, E.KeyPtr, KeyLength);
    }
    return *this;
  }

  StringSetEntry& operator=(StringSetEntry&& E) noexcept {
    if (utils::is_likely(this != &E)) {
      if (KeyPtr) {
        free(KeyPtr);
      }
      KeyLength = E.KeyLength;
      KeyPtr = E.KeyPtr;
      E.KeyPtr = nullptr;
    }
    return *this;
  }

  [[nodiscard]] utils::ADT::StringRef getEntry() const {
    return utils::ADT::StringRef(KeyPtr, KeyLength);
  }

  [[nodiscard]] size_ty getSize() const { return KeyLength; }

  ~StringSetEntry() {
    if (KeyPtr) {
      free(KeyPtr);
    }
  }
};

template <std::size_t SmallLength = 8, std::size_t SmallBucket = 4>
class StringSetImpl {
public:
  using Entry = StringSetEntry;
  using size_ty = Entry::size_ty;
  using Table = utils::ADT::SmallVector<Entry, SmallBucket>;

protected:
  std::array<Table, SmallLength> SmallTables;
  std::map<size_ty, Table> LargeTables;

  StringSetImpl() = default;

  StringSetImpl(const StringSetImpl& O) {
    for (size_ty i = 0; i < SmallLength; i++) {
      SmallTables[i] = O.SmallTables[i];
    }
    LargeTables = O.LargeTables;
  }

  StringSetImpl(StringSetImpl&& O) noexcept {
    SmallTables = std::move(O.SmallTables);
    LargeTables = std::move(O.LargeTables);
  }

  StringSetImpl& operator=(const StringSetImpl& O) {
    if (utils::is_likely(this != &O)) {
      SmallTables = O.SmallTables;
      LargeTables = O.LargeTables;
    }
    return *this;
  }

  StringSetImpl& operator=(StringSetImpl&& O) noexcept {
    if (utils::is_likely(this != &O)) {
      SmallTables = std::move(O.SmallTables);
      LargeTables = std::move(O.LargeTables);
    }
    return *this;
  }

  ~StringSetImpl() = default;
};

} // namespace

namespace utils {
namespace ADT {

template <std::size_t SL = 8, std::size_t SB = 4>
class StringSet : public StringSetImpl<SL, SB> {
public:
  using Base = StringSetImpl<SL, SB>;
  using Entry = typename Base::Entry;
  using size_ty = typename Base::size_ty;
  using Base::LargeTables;
  using Base::SmallTables;

private:
  size_ty Size = 0;

public:
  StringSet() = default;
  StringSet(const StringSet& S) : Base(S), Size(S.Size) {}
  StringSet(StringSet&& S) noexcept : Base(std::move(S)), Size(S.Size) {}

  bool insert(StringRef Key) {
    size_ty L = Key.size();
    if (is_likely(L < SL)) {
      auto& Table = SmallTables[L];
      for (const auto& E : Table)
        if (E.getEntry() == Key)
          return false;
      Table.emplace_back(Key);
      ++Size;
      return true;
    } else {
      auto& Table = LargeTables[L];
      for (const auto& E : Table)
        if (E.getEntry() == Key)
          return false;
      Table.emplace_back(Key);
      ++Size;
      return true;
    }
  }

  bool contains(StringRef Key) const {
    size_ty L = Key.size();
    if (is_likely(L < SL)) {
      for (const auto& E : SmallTables[L])
        if (E.getEntry() == Key)
          return true;
      return false;
    } else {
      auto It = LargeTables.find(L);
      if (It == LargeTables.end())
        return false;
      for (const auto& E : It->second)
        if (E.getEntry() == Key)
          return true;
      return false;
    }
  }

  size_ty size() const { return Size; }
};

} // namespace ADT
} // namespace utils

#endif