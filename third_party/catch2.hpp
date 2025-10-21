#pragma once

#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mini_catch {

class AssertionFailure : public std::exception {
public:
    AssertionFailure(std::string expr, std::string file, int line)
        : message_(std::move(expr)), file_(std::move(file)), line_(line) {}

    const char *what() const noexcept override { return message_.c_str(); }
    const std::string &expression() const noexcept { return message_; }
    const std::string &file() const noexcept { return file_; }
    int line() const noexcept { return line_; }

private:
    std::string message_;
    std::string file_;
    int line_;
};

class TestRegistry {
public:
    using TestFunc = std::function<void()>;

    static TestRegistry &instance() {
        static TestRegistry registry;
        return registry;
    }

    void add(const std::string &name, TestFunc func) { tests_.push_back({name, std::move(func)}); }

    int runAll() {
        int failures = 0;
        for (const auto &test : tests_) {
            try {
                test.func();
                std::cout << "[ OK ] " << test.name << "\n";
            } catch (const AssertionFailure &af) {
                ++failures;
                std::cout << "[FAIL] " << test.name << "\n"
                          << "  Expression: " << af.expression() << "\n"
                          << "  Location: " << af.file() << ':' << af.line() << "\n";
            } catch (const std::exception &ex) {
                ++failures;
                std::cout << "[ERR ] " << test.name << "\n"
                          << "  Exception: " << ex.what() << "\n";
            } catch (...) {
                ++failures;
                std::cout << "[ERR ] " << test.name << "\n"
                          << "  Unknown exception" << "\n";
            }
        }
        std::cout << tests_.size() - failures << "/" << tests_.size() << " tests passed\n";
        return failures;
    }

private:
    struct TestCase { std::string name; TestFunc func; };
    std::vector<TestCase> tests_;
};

struct Registrar {
    Registrar(const std::string &name, TestRegistry::TestFunc func) {
        TestRegistry::instance().add(name, std::move(func));
    }
};

class Approx {
public:
    explicit Approx(double value) : value_(value) {}

    Approx &epsilon(double epsilon) {
        epsilon_ = epsilon;
        return *this;
    }

    Approx &margin(double margin) {
        margin_ = margin;
        return *this;
    }

    bool compare(double other) const {
        double diff = std::fabs(other - value_);
        double tolerance = std::max(margin_, epsilon_ * std::fabs(value_));
        return diff <= tolerance;
    }

    double value() const { return value_; }

private:
    double value_;
    double epsilon_{1e-12};
    double margin_{1e-12};
};

inline bool operator==(double lhs, const Approx &rhs) { return rhs.compare(lhs); }
inline bool operator==(const Approx &lhs, double rhs) { return lhs.compare(rhs); }
inline bool operator!=(double lhs, const Approx &rhs) { return !rhs.compare(lhs); }
inline bool operator!=(const Approx &lhs, double rhs) { return !lhs.compare(rhs); }

} // namespace mini_catch

#define CATCH_INTERNAL_UNIQUE_NAME2(name, line) name##line
#define CATCH_INTERNAL_UNIQUE_NAME(name, line) CATCH_INTERNAL_UNIQUE_NAME2(name, line)
#define CATCH_INTERNAL_TEST_NAME CATCH_INTERNAL_UNIQUE_NAME(test_case_, __LINE__)

#define TEST_CASE(name, tags)                                                                 \
    static void CATCH_INTERNAL_TEST_NAME();                                                   \
    static mini_catch::Registrar CATCH_INTERNAL_UNIQUE_NAME(auto_reg_, __LINE__)(             \
        name, CATCH_INTERNAL_TEST_NAME);                                                      \
    static void CATCH_INTERNAL_TEST_NAME()

#define REQUIRE(expr)                                                                         \
    do {                                                                                      \
        if (!(expr)) {                                                                        \
            std::ostringstream oss;                                                           \
            oss << "REQUIRE(" << #expr << ") failed";                                        \
            throw mini_catch::AssertionFailure(oss.str(), __FILE__, __LINE__);                \
        }                                                                                     \
    } while (false)

#define REQUIRE_THROWS_AS(expr, exception_type)                                               \
    do {                                                                                      \
        bool caught = false;                                                                  \
        try {                                                                                 \
            expr;                                                                             \
        } catch (const exception_type &) {                                                    \
            caught = true;                                                                    \
        }                                                                                     \
        if (!caught) {                                                                        \
            std::ostringstream oss;                                                           \
            oss << "REQUIRE_THROWS_AS(" << #expr << ", " << #exception_type                  \
                << ") failed";                                                               \
            throw mini_catch::AssertionFailure(oss.str(), __FILE__, __LINE__);                \
        }                                                                                     \
    } while (false)

#ifdef CATCH_CONFIG_MAIN
int main() {
    return mini_catch::TestRegistry::instance().runAll();
}
#endif

using mini_catch::Approx;

