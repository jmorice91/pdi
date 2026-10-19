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

#include <set>
#include <string>

#include <gtest/gtest.h>
#include <paraconf.h>
#include <pdi/error.h>
#include <pdi/paraconf_wrapper.h>

#include "../damaris_pdi_event_discovery.h"

using namespace damaris_pdi;
using Names = std::set<std::string>;

namespace {

class Yaml
{
	PDI::Paraconf_wrapper m_silence_paraconf_errors; // as inside a PDI plugin: absent keys don't abort
	PC_tree_t m_tree;

public:
	explicit Yaml(const char* text)
		: m_tree{PC_parse_string(text)}
	{}

	~Yaml() { PC_tree_destroy(&m_tree); }

	PC_tree_t get(const char* path) const { return PC_get(m_tree, path); }
};

Names discover(const char* yaml)
{
	Yaml y{yaml};
	return discover_plugin_events(y.get(".plugins")).names;
}

} // namespace

// Plugin sections taken from the exadost_workflows server configs.

TEST(EventDiscovery, DeclHdf5WritesOnDataShareHaveNoEvent)
{
	EXPECT_EQ(discover(R"(
plugins:
  mpi:
  trace: ~
  decl_hdf5:
    file: asy_data_2.h5
    write:
      main_field:
        dataset_selection: { start: [$iter, 0, 0] }
)"), Names{});
}

TEST(EventDiscovery, DeclHdf5FileLevelOnEventInList)
{
	EXPECT_EQ(discover(R"(
plugins:
  decl_hdf5:
    - file: a.h5
      on_event: [damaris_server_write]
      write: [x]
    - file: b.h5
      on_event: other_event
      write: { y: { dataset_selection: { start: [1] } } }
)"), (Names{"damaris_server_write", "other_event"}));
}

TEST(EventDiscovery, VelocManagedAndCustom)
{
	EXPECT_EQ(discover(R"(
plugins:
  veloc:
    failure: 0
    managed_checkpointing:
      protect_data: [iter, main_field]
      synchronize_on: newiter
      when: '$iter % 5 = 0'
    custom_checkpointing:
      custom_checkpoint: { start_on: cp_start, end_on: cp_end, route_file_on: cp_route }
      custom_recover: { start_on: rec_start }
)"), (Names{"newiter", "cp_start", "cp_end", "cp_route", "rec_start"}));
}

TEST(EventDiscovery, PycallMapKeys)
{
	EXPECT_EQ(discover(R"(
plugins:
  pycall:
    on_event:
      newiter: { with: { i: $iter }, exec: "pass" }
      main_loop: { exec: "pass" }
    on_data:
      main_field: "pass"
)"), (Names{"newiter", "main_loop"}));
}

TEST(EventDiscovery, UserCodeMapAndListOfMaps)
{
	EXPECT_EQ(discover("plugins: { user_code: { on_event: { ev_a: { f: {} } } } }"), Names{"ev_a"});
	EXPECT_EQ(discover("plugins: { user_code: { on_event: [ { ev_b: { f: {} } }, { ev_c: { g: {} } } ] } }"), (Names{"ev_b", "ev_c"}));
}

TEST(EventDiscovery, SetValueOnlyOnEvent)
{
	EXPECT_EQ(discover(R"(
plugins:
  set_value:
    on_init: [ { set: [ { x: 1 } ] } ]
    on_event: { tick: [ { set: [ { x: 2 } ] } ] }
    on_data: { y: [ { set: [ { x: 3 } ] } ] }
    on_finalize: [ { set: [ { x: 4 } ] } ]
)"), Names{"tick"});
}

TEST(EventDiscovery, TimerStartStop)
{
	EXPECT_EQ(discover(R"(
plugins:
  timer:
    - output_to: timings.txt
    - loop_timer: { start: loop_begin, stop: loop_end }
    - io_timer: [ { start: io_begin, stop: io_end } ]
)"), (Names{"loop_begin", "loop_end", "io_begin", "io_end"}));
}

TEST(EventDiscovery, TraceDoesNotWidenButIsNoted)
{
	Yaml y{"plugins: { trace: ~ }"};
	Discovered_events d = discover_plugin_events(y.get(".plugins"));
	EXPECT_TRUE(d.names.empty());
	EXPECT_TRUE(d.warnings.empty());
	EXPECT_EQ(d.notes.size(), 1u);
}

TEST(EventDiscovery, UnknownPluginFallsBackToOnEventWithWarning)
{
	Yaml y{"plugins: { deisa: { on_event: [ts_start] }, my_plugin: { on_event: { custom: {} } } }"};
	Discovered_events d = discover_plugin_events(y.get(".plugins"));
	EXPECT_EQ(d.names, (Names{"ts_start", "custom"}));
	EXPECT_EQ(d.warnings.size(), 2u);
}

TEST(EventDiscovery, CombinedServerConfig)
{
	// pdi_veloc_deisa_server.yml-like: VeloC + pycall on the same event
	EXPECT_EQ(discover(R"(
plugins:
  mpi:
  veloc: { managed_checkpointing: { synchronize_on: newiter } }
  pycall: { on_event: { newiter: { exec: "pass" } } }
)"), Names{"newiter"});
}

TEST(FwdEvents, Modes)
{
	Yaml y{R"(
absent_parent: {}
s_auto: auto
s_all: all
list: [a, b]
map_extra: { extra: [c] }
map_noauto: { auto: false, extra: [d] }
bad: everything
bad_key: { auto: true, more: [x] }
)"};
	EXPECT_EQ(parse_fwd_events(y.get(".absent_parent.fwd_events")).mode, Fwd_events_cfg::Mode::Auto);
	EXPECT_EQ(parse_fwd_events(y.get(".s_auto")).mode, Fwd_events_cfg::Mode::Auto);
	EXPECT_EQ(parse_fwd_events(y.get(".s_all")).mode, Fwd_events_cfg::Mode::All);

	Fwd_events_cfg list = parse_fwd_events(y.get(".list"));
	EXPECT_EQ(list.mode, Fwd_events_cfg::Mode::List);
	EXPECT_EQ(list.extra, (std::vector<std::string>{"a", "b"}));

	Fwd_events_cfg extra = parse_fwd_events(y.get(".map_extra"));
	EXPECT_EQ(extra.mode, Fwd_events_cfg::Mode::Auto);
	EXPECT_EQ(extra.extra, std::vector<std::string>{"c"});

	Fwd_events_cfg noauto = parse_fwd_events(y.get(".map_noauto"));
	EXPECT_EQ(noauto.mode, Fwd_events_cfg::Mode::List);
	EXPECT_EQ(noauto.extra, std::vector<std::string>{"d"});

	EXPECT_THROW(parse_fwd_events(y.get(".bad")), PDI::Error);
	EXPECT_THROW(parse_fwd_events(y.get(".bad_key")), PDI::Error);
}
