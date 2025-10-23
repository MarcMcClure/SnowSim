#pragma once

#include <string>
#include <unordered_map>

#include "catch_amalgamated.hpp"

namespace test_support {

namespace detail {

// Helper that trims trailing newline characters so reporter output stays
// compact and comparisons ignore platform-specific line endings.
inline void trim_trailing_newlines(std::string& text) {
    while (!text.empty()) {
        const char c = text.back();
        if (c == '\n' || c == '\r') {
            text.pop_back();
        } else {
            break;
        }
    }
}

} // namespace detail

// Listener that requests Catch2 to capture std::cerr/std::cout and lets tests
// assert that redirected stderr matches an expected warning exactly. Keeping
// this in one place means individual tests only need a single macro call.
struct WarningListener final : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    WarningListener(Catch::IConfig const* config)
        : Catch::EventListenerBase(config) {
        // Signal to Catch2 that we need stream redirection so stats.stdErr is populated.
        m_preferences.shouldRedirectStdOut = true;
    }

    void testCasePartialEnded(Catch::TestCaseStats const& stats,
                              std::uint64_t) override {
        const std::string testName = stats.testInfo ? stats.testInfo->name : "";
        auto it = s_expectations.find(testName);
        if (it == s_expectations.end()) {
            return;
        }

        Expectation expectation = it->second;
        s_expectations.erase(it);

        std::string actual = stats.stdErr;
        detail::trim_trailing_newlines(actual);

        auto* capture = Catch::getCurrentContext().getResultCapture();
        if (!capture) {
            return;
        }

        const std::string& expected = expectation.expected;
        const std::string expression =
            std::string("stderr warning == \"") + expected + '"';

        Catch::AssertionInfo info{
            Catch::StringRef("REQUIRE_WARNING"),
            expectation.callSite,
            Catch::StringRef(expression),
            Catch::ResultDisposition::ContinueOnFailure
        };

        const bool matched = (actual == expected);

        capture->notifyAssertionStarted(info);

        if (!matched) {
            Catch::IResultCapture::emplaceUnscopedMessage(
                Catch::MessageBuilder(info.macroName,
                                      info.lineInfo,
                                      Catch::ResultWas::Info)
                << "expected: \"" << expected << "\"\n  actual: \"" << actual << '"');
        }

        Catch::AssertionReaction reaction;
        capture->handleNonExpr(
            info,
            matched ? Catch::ResultWas::Ok
                    : Catch::ResultWas::ExpressionFailed,
            reaction);
    }

    // Store the next expected warning for a given test case name.
    static void expectWarning(std::string testName,
                              std::string expected,
                              Catch::SourceLineInfo callSite) {
        detail::trim_trailing_newlines(expected);
        s_expectations.erase(testName);
        s_expectations.emplace(std::move(testName),
                               Expectation{ std::move(expected), callSite });
    }

private:
    struct Expectation {
        std::string expected;
        Catch::SourceLineInfo callSite;

        Expectation(std::string expected_,
                    Catch::SourceLineInfo site):
            expected(std::move(expected_)),
            callSite(site) {}
    };

    static inline std::unordered_map<std::string, Expectation> s_expectations;
};

inline void expect_warning(std::string expectedWarning,
                           Catch::SourceLineInfo callSite) {
    detail::trim_trailing_newlines(expectedWarning);
    auto* capture = Catch::getCurrentContext().getResultCapture();
    if (capture) {
        WarningListener::expectWarning(
            capture->getCurrentTestName(),
            std::move(expectedWarning),
            callSite);
    }
}

} // namespace test_support

CATCH_REGISTER_LISTENER(test_support::WarningListener);

// Consistent with Catch's assertion naming, fails the run if stderr differs from expectedText.
#define REQUIRE_WARNING(expectedText) \
    ::test_support::expect_warning(std::string(expectedText), CATCH_INTERNAL_LINEINFO)
