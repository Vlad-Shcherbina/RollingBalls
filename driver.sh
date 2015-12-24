ulimit -c unlimited

TIME_FMT="
  user: %U s
  wall: %e s
  kernel: %S s
  peak mem: %M k
"

# ASAN_OPTIONS="disable_core=0:unmap_shadow_on_exit=1:abort_on_error=1" \
ASAN_SYMBOLIZER_PATH=$(which llvm-symbolizer-3.4) \
MSAN_SYMBOLIZER_PATH=$(which llvm-symbolizer-3.4) \
/usr/bin/time --format="$TIME_FMT" ./main "$@"

# to extract coredump:
#   gdb main core -batch -ex bt
