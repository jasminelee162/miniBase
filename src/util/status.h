// src/util/status.h
#pragma once
#include <string>

namespace minidb {

enum class Status {
    OK = 0,
    NOT_FOUND,
    IO_ERROR,
    BUFFER_FULL,
    INVALID_PARAM,
    PERMISSION_DENIED
};

class StatusWith {
public:
    StatusWith(Status status) : status_(status) {}
    StatusWith(Status status, std::string msg) : status_(status), message_(std::move(msg)) {}
    
    bool ok() const { return status_ == Status::OK; }
    Status status() const { return status_; }
    const std::string& message() const { return message_; }
    
private:
    Status status_;
    std::string message_;
};

}