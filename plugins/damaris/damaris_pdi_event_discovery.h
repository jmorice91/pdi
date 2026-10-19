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

#ifndef DAMARIS_PDI_EVENT_DISCOVERY_H_
#define DAMARIS_PDI_EVENT_DISCOVERY_H_

#include <set>
#include <string>
#include <vector>

#include <paraconf.h>

namespace damaris_pdi {

/**
 * Which simulation events are forwarded to the Damaris server (`fwd_events:'
 * key, next to `fwd_mode:' in the server-side PDI config):
 *
 *   fwd_events: auto                           # default: events awaited by the server plugins
 *   fwd_events: all                            # every event except Damaris control events
 *   fwd_events: [newiter, main_loop]           # explicit list
 *   fwd_events: { auto: true, extra: [ev] }    # auto + additions (auto: false = extra only)
 *
 * Forwarding an event no server plugin waits for has no effect on the result,
 * so `auto' only saves traffic (one block per event in immediate mode); the
 * risk is missing an event, hence the startup log and the `extra' escape hatch.
 */
struct Fwd_events_cfg {
	enum class Mode { Auto, All, List };
	Mode mode = Mode::Auto;
	std::vector<std::string> extra; ///< explicit list (List) or additions (Auto)
};

/** Parses the `fwd_events:' node; an absent node means `auto'.
 * Throws PDI::Spectree_error on a malformed value. */
Fwd_events_cfg parse_fwd_events(PC_tree_t node);

/** Result of scanning the `plugins:' section of the server-side PDI config. */
struct Discovered_events {
	std::set<std::string> names;       ///< event names awaited by the server plugins
	std::vector<std::string> warnings; ///< plugins not in the table (generic `on_event' fallback used)
	std::vector<std::string> notes;    ///< informational, e.g. `trace' listens to every event
};

/**
 * Collects the event names the server-side plugins wait for, using the
 * plugin <-> event-key table in damaris_pdi_event_discovery.cxx. Plugins not in
 * the table fall back to the common `on_event' convention and get a warning.
 */
Discovered_events discover_plugin_events(PC_tree_t plugins);

} // namespace damaris_pdi

#endif // DAMARIS_PDI_EVENT_DISCOVERY_H_
