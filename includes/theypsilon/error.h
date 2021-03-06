#ifndef TY_ERROR_H
#define TY_ERROR_H

#include <string>
#include <iostream>
#include <variant>
#include <optional>

#define TY_INTERNAL_FILE_CTX std::string(__FILE__) + "@" + std::to_string(__LINE__) + ": "

#define RETURN_ERROR(msg) \
    return ty::error{TY_INTERNAL_FILE_CTX + msg};

#define RETURN_OK \
    return ty::error::none();

#define TRY_RESULT(type, var, expr) \
    auto _##var##_result = expr; \
    if (!_##var##_result.is_ok()) { \
        return _##var##_result.catch_error(); \
    } \
    type& var = _##var##_result.get_ref();

#define TRY_ERROR(expr) { \
    auto _try_error = expr; \
    if (_try_error) { \
        return _try_error; \
    } \
}

#define TRY_NOT_NULL(type, var, expr) \
    type var = expr; \
    if (var == nullptr) { \
        return ty::error{TY_INTERNAL_FILE_CTX + "Error, result is nullptr."}; \
    }

#define TRY_NON_NEG(expr) { \
    auto _non_neg = expr; \
    if (_non_neg < 0) { \
        return ty::error{TY_INTERNAL_FILE_CTX + "Error, result is less than 0: " + std::to_string(_non_neg) + "."}; \
    } \
}

#define TRY_IS_ZERO(expr) { \
    auto _zero = expr; \
    if (_zero != 0) { \
        return ty::error{TY_INTERNAL_FILE_CTX + "Error, result is not 0: " + std::to_string(_zero) + "."}; \
    } \
}

#define TRY_IS_TRUE(expr) { \
    auto _true = expr; \
    if (!_true) { \
        return ty::error{TY_INTERNAL_FILE_CTX + "Error, result is false."}; \
    } \
}

#define TRY_NOT_GL_ERROR() { \
    auto _gl_err = glGetError(); \
    if (_gl_err != GL_NO_ERROR) { \
        return ty::error{ TY_INTERNAL_FILE_CTX + std::to_string(int(_gl_err)) }; \
    } \
}

namespace ty {
    struct error {
    private:
        explicit error() noexcept {}
        std::optional<std::string> err;
    public:
        static error none() noexcept {
            return error{};
        }
        explicit error(std::string message) noexcept : err{ message } {
            if (message.empty()) { // breakpoint
                err = "returned error with empty message";
            }
        }
        std::string message() const noexcept {
            return err.value_or("");
        }
        operator bool() const noexcept {
            return err.has_value();
        }
    };

    template <typename T>
    struct result {
        result(error e) noexcept : content{ e ? e : error{"result constructed with empty error" } } {}
        result(T&& val) noexcept(noexcept(T{ std::forward<T>(val) })) : content{ std::forward<T>(val) } { }
        bool is_ok() const noexcept {
            return std::holds_alternative<T>(content);
        }
        error catch_error() const noexcept {
            if (is_ok()) {
                return error{ "result does not contain error" };
            } else {
                return std::get<error>(content);
            }
        }
        T get_copy() const noexcept {
            abort_on_error();
            return std::get<T>(content);
        }

        T& get_ref() noexcept {
            abort_on_error();
            return std::get<T>(content);
        }

        const T& get_const_ref() const noexcept {
            abort_on_error();
            return std::get<T>(content);
        }
    private:
        void abort_on_error() const noexcept {
            if (std::holds_alternative<error>(content)) {
                std::cerr << "[FATAL] Tried to use ::get_*() on a result<T> with an error.\n";
                std::terminate();
            }
        }
        std::variant<T, error> content;
    };
}

#endif