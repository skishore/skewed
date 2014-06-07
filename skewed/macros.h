#ifndef LIB_SKEWED_MACROS
#define LIB_SKEWED_MACROS

#define CHECK(expression) assert(expression)
#define CHECK_NOTNULL(pointer) assert(pointer != nullptr)
#define LOG(...) printf(__VA_ARGS__); printf("\n")

// Comment out this #define to run in release mode.
#define LIB_SKEWED_DEBUG

#ifdef LIB_SKEWED_DEBUG
// Debug mode: DCHECKs are run and DLOGs are printed.
#include <assert.h>
#define DCHECK(...) CHECK(__VA_ARGS__)
#define DCHECK_NOTNULL(...) CHECK_NOTNULL(__VA_ARGS__)
#define DLOG(...) LOG(__VA_ARGS__)
#else
// Release mode - CHECKs and DLOGs are compiled out.
#define DCHECK(...)
#define DCHECK_NOTNULL(...)
#define DLOG(...)
#endif

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete; \
  void operator=(const TypeName&) = delete

#endif  // LIB_SKEWED_MACROS
