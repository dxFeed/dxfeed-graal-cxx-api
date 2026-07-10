#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

// TODO: serialization [EN-8232]
struct DXFCPP_EXPORT Error {
    static constexpr std::size_t UNKNOWN_ID{static_cast<std::size_t>(-1)};

    std::size_t id{UNKNOWN_ID};
    std::size_t causeId{UNKNOWN_ID};
    std::thread::id threadId{};
    std::size_t groupId{};
    std::string location{};
    std::string message{};

    Error(const Error &) = delete;
    Error &operator=(const Error &) = delete;
    Error(Error &&) noexcept;
    Error &operator=(Error &&) noexcept;
    ~Error() noexcept;
    Error() noexcept;
    Error(std::size_t causeId, std::size_t groupId, std::string location, std::string message) noexcept;
};

// TODO: implement retrieving, grouping methods
class DXFCPP_EXPORT ErrorHandlingManager {
    static constexpr std::size_t DEFAULT_ERROR_COLLECTION_CAPACITY{1024ULL};
    static inline const Error NO_ERROR{Error::UNKNOWN_ID, 0, "", "NO ERROR"};

    std::mutex errorCollectionMutex_{};
    std::unordered_map<std::size_t, Error> errorCollection_{};
    std::size_t maxErrorCollectionCapacity_{DEFAULT_ERROR_COLLECTION_CAPACITY};
    std::atomic<std::size_t> nextId{0};

    ErrorHandlingManager() noexcept;

    public:
    ErrorHandlingManager(const ErrorHandlingManager &) = delete;
    ErrorHandlingManager &operator=(const ErrorHandlingManager &) = delete;
    ~ErrorHandlingManager() noexcept;

    static std::shared_ptr<ErrorHandlingManager> getInstance();

    const Error &registerError(Error error) noexcept;

    const Error &getLastError() noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()