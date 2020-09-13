/*
    The Capo Library
    Code covered by the MIT License
    Author: mutouyun (http://orzz.org)
*/

#pragma once

namespace capo {

////////////////////////////////////////////////////////////////

class noncopyable
{
protected:
    noncopyable(void) = default;
    ~noncopyable(void) = default;
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

////////////////////////////////////////////////////////////////

} // namespace capo