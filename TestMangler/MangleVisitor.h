#ifndef FORTRAN_FLANG_MANGLE_VISITOR_H
#define FORTRAN_FLANG_MANGLE_VISITOR_H

#include "flang/Parser/parse-tree-visitor.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Parser/parsing.h"

#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string_view>

namespace Testgen {

template <typename T> struct DeferredAction {
  virtual void run(T &node) = 0;
  virtual ~DeferredAction<T>() = default;
};
typedef DeferredAction<Fortran::parser::SubroutineSubprogram> EndAction;

// Deferred actions that manipulate the order and existence of clauses
struct DeferredClauseAction {
  virtual void run(std::list<Fortran::parser::OmpClause> &list) {}
  virtual void run(std::list<Fortran::parser::OmpAtomicClause> &list) {}

  DeferredClauseAction(size_t i) : index(i) {}
  virtual ~DeferredClauseAction() = default;

private:
  size_t index;
};

struct MangleVisitor {
  template <typename A> bool Pre(A &) { return true; }
  template <typename A> void Post(A &) {}

  // Function template for scheduling deferred actions that
  // modify clause positions
  template <typename T> void VisitClause(T &, size_t, bool) {}

  void Post(Fortran::parser::OpenMPExecutableAllocate &);
  void Post(Fortran::parser::OpenMPAllocatorsConstruct &);
  void Post(Fortran::parser::SubroutineSubprogram &);
  void Post(Fortran::parser::OmpClauseList &);
  void Post(Fortran::parser::OmpAtomicClauseList &);

  MangleVisitor(size_t o = 0) : offset{o} {}

  // This test format expects directive tests to be inside SubroutineSubprograms
  void addAction(std::unique_ptr<EndAction> a) {
    endActions.push_back(std::move(a));
  }

  void addAction(std::unique_ptr<DeferredClauseAction> a) {
    if (!clauseAction.has_value()) {
      clauseAction = {std::move(a)};
    } else {
      llvm::errs() << "Only one deferred clause action can"
                      " be scheduled at a time\n";
      exit(1);
    }
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
  size_t offset, index{0};
  bool modified{false};

  // Contains functions names that only trigger once
  // Used with the RUN_ONCE macro
  std::set<std::string_view> uniqueEvents;

  // Actions that occur at the end of a subroutine program
  // This will often occur at the end of the test;
  std::vector<std::unique_ptr<EndAction>> endActions;

  // No more than one positional clause action can be scheduled at a time
  std::optional<std::unique_ptr<DeferredClauseAction>> clauseAction;

  // Track index and modified status
  void setModified() { modified = true; }
  bool canModify() { return ++index == offset; }

  template <typename T> void applyClauseAction(T &clauseList) {
    if (clauseAction.has_value()) {
      (*clauseAction)->run(clauseList.v);
      clauseAction = std::nullopt;
    }
  }
};

} // namespace Testgen
#endif /* FORTRAN_FLANG_MANGLE_VISITOR_H */
