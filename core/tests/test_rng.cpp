#include "bo/core/rng.h"
#include "doctest/doctest.h"

using namespace bo::core;

TEST_CASE("same seed gives the same sequence") {
    RngSet a(42);
    RngSet b(42);
    for (int i = 0; i < 1000; ++i) {
        CHECK(a.stream(RngStream::Migration).next_u32() == b.stream(RngStream::Migration).next_u32());
    }
}

TEST_CASE("streams are independent: drawing from one does not shift another") {
    RngSet a(7);
    RngSet b(7);
    for (int i = 0; i < 100; ++i) {
        a.stream(RngStream::Hazards).next_u32();
    }
    for (int i = 0; i < 100; ++i) {
        CHECK(a.stream(RngStream::Gods).next_u32() == b.stream(RngStream::Gods).next_u32());
    }
}

TEST_CASE("different streams produce different sequences") {
    RngSet a(7);
    int equal = 0;
    for (int i = 0; i < 100; ++i) {
        equal += a.stream(RngStream::Crime).next_u32() == a.stream(RngStream::Disease).next_u32() ? 1 : 0;
    }
    CHECK(equal < 3);
}

TEST_CASE("next_below stays in range") {
    Pcg32 p(1, 1);
    for (int i = 0; i < 10000; ++i) {
        CHECK(p.next_below(6) < 6);
    }
    CHECK(p.next_below(0) == 0);
}

TEST_CASE("PCG32 reference value is stable across platforms") {
    // Golden value: changes only if the generator changes (name the reason in the commit).
    Pcg32 p(42, 54);
    CHECK(p.next_u32() == 0xa15c02b7U);
}
