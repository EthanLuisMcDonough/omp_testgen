#include "MangleVisitor.h"
#include "flang/Common/idioms.h"
#include "flang/Parser/tools.h"

#include <cstring>
#include <variant>

// Only check a specified function once
#define RUN_ONCE(fn, ...) \
  if (!uniqueRegistered(#fn)) { \
    registerUniqueEvent(#fn); \
    if (canModify()) { \
      fn(__VA_ARGS__); \
      setModified(); \
    } \
  }

#define RUN_CHECK(b) \
  if (canModify()) { \
    b; \
    setModified(); \
  }

using namespace Fortran::parser;
using namespace Testgen;

Name genName(const char *str) {
  CharBlock chars{str, strlen(str)};
  Name name{chars};
  return name;
}

OmpObject genOmpDataref(const char *str) {
  DataRef ref(genName(str));
  Designator desig(std::move(ref));
  return OmpObject(std::move(desig));
}

OmpClause cloneConstruct(const OmpClause &x) {
  OmpClause::Acquire defaultClause;
  OmpClause c(std::move(defaultClause));
  memcpy(&c, &x, sizeof(OmpClause));
  return c;
}

OmpAtomicClause cloneConstruct(const OmpAtomicClause &x) {
  OmpClause::Acquire defaultClause;
  OmpClause c(std::move(defaultClause));
  OmpAtomicClause ac(std::move(c));
  memcpy(&ac, &x, sizeof(OmpClause));
  return ac;
}

namespace DeferredActions {
// Violates purity constraint
struct MakePure : public DeferredAction<SubroutineSubprogram> {
  void run(SubroutineSubprogram &node) override {
    auto &sub{std::get<Statement<SubroutineStmt>>(node.t).statement};
    auto &prefix{std::get<std::list<PrefixSpec>>(sub.t)};
    for (const auto &spec : prefix) {
      if (std::holds_alternative<PrefixSpec::Pure>(spec.u)) {
        return;
      }
    }
    prefix.emplace_back(PrefixSpec::Pure());
  }
};

// Adds a scalar allocatable variable to the subprogram provided
// an allocatable namelist is alrady present
struct CreateNewAllocatable : public DeferredAction<SubroutineSubprogram> {
  CreateNewAllocatable(const char *n) : ident_name{n} {}

  void run(SubroutineSubprogram &node) override {
    auto &spec{std::get<SpecificationPart>(node.t)};
    auto &decls{std::get<std::list<DeclarationConstruct>>(spec.t)};
    for (auto &decl : decls) {
      if (auto *decl_stmt{extractAllocSpec(decl)}) {
        auto &variables = std::get<std::list<EntityDecl>>(decl_stmt->t);
        std::optional<ArraySpec> arrSpec;
        std::optional<CoarraySpec> coarrSpec;
        std::optional<CharLength> charLen;
        std::optional<Initialization> init;
        variables.emplace_back(genName(ident_name), std::move(arrSpec),
            std::move(coarrSpec), std::move(charLen), std::move(init));
        break;
      }
    }
  }

private:
  const char *ident_name;

  // Extracts decl list from decl construct if type is allocatable
  TypeDeclarationStmt *extractAllocSpec(DeclarationConstruct &decl) {
    if (auto *spec_constr{std::get_if<SpecificationConstruct>(&decl.u)}) {
      if (auto *declStmt{Unwrap<TypeDeclarationStmt>(*spec_constr)}) {
        const auto &attrs{std::get<std::list<AttrSpec>>(declStmt->t)};
        for (const auto &attr : attrs) {
          if (std::holds_alternative<Allocatable>(attr.u)) {
            return declStmt;
          }
        }
      }
    }
    return nullptr;
  }
};

struct DeleteClause : public DeferredClauseAction {
  void run(OmpClauses &list, OmpClauseIter iter) override { list.erase(iter); }
  void run(OmpAtomicClauses &list, OmpAtomicClauseIter iter) override {
    list.erase(iter);
  }
  DeleteClause(std::variant<OmpClauseIter, OmpAtomicClauseIter> i)
      : DeferredClauseAction(i) {}
};

struct DuplicateClause : public DeferredClauseAction {
  void run(OmpClauses &list, OmpClauseIter iter) override {
    list.push_back(cloneConstruct(*iter));
  }
  void run(OmpAtomicClauses &list, OmpAtomicClauseIter iter) override {
    list.push_back(cloneConstruct(*iter));
  }
  DuplicateClause(std::variant<OmpClauseIter, OmpAtomicClauseIter> i)
      : DeferredClauseAction(i) {}
};
} // namespace DeferredActions

namespace AssociationProperties {
// "association properties": [ "allocator structured block" ]
void allocatorStructuredBlock(
    OpenMPAllocatorsConstruct &x, MangleVisitor &visitor) {
  const char *n = "unused_allocators_var";
  auto &clauses = std::get<OmpClauseList>(x.t).v;
  for (auto &clause : clauses) {
    if (auto *allocator{std::get_if<OmpClause::Allocate>(&clause.u)}) {
      auto &objs{std::get<OmpObjectList>(allocator->v.t).v};
      objs.emplace_back(genOmpDataref(n));
      visitor.addAction(
          std::make_unique<DeferredActions::CreateNewAllocatable>(n));
      break;
    }
  }
}
void allocatorStructuredBlock(
    OpenMPExecutableAllocate &x, MangleVisitor &visitor) {
  const char *n = "unused_allocate_var";
  auto &objs{std::get<std::optional<OmpObjectList>>(x.t)};
  if (objs.has_value()) {
    objs->v.push_back(genOmpDataref(n));
  } else {
    std::list<OmpObject> o;
    o.push_back(genOmpDataref(n));
    objs = {OmpObjectList(std::move(o))};
  }
  visitor.addAction(std::make_unique<DeferredActions::CreateNewAllocatable>(n));
}
} // namespace AssociationProperties

namespace SharedProperties {
// Should throw error if impure directive appears in pure function
void sharedPure(MangleVisitor &visitor) {
  visitor.addAction(std::make_unique<DeferredActions::MakePure>());
}
} // namespace SharedProperties

namespace ClauseProperties {

template <typename T> void required(T index, MangleVisitor &visitor) {
  visitor.addAction(std::make_unique<DeferredActions::DeleteClause>(index));
}

template <typename T> void unique(T index, MangleVisitor &visitor) {
  visitor.addAction(std::make_unique<DeferredActions::DuplicateClause>(index));
}

} // namespace ClauseProperties

void MangleVisitor::Post(OpenMPExecutableAllocate &x) {
  RUN_CHECK(AssociationProperties::allocatorStructuredBlock(x, *this))
  RUN_ONCE(SharedProperties::sharedPure, *this)
}

void MangleVisitor::Post(OpenMPAllocatorsConstruct &x) {
  RUN_CHECK(AssociationProperties::allocatorStructuredBlock(x, *this))
  RUN_ONCE(SharedProperties::sharedPure, *this)
}

template <typename I>
void MangleVisitor::VisitClause(const OmpClause::Device &c, I iter, bool _) {
  RUN_CHECK(ClauseProperties::unique(iter, *this))
}

/// Event execution
void MangleVisitor::Post(SubroutineSubprogram &x) {
  for (const auto &action : endActions) {
    action->run(x);
  }
  endActions.clear();
}

void MangleVisitor::Post(OmpClauseList &x) {
  // Feed values and iterators into VisitClause
  for (OmpClauseIter it = x.v.begin(); it != x.v.end(); it++) {
    std::visit([&](auto &&c) { VisitClause(c, it, false); }, it->u);
  }

  applyClauseAction(x);
}

void MangleVisitor::Post(OmpAtomicClauseList &x) {
  // Feed values and indices into VisitClause
  for (OmpAtomicClauseIter it = x.v.begin(); it != x.v.end(); it++) {
    Fortran::common::visit(
        Fortran::common::visitors{
            [&](const OmpClause &clause) {
              std::visit(
                  [&](auto &&c) { VisitClause(c, it, false); }, clause.u);
            },
            [&](const OmpMemoryOrderClause &orderClause) {
              std::visit(
                  [&](auto &&c) { VisitClause(c, it, true); }, orderClause.v.u);
            },
        },
        it->u);
  }

  applyClauseAction(x);
}
