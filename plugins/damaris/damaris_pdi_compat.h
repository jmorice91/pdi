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

#ifndef DAMARIS_PDI_COMPAT_H_
#define DAMARIS_PDI_COMPAT_H_

#include <functional>
#include <string>

#include <pdi/context.h>
#include <pdi/error.h>
#include <pdi/ref_any.h>
#include <pdi/version.h>

// PDI up to 1.12 (and early 1.13 alphas) registers callbacks through
// Context::callbacks(); newer PDI merged Callbacks into the context (on_data /
// on_data_remove / on_event) and removed <pdi/callbacks.h>. Both APIs exist
// under the version string 1.13.0-alpha, so detect the API of the PDI actually
// included: the old <pdi/context.h> includes <pdi/callbacks.h>, whose include
// guard is then defined. (__has_include would also see a stale callbacks.h of
// another PDI installed on the default include path.)
#ifdef PDI_CALLBACKS_H_
#define DAMARIS_PDI_HAS_CALLBACKS 1
#endif

namespace damaris_pdi {

/// Called when `name' (any data if empty) becomes available.
inline std::function<void()> pdi_on_data(PDI::Context& ctx, const std::function<void(const std::string&, PDI::Ref)>& callback, const std::string& name = {})
{
#ifdef DAMARIS_PDI_HAS_CALLBACKS
	return ctx.callbacks().add_data_callback(callback, name);
#else
	return ctx.on_data(callback, name);
#endif
}

/// Called when `name' (any data if empty) is about to be reclaimed.
inline std::function<void()> pdi_on_data_remove(PDI::Context& ctx, const std::function<void(const std::string&, PDI::Ref)>& callback, const std::string& name = {})
{
#ifdef DAMARIS_PDI_HAS_CALLBACKS
	return ctx.callbacks().add_data_remove_callback(callback, name);
#else
	return ctx.on_data_remove(callback, name);
#endif
}

/// Called on event `name' (any event if empty).
inline std::function<void()> pdi_on_event(PDI::Context& ctx, const std::function<void(const std::string&)>& callback, const std::string& name = {})
{
#ifdef DAMARIS_PDI_HAS_CALLBACKS
	return ctx.callbacks().add_event_callback(callback, name);
#else
	return ctx.on_event(callback, name);
#endif
}

/// PDI's configuration-error class: `Config_error' up to PDI 1.10.x, renamed
/// `Spectree_error' in PDI 1.11 (same (PC_tree_t, format, args...) constructor).
#if PDI_VERSION_MAJOR > 1 || (PDI_VERSION_MAJOR == 1 && PDI_VERSION_MINOR >= 11)
using Config_error = PDI::Spectree_error;
#else
using Config_error = PDI::Config_error;
#endif

} // namespace damaris_pdi

#endif // DAMARIS_PDI_COMPAT_H_
