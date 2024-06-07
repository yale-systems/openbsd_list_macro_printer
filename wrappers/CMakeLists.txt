file(
  GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/bin/$<CONFIGURATION>/openbsd_list_macro_printer
  CONTENT "${CMAKE_C_COMPILER} -fplugin=$<TARGET_FILE:openbsd_list_macro_printer> -fsyntax-only \"$@\""
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
