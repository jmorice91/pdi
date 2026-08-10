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

#ifndef DAMARIS_PDI_FORWARDING_STRATEGY_INTERFACE_H_
#define DAMARIS_PDI_FORWARDING_STRATEGY_INTERFACE_H_

#include <cstddef>
#include <string>

namespace damaris_pdi {

/**
 * Common interface for the strategies emitting PDI operations to
 * pdi_generic_channel. Two concrete implementations:
 *   - BatchedPdiForwardingStrategy   : groups the whole iteration into one blob
 *   - ImmediatePdiForwardingStrategy : writes each operation immediately
 *
 * Damaris_pdi_sim_async_forwarder delegates to whichever implementation is
 * selected via <forwarding_mode> in the Damaris config, without itself
 * knowing anything about the wire format.
 */
class DamarisPdiForwardingStrategyInterface
{
public:
    virtual ~DamarisPdiForwardingStrategyInterface() = default;

    /**
     * @param name  name of the shared PDI desc
     * @param data  pointer to the bytes to copy (never retained beyond the call)
     * @param size  size in bytes of `data`
     */
    virtual void forward_share(const std::string& name, const void* data, size_t size) = 0;

    virtual void forward_reclaim(const std::string& name) = 0;

    virtual void forward_event(const std::string& name) = 0;

    /**
     * Called from the client-side iteration_end hook.
     * Batched: serializes and writes the accumulated blob, then clears it.
     * Immediate: simply resets the cyclic block counter.
     */
    virtual void on_iteration_end() = 0;
};

} // namespace damaris_pdi

#endif // DAMARIS_PDI_FORWARDING_STRATEGY_INTERFACE_H_
