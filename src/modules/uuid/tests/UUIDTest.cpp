/**
 * @file
 */

#include <gtest/gtest.h>
#include "uuid/UUIDUtil.h"

namespace uuid {

TEST(UUIDTest, test) {
	ASSERT_TRUE(uuid::generateUUID() != "") << "No uuid implementation was found";
}

}
