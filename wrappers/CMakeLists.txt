

if(APPLE)
  set(WRAPPER openbsd_list_macro_printer_mac.in)
else()
  set(WRAPPER openbsd_list_macro_printer.in)
endif()

file(
  GENERATE
  OUTPUT ${PROJECT_BINARY_DIR}/bin/$<CONFIGURATION>/openbsd_list_macro_printer
  INPUT ${WRAPPER}
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
