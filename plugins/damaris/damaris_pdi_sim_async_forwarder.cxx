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

#include <assert.h>
#include <pdi/ref_any.h>
#include <damaris/interoperability/pdi/PdiProtocol.hpp>   // MessageType, TypeDescriptor, WireFormat
#include "damaris_pdi_sim_async_forwarder.h"
#include "damaris_pdi_batched_forwarding_strategy.h"
#include "damaris_pdi_immediate_forwarding_strategy.h"


namespace damaris_pdi {

namespace {

std::unique_ptr<DamarisPdiForwardingStrategyInterface> make_strategy(Damaris_cfg& config, std::unique_ptr<Damaris_wrapper>& damaris_wrapper)
{
    // NOTE: get_pdi_forwarding_mode() is a placeholder name -- to be
    // matched to the real accessor once added to Damaris_cfg (same remark
    // as get_arch_domains_count() earlier: read once here, must be read
    // identically on the PdiPlugin side for decoding, or the wire format
    // silently desyncs).
    using damaris::interoperability::pdi::ForwardingMode;

    switch (config.get_pdi_forwarding_mode()) {
    case ForwardingMode::Immediate:
        return std::make_unique<DamarisPdiImmediateForwardingStrategy>(config, damaris_wrapper);
    case ForwardingMode::Batched:
    default:
        return std::make_unique<DamarisPdiBatchedForwardingStrategy>(config, damaris_wrapper);
    }
}

} // anonymous namespace

Damaris_pdi_sim_async_forwarder::Damaris_pdi_sim_async_forwarder(
    Damaris_cfg& damaris_cfg,
    std::unique_ptr<Damaris_wrapper>& damaris_wrapper
)
    : m_config_ref(damaris_cfg),
      m_damaris_ref(damaris_wrapper),
      m_strategy(make_strategy(damaris_cfg, damaris_wrapper))
{
    //constructor body
}

void Damaris_pdi_sim_async_forwarder::forward_share(PDI::Context& ctx, const std::string& desc_name, PDI::Ref ref)
{
    if (!m_damaris_ref) return;
    ctx.logger().info("=>>   data becoming available in the store and forwarded asynchronously to Damaris: {}", desc_name);
    m_strategy->forward_share(desc_name, PDI::Ref_r{ref}.get(), ref.type()->buffersize());
}

void Damaris_pdi_sim_async_forwarder::forward_reclaim(PDI::Context& ctx, const std::string& desc_name, PDI::Ref ref)
{
    if (!m_damaris_ref) return;
    ctx.logger().info("<<= data stop being available in the store, reclaim forwarded asynchronously to Damaris: {}", desc_name);
    m_strategy->forward_reclaim(desc_name);
}

void Damaris_pdi_sim_async_forwarder::forward_event(PDI::Context& ctx, const std::string& event_name)
{
    if (!m_damaris_ref) return;
    ctx.logger().info("!!!                            named event forwarded asynchronously to Damaris: {}", event_name);
    m_strategy->forward_event(event_name);
}

void Damaris_pdi_sim_async_forwarder::on_iteration_end()
{
    if (m_strategy) m_strategy->on_iteration_end();
}

}