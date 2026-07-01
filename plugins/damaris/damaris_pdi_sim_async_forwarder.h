/*******************************************************************************
 * Copyright (C) 2015-2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2025-2026 National Institute for Research in Digital Science and Technology (Inria)
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

#ifndef Damaris_pdi_sim_async_forwarder_H_
#define Damaris_pdi_sim_async_forwarder_H_

#include <mpi.h>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <pdi/context.h>

#include <Damaris.h>
#include "damaris_cfg.h"
#include "damaris_wrapper.h"

using PDI::Context;
using std::list;
using std::string;
using std::unique_ptr;

namespace damaris_pdi {

class Damaris_pdi_sim_async_forwarder
{
	Damaris_cfg& m_config_ref;
	unique_ptr<Damaris_wrapper>& m_damaris_ref;

public:
	Damaris_pdi_sim_async_forwarder(Damaris_cfg& damaris_cfg, unique_ptr<Damaris_wrapper>& damaris_wrapper);

	void forward_data(PDI::Context& ctx, std::string& desc_name);
	void forward_event(PDI::Context& ctx, std::string& event_name);
}; // class Damaris_pdi_sim_async_forwarder

} // namespace damaris_pdi

#endif // Damaris_pdi_sim_async_forwarder_H_
