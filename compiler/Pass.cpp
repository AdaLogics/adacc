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

#include "Pass.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include "llvm/Transforms/Utils.h"
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include "Runtime.h"
#include "Symbolizer.h"

//#include <time.h>
#include <ctime>

#include <random>

using namespace llvm;

#ifndef NDEBUG
#define DEBUG(X)                                                               \
  do {                                                                         \
    X;                                                                         \
  } while (false)
#else
#define DEBUG(X) ((void)0)
#endif

char SymbolizePass::ID = 0;

bool SymbolizePass::doInitialization(Module &M) {
  errs() << "Symbolizer module init\n";
  DEBUG(errs() << "Symbolizer module init\n");

  //errs() << "Creating a random number now huh\n"; 
  srand(time(NULL));
  int isecret;
  isecret = rand() % 100000 + 1;
  //printf("Random number: %d\n", isecret);


  errs() << "Going through the symboliser \n";
  errs() << "Analysing filename " << M.getSourceFileName() << "\n";
  // Redirect calls to external functions to the corresponding wrappers and
  // rename internal functions.
  for (auto &function : M.functions()) {
    auto name = function.getName();
    //errs() << "Getting function: " << name << "\n";
    if (isInterceptedFunction(function))
      function.setName(name + "_symbolized");
  }

  // Insert a constructor that initializes the runtime and any globals.
  Function *ctor;
  std::tie(ctor, std::ignore) = createSanitizerCtorAndInitFunctions(
      M, "__sym_ctor", "_sym_initialize", {}, {});
      //M, kSymCtorName, "_sym_initialize", {}, {});
  appendToGlobalCtors(M, ctor, 0);

  // Add a dtor function for cleaning up paths.
  IRBuilder<> IRB(M.getContext());
  Type *void_type = IRB.getVoidTy();
  FunctionType *FT = FunctionType::get(void_type, void_type, false);
  Function::Create(FT, Function::ExternalLinkage, "__dtor_runtime", M);

  appendToGlobalDtors(M, M.getFunction("__dtor_runtime"), 0);

  return true;
}

bool SymbolizePass::runOnFunction(Function &F) {
  auto functionName = F.getName();
  //if (functionName == kSymCtorName)
  if (functionName == "__sym_ctor")
    return false;

  if (functionName.find("sanitizer_cov_trace") != std::string::npos) {
     return false;
  }

  DEBUG(errs() << "Symbolizing function ");
  DEBUG(errs().write_escaped(functionName) << '\n');

  SmallVector<Instruction *, 0> allInstructions;
  allInstructions.reserve(F.getInstructionCount());
  for (auto &I : instructions(F))
    allInstructions.push_back(&I);

  //errs() << "Creating the symbolizer\n";
  Symbolizer symbolizer(*F.getParent());
  symbolizer.symbolizeFunctionArguments(F);
  //errs() << "My random number: " << symbolizer.my_random_number++ << "\n";


  
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist6(1,1000000000);

  //errs() << "dist6 random number " << dist6(rng) << "\n";
  symbolizer.my_random_number = dist6(rng);
  
  for (auto &basicBlock : F)
    symbolizer.insertBasicBlockNotification(basicBlock);

  for (auto *instPtr : allInstructions)
    symbolizer.visit(instPtr);

  symbolizer.finalizePHINodes();
  symbolizer.shortCircuitExpressionUses();

  // This inserts all of the code for pure concolic execution.
  auto *pureConcolic= getenv("SYMCC_PC");
  if (pureConcolic != nullptr) {
    errs() << "We are instrumenting for pure concolic execution\n";
    for (auto &basicBlock : F) {
        symbolizer.insertCovs(basicBlock);
    }
  } 
  //else {
  //  errs() << "We have no pure concolic execution\n";
  //}


  // DEBUG(errs() << F << '\n');
  assert(!verifyFunction(F, &errs()) &&
         "SymbolizePass produced invalid bitcode");

  return true;
}
