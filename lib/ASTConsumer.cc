#include "openbsd_list_macro_printer/ASTConsumer.hh"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include <array>
#include <cassert>
#include <format>
#include <llvm-17/llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

namespace openbsd_list_macro_printer {

/* Names of the OpenBSD list macros.
 *
 * TODO(Brent): Use an enum to represent these names instead.  */
static const constexpr std::array<std::string, 6> OpenBSDQueueMacroDeclNames = {
    std::string("SLIST_HEAD"),   std::string("LIST_HEAD"),
    std::string("SIMPLEQ_HEAD"), std::string("XSIMPLEQ_HEAD"),
    std::string("TAILQ_HEAD"),   std::string("STAILQ_HEAD")};

/* Represents a variable declaration whose type was declared using one of the
 * OpenBSD macros defined in `src/sys/sys/queue.h`. */
struct OpenBSDQueueMacroDecl {
  /* The name of the HEAD() macro invoked to create this declaration. One of:
   * - SLIST_HEAD
   * - LIST_HEAD
   * - SIMPLEQ_HEAD
   * - XSIMPLEQ_HEAD
   * - TAILQ_HEAD
   * - STAILQ_HEAD
   * */
  std::string OpenBSDListDeclarationMacroName;

  /* The record decl that this macro invocation declared. */
  const clang::RecordDecl *RecordDecl;

  /* The variable that this macro invocation declared the type of. */
  const clang::VarDecl *VarDecl;
};

/* A matcher callback that collects FieldDecls. */
class FieldDeclarationMatcherCallback
    : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  std::vector<const clang::FieldDecl *> Matches;

  virtual void
  run(const clang::ast_matchers::MatchFinder::MatchResult &Result) final {
    if (const auto FieldDecl =
            Result.Nodes.getNodeAs<clang::FieldDecl>("root")) {
      Matches.push_back(FieldDecl);
    }
  }
};

/* A matcher callback that collects RecordDecls declared using one of the
 * OpenBSD list declaration macro. */
class OpenBSDListMacroDeclMatchCallback
    : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  /* The name of the OpenBSD list declaration macro that this macro should be
   * collecting matches for. */
  std::string OpenBSDListDeclarationMacroName;

  std::vector<OpenBSDQueueMacroDecl> Matches;

  explicit OpenBSDListMacroDeclMatchCallback(
      std::string OpenBSDListDeclarationMacroName)
      : OpenBSDListDeclarationMacroName(OpenBSDListDeclarationMacroName){};

  virtual void
  run(const clang::ast_matchers::MatchFinder::MatchResult &Result) final {
    if (const auto VarDecl = Result.Nodes.getNodeAs<clang::VarDecl>("root")) {
      const auto RecordDecl = VarDecl->getType()->getAsRecordDecl();
      OpenBSDQueueMacroDecl MacroDecl(OpenBSDListDeclarationMacroName,
                                      RecordDecl, VarDecl);
      Matches.push_back(MacroDecl);
    }
  }
};

/* Finds all declarations expanded from the given OpenBSD list macro.
 * OpenBSDListDeclarationMacroName should be a member of the global variable
 * OpenBSDQueueMacroDeclNames. */
std::vector<OpenBSDQueueMacroDecl>
FindOpenBSDQueueMacroDecls(clang::ASTContext &Ctx,
                           std::string OpenBSDListDeclarationMacroName) {
  using namespace clang::ast_matchers;
  MatchFinder Finder;
  OpenBSDListMacroDeclMatchCallback Callback(OpenBSDListDeclarationMacroName);
  /* This matcher does most of the work. Here we are telling Clang to find all
   * variable declarations having a type that is a record decl (e.g., a struct),
   * whose declaration was expanded from a macro with the name
   * `OpenBSDListDeclarationMacroName`. We then bind the matched declaration to
   * the name "root" for the callback (defined above) to hook into.
   *
   * For instance, assuming "SLIST_HEAD" == OpenBSDListDeclarationMacroName,
   * then for this line of code:
   *
   * ```c
   * SLIST_HEAD(slisthead, entry) head = SLIST_HEAD_INITIALIZER(head);
   * ```
   *
   * The matcher would match the declaration for `head`, and bind it to the name
   * "root".
   */
  DeclarationMatcher Matcher =
      varDecl(hasType(recordDecl(isExpandedFromMacro(
                  std::string(OpenBSDListDeclarationMacroName)))))
          .bind("root");
  Finder.addMatcher(Matcher, &Callback);
  Finder.matchAST(Ctx);
  return Callback.Matches;
}

/* Tries to construct and print printf() calls for printing all the fields of
 * the pointer variable with the name `RecordDeclVarName` and base type
 * `RecordDecl`. Each printed printf() call is prefixed with `indent` number of
 * spaces. For instance, if we have the following code:
 *
 * ```c
 * struct entry {
 *   int a;
 *   char *b;
 *   SLIST_ENTRY(entry) entries;
 * };
 * struct entry *np;
 * ```
 *
 * Then calling this method with indent=4, RecordDeclVarName="np", and
 * RecordDecl = <the declaration for the `entry` type>, should print:
 *
 * ```c
 *     printf("%d", np->a);
 *     printf("%s", np->b);
 * ```
 */
void PrintPrintersForRecordDeclFields(unsigned indent,
                                      std::string RecordDeclVarName,
                                      const clang::RecordDecl *RecordDecl) {

  for (const auto FieldDecl : RecordDecl->fields()) {
    std::string FormatSpecifier;
    std::string PrintfArgument =
        RecordDeclVarName + "->" + FieldDecl->getNameAsString();
    auto Type = FieldDecl->getType().getTypePtr();
    if (Type->isPointerType() && Type->getPointeeType()->isCharType()) {
      FormatSpecifier = "%s";
    }
    /* If we didn't already determine a format specifier for this type, try to
     * do so now. */
    if (Type->isUnsignedIntegerType()) {
      FormatSpecifier = "%u";
    } else if (Type->isSignedIntegerType()) {
      FormatSpecifier = "%d";
    }
    /* If we inferred a type for this field, print out its printer. */
    if (!FormatSpecifier.empty()) {
      llvm::outs() << std::format("{}printf(\"{}\", {});\n",
                                  std::string(indent, ' '), FormatSpecifier,
                                  PrintfArgument);
    }
  }
}

/* Prints the list iterator function for a declaration declared using the
 * SLIST_HEAD() macro. */
void PrintListIteratorForSLIST_HEADDecl(clang::ASTContext &Ctx,
                                        OpenBSDQueueMacroDecl SLIST_HEADDecl) {
  auto DeclName = SLIST_HEADDecl.VarDecl->getNameAsString();
  auto RecordDecl = SLIST_HEADDecl.RecordDecl;
  const clang::FieldDecl *slh_firstFieldRecordDeclSLIST_ENTRYField;
  auto slh_firstFieldDecl = *RecordDecl->field_begin();
  auto slh_firstFieldRecordDecl =
      slh_firstFieldDecl->getType()->getPointeeType()->getAsRecordDecl();

  /* Find the field in the entry declaration that was expanded from
   * SLIST_ENTRY(). */
  using namespace clang::ast_matchers;
  MatchFinder Finder;
  FieldDeclarationMatcherCallback FDMC;
  DeclarationMatcher SLIST_ENTRYMatcher = recordDecl(has(
      fieldDecl(
          hasType(recordDecl(isExpandedFromMacro(std::string("SLIST_ENTRY")))))
          .bind("root")));
  Finder.addMatcher(SLIST_ENTRYMatcher, &FDMC);
  Finder.match(*slh_firstFieldRecordDecl, Ctx);
  /* SLIST_ENTRY() should be called exactly once in the declaration of list's
   * entry type. */
  assert(1 == FDMC.Matches.size());
  slh_firstFieldRecordDeclSLIST_ENTRYField = FDMC.Matches.front();

  // NOTE(Brent): We assume that this is a decl of a struct.
  llvm::outs() << std::format(
      "{{\n    struct {} * __openbsd_list_iterator;\n    "
      "SLIST_FOREACH(__openbsd_list_iterator, &{}, {}) {{\n",
      slh_firstFieldRecordDecl->getNameAsString(), DeclName,
      slh_firstFieldRecordDeclSLIST_ENTRYField->getNameAsString());
  PrintPrintersForRecordDeclFields(8u, "__openbsd_list_iterator",
                                   slh_firstFieldRecordDecl);
  llvm::outs() << "    }\n}\n";
}

/* We only define this constructor because Clang requires it. */
ASTConsumer::ASTConsumer(clang::CompilerInstance &CI) { (void)CI; }

/* This is the method we have to override to tell Clang what do when we run our
plugin. */
void ASTConsumer::HandleTranslationUnit(clang::ASTContext &Ctx) {
  auto first = true;
  /* Run the printer on every kind of OpenBSD list macro. */
  for (const auto &OpenBSDQueueMacroDeclName : OpenBSDQueueMacroDeclNames) {
    auto Matches = FindOpenBSDQueueMacroDecls(Ctx, OpenBSDQueueMacroDeclName);
    for (auto Match : Matches) {
      /* Print a banner to separate different lists.
       *
       * TODO(Brent): Change the output format to something easily
       * machine-readable like JSON.
       */
      if (!first) {
        llvm::outs() << std::string(80, '#') << '\n';
      }
      /* TODO(Brent): Add printers for the other declaration macros. */
      if ("SLIST_HEAD" == Match.OpenBSDListDeclarationMacroName) {
        PrintListIteratorForSLIST_HEADDecl(Ctx, Match);
      }
      first = false;
    }
  }
}
} // namespace openbsd_list_macro_printer