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
#include "damaris_pdi_sim_async_forwarder.h"


namespace damaris_pdi {

Damaris_pdi_sim_async_forwarder::Damaris_pdi_sim_async_forwarder(
	Damaris_cfg& damaris_cfg, 
    unique_ptr<Damaris_wrapper>& damaris_wrapper
)
    : m_config_ref(damaris_cfg),
      m_damaris_ref(damaris_wrapper)
{
    //constructor body
}


void Damaris_pdi_sim_async_forwarder::forward_data(PDI::Context& ctx, std::string& desc_name)
{
    assert(m_damaris_ref && "Damaris not initialized");
    //m_damaris_ref->foo(...);
}

void Damaris_pdi_sim_async_forwarder::forward_event(PDI::Context& ctx, std::string& event_name)
{
    assert(m_damaris_ref && "Damaris not initialized");
    //m_damaris_ref->foo(...);
}

}