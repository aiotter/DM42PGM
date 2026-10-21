#ifndef DMCP_TEST_SUPPORT_H
#define DMCP_TEST_SUPPORT_H

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif
extern "C" {
#include "dmcp.h"
}

// Keep SDK declarations, but remove ROM function-address mappings so each test
// supplies the stubs it needs instead of calling the firmware's fixed addresses.
#include "dmcp_test_undef.h"

// Each test and its included implementation use this host-side SDK service state.
static sys_sdb_t test_sdb = {};
#undef sdb
#define sdb test_sdb

#endif
