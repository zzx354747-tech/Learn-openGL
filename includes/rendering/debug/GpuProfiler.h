#pragma once

#include <array>
#include <string>
#include <vector>

#include <glad/gl.h>

struct GpuPassTiming
{
    std::string name;
    double milliseconds = 0.0;
    bool valid = false;
};

class GpuProfiler
{
public:
    GpuProfiler() = default;
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    void beginFrame();
    bool beginPass(const char* name);
    void endPass();

    std::vector<GpuPassTiming> timings() const;

private:
    static constexpr int QueryBufferCount = 4;

    struct QuerySlot
    {
        GLuint id = 0;
        bool pending = false;
    };

    struct PassState
    {
        std::string name;
        std::array<QuerySlot, QueryBufferCount> slots{};
        int nextSlot = 0;
        double smoothedMilliseconds = 0.0;
        bool valid = false;
    };

    PassState& findOrCreate(const char* name);
    void collectReadyResults(PassState& pass);

    std::vector<PassState> passes_;
    int activePass_ = -1;
    int activeSlot_ = -1;
};

class ScopedGpuPass
{
public:
    ScopedGpuPass(GpuProfiler& profiler, const char* name)
        : profiler_(profiler), active_(profiler_.beginPass(name))
    {}

    ~ScopedGpuPass()
    {
        if (active_)
            profiler_.endPass();
    }

    ScopedGpuPass(const ScopedGpuPass&) = delete;
    ScopedGpuPass& operator=(const ScopedGpuPass&) = delete;

private:
    GpuProfiler& profiler_;
    bool active_ = false;
};
