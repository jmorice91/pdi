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

#ifndef DAMARIS_PDI_IMMEDIATE_FORWARDING_STRATEGY_H_
#define DAMARIS_PDI_IMMEDIATE_FORWARDING_STRATEGY_H_

#include <stdexcept>
#include <string>
#include <vector>

#include <damaris/interoperability/pdi/WireFormat.hpp>
#include "damaris_pdi_forwarding_strategy_interface.h"

#include "damaris_cfg.h"
#include "damaris_wrapper.h"

namespace damaris_pdi {

/**
 * Writes each PDI operation immediately, in its own Damaris block, as soon
 * as the corresponding callback fires -- symmetric to what DamarisPlugin
 * already does for classic variables (_set_position/_write_block without
 * waiting for iteration_end).
 *
 * Trade-off: asynchronous overlap/propagation throughout the iteration, in
 * exchange for a footprint on the global <domains count> budget that is
 * proportional to the number of PDI operations (not to the data volume).
 * Cf. "Point 2" of the earlier discussion -- to be measured empirically
 * against BatchedPdiForwardingStrategy on the real workload.
 */
class DamarisPdiImmediateForwardingStrategy : public DamarisPdiForwardingStrategyInterface
{
    Damaris_cfg& m_config_ref;
    std::unique_ptr<Damaris_wrapper>& m_damaris_ref;

    int32_t m_next_block = 0;

    void write_single_message(
        damaris::interoperability::pdi::MessageType type,
        const std::string& name,
        const void* data,
        size_t size)
    {
        if (!m_damaris_ref) return;

        int32_t available_blocks = m_config_ref.get_arch_domains_count();
        if (m_next_block >= available_blocks) {
            throw std::runtime_error(
                "pdi_generic_channel (immediate mode): no more blocks available this iteration (" +
                std::to_string(available_blocks) + ") -- increase <domains count>, "
                "or switch back to forwarding_mode: batched"
            );
        }

        std::vector<std::byte> blob; // no num_messages prefix -- a single message per block
        damaris::interoperability::pdi::append_message(blob, type, name, data, size);
        damaris_write_block("pdi_generic_channel", m_next_block++, blob.data());
    }

public:
    DamarisPdiImmediateForwardingStrategy(Damaris_cfg& config, std::unique_ptr<Damaris_wrapper>& damaris_wrapper)
        : m_config_ref(config), m_damaris_ref(damaris_wrapper)
    {
    }

    void forward_share(const std::string& name, const void* data, size_t size) override
    {
        write_single_message(damaris::interoperability::pdi::MessageType::Share, name, data, size);
    }

    void forward_reclaim(const std::string& name) override
    {
        write_single_message(damaris::interoperability::pdi::MessageType::Reclaim, name, nullptr, 0);
    }

    void forward_event(const std::string& name) override
    {
        write_single_message(damaris::interoperability::pdi::MessageType::Event, name, nullptr, 0);
    }

    void on_iteration_end() override
    {
        m_next_block = 0;
    }
};

} // namespace damaris_pdi

#endif // DAMARIS_PDI_IMMEDIATE_FORWARDING_STRATEGY_H_
