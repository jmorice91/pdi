/*******************************************************************************
 * Copyright (C) 2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2026 National Institute for Research in Digital Science and Technology (Inria)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#ifndef DAMARIS_PDI_BATCHED_FORWARDING_STRATEGY_H_
#define DAMARIS_PDI_BATCHED_FORWARDING_STRATEGY_H_

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <damaris/interoperability/pdi/WireFormat.hpp>
#include "damaris_pdi_forwarding_strategy_interface.h"

#include "damaris_cfg.h"
#include "damaris_wrapper.h"

namespace damaris_pdi {

/**
 * Groups all PDI operations of one iteration (data AND metadata, with no
 * distinction between them -- cf. architecture synthesis section 6) into a
 * single buffer, written across as many blocks as needed at
 * on_iteration_end().
 *
 * Trade-off: propagation latency until the end of the iteration, in
 * exchange for a minimal footprint on the global <domains count> budget
 * (independent of the number of PDI operations, only dependent on the
 * total byte volume).
 */
class DamarisPdiBatchedForwardingStrategy : public DamarisPdiForwardingStrategyInterface
{
    Damaris_cfg& m_config_ref;
    std::unique_ptr<Damaris_wrapper>& m_damaris_ref;

    std::vector<std::byte> m_iteration_buffer = damaris::interoperability::pdi::begin_batch();
    uint32_t m_message_count = 0;

public:
    DamarisPdiBatchedForwardingStrategy(Damaris_cfg& config, std::unique_ptr<Damaris_wrapper>& damaris_wrapper)
        : m_config_ref(config), m_damaris_ref(damaris_wrapper)
    {
    }

    void forward_share(const std::string& name, const void* data, size_t size) override
    {
        damaris::interoperability::pdi::append_message(
            m_iteration_buffer, damaris::interoperability::pdi::MessageType::Share, name, data, size);
        damaris::interoperability::pdi::increment_batch_count(m_iteration_buffer);
        ++m_message_count;
    }

    void forward_reclaim(const std::string& name) override
    {
        damaris::interoperability::pdi::append_message(
            m_iteration_buffer, damaris::interoperability::pdi::MessageType::Reclaim, name, nullptr, 0);
        damaris::interoperability::pdi::increment_batch_count(m_iteration_buffer);
        ++m_message_count;
    }

    void forward_event(const std::string& name) override
    {
        damaris::interoperability::pdi::append_message(
            m_iteration_buffer, damaris::interoperability::pdi::MessageType::Event, name, nullptr, 0);
        damaris::interoperability::pdi::increment_batch_count(m_iteration_buffer);
        ++m_message_count;
    }

    void on_iteration_end() override
    {
        if (!m_damaris_ref || m_message_count == 0) {
            m_iteration_buffer = damaris::interoperability::pdi::begin_batch();
            m_message_count = 0;
            return;
        }

        size_t block_capacity = m_config_ref.get_generic_channel_layout_size(); // e.g. 65536
        int32_t available_blocks = m_config_ref.get_arch_domains_count();

        int32_t blocks_needed = static_cast<int32_t>(
            (m_iteration_buffer.size() + block_capacity - 1) / block_capacity);

        if (blocks_needed > available_blocks) {
            throw std::runtime_error(
                "pdi_generic_channel (batched mode): iteration needs " +
                std::to_string(blocks_needed) + " blocks, only " +
                std::to_string(available_blocks) + " available "
                "(count in <architecture><domains> too small, or layout dimensions too small)"
            );
        }

        // damaris_write_block always writes exactly block_capacity bytes from the pointer;
        // pad the buffer to a full multiple so we never read beyond the allocation.
        m_iteration_buffer.resize(static_cast<size_t>(blocks_needed) * block_capacity, std::byte{0});

        for (int32_t b = 0; b < blocks_needed; ++b) {
            size_t offset = static_cast<size_t>(b) * block_capacity;
            damaris_write_block("pdi_generic_channel", b, m_iteration_buffer.data() + offset);
        }

        m_iteration_buffer = damaris::interoperability::pdi::begin_batch();
        m_message_count = 0;
    }
};

} // namespace damaris_pdi

#endif // DAMARIS_PDI_BATCHED_FORWARDING_STRATEGY_H_
