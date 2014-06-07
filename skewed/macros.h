#ifndef LIB_SKEWED_MACROS
#define LIB_SKEWED_MACROS

#include <assert.h>

#define CHECK(expression) assert(expression)
#define CHECK_NOTNULL(pointer) assert(pointer != nullptr)

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete; \
  void operator=(const TypeName&) = delete

#endif  // LIB_SKEWED_MACROS
