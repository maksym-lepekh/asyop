// Copyright 2018-2019 Maksym Lepekh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <condition_variable>
#include <mutex>

class barrier
{
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::size_t m_count;
    std::size_t m_generation;
    const std::size_t m_threshold;

public:
    explicit barrier(std::size_t count)
        : m_count(count)
        , m_threshold(count)
        , m_generation(0)
    { }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto gen = m_generation;

        if (--m_count == 0)
        {
            m_generation++;
            m_count = m_threshold;
            m_cv.notify_all();
        }
        else
        {
            m_cv.wait(lock, [this, gen] { return gen != m_generation; });
        }
    }
};
