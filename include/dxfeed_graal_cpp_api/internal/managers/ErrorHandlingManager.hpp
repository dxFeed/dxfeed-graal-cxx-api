#pragma once

#include "../Conf.hpp"

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {

//TODO: serialization
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
    Error(Error &&) noexcept = default;
    Error &operator=(Error &&) noexcept = default;
    ~Error() = default;

    Error() = default;

    Error(std::size_t causeId, std::size_t groupId, std::string location, std::string message) noexcept
        : causeId{causeId}, threadId{std::this_thread::get_id()}, groupId{groupId}, location{std::move(location)},
          message{std::move(message)} {}
};

//TODO: implement retrieving, grouping methods
class DXFCPP_EXPORT ErrorHandlingManager {
    static constexpr std::size_t DEFAULT_ERROR_COLLECTION_CAPACITY{1024ULL};
    static inline const Error NO_ERROR{Error::UNKNOWN_ID, 0, "", "NO ERROR"};

    std::mutex errorCollectionMutex_{};
    std::unordered_map<std::size_t, Error> errorCollection_{};
    std::size_t maxErrorCollectionCapacity_{DEFAULT_ERROR_COLLECTION_CAPACITY};
    std::atomic<std::size_t> nextId{0};

    ErrorHandlingManager() = default;

  public:
    ErrorHandlingManager(const ErrorHandlingManager &) = delete;
    ErrorHandlingManager &operator=(const ErrorHandlingManager &) = delete;
    ~ErrorHandlingManager() = default;

  public:
    static std::shared_ptr<ErrorHandlingManager> getInstance() {
        static std::shared_ptr<ErrorHandlingManager> instance{new ErrorHandlingManager};

        return instance;
    }

    const Error &registerError(Error error) noexcept {
        std::lock_guard lock{errorCollectionMutex_};

        std::size_t id = nextId++;
        error.id = id;
        errorCollection_[id] = std::move(error);

        if (errorCollection_.size() > maxErrorCollectionCapacity_) {
            errorCollection_.erase(id - maxErrorCollectionCapacity_);
        }

        return errorCollection_[id];
    }

    const Error& getLastError() noexcept {
        return errorCollection_[nextId - 1];
    }
};

} // namespace dxfcpp