#include "utils/ADT/SmallVector.hpp"
#include "utils/memory.hpp"

using namespace utils::ADT;
void* SmallVectorBase::replaceAllocation(void* NewElems, std::size_t TSize,
                                         std::size_t NewCapacity,
                                         std::size_t VSize) {
  void* NewElemsReplace = utils::malloc(NewCapacity * TSize);
  if (VSize) {
    std::memcpy(NewElemsReplace, NewElems, VSize * TSize);
  }
  free(NewElems);
  return NewElemsReplace;
}

void* SmallVectorBase::malloc4grow(void* FirstElem, std::size_t MinSize,
                                   std::size_t TSize,
                                   std::size_t& NewCapacity) {
  NewCapacity = getNewCapacity(MinSize, this->capacity());
  // Even if capacity is not 0 now, if the vector was originally created with
  // capacity 0, it's possible for the malloc to return FirstEl.
  void* NewElems = std::malloc(NewCapacity * TSize);

  if (utils::is_unlikely(NewElems == FirstElem)) {
    // get an another piece of mem, avoid memcpy to itself
    NewElems = replaceAllocation(NewElems, TSize, NewCapacity);
  }
  return NewElems;
}

void SmallVectorBase::grow_pod(void* FirstElem, std::size_t MinSize,
                               std::size_t TSize) {
  size_t NewCapacity = getNewCapacity(MinSize, this->capacity());
  void* NewElems;
  if (this->BeginWith == FirstElem) {
    NewElems = utils::malloc(NewCapacity * TSize);
    if (NewElems == FirstElem) {
      NewElems = replaceAllocation(NewElems, TSize, NewCapacity);
    }

    // Copy the elements over.  No need to run dtors on PODs.
    std::memcpy(NewElems, this->BeginWith, size() * TSize);
  } else {
    // If this wasn't grown from the inline copy, grow the allocated space.
    NewElems = utils::realloc(this->BeginWith, NewCapacity * TSize);
    if (NewElems == FirstElem) {
      NewElems = replaceAllocation(NewElems, TSize, NewCapacity, size());
    }
  }

  this->set_allocation_range(NewElems, NewCapacity);
}

using namespace utils::ADT;
