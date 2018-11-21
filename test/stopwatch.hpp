/*
    The Capo Library
    Code covered by the MIT License

    Author: mutouyun (http://orzz.org)
*/

#pragma once

#include <chrono>   // std::chrono
#include <array>    // std::array
#include <utility>  // std::pair, std::declval
#include <cstddef>  // size_t

namespace capo {

////////////////////////////////////////////////////////////////
/// Stopwatch - can store multiple sets of time
////////////////////////////////////////////////////////////////

template <size_t CountN = 1, class ClockT = std::chrono::steady_clock>
class stopwatch : public ClockT
{
    using base_t = ClockT;

    static_assert(CountN > 0, "The count must be greater than 0");

public:
    using rep        = typename ClockT::rep;
    using period     = typename ClockT::period;
    using duration   = typename ClockT::duration;
    using time_point = typename ClockT::time_point;

private:
    using pair_t = std::pair<time_point/*start*/, time_point/*paused*/>;

    std::array<pair_t, CountN> points_;
    bool is_stopped_ = true;

public:
    stopwatch(bool start_watch = false)
    {
        if (start_watch) start();
    }

public:
    bool is_stopped(void) const
    {
        return is_stopped_;
    }

    template <size_t N = 0>
    bool is_paused(void) const
    {
        return (points_[N].second != points_[N].first);
    }

    template <size_t N = 0>
    duration elapsed(void)
    {
        if (is_stopped())
            return duration::zero();
        else
        if (is_paused<N>())
            return (points_[N].second - points_[N].first);
        else
            return ClockT::now() - points_[N].first;
    }

    template <typename ToDur, size_t N = 0>
    auto elapsed(void) -> decltype(std::declval<ToDur>().count())
    {
        return std::chrono::duration_cast<ToDur>(elapsed<N>()).count();
    }

    template <size_t N = 0>
    void pause(void)
    {
        points_[N].second = ClockT::now();
    }

    template <size_t N = 0>
    void restart(void)
    {
        points_[N].second = points_[N].first =
            ClockT::now() - (points_[N].second - points_[N].first);
    }

    void start(void)
    {
        time_point now = ClockT::now();
        for (auto& pt : points_)
        {
            pt.second = pt.first = now - (pt.second - pt.first);
        }
        is_stopped_ = false;
    }

    void stop(void)
    {
        for (auto& pt : points_)
        {
            pt.second = pt.first;
        }
        is_stopped_ = true;
    }
};

} // namespace capo
