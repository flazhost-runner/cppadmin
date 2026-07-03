#pragma once
#include <stdexcept>
#include <string>

// Central exception hierarchy — services throw these; exception handler maps them to HTTP.
// NEVER return error objects or use result types for expected-failure paths.

struct AppError : std::runtime_error {
    int statusCode;
    explicit AppError(const std::string &msg, int code = 500)
        : std::runtime_error(msg), statusCode(code) {}
};

struct NotFoundError : AppError {
    explicit NotFoundError(const std::string &msg = "Not Found")
        : AppError(msg, 404) {}
};

struct ConflictError : AppError {
    explicit ConflictError(const std::string &msg = "Conflict")
        : AppError(msg, 409) {}
};

struct ValidationError : AppError {
    explicit ValidationError(const std::string &msg = "Validation Failed")
        : AppError(msg, 422) {}
};

struct UnauthorizedError : AppError {
    explicit UnauthorizedError(const std::string &msg = "Unauthorized")
        : AppError(msg, 401) {}
};

struct ForbiddenError : AppError {
    explicit ForbiddenError(const std::string &msg = "Forbidden")
        : AppError(msg, 403) {}
};
