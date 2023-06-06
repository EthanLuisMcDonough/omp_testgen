#include "flang/Frontend/CompilerInstance.h"
#include "flang/Frontend/FrontendActions.h"
#include "flang/Frontend/FrontendPluginRegistry.h"
#include "flang/Parser/dump-parse-tree.h"
#include "flang/Parser/parsing.h"
#include "flang/Parser/unparse.h"

#include "llvm/Support/ErrorHandling.h"

#include "MangleVisitor.h"

#include <cstdlib>

using namespace Fortran::frontend;
using namespace Testgen;

class MangleAction : public PluginParseTreeAction {
  void executeAction() override {
    const char *offset_evar = std::getenv("MANGLE_OFFSET");
    if (!offset_evar) {
      llvm::errs() << "MANGLE_OFFSET environment variable "
                      "not present\n";
      exit(1);
    }

    int offset = atoi(offset_evar);
    if (offset <= 0) {
      llvm::errs() << "MANGLE_OFFSET environment variable "
                      "must be greater than zero\n";
      exit(1);
    }

    auto &parseTree{getParsing().parseTree()};
    auto &invoc = getInstance().getInvocation();

    MangleVisitor visitor(offset);
    Fortran::parser::Walk(parseTree, visitor);

    if (!visitor.wasModified()) {
      llvm::errs() << "No error with given index\n";
      exit(1);
    }

    visitor.printHeader();
    Unparse(llvm::outs(), *parseTree, Fortran::parser::Encoding::UTF_8, true,
        false, nullptr,
        invoc.getUseAnalyzedObjectsForUnparse() ? &invoc.getAsFortran()
                                                : nullptr);
  }

  bool beginSourceFileAction() override { return runPrescan() && runParse(); }
};

static FrontendPluginRegistry::Add<MangleAction> X("test-mangle",
    "Transforms a successful OpenMP test into various XFAIL tests");
