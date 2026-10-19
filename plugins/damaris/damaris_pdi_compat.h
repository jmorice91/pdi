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

#include <pdi/error.h>
#include <pdi/version.h>

namespace damaris_pdi {

/// PDI's configuration-error class: `Config_error' up to PDI 1.10.x, renamed
/// `Spectree_error' in PDI 1.11 (same (PC_tree_t, format, args...) constructor).
#if PDI_VERSION_MAJOR > 1 || (PDI_VERSION_MAJOR == 1 && PDI_VERSION_MINOR >= 11)
using Config_error = PDI::Spectree_error;
#else
using Config_error = PDI::Config_error;
#endif

} // namespace damaris_pdi

#endif // DAMARIS_PDI_COMPAT_H_
