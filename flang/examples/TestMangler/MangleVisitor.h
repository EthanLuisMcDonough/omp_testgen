#ifndef FORTRAN_FLANG_MANGLE_VISITOR_H
#define FORTRAN_FLANG_MANGLE_VISITOR_H

#include "flang/Parser/parse-tree-visitor.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Parser/parsing.h"

#include <memory>
#include <stack>

namespace Fortran {
namespace parser {

template <typename T> struct DeferredAction { virtual void run(T &node) = 0; };

struct MangleVisitor {
  template <typename A> bool Pre(A &) { return true; }
  template <typename A> void Post(A &) {}

  void Post(OpenMPExecutableAllocate &);
  void Post(OpenMPAllocatorsConstruct &);
  void Post(SubroutineSubprogram &);

  MangleVisitor(int o = 0) : offset{o} {}

  // This test format expects directive tests to be inside SubroutineSubprograms
  void addEndAction(std::unique_ptr<DeferredAction<SubroutineSubprogram>> a) {
    endActions.push_back(std::move(a));
  }

  void printHeader() {
    llvm::outs() << "! ERROR: " << offset << " / " << index << "\n";
  }
  bool wasModified() { return modified; }

private:
  int offset, index{0};
  bool modified{false};
  std::vector<std::unique_ptr<DeferredAction<SubroutineSubprogram>>> endActions;

  void setModified() { modified = true; }
  bool canModify() { return ++index == offset; }
};

} // namespace parser
} // namespace Fortran
#endif /* FORTRAN_FLANG_MANGLE_VISITOR_H */
