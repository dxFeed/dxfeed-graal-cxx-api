// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "UiMailbox.hpp"

#include <dxfeed_graal_cpp_api/api.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <fmt/chrono.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <ctime>
#include <iostream>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace dxfcpp;
using dxfeed::time_and_sales_sample::TimeAndSaleRow;
using dxfeed::time_and_sales_sample::UiMailbox;
using dxfeed::time_and_sales_sample::ViewState;

namespace {

constexpr std::size_t NUMBER_OF_PRESENT_TRADES = 30;

std::string trim(std::string value) {
    const auto isNotSpace = [](unsigned char value) {
        return !std::isspace(value);
    };
    const auto first = std::ranges::find_if(value, isNotSpace);
    const auto last = std::ranges::find_if(value | std::views::reverse, isNotSpace).base();

    if (first >= last) {
        return {};
    }

    return {first, last};
}

TimeAndSaleRow toRow(const std::shared_ptr<TimeAndSale> &event) {
    return TimeAndSaleRow{
        .time = event->getTime(),
        .index = event->getIndex(),
        .exchangeCode = event->getExchangeCode(),
        .price = event->getPrice(),
        .size = event->getSize(),
        .bidPrice = event->getBidPrice(),
        .askPrice = event->getAskPrice(),
        .saleConditions = event->getExchangeSaleConditionsOpt().value_or(""),
        .remove = EventFlag::isRemove(event),
    };
}

/**
 * Owns the dxFeed subscriptions used by the window and converts their callbacks into UI state updates.
 *
 * A Profile subscription supplies the instrument description. Regional TimeAndSale symbols use their base symbol for
 * this subscription because Profile events are only available for composite symbols. Each non-empty symbol also owns
 * one IndexedTxModel<TimeAndSale> configured for snapshot and batch processing.
 */
class FeedController final {
    std::shared_ptr<DXFeed> feed_{DXFeed::getInstance()};
    std::shared_ptr<UiMailbox> mailbox_{std::make_shared<UiMailbox>(NUMBER_OF_PRESENT_TRADES)};
    std::shared_ptr<DXFeedSubscription> profileSubscription_{};
    std::shared_ptr<IndexedTxModel<TimeAndSale>> timeAndSalesModel_{};

    public:
    /// Creates the shared Profile subscription and installs its event listener.
    FeedController() : profileSubscription_(feed_->createSubscription(Profile::TYPE)) {
        const std::weak_ptr weakMailbox{mailbox_};

        profileSubscription_->addEventListener<Profile>([weakMailbox](const auto &profiles) {
            const auto mailbox = weakMailbox.lock();

            if (!mailbox) {
                return;
            }

            for (const auto &profile : profiles) {
                mailbox->publishProfile(profile->getEventSymbol(), profile->getDescriptionOpt());
            }
        });
    }

    /// Closes the active IndexedTxModel and Profile subscription.
    ~FeedController() {
        if (timeAndSalesModel_) {
            timeAndSalesModel_->close();
        }

        if (profileSubscription_) {
            profileSubscription_->close();
        }
    }

    FeedController(const FeedController &) = delete;
    FeedController &operator=(const FeedController &) = delete;

    /**
     * Switches the window to a new symbol.
     *
     * The previous model is closed before the mailbox is reset. An empty or whitespace-only symbol clears the view and
     * leaves no TimeAndSale model active. A regional symbol such as AAPL&Q remains unchanged for TimeAndSale, while its
     * base symbol AAPL is used for Profile.
     *
     * @param symbol Symbol entered by the user; leading and trailing whitespace is removed.
     */
    void subscribe(std::string symbol) {
        symbol = trim(std::move(symbol));

        if (timeAndSalesModel_) {
            timeAndSalesModel_->close();
            timeAndSalesModel_.reset();
        }

        const auto profileSymbol = MarketEventSymbols::getBaseSymbol(symbol);
        const auto generation = mailbox_->reset(symbol, profileSymbol);

        profileSubscription_->setSymbols(profileSymbol.empty() ? std::vector<std::string>{}
                                                              : std::vector{profileSymbol});

        if (symbol.empty()) {
            return;
        }

        const std::weak_ptr weakMailbox{mailbox_};
        timeAndSalesModel_ =
            IndexedTxModel<TimeAndSale>::newBuilder()
                ->withBatchProcessing(true)
                ->withSnapshotProcessing(true)
                ->withFeed(feed_)
                ->withSymbol(symbol)
                ->withListener([weakMailbox, generation](const auto &, const auto &events, bool isSnapshot) {
                    const auto mailbox = weakMailbox.lock();

                    if (!mailbox) {
                        return;
                    }

                    std::vector<TimeAndSaleRow> rows;
                    rows.reserve(events.size());
                    std::ranges::transform(events, std::back_inserter(rows), toRow);
                    mailbox->publishTrades(generation, rows, isSnapshot);
                })
                ->build();
    }

    /**
     * Returns a new detached view only when a feed callback or subscription change modified the visible state.
     *
     * @return Updated UI data, or std::nullopt when no redraw data needs to be copied.
     */
    [[nodiscard]] std::optional<ViewState> takeViewIfChanged() const {
        return mailbox_->takeIfDirty();
    }
};

std::string formatTime(std::int64_t milliseconds) {
    const std::time_t seconds = milliseconds / 1000;

    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &seconds);
#else
    localtime_r(&seconds, &localTime);
#endif

    return fmt::format("{:%H:%M:%S}.{:03}", localTime, (milliseconds % 1000 + 1000) % 1000);
}

std::string formatExchangeCode(std::int16_t code) {
    if (code <= 0) {
        return {};
    }

    return {static_cast<char>(code)};
}

void renderTable(const std::vector<TimeAndSaleRow> &rows) {
    constexpr auto flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                           ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("time-and-sales", 8, flags, ImVec2{0.0F, 0.0F})) {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 105.0F);
    ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthStretch, 1.6F);
    ImGui::TableSetupColumn("EX", ImGuiTableColumnFlags_WidthFixed, 38.0F);
    ImGui::TableSetupColumn("Price");
    ImGui::TableSetupColumn("Size");
    ImGui::TableSetupColumn("Bid");
    ImGui::TableSetupColumn("Ask");
    ImGui::TableSetupColumn("SC");
    ImGui::TableHeadersRow();

    for (const auto &row : rows) {
        const auto time = formatTime(row.time);
        const auto exchangeCode = formatExchangeCode(row.exchangeCode);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(time.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%lld", static_cast<long long>(row.index));
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(exchangeCode.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%.6g", row.price);
        ImGui::TableNextColumn();
        ImGui::Text("%.6g", row.size);
        ImGui::TableNextColumn();
        ImGui::Text("%.6g", row.bidPrice);
        ImGui::TableNextColumn();
        ImGui::Text("%.6g", row.askPrice);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(row.saleConditions.c_str());
    }

    ImGui::EndTable();
}

void glfwErrorCallback(int error, const char *description) {
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

int runUi() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error{"Unable to initialize GLFW"};
    }

#if defined(__APPLE__)
    constexpr auto glslVersion = "#version 150";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    constexpr auto glslVersion = "#version 130";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow *window = glfwCreateWindow(1100, 620, "DXFeed Time & Sales", nullptr, nullptr);

    if (!window) {
        glfwTerminate();

        throw std::runtime_error{"Unable to create a GLFW window"};
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    ImFontConfig fontConfig{};
    fontConfig.SizePixels = 15.0F;
    ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontDefaultVector(&fontConfig);

    const auto contentScale = std::max(1.0F, ImGui_ImplGlfw_GetContentScaleForWindow(window));

    ImGui::GetStyle().ScaleAllSizes(contentScale);
    ImGui::GetStyle().FontScaleDpi = contentScale;

    try {
        FeedController controller;
        std::array<char, 128> symbolInput{};
        ViewState view{};

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if (const auto changed = controller.takeViewIfChanged()) {
                view = *changed;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            const auto viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);

            constexpr auto windowFlags =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

            ImGui::Begin("DXFeed Time & Sales", nullptr, windowFlags);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Symbol");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180.0F);

            const bool enterPressed = ImGui::InputText("##symbol", symbolInput.data(), symbolInput.size(),
                                                       ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();

            if (enterPressed || ImGui::Button("Subscribe")) {
                controller.subscribe(symbolInput.data());
            }

            ImGui::SameLine();
            ImGui::TextUnformatted(view.description.c_str());
            ImGui::Separator();

            if (view.symbol.empty()) {
                ImGui::TextDisabled("Enter a symbol (for example AAPL) and press Enter or Subscribe.");
            } else if (view.rows.empty()) {
                ImGui::TextDisabled("Waiting for TimeAndSale snapshot for %s...", view.symbol.c_str());
            }

            renderTable(view.rows);
            ImGui::End();

            ImGui::Render();

            int displayWidth{};
            int displayHeight{};

            glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
            glViewport(0, 0, displayWidth, displayHeight);
            glClearColor(0.94F, 0.94F, 0.94F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    } catch (...) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();

        throw;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

} // namespace

int main() {
    try {
        return runUi();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
