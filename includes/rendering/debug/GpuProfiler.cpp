#include "rendering/debug/GpuProfiler.h"

#include <algorithm>

GpuProfiler::~GpuProfiler()
{
    for (PassState& pass : passes_)
    {
        for (QuerySlot& slot : pass.slots)
        {
            if (slot.id != 0)
                glDeleteQueries(1, &slot.id);
        }
    }
}

void GpuProfiler::beginFrame()
{
    for (PassState& pass : passes_)
        collectReadyResults(pass);
}

bool GpuProfiler::beginPass(const char* name)
{
    // GL_TIME_ELAPSED queries cannot overlap. A caller accidentally attempting
    // to nest a pass is ignored instead of generating GL_INVALID_OPERATION.
    if (activePass_ >= 0)
        return false;

    PassState& pass = findOrCreate(name);
    collectReadyResults(pass);

    for (int offset = 0; offset < QueryBufferCount; ++offset)
    {
        const int slotIndex = (pass.nextSlot + offset) % QueryBufferCount;
        QuerySlot& slot = pass.slots[slotIndex];
        if (slot.pending)
            continue;

        glBeginQuery(GL_TIME_ELAPSED, slot.id);
        activePass_ = static_cast<int>(&pass - passes_.data());
        activeSlot_ = slotIndex;
        return true;
    }

    // The GPU is more than four frames behind for this pass. Skip this sample
    // rather than blocking the CPU on a query result.
    return false;
}

void GpuProfiler::endPass()
{
    if (activePass_ < 0 || activeSlot_ < 0)
        return;

    glEndQuery(GL_TIME_ELAPSED);
    PassState& pass = passes_[activePass_];
    pass.slots[activeSlot_].pending = true;
    pass.nextSlot = (activeSlot_ + 1) % QueryBufferCount;
    activePass_ = -1;
    activeSlot_ = -1;
}

std::vector<GpuPassTiming> GpuProfiler::timings() const
{
    std::vector<GpuPassTiming> result;
    result.reserve(passes_.size());
    for (const PassState& pass : passes_)
        result.push_back({pass.name, pass.smoothedMilliseconds, pass.valid});
    return result;
}

GpuProfiler::PassState& GpuProfiler::findOrCreate(const char* name)
{
    const auto found = std::find_if(
        passes_.begin(), passes_.end(),
        [name](const PassState& pass) { return pass.name == name; });
    if (found != passes_.end())
        return *found;

    passes_.push_back({});
    PassState& pass = passes_.back();
    pass.name = name;
    for (QuerySlot& slot : pass.slots)
        glGenQueries(1, &slot.id);
    return pass;
}

void GpuProfiler::collectReadyResults(PassState& pass)
{
    for (QuerySlot& slot : pass.slots)
    {
        if (!slot.pending)
            continue;

        GLint available = GL_FALSE;
        glGetQueryObjectiv(slot.id, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available != GL_TRUE)
            continue;

        GLuint64 nanoseconds = 0;
        glGetQueryObjectui64v(slot.id, GL_QUERY_RESULT, &nanoseconds);
        const double milliseconds = static_cast<double>(nanoseconds) / 1000000.0;
        pass.smoothedMilliseconds = pass.valid
            ? pass.smoothedMilliseconds * 0.85 + milliseconds * 0.15
            : milliseconds;
        pass.valid = true;
        slot.pending = false;
    }
}
