#include <lift/Lift.h>

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

class CompletedCtx : public lift::IRequestCb
{
public:
    lift::EventLoop* m_event_loop;

    uint64_t m_success = 0;
    uint64_t m_error = 0;

    auto OnComplete(lift::Request request) -> void override
    {
        if(request->GetStatus() == lift::RequestStatus::SUCCESS)
        {
            ++m_success;
        }
        else
        {
            ++m_error;
        }

        // And request again!
        m_event_loop->AddRequest(std::move(request));
    }
};

static auto print_usage(
    const std::string& program_name
) -> void
{
    std::cout << program_name << "<url> <duration_seconds> <connections> <threads>" << std::endl;
}

int main(int argc, char* argv[])
{
    if(argc < 5)
    {
        print_usage(argv[0]);
        return 0;
    }

    using namespace std::chrono_literals;

    std::string url(argv[1]);
    uint64_t duration_s  = std::stoul(argv[2]);
    uint64_t connections = std::stoul(argv[3]);
    uint64_t threads     = std::stoul(argv[4]);

    // Initialize must be called first before using the LiftHttp library.
    lift::initialize();

    std::vector<std::unique_ptr<lift::EventLoop>> loops;
    for(uint64_t i = 0; i < threads; ++i)
    {
        auto event_loop = std::make_unique<lift::EventLoop>(std::make_unique<CompletedCtx>());
        auto& request_pool = event_loop->GetRequestPool();
        auto& completed_ctx = static_cast<CompletedCtx&>(event_loop->GetRequestCallback());
        completed_ctx.m_event_loop = event_loop.get();

        for(uint64_t j = 0; j < connections; ++j)
        {
            auto request = request_pool.Produce(url, 1000ms);
            request->SetFollowRedirects(false);
            request->AddHeader("Connection", "Keep-Alive");
            event_loop->AddRequest(std::move(request));
        }

        loops.emplace_back(std::move(event_loop));
    }

    std::chrono::seconds seconds(duration_s);
    std::this_thread::sleep_for(seconds);

    uint64_t success = 0;
    uint64_t error = 0;
    for(auto& loop : loops)
    {
        auto& completed_ctx = static_cast<CompletedCtx&>(loop->GetRequestCallback());

        std::cout << completed_ctx.m_success << " " << completed_ctx.m_error << std::endl;

        success += completed_ctx.m_success;
        error = completed_ctx.m_error;
        loop->Stop();
    }

    std::cout << success << " " << error << std::endl;

    lift::cleanup();

    return 0;
}