// This file is part of SymCC.
//
// SymCC is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// SymCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// SymCC. If not, see <https://www.gnu.org/licenses/>.

#include <Runtime.h>

#include <array>
#include <cassert>
#include <numeric>

#include "RuntimeCommon.h"
#include "Shadow.h"
#include "GarbageCollection.h"

namespace {

constexpr int kMaxFunctionArguments = 256;

/// Global storage for function parameters and the return value.
SymExpr g_return_value;
std::array<SymExpr, kMaxFunctionArguments> g_function_arguments;
// TODO make thread-local

} // namespace

void _sym_set_return_expression(SymExpr expr) { g_return_value = expr; }

SymExpr _sym_get_return_expression(void) {
  auto *result = g_return_value;
  // TODO this is a safeguard that can eventually be removed
  g_return_value = nullptr;
  return result;
}

void _sym_set_parameter_expression(uint8_t index, SymExpr expr) {
  g_function_arguments[index] = expr;
}

SymExpr _sym_get_parameter_expression(uint8_t index) {
  return g_function_arguments[index];
}

void _sym_memcpy(uint8_t *dest, const uint8_t *src, size_t length) {
  if (isConcrete(src, length) && isConcrete(dest, length))
    return;

  ReadOnlyShadow srcShadow(src, length);
  ReadWriteShadow destShadow(dest, length);
  std::copy(srcShadow.begin(), srcShadow.end(), destShadow.begin());
}

void _sym_memset(uint8_t *memory, SymExpr value, size_t length) {
  if ((value == nullptr) && isConcrete(memory, length))
    return;

  ReadWriteShadow shadow(memory, length);
  std::fill(shadow.begin(), shadow.end(), value);
}

void _sym_memmove(uint8_t *dest, const uint8_t *src, size_t length) {
  if (isConcrete(src, length) && isConcrete(dest, length))
    return;

  ReadOnlyShadow srcShadow(src, length);
  ReadWriteShadow destShadow(dest, length);
  if (dest > src)
    std::copy_backward(srcShadow.begin(), srcShadow.end(), destShadow.end());
  else
    std::copy(srcShadow.begin(), srcShadow.end(), destShadow.begin());
}

SymExpr _sym_read_memory(uint8_t *addr, size_t length, bool little_endian) {
  assert(length && "Invalid query for zero-length memory region");

#ifdef DEBUG_RUNTIME
  std::cerr << "Reading " << length << " bytes from address " << P(addr)
            << std::endl;
  dump_known_regions();
#endif

  // If the entire memory region is concrete, don't create a symbolic expression
  // at all.
  if (isConcrete(addr, length))
    return nullptr;

  ReadOnlyShadow shadow(addr, length);
  return std::accumulate(shadow.begin_non_null(), shadow.end_non_null(),
                         static_cast<SymExpr>(nullptr),
                         [&](SymExpr result, SymExpr byteExpr) {
                           if (result == nullptr)
                             return byteExpr;

                           return little_endian
                                      ? _sym_concat_helper(byteExpr, result)
                                      : _sym_concat_helper(result, byteExpr);
                         });
}

void _sym_write_memory(uint8_t *addr, size_t length, SymExpr expr,
                       bool little_endian) {
  assert(length && "Invalid query for zero-length memory region");

#ifdef DEBUG_RUNTIME
  std::cerr << "Writing " << length << " bytes to address " << P(addr)
            << std::endl;
  dump_known_regions();
#endif

  if (expr == nullptr && isConcrete(addr, length))
    return;

  ReadWriteShadow shadow(addr, length);
  if (expr == nullptr) {
    std::fill(shadow.begin(), shadow.end(), nullptr);
  } else {
    size_t i = 0;
    for (SymExpr &byteShadow : shadow) {
      byteShadow = little_endian
                       ? _sym_extract_helper(expr, 8 * (i + 1) - 1, 8 * i)
                       : _sym_extract_helper(expr, (length - i) * 8 - 1,
                                             (length - i - 1) * 8);
      i++;
    }
  }
}

SymExpr _sym_build_extract(SymExpr expr, uint64_t offset, uint64_t length,
                           bool little_endian) {
  size_t totalBits = _sym_bits_helper(expr);
  assert((totalBits % 8 == 0) && "Aggregate type contains partial bytes");

  SymExpr result;
  if (little_endian) {
    result = _sym_extract_helper(expr, totalBits - offset * 8 - 1,
                                 totalBits - offset * 8 - 8);
    for (size_t i = 1; i < length; i++) {
      result = _sym_concat_helper(
          _sym_extract_helper(expr, totalBits - (offset + i) * 8 - 1,
                              totalBits - (offset + i + 1) * 8),
          result);
    }
  } else {
    result = _sym_extract_helper(expr, totalBits - offset * 8 - 1,
                                 totalBits - (offset + length) * 8);
  }

  return result;
}

SymExpr _sym_build_bswap(SymExpr expr) {
  size_t bits = _sym_bits_helper(expr);
  assert((bits % 16 == 0) && "bswap is not applicable");
  return _sym_build_extract(expr, 0, bits / 8, true);
}

void _sym_register_expression_region(SymExpr *start, size_t length) {
  registerExpressionRegion({start, length});
}

void __s2anitizer_cov_trace_pc_guard_init(uint32_t *start,
                                                    uint32_t *stop) {
    printf("Hulla hop from init\n");
  static uint64_t N;  // Counter for the guards.
  if (start == stop || *start) return;  // Initialize only once.
  printf("INIT: %p %p\n", start, stop);
  for (uint32_t *x = start; x < stop; x++)
    *x = ++N;  // Guards should start from 1.
}
//  std::cerr << "in __sanitizer_cov_trace_pc_guard_init 123\n";
//  printf("HAAAAAAAAAAAAAAAAAAllo\n");
/*
  static uint64_t N;  // Counter for the guards.
  if (start == stop || *start) return;  // Initialize only once.
  printf("INIT: %p %p\n", start, stop);
  for (uint32_t *x = start; x < stop; x++)
    *x = ++N;  // Guards should start from 1.
*/
//}

// This callback is inserted by the compiler on every edge in the
// control flow (some optimizations apply).
// Typically, the compiler will emit the code like this:
//    if(*guard)
//      __sanitizer_cov_trace_pc_guard(guard);
// But for large functions it will emit a simple call:
//    __sanitizer_cov_trace_pc_guard(guard);
void __s2anitizer_cov_trace_pc_guard(uint32_t *guard) {
  std::cerr << "in __sanitizer_cov_trace_pc_guard\n";
  if (guard != nullptr) {
    std::cerr << "Guard: " << *guard << "\n";
  }
}

char *perm_start = NULL;
char *perm_end = NULL;

char *get_perm_start(){
	return perm_start;
}
char *get_perm_end(){
	return perm_end;
}


void __s2anitizer_cov_8bit_counters_init(char *start, char *end) {
    if (perm_start == NULL) perm_start = start;
    if (perm_end == NULL) perm_end = end;

    std::cerr << "In void __s2anitizer_cov_8bit_counters_init\n"; 
    char *tmp = perm_start;
    while (tmp < perm_end) {
        char c = *tmp;
        printf("counter val: %d\n", (int)c);
        tmp++;
    }
}

void iterate_8bit_counters() {
    std::cerr << "In iterate counters\n"; 
    char *tmp = perm_start;
    while (tmp < perm_end) {
        char c = *tmp;
        printf("counter val: %d\n", (int)c);
        tmp++;
    }
}
/*
  if (!*guard) return;  // Duplicate the guard check.
  // If you set *guard to 0 this code will not be called again for this edge.
  // Now you can get the PC and do whatever you want:
  //   store it somewhere or symbolize it and print right away.
  // The values of `*guard` are as you set them in
  // __sanitizer_cov_trace_pc_guard_init and so you can make them consecutive
  // and use them to dereference an array or a bit vector.
  void *PC = __builtin_return_address(0);
  char PcDescr[1024];
  // This function is a part of the sanitizer run-time.
  // To use it, link with AddressSanitizer or other sanitizer.
  __sanitizer_symbolize_pc(PC, "%p %F %L", PcDescr, sizeof(PcDescr));
  printf("guard: %p %x PC %s\n", guard, *guard, PcDescr);
*/
//}
