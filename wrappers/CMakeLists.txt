file(
  GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/bin/$<CONFIGURATION>/openbsd_list_macro_printer
  INPUT openbsd_list_macro_printer.in
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
