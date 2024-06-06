add_library(openbsd_list_macro_printer SHARED ASTConsumer.cc PluginASTAction.cc)

target_compile_features(openbsd_list_macro_printer PUBLIC cxx_std_20)
target_compile_options(
  openbsd_list_macro_printer
  PUBLIC -Wall -Wextra
         # -Werror
         -Wpedantic -Wno-unused-parameter -fno-exceptions -fno-rtti)
