# Running tests

Requires Make, Awk, and a C++11 compiler.

Files named `*_test.cc` in this directory are automatically built and run.
The default target builds and runs the tests.

The build generates `dmcp_test_undef.h` from the SDK's `dmcp/lft_ifc.h` to remove
all fixed-address function mappings. Shared test support redirects `sdb` to test
state; each test defines its required function stubs.

`CUSTOM_TEST` is defined only when building tests.

```sh
make -f tests/Makefile BUILD_DIR=/tmp/dm42pgm-tests
```

Set `CXX` to select the compiler. Set `SANITIZE=1` when building in a fresh
`BUILD_DIR` to enable AddressSanitizer.
