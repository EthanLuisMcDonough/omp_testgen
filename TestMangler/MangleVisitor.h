#ifndef FORTRAN_FLANG_MANGLE_VISITOR_H
#define FORTRAN_FLANG_MANGLE_VISITOR_H

#include "flang/Parser/parse-tree-visitor.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Parser/parsing.h"

#include <memory>
#include <set>
#include <stack>
#include <string_view>

namespace Testgen {

template <typename T> struct DeferredAction { virtual void run(T &node) = 0; };
typedef DeferredAction<Fortran::parser::SubroutineSubprogram> EndAction;

struct MangleVisitor {
  template <typename A> bool Pre(A &) { return true; }
  template <typename A> void Post(A &) {}

  void Post(Fortran::parser::OpenMPExecutableAllocate &);
  void Post(Fortran::parser::OpenMPAllocatorsConstruct &);
  void Post(Fortran::parser::SubroutineSubprogram &);

  MangleVisitor(int o = 0) : offset{o} {}

  // This test format expects directive tests to be inside SubroutineSubprograms
  void addEndAction(std::unique_ptr<EndAction> a) {
    endActions.push_back(std::move(a));
  }

  void printHeader() {
    llvm::outs() << "! ERROR: " << offset << " / " << index << "\n";
  }
  bool wasModified() { return modified; }

  void registerUniqueEvent(std::string_view s) { uniqueEvents.insert(s); }
  bool uniqueRegistered(std::string_view s) {
    return uniqueEvents.find(s) != uniqueEvents.end();
  }

private:
  int offset, index{0};
  bool modified{false};

  // Contains functions names that only trigger once
  // Used with the RUN_ONCE macro
  std::set<std::string_view> uniqueEvents;

  // Actions that occur at the end of a subroutine program
  // This will often occur at the end of the test;
  std::vector<std::unique_ptr<EndAction>> endActions;

  // Track index and modified status
  void setModified() { modified = true; }
  bool canModify() { return ++index == offset; }
};

} // namespace Testgen
#endif /* FORTRAN_FLANG_MANGLE_VISITOR_H */
