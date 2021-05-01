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

#include <llvm/IR/LegacyPassManager.h>
#include "llvm/Transforms/Utils.h"
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "Pass.h"

void addSymbolizePass(const llvm::PassManagerBuilder & /* unused */,
                      llvm::legacy::PassManagerBase &PM) {
  PM.add(llvm::createLowerSwitchPass());
        
//          FunctionPass *lower = createLowerSwitchPass();
  //          lower->runOnFunction(F);

  PM.add(new SymbolizePass());
}

// Make the pass known to opt.
static llvm::RegisterPass<SymbolizePass> X("symbolize", "Symbolization Pass");
// Tell frontends to run the pass automatically.
static struct llvm::RegisterStandardPasses
    Y(llvm::PassManagerBuilder::EP_VectorizerStart, addSymbolizePass);
static struct llvm::RegisterStandardPasses
    Z(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, addSymbolizePass);
