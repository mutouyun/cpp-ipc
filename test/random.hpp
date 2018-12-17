/*
    The Capo Library
    Code covered by the MIT License

    Author: mutouyun (http://orzz.org)
*/

#pragma once

#include <random>   // std::default_random_engine, std::uniform_int_distribution
#include <utility>  // std::forward

namespace capo {

////////////////////////////////////////////////////////////////
/// Simple way of generating random numbers
////////////////////////////////////////////////////////////////

template <class Engine       = std::default_random_engine, 
          class Distribution = std::uniform_int_distribution<>>
class random : public Distribution
{
    using base_t = Distribution;

public:
    using engine_type       = Engine;
    using distribution_type = Distribution;
    using result_type       = typename distribution_type::result_type;
    using param_type        = typename distribution_type::param_type;

private:
    engine_type engine_;

public:
    template <typename... T>
    random(T&&... args)
        : base_t (std::forward<T>(args)...)
        , engine_(std::random_device{}())
    {}

    result_type operator()(void)
    {
        return base_t::operator()(engine_);
    }

    result_type operator()(const param_type& parm)
    {
        return base_t::operator()(engine_, parm);
    }
};

} // namespace capo
