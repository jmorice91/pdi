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

#include <functional>
#include <unordered_map>

#include <pdi/error.h>
#include <pdi/paraconf_wrapper.h>

#include "damaris_pdi_event_discovery.h"

namespace damaris_pdi {

namespace {

using Names = std::set<std::string>;

bool exists(PC_tree_t node)
{
	return !PC_status(node);
}

/// `ev' or `[ev1, ev2]'
void add_names(PC_tree_t node, Names& out)
{
	if (!exists(node)) return;
	if (PDI::is_list(node)) {
		PDI::each(node, [&](PC_tree_t item) { out.insert(PDI::to_string(item)); });
	} else {
		out.insert(PDI::to_string(node));
	}
}

/// `{ev1: ..., ev2: ...}': the keys are the event names
void add_map_keys(PC_tree_t node, Names& out)
{
	if (!exists(node) || !PDI::is_map(node)) return;
	PDI::each(node, [&](PC_tree_t key, PC_tree_t) { out.insert(PDI::to_string(key)); });
}

/// `{ev: ...}' or `[{ev1: ...}, {ev2: ...}]'
void add_map_keys_or_list_of_maps(PC_tree_t node, Names& out)
{
	if (!exists(node)) return;
	if (PDI::is_list(node)) {
		PDI::each(node, [&](PC_tree_t item) { add_map_keys(item, out); });
	} else {
		add_map_keys(node, out);
	}
}

/// Generic `on_event' convention: a name, a list of names, or a map keyed by names.
void add_on_event(PC_tree_t node, Names& out)
{
	if (!exists(node)) return;
	if (PDI::is_map(node)) {
		add_map_keys(node, out);
	} else if (PDI::is_list(node) && PDI::len(node) > 0 && PDI::is_map(PC_get(node, "[0]"))) {
		add_map_keys_or_list_of_maps(node, out);
	} else {
		add_names(node, out);
	}
}

using Extractor = std::function<void(PC_tree_t plugin_cfg, Names& out)>;

/**
 * Plugin <-> event settings: for each PDI plugin, where its configuration
 * names the events it waits for. Only these locations are read -- a blind
 * recursive search on key names is unsafe (e.g. timer's `start' event key vs
 * decl_hdf5's numeric `dataset_selection.start').
 *
 * Not events: set_value `on_init'/`on_finalize'/`on_data' and pycall `on_data'
 * are triggered by PDI init/finalize or by data sharing, which the server PDI
 * context handles itself.
 */
const std::unordered_map<std::string, Extractor>& plugin_event_table()
{
	static const std::unordered_map<std::string, Extractor> table = {
		// file-level `on_event' in each file entry (single map or list of maps)
		{"decl_hdf5", [](PC_tree_t cfg, Names& out) {
			 PDI::opt_each(cfg, [&](PC_tree_t file) { add_names(PC_get(file, ".on_event"), out); });
		 }},
		{"decl_netcdf", [](PC_tree_t cfg, Names& out) {
			 PDI::opt_each(cfg, [&](PC_tree_t file) { add_names(PC_get(file, ".on_event"), out); });
		 }},
		{"pycall", [](PC_tree_t cfg, Names& out) { add_map_keys(PC_get(cfg, ".on_event"), out); }},
		{"set_value", [](PC_tree_t cfg, Names& out) { add_map_keys(PC_get(cfg, ".on_event"), out); }},
		{"user_code", [](PC_tree_t cfg, Names& out) { add_map_keys_or_list_of_maps(PC_get(cfg, ".on_event"), out); }},
		// timer: [{name: {start: ev, stop: ev}}] or {name: [{start: ev, stop: ev}, ...]}
		{"timer", [](PC_tree_t cfg, Names& out) {
			 PDI::opt_each(cfg, [&](PC_tree_t item) {
				 if (!PDI::is_map(item)) return;
				 PDI::each(item, [&](PC_tree_t key, PC_tree_t value) {
					 if (PDI::to_string(key) == "output_to") return;
					 PDI::opt_each(value, [&](PC_tree_t timer) {
						 add_names(PC_get(timer, ".start"), out);
						 add_names(PC_get(timer, ".stop"), out);
					 });
				 });
			 });
		 }},
		{"veloc", [](PC_tree_t cfg, Names& out) {
			 PC_tree_t managed = PC_get(cfg, ".managed_checkpointing");
			 for (const char* key: {".checkpoint_on", ".recover_on", ".synchronize_on"}) {
				 add_names(PC_get(managed, key), out);
			 }
			 for (const char* section: {".custom_checkpointing.custom_checkpoint", ".custom_checkpointing.custom_recover"}) {
				 PC_tree_t custom = PC_get(cfg, section);
				 for (const char* key: {".start_on", ".end_on", ".route_file_on"}) {
					 add_names(PC_get(custom, key), out);
				 }
			 }
		 }},
		// no event-triggered behaviour
		{"mpi", [](PC_tree_t, Names&) {}},
		{"serialize", [](PC_tree_t, Names&) {}},
		{"json", [](PC_tree_t, Names&) {}},
		// listens to every event, but only for logging: does not widen `auto'
		{"trace", [](PC_tree_t, Names&) {}},
	};
	return table;
}

} // anonymous namespace

Fwd_events_cfg parse_fwd_events(PC_tree_t node)
{
	// Missing optional keys must yield PC_NODE_NOT_FOUND, not abort the process.
	PDI::Paraconf_wrapper silence_paraconf_errors;
	Fwd_events_cfg cfg;
	if (!exists(node)) return cfg;

	auto read_list = [](PC_tree_t list) {
		std::vector<std::string> names;
		PDI::each(list, [&](PC_tree_t item) { names.emplace_back(PDI::to_string(item)); });
		return names;
	};

	if (PDI::is_scalar(node)) {
		const std::string value = PDI::to_string(node);
		if (value == "auto") {
			cfg.mode = Fwd_events_cfg::Mode::Auto;
		} else if (value == "all") {
			cfg.mode = Fwd_events_cfg::Mode::All;
		} else {
			throw PDI::Spectree_error{node, "fwd_events: expected `auto', `all', a list of event names or {{auto, extra}}, got `{}'", value};
		}
	} else if (PDI::is_list(node)) {
		cfg.mode = Fwd_events_cfg::Mode::List;
		cfg.extra = read_list(node);
	} else if (PDI::is_map(node)) {
		bool use_auto = true;
		PDI::each(node, [&](PC_tree_t key_tree, PC_tree_t value) {
			const std::string key = PDI::to_string(key_tree);
			if (key == "auto") {
				const std::string flag = PDI::to_string(value);
				if (flag == "true" || flag == "yes" || flag == "1") {
					use_auto = true;
				} else if (flag == "false" || flag == "no" || flag == "0") {
					use_auto = false;
				} else {
					throw PDI::Spectree_error{value, "fwd_events.auto: expected true or false, got `{}'", flag};
				}
			} else if (key == "extra") {
				cfg.extra = PDI::is_list(value) ? read_list(value) : std::vector<std::string>{PDI::to_string(value)};
			} else {
				throw PDI::Spectree_error{key_tree, "fwd_events: unknown key `{}' (expected `auto' or `extra')", key};
			}
		});
		cfg.mode = use_auto ? Fwd_events_cfg::Mode::Auto : Fwd_events_cfg::Mode::List;
	}
	return cfg;
}

Discovered_events discover_plugin_events(PC_tree_t plugins)
{
	// Missing optional keys must yield PC_NODE_NOT_FOUND, not abort the process.
	PDI::Paraconf_wrapper silence_paraconf_errors;
	Discovered_events result;
	if (!exists(plugins) || !PDI::is_map(plugins)) return result;

	const auto& table = plugin_event_table();
	PDI::each(plugins, [&](PC_tree_t name_tree, PC_tree_t plugin_cfg) {
		const std::string plugin = PDI::to_string(name_tree);
		auto it = table.find(plugin);
		if (it != table.end()) {
			it->second(plugin_cfg, result.names);
			if (plugin == "trace") {
				result.notes.emplace_back("plugin `trace' listens to every event; use `fwd_events: all' to forward them all");
			}
		} else {
			// Unknown plugin: try the common `on_event' convention at top level and in list items.
			PDI::opt_each(plugin_cfg, [&](PC_tree_t item) { add_on_event(PC_get(item, ".on_event"), result.names); });
			result.warnings.emplace_back(
				"plugin `" + plugin + "' is not in the plugin <-> event table: only its `on_event' key was scanned; "
				"add missing events with `fwd_events: {auto: true, extra: [...]}'"
			);
		}
	});
	return result;
}

} // namespace damaris_pdi
