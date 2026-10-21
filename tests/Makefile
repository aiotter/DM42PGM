.DEFAULT_GOAL := check

TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SOURCE_ROOT := $(abspath $(TEST_DIR)/..)
BUILD_DIR ?= /tmp/dm42pgm-tests
override BUILD_DIR := $(abspath $(BUILD_DIR))
SANITIZE ?= 0

TEST_FLAGS := $(CPPFLAGS) $(CXXFLAGS) -std=c++11 -Wall -Wextra -UNDEBUG -DCUSTOM_TEST \
	-I$(BUILD_DIR) -I$(SOURCE_ROOT)/dm -I$(SOURCE_ROOT)/dmcp -I$(SOURCE_ROOT)/free42
ifeq ($(SANITIZE),1)
TEST_FLAGS += -fsanitize=address -fno-omit-frame-pointer
endif

TESTS := $(basename $(notdir $(wildcard $(TEST_DIR)/*_test.cc)))
BINARIES := $(addprefix $(BUILD_DIR)/,$(TESTS))
CHECK_TARGETS := $(addprefix check-,$(TESTS))
DMCP_UNDEF_HEADER := $(BUILD_DIR)/dmcp_test_undef.h

.PHONY: all check clean $(CHECK_TARGETS)
all: $(DMCP_UNDEF_HEADER) $(BINARIES)

$(BUILD_DIR):
	mkdir -p "$@"

$(DMCP_UNDEF_HEADER): $(SOURCE_ROOT)/dmcp/lft_ifc.h $(TEST_DIR)/Makefile | $(BUILD_DIR)
	awk '$$1 == "#define" && $$2 != "LIBRARY_FN_BASE" && /LIBRARY_FN_BASE/ { print "#undef " $$2 }' "$<" > "$@"

$(BUILD_DIR)/%: $(TEST_DIR)/%.cc $(DMCP_UNDEF_HEADER) | $(BUILD_DIR)
	$(CXX) $(TEST_FLAGS) -MMD -MP -MF "$@.d" "$<" $(LDFLAGS) -o "$@"

# Give each suite its own target so make -j can run suites concurrently.
check: $(CHECK_TARGETS)

$(CHECK_TARGETS): check-%: $(BUILD_DIR)/%
	@echo "RUN $*"
	@"$<"

clean:
	rm -f $(BINARIES) $(addsuffix .d,$(BINARIES)) $(DMCP_UNDEF_HEADER)

-include $(addsuffix .d,$(BINARIES))
