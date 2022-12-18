#include <iostream>

#include "gtest/gtest.h"

#include "libimp/error.h"
#include "libimp/result.h"
#include "libimp/fmt.h"

namespace {

class custom_error_category : public imp::error_category {
public:
  std::string message(imp::result_code r) const {
    return !r ? "success" : "failure";
  }
};

} // namespace

TEST(error, error_code) {
  imp::error_code ecode;
  EXPECT_FALSE(ecode);
  std::cout << ecode.message() << "\n";
  EXPECT_EQ(ecode.message(), "[0, \"success\"]");

  custom_error_category cec;
  ecode = {123, cec};
  EXPECT_TRUE(ecode);
  std::cout << ecode.message() << "\n";
  EXPECT_EQ(ecode.message(), cec.message(123));
}