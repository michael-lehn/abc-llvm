#ifndef TEST_UNIT_CHECK_HPP
#define TEST_UNIT_CHECK_HPP

//
// Minimal support for unit tests:
//
//  CHECK(cond)                 cond must hold
//  CHECK_EQ(a, b)              a == b must hold (values are printed on failure)
//  CHECK_KNOWN_BUG(test, issue, cond)
//                              cond describes correct behavior that currently
//                              does not hold because of the known compiler bug
//                              'issue' (e.g. "#0005", see
//                              test/bugs_and_open_design_questions.md). 'test'
//                              is the stable test ID of the check (e.g.
//                              "T0170"). Reported as XFAIL; if cond holds it is
//                              reported as XPASS and the test fails, so that
//                              the check gets turned into a CHECK once the bug
//                              is fixed.
//
// A unit test ends with 'return check::summary(argv[0]);'
//

#include <iostream>

namespace check {

inline int numChecks;
inline int numFailed;
inline int numKnownBugs;
inline int numFixedBugs;

inline void
report(bool ok, const char *cond, const char *file, int line)
{
    ++numChecks;
    if (!ok) {
	++numFailed;
	std::cout << file << ":" << line << ": FAILED: " << cond << std::endl;
    }
}

template <typename A, typename B>
void
reportEq(const A &a, const B &b, const char *aStr, const char *bStr,
         const char *file, int line)
{
    ++numChecks;
    if (!(a == b)) {
	++numFailed;
	std::cout << file << ":" << line << ": FAILED: " << aStr
	          << " == " << bStr << "\n    left:  " << a
	          << "\n    right: " << b << std::endl;
    }
}

// Output lines start with "XFAIL <test> <issue>" or "XPASS <test> <issue>"
// (parsed by test/run-tests.sh)
inline void
reportKnownBug(const char *test, const char *issue, bool ok, const char *cond,
               const char *file, int line)
{
    ++numChecks;
    if (ok) {
	++numFixedBugs;
    } else {
	++numKnownBugs;
    }
    std::cout << (ok ? "XPASS " : "XFAIL ") << test << " " << issue << " "
              << file << ":" << line << ": " << cond << std::endl;
}

inline int
summary(const char *name)
{
    std::cout << name << ": " << numChecks << " checks, " << numFailed
              << " failed, " << numKnownBugs << " known bugs, "
              << numFixedBugs << " unexpectedly passed" << std::endl;
    return numFailed || numFixedBugs ? 1 : 0;
}

} // namespace check

#define CHECK(cond) check::report((cond), #cond, __FILE__, __LINE__)

#define CHECK_EQ(a, b) check::reportEq((a), (b), #a, #b, __FILE__, __LINE__)

#define CHECK_KNOWN_BUG(test, issue, cond)                                     \
    check::reportKnownBug(test, issue, (cond), #cond, __FILE__, __LINE__)

#endif // TEST_UNIT_CHECK_HPP
