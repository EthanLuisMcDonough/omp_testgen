#include "MangleVisitor.h"
#include "flang/Parser/tools.h"

#include <cstring>
#include <variant>

#define RUN_CHECK(b) \
  if (canModify()) { \
    b; \
    setModified(); \
  }

using namespace Fortran::parser;

Name gen_name(const char *str) {
  CharBlock chars{str, strlen(str)};
  Name name{chars};
  return name;
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
        variables.emplace_back(gen_name(ident_name), std::move(arrSpec),
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
} // namespace DeferredActions

namespace AssociationProperties {
// "association properties": [ "allocator structured block" ]
// OpenMPExecutableAllocate | OpenMPAllocatorsConstruct
template <typename T>
void allocatorStructuredBlock(T &x, MangleVisitor &visitor) {
  const char *n = "unused_allocate_var";
  auto &alloc = std::get<Statement<AllocateStmt>>(x.t).statement;
  auto &allocations = std::get<std::list<Allocation>>(alloc.t);
  std::list<AllocateShapeSpec> shape;
  std::optional<AllocateCoarraySpec> coarraySpec;
  allocations.emplace_back(
      gen_name(n), std::move(shape), std::move(coarraySpec));
  visitor.addEndAction(
      std::make_unique<DeferredActions::CreateNewAllocatable>(n));
}
} // namespace AssociationProperties

namespace SharedProperties {
// Opposite of pure
// Should throw error if directive appears in impure function
void sharedPure(MangleVisitor &visitor) {
  visitor.addEndAction(std::make_unique<DeferredActions::MakePure>());
}
} // namespace SharedProperties

void MangleVisitor::Post(OpenMPExecutableAllocate &x) {
  RUN_CHECK(AssociationProperties::allocatorStructuredBlock(x, *this))
  RUN_CHECK(SharedProperties::sharedPure(*this))
}

void MangleVisitor::Post(OpenMPAllocatorsConstruct &x) {
  RUN_CHECK(AssociationProperties::allocatorStructuredBlock(x, *this))
  RUN_CHECK(SharedProperties::sharedPure(*this))
}

void MangleVisitor::Post(SubroutineSubprogram &x) {
  for (const auto &action : endActions) {
    action->run(x);
  }
  endActions.clear();
}
