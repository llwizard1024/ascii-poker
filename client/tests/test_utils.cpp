#include <catch2/catch_amalgamated.hpp>
#include "utils/helper.h"

TEST_CASE("Example test int", "sum") {
    int result = utils::sum(10, 20);
    REQUIRE(result == 30);
}

TEST_CASE("Example test double", "sum") {
    double result = utils::sum(5.5, 10.0);
    REQUIRE(result == 15.5);
}