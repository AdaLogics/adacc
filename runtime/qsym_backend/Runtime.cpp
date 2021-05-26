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

//
// Definitions that we need for the Qsym backend
//

#include "Runtime.h"
#include "GarbageCollection.h"

/*
// C++
#if __has_include(<filesystem>)
#define HAVE_FILESYSTEM 1
#elif __has_include(<experimental/filesystem>)
#define HAVE_FILESYSTEM 0
#else
#error "We need either <filesystem> or the older <experimental/filesystem>."
#endif
*/

#include <atomic>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <unordered_set>

/*
#if HAVE_FILESYSTEM
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
*/

#ifdef DEBUG_RUNTIME
#include <chrono>
#endif

// C
#include <cstdio>

// Qsym
#include <afl_trace_map.h>
#include <call_stack_manager.h>
#include <expr_builder.h>
#include <solver.h>

// LLVM
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/ArrayRef.h>

// Runtime
#include <Config.h>
#include <LibcWrappers.h>
#include <Shadow.h>

namespace qsym {
   
ExprBuilder *g_expr_builder;
Solver *g_solver;
CallStackManager g_call_stack_manager;
z3::context *g_z3_context;

} // namespace qsym

namespace {

/// Indicate whether the runtime has been initialized.
std::atomic_flag g_initialized = ATOMIC_FLAG_INIT;

/// The file that contains out input.
std::string inputFileName;

void deleteInputFile() { 
    std::cout << " in delete input file\n";
    std::remove(inputFileName.c_str()); 
}

//std::vector<char> my_input_copy;
std::vector<unsigned int> counters;
std::vector<unsigned int> new_counters;

std::vector<uint32_t> takens_0;
std::vector<uint32_t> takens_1;

uint32_t curr_trace_map = 0;
std::vector<uint32_t> trace_maps;

bool previosuly_blocked_a_save = false;

uint32_t just_visited_bb = 0;

std::map<uint32_t, uint32_t> old_counter_map;
std::map<uint32_t, uint32_t> counter_map;

//bool is_pure_concolic = false;


/// A mapping of all expressions that we have ever received from Qsym to the
/// corresponding shared pointers on the heap.
///
/// We can't expect C clients to handle std::shared_ptr, so we maintain a single
/// copy per expression in order to keep the expression alive. The garbage
/// collector decides when to release our shared pointer.
///
/// std::map seems to perform slightly better than std::unordered_map on our
/// workload.
std::map<SymExpr, qsym::ExprRef> allocatedExpressions;

SymExpr registerExpression(const qsym::ExprRef &expr) {
  SymExpr rawExpr = expr.get();

  if (allocatedExpressions.count(rawExpr) == 0) {
    // We don't know this expression yet. Create a copy of the shared pointer to
    // keep the expression alive.
    allocatedExpressions[rawExpr] = expr;
  }

  return rawExpr;
}

} // namespace

using namespace qsym;

/*
#if HAVE_FILESYSTEM
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif
*/

static int dtor_done = 0;

void __dtor_runtime(void) {
    std::cerr << "finished exection, going into dtor\n";
    if (g_config.is_pure_concolic) {
        //std::cerr << "dtoring\n";
        // A quick hack because we can have multiple calls to dtor
        // due to our lazy implementation.
        // This whole dtor should probably be substituted for atexit
        if (dtor_done == 1) {
            return;
        }
        dtor_done = 1;

      // Now let's check if there is a difference in corpus
      bool should_save = false;
        std::cerr << "Inside of dtor, going through all counters\n";
        
        // Write the updated counters to our corpus file.
        //
     
      std::cerr << "Preparing to write corpus counter\n";

      bool has_had_change = false;
          for (auto& new_c: counter_map) {
            uint32_t k,v;
            k = new_c.first;
            v = new_c.second;
            if (old_counter_map.count(k) == 0) {
              old_counter_map[k] = v;
              has_had_change = true;
              //break;
            }
            if (old_counter_map.at(k) < v) {
              old_counter_map[k] = v;
              has_had_change = true;
              //break;
            }
          }


      //  std::cerr << "Counters we save:\n";
      //for (auto& it: old_counter_map) {
      //  std::cerr << "(" << it.first << ", " << it.second << ")\n";
      //}
      //  std::cerr << "-----------\n";

      //if (has_had_change) {
            ofstream myfile;
            myfile.open ("corpus_counters.stats", ios::trunc);
          for (auto& it: old_counter_map) {
              myfile << it.first << "\n";
              myfile << it.second << "\n";
        //    std::cerr << "(" << it.first << ", " << it.second << ")\n";
          }

            myfile.close();
     // }


            ofstream myfile0;
            myfile0.open ("prev_0s.txt", ios::trunc);
          for (auto& it: takens_0) {
              myfile0 << it << "\n";
        //    std::cerr << "(" << it.first << ", " << it.second << ")\n";
          }
            myfile0.close();

            ofstream myfile1;
            myfile1.open ("prev_1s.txt", ios::trunc);
          for (auto& it: takens_1) {
              myfile1 << it << "\n";
        //    std::cerr << "(" << it.first << ", " << it.second << ")\n";
          }
            myfile1.close();


            ofstream myfile2;
            myfile2.open ("trace_maps.txt", ios::trunc);
          for (auto& it: trace_maps) {
              myfile2 << it << "\n";
        //    std::cerr << "(" << it.first << ", " << it.second << ")\n";
          }
            myfile2.close();

        

        std::cerr << "Done going through the counters\n";
  }
    exit(0);
}


void _sym_initialize(void) {
  if (g_initialized.test_and_set())
    return;

  loadConfig();
  initLibcWrappers();
  std::cerr << "This is SymCC running with the QSYM backend" << std::endl;
  if (g_config.fullyConcrete) {
    std::cerr
        << "Performing fully concrete execution (i.e., without symbolic input)"
        << std::endl;
    return;
  }

  // Check the output directory
/*
  if (!fs::exists(g_config.outputDir) ||
      !fs::is_directory(g_config.outputDir)) {
    std::cerr << "Error: the output directory " << g_config.outputDir
              << " (configurable via SYMCC_OUTPUT_DIR) does not exist."
              << std::endl;
    exit(-1);
  }
*/
  // Qsym requires the full input in a file
  if (g_config.inputFile.empty()) {
    std::cerr << "Reading program input until EOF (use Ctrl+D in a terminal)..."
              << std::endl;
    std::istreambuf_iterator<char> in_begin(std::cin), in_end;
    std::vector<char> inputData(in_begin, in_end);
    inputFileName = std::tmpnam(nullptr);
    std::ofstream inputFile(inputFileName, std::ios::trunc);
    std::copy(inputData.begin(), inputData.end(),
              std::ostreambuf_iterator<char>(inputFile));
    inputFile.close();

#ifdef DEBUG_RUNTIME
    std::cerr << "Loaded input:" << std::endl;
    std::copy(inputData.begin(), inputData.end(),
              std::ostreambuf_iterator<char>(std::cerr));
    std::cerr << std::endl;
#endif

    atexit(deleteInputFile);

    // Restore some semblance of standard input
    auto *newStdin = freopen(inputFileName.c_str(), "r", stdin);
    if (newStdin == nullptr) {
      perror("Failed to reopen stdin");
      exit(-1);
    }
  } else {
    inputFileName = g_config.inputFile;
    std::cerr << "Making data read from " << inputFileName << " as symbolic"
              << std::endl;
  }

  g_z3_context = new z3::context{};
  g_solver =
      new Solver(inputFileName, g_config.outputDir, g_config.aflCoverageMap);
  g_expr_builder = g_config.pruning ? PruneExprBuilder::create()
                                    : SymbolicExprBuilder::create();
}


SymExpr _sym_build_integer(uint64_t value, uint8_t bits) {
  // Qsym's API takes uintptr_t, so we need to be careful when compiling for
  // 32-bit systems: the compiler would helpfully truncate our uint64_t to fit
  // into 32 bits.
  if constexpr (sizeof(uint64_t) == sizeof(uintptr_t)) {
    // 64-bit case: all good.
    return registerExpression(g_expr_builder->createConstant(value, bits));
  } else {
    // 32-bit case: use the regular API if possible, otherwise create an
    // llvm::APInt.
    if (uintptr_t value32 = value; value32 == value)
      return registerExpression(g_expr_builder->createConstant(value32, bits));

    return registerExpression(
        g_expr_builder->createConstant({64, value}, bits));
  }
}

SymExpr _sym_build_integer128(uint64_t high, uint64_t low) {
  std::array<uint64_t, 2> words = {low, high};
  return registerExpression(g_expr_builder->createConstant({128, words}, 128));
}

SymExpr _sym_build_null_pointer() {
  return registerExpression(
      g_expr_builder->createConstant(0, sizeof(uintptr_t) * 8));
}

SymExpr _sym_build_true() {
  return registerExpression(g_expr_builder->createTrue());
}

SymExpr _sym_build_false() {
  return registerExpression(g_expr_builder->createFalse());
}

SymExpr _sym_build_bool(bool value) {
  return registerExpression(g_expr_builder->createBool(value));
}

#define DEF_BINARY_EXPR_BUILDER(name, qsymName)                                \
  SymExpr _sym_build_##name(SymExpr a, SymExpr b) {                            \
    return registerExpression(g_expr_builder->create##qsymName(                \
        allocatedExpressions.at(a), allocatedExpressions.at(b)));              \
  }

DEF_BINARY_EXPR_BUILDER(add, Add)
DEF_BINARY_EXPR_BUILDER(sub, Sub)
DEF_BINARY_EXPR_BUILDER(mul, Mul)
DEF_BINARY_EXPR_BUILDER(unsigned_div, UDiv)
DEF_BINARY_EXPR_BUILDER(signed_div, SDiv)
DEF_BINARY_EXPR_BUILDER(unsigned_rem, URem)
DEF_BINARY_EXPR_BUILDER(signed_rem, SRem)

DEF_BINARY_EXPR_BUILDER(shift_left, Shl)
DEF_BINARY_EXPR_BUILDER(logical_shift_right, LShr)
DEF_BINARY_EXPR_BUILDER(arithmetic_shift_right, AShr)

DEF_BINARY_EXPR_BUILDER(signed_less_than, Slt)
DEF_BINARY_EXPR_BUILDER(signed_less_equal, Sle)
DEF_BINARY_EXPR_BUILDER(signed_greater_than, Sgt)
DEF_BINARY_EXPR_BUILDER(signed_greater_equal, Sge)
DEF_BINARY_EXPR_BUILDER(unsigned_less_than, Ult)
DEF_BINARY_EXPR_BUILDER(unsigned_less_equal, Ule)
DEF_BINARY_EXPR_BUILDER(unsigned_greater_than, Ugt)
DEF_BINARY_EXPR_BUILDER(unsigned_greater_equal, Uge)
DEF_BINARY_EXPR_BUILDER(equal, Equal)
DEF_BINARY_EXPR_BUILDER(not_equal, Distinct)

DEF_BINARY_EXPR_BUILDER(bool_and, LAnd)
DEF_BINARY_EXPR_BUILDER(and, And)
DEF_BINARY_EXPR_BUILDER(bool_or, LOr)
DEF_BINARY_EXPR_BUILDER(or, Or)
DEF_BINARY_EXPR_BUILDER(bool_xor, Distinct)
DEF_BINARY_EXPR_BUILDER(xor, Xor)

#undef DEF_BINARY_EXPR_BUILDER

SymExpr _sym_build_neg(SymExpr expr) {
  return registerExpression(
      g_expr_builder->createNeg(allocatedExpressions.at(expr)));
}

SymExpr _sym_build_not(SymExpr expr) {
  return registerExpression(
      g_expr_builder->createNot(allocatedExpressions.at(expr)));
}

SymExpr _sym_build_sext(SymExpr expr, uint8_t bits) {
  return registerExpression(g_expr_builder->createSExt(
      allocatedExpressions.at(expr), bits + expr->bits()));
}

SymExpr _sym_build_zext(SymExpr expr, uint8_t bits) {
  return registerExpression(g_expr_builder->createZExt(
      allocatedExpressions.at(expr), bits + expr->bits()));
}

SymExpr _sym_build_trunc(SymExpr expr, uint8_t bits) {
  return registerExpression(
      g_expr_builder->createTrunc(allocatedExpressions.at(expr), bits));
}

std::vector<uintptr_t> site_ids;

bool map_initialsed = false;
bool prev_blocks_initialised = false;
bool tracemaps_initialised = false;

// 
// Function for verifying if we should save the values
// when pushing a path constraint.
// Return false when no saving should be done.
// Returns true when saving should be done.
// 
bool pure_concolic_should_save(SymExpr constraint, int taken,
                               uintptr_t site_id) {

  // Handle the case where the counters have not yet
  // been initialised. 
  if (!map_initialsed) {
      ifstream myfile("corpus_counters.stats");
      std::string line2;

      int first = 0;
      uint32_t cb_id = 0;
      uint32_t cb_count = 0;
      while (std::getline(myfile, line2)) {
        if (first == 0) {
            cb_id = (uint32_t)atol(line2.c_str());
            first++;
        } else {
            cb_count = (uint32_t)atol(line2.c_str());
            first =0;
            old_counter_map[cb_id] = cb_count;
            cb_id = 0;
            cb_count =0;
        }
      }

      // Now write the counters for logging.
      //std::cerr << "The counters we read:\n";
      //for (auto& it: old_counter_map) {
	  //	std::cerr << "(" << it.first << ", " << it.second << ")\n";
	  //}
	  // std::cerr << "-----------\n";
        map_initialsed = true;
  }

  if (!tracemaps_initialised) {
      uint32_t cb_id = 0;
    ifstream myfile("trace_maps.txt");
    std::string line3;
    while (std::getline(myfile, line3)) {
        cb_id = (uint32_t)atol(line3.c_str());
        trace_maps.push_back(cb_id);
    }
   tracemaps_initialised = true;
  }

  if (!prev_blocks_initialised) {
      uint32_t cb_id = 0;
    ifstream myfile("prev_0s.txt");
    std::string line3;
    while (std::getline(myfile, line3)) {
        cb_id = (uint32_t)atol(line3.c_str());
        takens_0.push_back(cb_id);
    }

    ifstream myfile1("prev_1s.txt");
    std::string line4;
    while (std::getline(myfile1, line4)) {
        cb_id = (uint32_t)atol(line4.c_str());
        takens_1.push_back(cb_id);
    }
    prev_blocks_initialised = true;
  }
   
  // Now let's check if there is a difference in corpus
  bool should_save = false;

  // Go through the counter map and see if we have bettered anything.
  for (auto& new_c: counter_map) {
    uint32_t k,v;
    k = new_c.first;
    v = new_c.second;
    if (old_counter_map.count(k) == 0) {
      std::cerr << "Found counter not in old map " << k << "\n";
      should_save = true;
      break;
    } 
    if (old_counter_map.at(k) < v) {
      should_save = true;
      break;
    }
  }
 
  // Now go through all of the branches we have solved so far. We must ensure
  // we have a true/false for each "previous branch". This is what we 
  // do right here.
  bool should_save2 = false;
  if (taken == 0 ) {
    bool found_target = false;
    for (auto &tb: takens_0) {
      if (tb == just_visited_bb) {
        found_target = true;
      }
    }
    // if we didn't find the target, it means we must solve this one.
    if (found_target == false) {
        takens_0.push_back(just_visited_bb);
        should_save2 = true;
    }

    // Let's also includel curr trace map in our takens
    found_target = false;
    for (auto &tb: takens_0) {
      if (tb == curr_trace_map) {
        found_target = true;
      }
    }
    // if we didn't find the target, it means we must solve this one.
    if (found_target == false) {
        takens_0.push_back(curr_trace_map);
        should_save2 = true;
    }
  } 
  else {
    bool found_target = false;
    for (auto &tb: takens_1) {
      if (tb == just_visited_bb) {
        found_target = true;
      }
    }
    // if we didn't find the target, it means we must solve this one.
    if (found_target == false) {
        takens_1.push_back(just_visited_bb);
        should_save2 = true;
    }

    //  lets do tracemap as well.
    found_target = false;
    for (auto &tb: takens_1) {
      if (tb == curr_trace_map) {
        found_target = true;
      }
    }
    // if we didn't find the target, it means we must solve this one.
    if (found_target == false) {
        takens_1.push_back(curr_trace_map);
        should_save2 = true;
    }
  }

  if (should_save2) should_save = true;

  // Check if it is in the trace map
  bool found_trace_map = false;
  bool should_save3 = false;
  for (auto &tmap: trace_maps) {
    if (tmap == curr_trace_map) {
      found_trace_map = true;
    }
  }
  if (found_trace_map == false) {
    trace_maps.push_back(curr_trace_map);
    should_save3 = true;
  }
  if (should_save3) should_save = true;

  // Am unsure if the stuff below should be necessary, since
  // we make the switch instructions verbose.
  // Ensure this is not a switch instruction.
  // We need a hack on switch instructions to allow analysis.
  // We save the site_id for all should_saves. should_save will be
  // true the first time the switch is called, but false all other times. We
  // need to fix this.
  if (should_save == false) {
    for (auto &sid : site_ids) {
      if (sid == site_id) {
        should_save = true;
      }
    }
  }
  if (should_save) {
    site_ids.push_back(site_id);
  }

  // Logging
  if (should_save) {
    std::cerr << "Should save\n";
  } else {
    std::cerr << "Should not save\n";
  }

  std::cerr << "Taken " << taken << "\n";
  std::cerr << "Siteid " << site_id << " , Taken " << taken << "\n";

  // Hack to allow permanent solving. For debugging
  static bool force_check = false;
  if (force_check) {
    should_save = true;
  }

  if (should_save) {
    std::cerr << "Saving\n";
    previosuly_blocked_a_save = false;
  } 
    else {
        previosuly_blocked_a_save = true;
    }

  return should_save;
}

void _sym_push_path_constraint(SymExpr constraint, int taken,
                               uintptr_t site_id) {
  std::cerr << "pushing path constraint\n";
  if (constraint == nullptr)
    return;

  
  bool should_save;
  if (g_config.is_pure_concolic) {
    should_save = pure_concolic_should_save(constraint, taken, site_id);
  } else {
    should_save = true;
  }
  g_solver->addJcc(allocatedExpressions.at(constraint), taken != 0, site_id, should_save);
}

SymExpr _sym_get_input_byte(size_t offset) {
  return registerExpression(g_expr_builder->createRead(offset));
}

SymExpr _sym_concat_helper(SymExpr a, SymExpr b) {
  return registerExpression(g_expr_builder->createConcat(
      allocatedExpressions.at(a), allocatedExpressions.at(b)));
}

SymExpr _sym_extract_helper(SymExpr expr, size_t first_bit, size_t last_bit) {
  return registerExpression(g_expr_builder->createExtract(
      allocatedExpressions.at(expr), last_bit, first_bit - last_bit + 1));
}

size_t _sym_bits_helper(SymExpr expr) { return expr->bits(); }

SymExpr _sym_build_bool_to_bits(SymExpr expr, uint8_t bits) {
  return registerExpression(
      g_expr_builder->boolToBit(allocatedExpressions.at(expr), bits));
}

//
// Floating-point operations (unsupported in Qsym)
//

#define UNSUPPORTED(prototype)                                                 \
  prototype { return nullptr; }

UNSUPPORTED(SymExpr _sym_build_float(double, int))
UNSUPPORTED(SymExpr _sym_build_fp_add(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_fp_sub(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_fp_mul(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_fp_div(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_fp_rem(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_fp_abs(SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_greater_than(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_greater_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_less_than(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_less_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered_not_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_ordered(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_greater_than(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_greater_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_less_than(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_less_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_unordered_not_equal(SymExpr, SymExpr))
UNSUPPORTED(SymExpr _sym_build_int_to_float(SymExpr, int, int))
UNSUPPORTED(SymExpr _sym_build_float_to_float(SymExpr, int))
UNSUPPORTED(SymExpr _sym_build_bits_to_float(SymExpr, int))
UNSUPPORTED(SymExpr _sym_build_float_to_bits(SymExpr))
UNSUPPORTED(SymExpr _sym_build_float_to_signed_integer(SymExpr, uint8_t))
UNSUPPORTED(SymExpr _sym_build_float_to_unsigned_integer(SymExpr, uint8_t))

#undef UNSUPPORTED
#undef H

static int idx_hop = 0;

void _symcc_cov_cb(uint32_t cb_id) {
std::cerr << "symcc_cov_cb\n";
    if (counter_map.count(cb_id) == 0) {
        // Insert the counter in the map.
        counter_map[cb_id] = 1;
        
        // Increment our trace integer
        curr_trace_map += cb_id;
      
        // Check if the counter is in the old map, and if not, save the values.
        // This check is most likely no longer needed and also dubious of nature,
        // so verify that this can be removed.
        if (old_counter_map.count(cb_id) == 0) {
            if (previosuly_blocked_a_save) {
                g_solver->checkAndSave("");
            }
        }
    } else {
        counter_map[cb_id]++;
    }
    just_visited_bb = cb_id;
}

//
// Call-stack tracing
//
void _sym_notify_call(uintptr_t site_id) {
  g_call_stack_manager.visitCall(site_id);
}

void _sym_notify_ret(uintptr_t site_id) {
  g_call_stack_manager.visitRet(site_id);
}

void _sym_notify_basic_block(uintptr_t site_id) {
  g_call_stack_manager.visitBasicBlock(site_id);
}

//
// Debugging
//

const char *_sym_expr_to_string(SymExpr expr) {
  static char buffer[4096];

  auto expr_string = expr->toString();
  auto copied = expr_string.copy(
      buffer, std::min(expr_string.length(), sizeof(buffer) - 1));
  buffer[copied] = '\0';

  return buffer;
}

bool _sym_feasible(SymExpr expr) {
  expr->simplify();

  g_solver->push();
  g_solver->add(expr->toZ3Expr());
  bool feasible = (g_solver->check() == z3::sat);
  g_solver->pop();

  return feasible;
}

//
// Garbage collection
//

void _sym_collect_garbage() {
  std::cerr << "collect garbage 1\n";
  if (allocatedExpressions.size() < g_config.garbageCollectionThreshold)
    return;
  std::cerr << "collect garbage 2\n";

#ifdef DEBUG_RUNTIME
  auto start = std::chrono::high_resolution_clock::now();
#endif

  auto reachableExpressions = collectReachableExpressions();
  for (auto expr_it = allocatedExpressions.begin();
       expr_it != allocatedExpressions.end();) {
    if (reachableExpressions.count(expr_it->first) == 0) {
      expr_it = allocatedExpressions.erase(expr_it);
    } else {
      ++expr_it;
    }
  }
  std::cerr << "collect garbage 3\n";

#ifdef DEBUG_RUNTIME
  auto end = std::chrono::high_resolution_clock::now();

  std::cerr << "After garbage collection: " << allocatedExpressions.size()
            << " expressions remain" << std::endl
            << "\t(collection took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << " milliseconds)" << std::endl;
#endif
  std::cerr << "collect garbage 4\n";
}
