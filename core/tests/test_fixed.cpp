#include "bo/core/fixed.h"
#include "doctest/doctest.h"

using namespace bo::core;

TEST_CASE("fixed point basic arithmetic") {
    CHECK((Fixed::from_int(816) / Fixed::from_int(15)).to_int() == 54);  // tiles per month, truncated
    CHECK(Fixed::from_int(3) / Fixed::from_int(2) == Fixed::ratio(3, 2));
    CHECK((Fixed::ratio(1, 2) * Fixed::from_int(10)).to_int() == 5);
    CHECK(Fixed::from_int(-3).to_int() == -3);
    CHECK(Fixed::ratio(1, 4) < Fixed::ratio(1, 2));
}
