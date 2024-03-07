// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <node/interface_ui.h>
#include <node/timeoffsets.h>
#include <sync.h>
#include <util/translation.h>
#include <warnings.h>

#include <chrono>
#include <deque>
#include <ranges>

using namespace std::chrono_literals;

void TimeOffsets::Add(std::chrono::seconds offset)
{
    LOCK(m_mutex);

    if (m_offsets.size() >= N) {
        m_offsets.pop_front();
    }
    m_offsets.push_back(offset);
}

std::chrono::seconds TimeOffsets::Median() const
{
    LOCK(m_mutex);

    // Only calculate the median if we have 5 or more offsets
    if (m_offsets.size() < 5) return {0s};

    auto sorted_copy = m_offsets;
    std::ranges::sort(sorted_copy);
    return sorted_copy[m_offsets.size() / 2];  // approximate median is good enough, keep it simple
}

bool TimeOffsets::WarnIfOutOfSync() const
{
    if (std::chrono::abs(Median()) <= m_warn_threshold) return false;

    bilingual_str msg{_("Your computer's date and time appear out of sync with the network, "
                        "this may lead to consensus failure. Please ensure it is correct.")};
    LogWarning("%s\n", msg.translated);

    if (!m_warning_emitted) {
        m_warning_emitted = true;
        SetMedianTimeOffsetWarning();
        uiInterface.ThreadSafeMessageBox(msg, "", CClientUIInterface::MSG_WARNING);
    }

    return true;
}
