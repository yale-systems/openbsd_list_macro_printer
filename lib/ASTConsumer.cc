#include "openbsd_list_macro_printer/ASTConsumer.hh"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include <array>
#include <cassert>
#include <format>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>

namespace openbsd_list_macro_printer {

/* Names of the OpenBSD list macros.
 *
 * TODO(Brent): Use an enum to represent these names instead.  */
static const std::array<std::string, 6> OpenBSDQueueMacroDeclNames = {
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
      OpenBSDQueueMacroDecl MacroDecl{OpenBSDListDeclarationMacroName,
                                          RecordDecl, VarDecl};
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
void PrintPrintersForRecordDeclFields(unsigned indent, std::string RecordDeclVarName,
                                      const clang::RecordDecl *RecordDecl, std::ofstream &file) {
  for (const auto FieldDecl : RecordDecl->fields()) {
    std::string FormatSpecifier;
    std::string PrintfArgument =
        RecordDeclVarName + "->" + FieldDecl->getNameAsString();
    auto Type = FieldDecl->getType().getTypePtr();
    if (Type->isPointerType() && Type->getPointeeType()->isCharType()) {
      FormatSpecifier = "%s";
    }
    if (Type->isUnsignedIntegerType()) {
      FormatSpecifier = "%u";
    } else if (Type->isSignedIntegerType()) {
      FormatSpecifier = "%d";
    }

    if (!FormatSpecifier.empty()) {
      file << std::string(indent, ' ') << std::format("printf(\"{}\", {});\n",
                                  FormatSpecifier, PrintfArgument);
    }
  }
}

void PrintListIteratorForLIST_HEADDecl(clang::ASTContext &Ctx,
                                       OpenBSDQueueMacroDecl LIST_HEADDecl,
                                       const std::string& entryName) {
  auto DeclName = LIST_HEADDecl.VarDecl->getNameAsString();
  auto RecordDecl = LIST_HEADDecl.RecordDecl;
  const clang::FieldDecl *lh_firstFieldRecordDeclLIST_ENTRYField;
  auto lh_firstFieldDecl = *RecordDecl->field_begin();
  auto lh_firstFieldRecordDecl =
      lh_firstFieldDecl->getType()->getPointeeType()->getAsRecordDecl();

  // Open a file based on the record name
  std::ofstream file(std::string{"/usr/src/table_src/"} + DeclName + "_tbl.c");

  if (!file.is_open()) {
    llvm::errs() << "Error opening file: " << DeclName + "_tbl.c" << "\n";
    return;
  }

  using namespace clang::ast_matchers;
  MatchFinder Finder;
  FieldDeclarationMatcherCallback FDMC;
  DeclarationMatcher LIST_ENTRYMatcher = recordDecl(has(
      fieldDecl(
          hasType(recordDecl(isExpandedFromMacro(std::string(entryName)))))
          .bind("root")));
  Finder.addMatcher(LIST_ENTRYMatcher, &FDMC);
  Finder.match(*lh_firstFieldRecordDecl, Ctx);
  assert(1 == FDMC.Matches.size());
  lh_firstFieldRecordDeclLIST_ENTRYField = FDMC.Matches.front();

  llvm::outs() << "Processing file " << DeclName << '\n';

  file << std::format(
      "{{\n    struct {} * __openbsd_list_iterator;\n    "
      "LIST_FOREACH(__openbsd_list_iterator, &{}, {}) {{\n",
      lh_firstFieldRecordDecl->getNameAsString(), DeclName,
      lh_firstFieldRecordDeclLIST_ENTRYField->getNameAsString());

  PrintPrintersForRecordDeclFields(8u, "__openbsd_list_iterator",
                                   lh_firstFieldRecordDecl, file);

  file << "    }\n}\n";

  file.close();
}

std::ofstream OpenFile(OpenBSDQueueMacroDecl LIST_HEADDecl) {
  auto DeclName = LIST_HEADDecl.VarDecl->getNameAsString();
  std::ofstream file(std::string{"/usr/src/table_src/"} + DeclName + "_tbl.c");

  if (!file.is_open()) {
    llvm::errs() << "Error opening file: " << DeclName + "_tbl.c" << "\n";
    return std::ofstream{};
  }

  return file; 
}

void GenerateIncludePaths(const clang::RecordDecl *recordDecl, std::ofstream& file) {
  const clang::FieldDecl *firstField = *recordDecl->field_begin();
  const clang::RecordDecl *elementTypeRecord =
      firstField->getType()->getPointeeType()->getAsRecordDecl();
  std::string elementTypeName = elementTypeRecord->getNameAsString(); // proc

  file << "#include <sys/types.h>\n"
       << "#include <sys/systm.h>\n"
       << "#include <sys/libkern.h>\n"
       << "#include <sys/malloc.h>\n"
       << "#include <sys/" << elementTypeName << ".h>\n"
       << "#include <sys/signal.h>\n"
       << "#include <sys/tty.h>\n"
       << "\n"
       << "#include <dbsc/value.h>"
       << "\n"
       << "#include \"osdb.h\"\n"
       << "#include \"osdb_mod.h\"\n"
       << "#include \"sqlite3ext.h\"\n"
       << "#include \"vtab_common.h\"\n"
       << "#include \"vtab_" << elementTypeName << ".h\"\n"
      << "\n"
      << "SQLITE_EXTENSION_INIT1\n\n";
}

void GenerateColumnCopyFunctionForStruct(clang::ASTContext &Ctx,
                                         OpenBSDQueueMacroDecl LIST_HEADDecl,
                                         const std::string& entryName,
                                         std::ofstream& file) {
  auto DeclName = LIST_HEADDecl.VarDecl->getNameAsString();
  auto RecordDecl = LIST_HEADDecl.RecordDecl;
  // const clang::FieldDecl *lh_firstFieldRecordDeclLIST_ENTRYField;
  auto lh_firstFieldDecl = *RecordDecl->field_begin();
  auto lh_firstFieldRecordDecl =
      lh_firstFieldDecl->getType()->getPointeeType()->getAsRecordDecl();

  const clang::FieldDecl *firstField = *RecordDecl->field_begin();
  const clang::RecordDecl *elementTypeRecord =
      firstField->getType()->getPointeeType()->getAsRecordDecl();
  std::string elementTypeName = elementTypeRecord->getNameAsString(); // proc

  using namespace clang::ast_matchers;
  MatchFinder Finder;
  FieldDeclarationMatcherCallback FDMC;
  DeclarationMatcher LIST_ENTRYMatcher = recordDecl(has(
      fieldDecl(
          hasType(recordDecl(isExpandedFromMacro(std::string(entryName)))))
          .bind("root")));
  Finder.addMatcher(LIST_ENTRYMatcher, &FDMC);
  Finder.match(*lh_firstFieldRecordDecl, Ctx);
  assert(1 == FDMC.Matches.size());
  // lh_firstFieldRecordDeclLIST_ENTRYField = FDMC.Matches.front();

  llvm::outs() << "Processing file " << DeclName << '\n';

  // Generate enum for the columns based on struct fields
  file << "enum col {\n";
  unsigned int colIndex = 0;
  for (const auto *field : lh_firstFieldRecordDecl->fields()) {
    file << "    VT_" << DeclName << "_" << field->getNameAsString()
         << " = " << colIndex++ << ",\n";
  }
  file << "    VT_" << DeclName << "_NUM_COLUMNS\n};\n\n";

  // Start generating the function that will copy the struct fields to columns
  file << "static int\n";
  file << "copy_columns(struct " << elementTypeName << " *curEntry, struct dbsc_value **columns, "
       << "struct timespec *when, MD5_CTX *context) {\n\n";

  // Iterate through the fields of the struct and generate code for assigning values
  for (const auto *field : lh_firstFieldRecordDecl->fields()) {
    // Check the type of the field to determine the correct function to use
    auto fieldType = field->getType().getTypePtr(); 
    if (fieldType->isEnumeralType()) {
      file << "    columns[VT_" << DeclName << "_" << field->getNameAsString() << "] = ";
      file << "new_dbsc_int64(static_cast<int64_t>(curEntry->" 
             << field->getNameAsString() << "), context); // TODO: need better enum representation \n";
    } else if (fieldType->isIntegerType()) {
      file << "    columns[VT_" << DeclName << "_" << field->getNameAsString() << "] = ";
      file << "new_dbsc_int64(curEntry->" << field->getNameAsString() << ", context);\n";
    } else if (fieldType->isPointerType() && fieldType->getPointeeType()->isCharType()) {
      file << "    columns[VT_" << DeclName << "_" << field->getNameAsString() << "] = ";
      // Assuming string field is a char pointer
      file << "new_dbsc_text(curEntry->" << field->getNameAsString() << ", "
           << "strlen(curEntry->" << field->getNameAsString() << ") + 1, context);\n";
    } else {
      file << "//    columns[VT_" << DeclName << "_" << field->getNameAsString() << "] = ";
      file << " TODO: Handle other types\n";
    }
  }

  file << "\n    return 0;\n";
  file << "}\n";

}

void GenerateSerialize(clang::ASTContext &Ctx,
                       const clang::RecordDecl *recordDecl,
                       const clang::VarDecl *varDecl,
                       std::ofstream &file) {
  std::string varName = varDecl->getNameAsString();  // Get the allproc variable name
  const clang::FieldDecl *firstField = *recordDecl->field_begin();
  const clang::RecordDecl *elementTypeRecord =
      firstField->getType()->getPointeeType()->getAsRecordDecl();
  std::string elementTypeName = elementTypeRecord->getNameAsString(); // proc

  const std::string tableName = std::string("all_") + elementTypeName + "s";

  file << "void vtab_" << elementTypeName << "_serialize(sqlite3 *real_db, struct timespec when) {\n";
  file << "    struct " << elementTypeName << " *entry = LIST_FIRST(&" << varName << ");\n\n";

  // === Create Table ===
  file << "    const char *create_stmt =\n";
  file << "        \"CREATE TABLE " << tableName << " (";

  bool first = true;
  std::vector<std::string> handledFields;
  for (const auto *field : elementTypeRecord->fields()) {
    auto fieldType = field->getType().getTypePtr();
    if (!(fieldType->isEnumeralType() || fieldType->isIntegerType() ||
          (fieldType->isPointerType() && fieldType->getPointeeType()->isCharType())))
      continue;

    if (!first) file << ", ";
    first = false;

    std::string fieldName = field->getNameAsString();
    handledFields.push_back(fieldName);

    file << fieldName << " ";
    if (fieldType->isEnumeralType() || fieldType->isIntegerType()) {
      file << "INTEGER";
    } else {
      file << "TEXT";
    }
  }
  file << ")\";\n";
  file << "    char *errMsg = NULL;\n";
  file << "    sqlite3_exec(real_db, create_stmt, NULL, NULL, &errMsg);\n\n";

  // === Insert Statement ===
  file << "    const char *insert_stmt = \"INSERT INTO " << tableName << " VALUES (";
  for (size_t i = 0; i < handledFields.size(); ++i) {
    if (i > 0) file << ", ";
    file << "?";
  }
  file << ")\";\n";
  file << "    sqlite3_stmt *stmt = NULL;\n";
  file << "    sqlite3_prepare_v2(real_db, insert_stmt, -1, &stmt, NULL);\n\n";

  // === While loop over linked list ===
  file << "    while (entry) {\n";
  file << "        osdb_value **columns = new_osdb_columns(VT_" << varName << "_NUM_COLUMNS);\n";

  file << "        int bindIndex = 1;\n";
  for (const auto &fieldName : handledFields) {
    file << "        {\n";
    file << "            osdb_value *val = columns[VT" << varName << "_" << fieldName << "];\n";
    file << "            switch (val->type) {\n";
    file << "                case INT64:\n";
    file << "                    sqlite3_bind_int64(stmt, bindIndex++, val->int64_value);\n";
    file << "                    break;\n";
    file << "                case TEXT:\n";
    file << "                    sqlite3_bind_text(stmt, bindIndex++, val->text_value, -1, SQLITE_STATIC);\n";
    file << "                    break;\n";
    file << "                default:\n";
    file << "                    sqlite3_bind_null(stmt, bindIndex++);\n";
    file << "                    break;\n";
    file << "            }\n";
    file << "        }\n";
  }

  file << "        sqlite3_step(stmt);\n";
  file << "        sqlite3_reset(stmt);\n";
  file << "        entry = LIST_NEXT(entry,  p_list);\n";
  file << "    }\n\n";

  file << "    sqlite3_finalize(stmt);\n";
  file << "}\n\n";
}

void GenerateVtabModule(std::ofstream& file, const std::string& recordName) {
    file << "/*\n** This following structure defines all the methods for the\n"
         << "** virtual table.\n*/\n";
    file << "static sqlite3_module " << recordName << "vtabModule = {\n"
         << "    /* iVersion    */ 0,\n"
         << "    /* xCreate     */ commonCreate,\n"
         << "    /* xConnect    */ commonConnect,\n"
         << "    /* xBestIndex  */ " << recordName << "vtabBestIndex,\n"
         << "    /* xDisconnect */ commonDisconnect,\n"
         << "    /* xDestroy    */ commonDisconnect,\n"
         << "    /* xOpen       */ commonOpen,\n"
         << "    /* xClose      */ commonClose,\n"
         << "    /* xFilter     */ commonFilter,\n"
         << "    /* xNext       */ commonNext,\n"
         << "    /* xEof        */ commonEof,\n"
         << "    /* xColumn     */ commonColumn,\n"
         << "    /* xRowid      */ " << recordName << "vtabRowid,\n"
         << "    /* xUpdate     */ " << recordName << "vtabUpdate,\n"
         << "    /* xBegin      */ 0,\n"
         << "    /* xSync       */ 0,\n"
         << "    /* xCommit     */ 0,\n"
         << "    /* xRollback   */ 0,\n"
         << "    /* xFindMethod */ 0,\n"
         << "    /* xRename     */ 0,\n"
         << "    /* xSavepoint  */ 0,\n"
         << "    /* xRelease    */ 0,\n"
         << "    /* xRollbackTo */ 0,\n"
         << "    /* xShadowName */ 0,\n"
         << "    /* xIntegrity  */ 0\n"
         << "};\n\n";

    file << "int\n"
         << "sqlite3_" << recordName << "vtab_init(sqlite3 *db, char **pzErrMsg,\n"
         << "    const sqlite3_api_routines *pApi, void *pAux)\n"
         << "{\n"
         << "    SQLITE_EXTENSION_INIT2(pApi);\n"
         << "    return sqlite3_create_module(db,\n"
         << "        vtable_type_to_name(((osdb_vtab *)pAux)->type), &" << recordName
         << "vtabModule,\n"
         << "        pAux);\n"
         << "}\n";
}

void GenerateVtabProcFunctions(clang::ASTContext &Ctx,
                               const clang::RecordDecl *recordDecl,
                               const clang::VarDecl *varDecl,
                               std::ofstream& file) {
  std::string varName = varDecl->getNameAsString();  // Get the allproc variable name
  const clang::FieldDecl *firstField = *recordDecl->field_begin();
  const clang::RecordDecl *elementTypeRecord =
      firstField->getType()->getPointeeType()->getAsRecordDecl();
  std::string elementTypeName = elementTypeRecord->getNameAsString(); // proc

  // Write the lock and unlock functions, replacing "proc" with the struct name
  file << "void\nvtab_" << elementTypeName << "_lock(void)\n{\n"
       << "    sx_slock(&" << varName << "_lock);\n"
       << "}\n\n";

  file << "void\nvtab_" << elementTypeName << "_unlock(void)\n{\n"
       << "    sx_sunlock(&" << varName << "_lock);\n"
       << "}\n\n";

  // Write the snapshot function, replacing "proc" with the struct name
  file << "void\nvtab_" << elementTypeName << "_snapshot(sqlite3_vtab *pVtab, struct timespec when)\n"
       << "{\n"
       << "    struct " << elementTypeName << " *prc = LIST_FIRST(&" << varName << ");\n\n"
       << "    osdb_snap *snap = malloc(sizeof(struct osdb_snap), M_SQLITE, M_WAITOK);\n"
       << "    snap->when = when;\n"
       << "    snap->snap_table = new_osdb_table(VT_" << varName << "_NUM_COLUMNS" << ");\n"
       << "    MD5Init(&snap->context);\n\n"
       << "    while (prc) {\n"
       << "        struct dbsc_value **columns = new_osdb_columns(VT_" << varName << "_NUM_COLUMNS" << ");\n"
       << "        if (!columns) {\n"
       << "            return;\n"
       << "        }\n"
       << "        copy_columns(prc, columns, &snap->when, &snap->context);\n"
       << "        osdb_table_push(snap->snap_table, columns);\n"
       << "        prc = LIST_NEXT(prc, p_list);\n"
       << "    }\n\n"
       << "    MD5Final(snap->digest, &snap->context);\n"
       << "#ifdef DEBUG\n"
       << "    printf(\"" << elementTypeName << " digest: \");\n"
       << "    for (size_t i = 0; i < 16; i++) {\n"
       << "        printf(\"%02hhx\", snap->digest[i]);\n"
       << "    }\n"
       << "    printf(\"\\n\");\n"
       << "#endif\n"
       << "    osdb_snapshot_rotate((struct osdb_vtab *)pVtab, snap);\n"
       << "}\n\n";

  file << "static int\n" << elementTypeName << "vtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid)\n"
       << "{\n"
       << "    common_cursor *pCur = (common_cursor *)cur;\n"
       << "    struct dbsc_value *pid_value = pCur->row->columns[VT_" << varName << "_p_pid];\n"
       << "    *pRowid = pid_value->int64_value;\n"
       << "    printf(\"" << elementTypeName << "_rowid was called, returning %lld\\n\", *pRowid);\n"
       << "    return SQLITE_OK;\n"
       << "}\n\n";

  // Write the BestIndex function, replacing "proc" with the struct name
  file << "static int\n" << elementTypeName << "vtabBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo)\n"
       << "{\n"
       << "    pIdxInfo->estimatedCost = (double)10;\n"
       << "    pIdxInfo->estimatedRows = 10;\n"
       << "    return SQLITE_OK;\n"
       << "}\n\n";

  // Write the Update function, replacing "proc" with the struct name
  file << "static int\n" << elementTypeName << "vtabUpdate(sqlite3_vtab *pVTab, int argc, sqlite3_value **argv, sqlite_int64 *pRowid)\n"
       << "{\n"
       << "    struct timespec when;\n"
       << "    nanotime(&when);\n"
       << "    vtab_" << elementTypeName << "_snapshot(pVTab, when);\n"
       << "    if (osdb_snapshot_compare((struct osdb_vtab *)pVTab) <= 0) {\n"
       << "#ifdef DEBUG\n"
       << "        printf(\"" << elementTypeName << " digest mismatch: UPDATE failed\\n\");\n"
       << "#endif\n"
       << "        return SQLITE_ABORT;\n"
       << "    }\n\n"
       << "    if ((argc == 1) && (argv[0] != NULL)) {\n"
       << "        int p_pid = sqlite3_value_int64(argv[0]);\n"
       << "#ifdef DEBUG\n"
       << "        printf(\"argc %d argv[0] %d, rowID, %lld\\n\", argc, p_pid, *pRowid);\n"
       << "        printf(\"Killing PID %d.\\n\", p_pid);\n"
       << "#endif\n"
       << "        kern_kill(curthread, p_pid, SIGKILL);\n"
       << "        return SQLITE_OK;\n"
       << "    }\n\n"
       << "    if ((argc > 1) && (sqlite3_value_type(argv[0]) != SQLITE_NULL)) {\n"
       << "        int core = sqlite3_value_int64(argv[2]);\n"
       << "        int pid = sqlite3_value_int64(argv[5]);\n"
       << "        cpuset_t *mask = malloc(sizeof(cpuset_t), M_TEMP, M_WAITOK | M_ZERO);\n"
       << "#ifdef DEBUG\n"
       << "        int row = sqlite3_value_int64(argv[0]);\n"
       << "        printf(\"UPDATE row %d core %d pid %d\\n\", row, core, pid);\n"
       << "#endif\n"
       << "        CPU_SET(core, mask);\n"
       << "        cpuset_setproc(pid, NULL, mask, NULL, false);\n"
       << "        free(mask, M_TEMP);\n"
       << "    }\n\n"
       << "    return SQLITE_OK;\n"
       << "}\n\n";

  // Call the function to generate the sqlite3_module with the correct struct name
  GenerateVtabModule(file, elementTypeName);
}

void LogStructRelationships(const clang::RecordDecl *RecordDecl) {
  std::ofstream file("/usr/src/table_src/struct_relationships.txt", std::ios::app);

  if (!file.is_open()) {
    llvm::errs() << "Error opening struct_relationships.txt\n";
    return;
  }

  std::string currentStructName{}; 

  const clang::RecordDecl *matchedStruct = nullptr;

  // Find the first struct type pointed to by any field
  for (const auto *FieldDecl : RecordDecl->fields()) {
    const auto *fieldType = FieldDecl->getType().getTypePtr();

    if (fieldType->isPointerType()) {
      const auto *pointeeType = fieldType->getPointeeType()->getAsRecordDecl();
      if (pointeeType && pointeeType->isStruct()) {
        matchedStruct = pointeeType;
        currentStructName = pointeeType->getNameAsString(); 
        break; // Exit once the first matching struct is found
      }
    }
  }

  if (!matchedStruct) {
    file.close();
    return; // No matching struct found, nothing to log
  }

  // Iterate through the fields of the matched struct
  std::vector<std::string> relatedStructs;
  for (const auto *FieldDecl : matchedStruct->fields()) {
    const auto *fieldType = FieldDecl->getType().getTypePtr();

    if (fieldType->isPointerType()) {
      const auto *pointeeType = fieldType->getPointeeType()->getAsRecordDecl();
      if (pointeeType && pointeeType->isStruct()) {
        relatedStructs.push_back(pointeeType->getNameAsString());
      }
    }
  }

  // Log the relationships to the file
  if (!relatedStructs.empty()) {
    file << currentStructName << ":";
    for (const auto &relatedStruct : relatedStructs) {
      file << " " << relatedStruct;
    }
    file << "\n";
  }

  file.close();
}

/* We only define this constructor because Clang requires it. */
ASTConsumer::ASTConsumer(clang::CompilerInstance &CI) { (void)CI; }

/* This is the method we have to override to tell Clang what do when we run our
plugin. */
void ASTConsumer::HandleTranslationUnit(clang::ASTContext &Ctx) {
  std::unordered_set<const clang::RecordDecl*> ProcessedRecords;
  auto first = true;
  /* Run the printer on every kind of OpenBSD list macro. */
  for (const auto &OpenBSDQueueMacroDeclName : OpenBSDQueueMacroDeclNames) {
    auto Matches = FindOpenBSDQueueMacroDecls(Ctx, OpenBSDQueueMacroDeclName);
    for (auto Match : Matches) {
      if(ProcessedRecords.find(Match.RecordDecl) != ProcessedRecords.end()) {
        llvm::outs() << "skipped declaration" << '\n'; 
        continue;
      }
      ProcessedRecords.insert(Match.RecordDecl);
      LogStructRelationships(Match.RecordDecl);
      /* Print a banner to separate different lists.
       *
       * TODO(Brent): Change the output format to something easily
       * machine-readable like JSON.
       */
      if (!first) {
        llvm::outs() << std::string(80, '#') << '\n';
      }
      std::ofstream openFile = OpenFile(Match);
      if(!openFile.is_open()) {
        continue; 
      }
      GenerateIncludePaths(Match.RecordDecl, openFile);
      /* TODO(Brent): Add printers for the other declaration macros. */
      if ("SLIST_HEAD" == Match.OpenBSDListDeclarationMacroName) {
        GenerateColumnCopyFunctionForStruct(Ctx, Match, "SLIST_ENTRY", openFile);
      } else if("LIST_HEAD" == Match.OpenBSDListDeclarationMacroName) {
        GenerateColumnCopyFunctionForStruct(Ctx, Match, "LIST_ENTRY", openFile);
      } else if("TAILQ_HEAD" == Match.OpenBSDListDeclarationMacroName) {
        llvm::outs() << "TAILQ HEAD DECLARATION" << '\n'; 
        GenerateColumnCopyFunctionForStruct(Ctx, Match, "TAILQ_ENTRY", openFile);
      } else if("STAILQ_HEAD" == Match.OpenBSDListDeclarationMacroName) {
        llvm::outs() << "STAILQ HEAD DECLARATION" << '\n'; 
        GenerateColumnCopyFunctionForStruct(Ctx, Match, "STAILQ_ENTRY", openFile);
      } 
      GenerateVtabProcFunctions(Ctx, Match.RecordDecl, Match.VarDecl, openFile);
      GenerateSerialize(Ctx, Match.RecordDecl, Match.VarDecl, openFile);
      openFile.close();
      first = false;
    }
  }
}
} // namespace openbsd_list_macro_printer