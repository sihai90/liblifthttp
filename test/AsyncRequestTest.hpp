#pragma once

#include "catch.hpp"

#include <lift/Lift.hpp>

#include <chrono>

TEST_CASE("Async 100 requests")
{
    constexpr std::size_t COUNT = 100;

    lift::EventLoop ev{};
    auto& rp = ev.GetRequestPool();

    for(std::size_t i = 0; i < COUNT; ++i)
    {
        auto r = rp.Produce(
            "http://localhost:80/",
            [](lift::RequestHandle rh) -> void {
                REQUIRE(rh->GetCompletionStatus() == lift::RequestStatus::SUCCESS);
                REQUIRE(rh->GetResponseStatusCode() == lift::http::StatusCode::HTTP_200_OK);
            },
            std::chrono::milliseconds{100}
        );

        ev.StartRequest(std::move(r));
    }

    while (ev.GetActiveRequestCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
}