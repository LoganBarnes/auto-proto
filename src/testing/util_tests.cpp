#include "util/util.h"
#include "util/generic_guard.h"
#include <gtest/gtest.h>

namespace proj {
namespace test {

namespace {

class GenericGuardTests : public ::testing::Test {
protected:
    void SetUp() override {
        GenericGuardTests::push_called = false;
        GenericGuardTests::pop_called = false;
    }

    void in_scope_checks() {
        ASSERT_TRUE(GenericGuardTests::push_called);
        ASSERT_FALSE(GenericGuardTests::pop_called);
    }

    void out_of_scope_checks() {
        ASSERT_TRUE(GenericGuardTests::push_called);
        ASSERT_TRUE(GenericGuardTests::pop_called);
    }

    static bool push_called;
    static bool pop_called;

    static void void_push() { GenericGuardTests::push_called = true; }
    static void void_pop() { GenericGuardTests::pop_called = true; }

    static void float_push(float) { GenericGuardTests::push_called = true; }
    static void float_pop(float) { GenericGuardTests::pop_called = true; }

    static void string_push(const std::string&) { GenericGuardTests::push_called = true; }
    static void string_pop(const std::string&) { GenericGuardTests::pop_called = true; }

    static void int_and_string_push(int, const std::string&) { GenericGuardTests::push_called = true; }
    static void int_and_string_pop(int, const std::string&) { GenericGuardTests::pop_called = true; }
};

bool GenericGuardTests::push_called;
bool GenericGuardTests::pop_called;

} // namespace

TEST_F(GenericGuardTests, no_lambdas_void_push_void_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::void_push, &GenericGuardTests::void_pop);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_float_push_void_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::float_push, &GenericGuardTests::void_pop, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_void_push_float_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::void_push, &GenericGuardTests::float_pop, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_float_push_float_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::float_push, &GenericGuardTests::float_pop, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_string_push_void_pop) {
    {
        auto guard
            = util::make_guard(&GenericGuardTests::string_push, &GenericGuardTests::void_pop, "Will this compile?");
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_void_push_string_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::void_push,
                                      &GenericGuardTests::string_pop,
                                      "Of course it will compile!");
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_string_push_string_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::string_push,
                                      &GenericGuardTests::string_pop,
                                      std::string("This will too!"));
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_int_and_string_push_void_pop) {
    {
        auto guard
            = util::make_guard(&GenericGuardTests::int_and_string_push, &GenericGuardTests::void_pop, 3, "Multi-arg?");
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_void_push_int_and_string_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::void_push, &GenericGuardTests::int_and_string_pop, 7, "");
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, No_lambdas_int_and_string_push_int_and_string_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::int_and_string_push,
                                      &GenericGuardTests::int_and_string_pop,
                                      21,
                                      std::string());
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, lambda_push_void_pop) {
    {
        auto guard = util::make_guard([] { void_push(); }, &GenericGuardTests::void_pop);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, lambda_push_float_pop) {
    {
        auto guard = util::make_guard([] { float_push(0.f); }, &GenericGuardTests::float_pop, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, void_push_lambda_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::void_push, [] { float_pop(0.f); });
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, float_push_lambda_pop) {
    {
        auto guard = util::make_guard(&GenericGuardTests::float_push, [] { void_pop(); }, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, lambda_push_lambda_pop) {
    {
        auto guard = util::make_guard([] { float_push(0.f); }, [] { int_and_string_pop(73, "blarg"); });
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST_F(GenericGuardTests, float_lambda_push_float_lambda_pop) {
    {
        auto guard = util::make_guard([](float arg) { float_push(arg); }, [](float arg) { float_pop(arg); }, 0.f);
        in_scope_checks();
    }
    out_of_scope_checks();
}

TEST(UtilTests, to_upper_works) {
    EXPECT_EQ(util::to_upper(""), "");
    EXPECT_EQ(util::to_upper("the quick brown fox jumps over the lazy dog"),
              "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
    EXPECT_EQ(util::to_upper("BlaRgy bLarG!"), "BLARGY BLARG!");
}

TEST(UtilTests, to_lower_works) {
    EXPECT_EQ(util::to_lower(""), "");
    EXPECT_EQ(util::to_lower("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"),
              "the quick brown fox jumps over the lazy dog");
    EXPECT_EQ(util::to_lower("BlaRgy bLarG!"), "blargy blarg!");
}

} // namespace test
} // namespace proj
