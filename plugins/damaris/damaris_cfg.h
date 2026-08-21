/*******************************************************************************
 * Copyright (C) 2015-2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2024-2026 National Institute for Research in Digital Science and Technology (Inria)
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

#ifndef DAMARIS_CFG_H_
#define DAMARIS_CFG_H_

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <pdi/pdi_fwd.h>
#include <pdi/context.h>
#include <pdi/expression.h>

// Definitions of the Damaris XML tag generators
#include <Damaris.h>
#include <damaris/model/ModifyModel.hpp>
#include <damaris/util/DamarisVar.hpp>
#include <damaris/interoperability/pdi/PdiProtocol.hpp>   // MessageType, TypeDescriptor, WireFormat

#include "damaris_wrapper.h"

namespace damaris_pdi {

typedef struct placement {
	int start;
	int stride;
	int blocksize;
	std::vector<int> mask{}; //std::vector<char> mask {} ???
} placement;

typedef struct dedicated {
	int core;
	int node;
} dedicated;

struct Architecture_type {
	int domain;
	placement arch_placement;
	dedicated arch_cores_or_nodes;
};

enum class Desc_type {
	DATA_TO_WRITE,
	DATA_TO_WRITE_WITH_BLOCK
		//,DATA_TO_READ
		,
	PRM_REQUIRED_METADATA,
	PRM_TO_GET,
	PRM_TO_SET,
	IS_CLIENT_GET,
	CLIENT_COMM_GET
};

enum class Event_type {
	DAMARIS_INITIALIZE = 0,
	DAMARIS_START = 1,
	DAMARIS_SET_POSITION = 2,
	DAMARIS_SET_BLOCK_POSITION = 3,
	DAMARIS_WRITE = 4,
	DAMARIS_WRITE_BLOCK = 5,
	DAMARIS_CLIENT_COMM_GET = 6,
	DAMARIS_PARAMETER_SET = 7,
	DAMARIS_PARAMETER_GET = 8,
	DAMARIS_END_ITERATION = 9,
	DAMARIS_GET_ITERATION = 10,
	DAMARIS_SIGNAL = 11,
	DAMARIS_BIND = 12,
	DAMARIS_STOP = 13,
	DAMARIS_FINALIZE = 14
};

/** Map Event_type to its default event name string.
 *  Returns a string literal (static storage duration, no destructor) so this is safe
 *  to call from any destructor regardless of DSO finalization order. */
inline const char* event_name_for(Event_type event_type)
{
    switch (event_type) {
        case Event_type::DAMARIS_INITIALIZE:         return "initialize";
        case Event_type::DAMARIS_START:              return "damaris_start";
        case Event_type::DAMARIS_SET_POSITION:       return "damaris_set_position";
        case Event_type::DAMARIS_SET_BLOCK_POSITION: return "damaris_set_block_position";
        case Event_type::DAMARIS_WRITE:              return "damaris_write";
        case Event_type::DAMARIS_WRITE_BLOCK:        return "damaris_write_block";
        case Event_type::DAMARIS_CLIENT_COMM_GET:    return "damaris_client_comm_get";
        case Event_type::DAMARIS_PARAMETER_SET:      return "damaris_parameter_set";
        case Event_type::DAMARIS_PARAMETER_GET:      return "damaris_parameter_get";
        case Event_type::DAMARIS_END_ITERATION:      return "damaris_end_iteration";
        case Event_type::DAMARIS_GET_ITERATION:      return "damaris_get_iteration";
        case Event_type::DAMARIS_SIGNAL:             return "damaris_signal";
        case Event_type::DAMARIS_BIND:               return "damaris_bind";
        case Event_type::DAMARIS_STOP:               return "damaris_stop";
        case Event_type::DAMARIS_FINALIZE:           return "finalize";
    }
    throw std::out_of_range("unknown Event_type in event_name_for");
}

struct Dataset_Write_Info {
	PDI::Expression when = "1"; //By default, always write as long as there are iteration going on
	/*int64_t* */ PDI::Expression position[3] = {"0", "0", "0"}; //Max Dim is 3
	/*int32_t */ PDI::Expression block = "0"; //when domain = 1, which is the default behaviour
	std::string dataset_name;
};

struct DamarisXMLGenerators {
	damaris::model::DamarisParameterXML params_; // Layouts are typically descrived using parameters. Parameters can be shared between layouts.
	damaris::model::DamarisVarXML variable_; // A variable
	damaris::model::DamarisLayoutXML layout_; // each variable has one layout (layouts can be sahred)
	damaris::model::DamarisMeshXML mesh_; // Contains extra elements that need to be part of a Damaris <mesh> for visualization
	damaris::model::DamarisStoreXML store_; // Contains the extra elements that need to be part of a Damaris <store> (for HDF5 file w/r)
};

//CONST for PDI plugin of Damaris
struct PDI_Common_Event_Names {
    static constexpr const char* ON_INITIALIZE  = "on_initialize";
    static constexpr const char* ON_EVENT       = "on_event";
	//...
    static constexpr const char* ON_FINALIZE    = "on_finalize";
};

// std::unordered_map<std::string, std::vector<std::string>> m_plugins_custom_events =
// {
//     { PDI_Common_Event_Names::ON_INITIALIZE, {} },
//     { PDI_Common_Event_Names::ON_EVENT,      { "decl_hdf5", "user_code" } },
// 	//...
//     { PDI_Common_Event_Names::ON_FINALIZE,   {}                         },
// };
extern std::unordered_map<std::string, std::vector<std::string>> m_plugins_custom_events;


class Damaris_cfg
{
	std::string m_xml_config_object;
	damaris::model::ModifyModel damarisXMLModifyModel;

	std::string m_init_on_event = "";
	std::string m_start_on_event = "";
	std::string m_stop_on_event = "";
	std::string m_end_iteration_on_event = "";
	std::string m_finalize_on_event = "";


	int m_arch_domains = 1;
	int m_dc_cores_pernode;
	int m_dc_nodes;


	int m_placement_start;
	int m_placement_stride;
	int m_placement_blocksize;
	std::string m_placement_mask_str;

	PDI::Expression m_communicator;

	std::unordered_map<std::string, damaris::model::DamarisVarXML> m_datasets;
	std::unordered_map<std::string, damaris::model::DamarisLayoutXML> m_layouts;
	std::unordered_map<std::string, damaris::model::DamarisParameterXML> m_parameters;
	std::unordered_map<std::string, damaris::model::DamarisStoreXML> m_storages;
	std::unordered_map<std::string, damaris::model::DamarisGroupXML> m_groups;
	std::unordered_map<std::string, damaris::model::DamarisPluginXML> m_damaris_plugins;

	std::unordered_map<std::string, Desc_type> m_descs;
	std::unordered_map<std::string, Event_type> m_events;
	std::unordered_map<std::string, Dataset_Write_Info> m_datasets_to_write;
	std::list<std::string> m_after_write_events;
	//<metadata_name, <parameter_name, PRM_TO_GET/PRM_TO_SET>>
	std::unordered_map<std::string, std::pair<std::string, Desc_type>> m_parameter_to_update;

	//In damaris parameter play similar role than metadata in PDI, in that other variable can be expressed by them
	//  Here we express parameter in terms of PDI metadata, and when metadata are expose,
	//      we need to be awards in order to update parameter value for Damaris
	//   This variable is intended for that
	std::unordered_map<std::string, std::unordered_map<std::string, bool>> m_parameter_depends_on;
	std::unordered_map<std::string, PDI::Expression> m_parameter_expression;
	std::unordered_map<std::string, std::unordered_map<std::string, bool>> m_layout_depends_on;
	std::unordered_map<std::string, PDI::Expression> m_layout_expression;

	static std::string m_is_client_dataset_name;
	static std::string m_client_comm_get_dataset_name;

	//PDI plugin of Damaris
	/// <desc_name, is_metadata>
	std::unordered_map<std::string, bool> m_pdi_plugin_descs; 
	/// <configured_event_name> is used as customed event, configured in plugin config sections.
	std::vector<std::string> m_pdi_plugin_configured_event_names;
	size_t m_pdi_generic_layout_size = 65536;//To be set to the max size of forwarded data
	damaris::interoperability::pdi::ForwardingMode m_pdi_forwarding_mode;
	std::string m_pdi_damaris_generic_channel = "";

	const std::string XML_CONFIG_TEMPLATE = R"V0G0N(<?xml version="1.0"?>
 <simulation name="_SIM_NAME_" language="c" xmlns="http://damaris.gforge.inria.fr/damaris/model">
     <architecture>
         <domains count="_DOMAINS_REGEX_"/>
         <dedicated cores="_DC_REGEX_" nodes="_DN_REGEX_"/>
         <buffer name="_SHMEM_NAME_" size="_SHMEM_BUFFER_BYTES_REGEX_" />
         <placement />
         <queue  name="queue" size="300" />
     </architecture>
 
     <data>
         _DATASET_ELEMENT_REGEX_
     </data>
 
     <storage>
         _STORAGE_ELEMENT_REGEX_        
     </storage>
  
     <actions>
     </actions>
 
     <scripts>
     _SCRIPTS_REGEX_
     </scripts>
 
    <plugins>
     _PLUGINS_REGEX_
    </plugins>

     <log FileName="./log/_SIM_LOG_NAME_" RotationSize="_LOG_ROTATION_SIZE_" LogFormat="[%TimeStamp%]: %Message%"  Flush="_LOG_FLUSH_"  LogLevel="_LOG_LEVEL_" />
     
 </simulation>)V0G0N";


protected:
	void parse_architecture_tree(PDI::Context& ctx, PC_tree_t arch_tree);
	void parse_parameters_tree(PDI::Context& ctx, PC_tree_t parameters_tree_list);
	void parse_datasets_tree(PDI::Context& ctx, PC_tree_t datasets_tree_list);
	void parse_layouts_tree(PDI::Context& ctx, PC_tree_t layouts_tree_list);
	void parse_storages_tree(PDI::Context& ctx, PC_tree_t storages_tree_list);
	void parse_write_tree(PDI::Context& ctx, PC_tree_t write_tree_list);
	void parse_pdi_plugin_cfg_tree(PDI::Context& ctx, PC_tree_t pdi_cfg_tree, const std::string& pdi_yaml_path = "");
	void parse_parameter_to_update_tree(PDI::Context& ctx, PC_tree_t ptu_tree_list, Desc_type op_type);
	void parse_log_tree(PDI::Context& ctx, PC_tree_t config);

	void init_xml_config_object()
	{
		m_xml_config_object = XML_CONFIG_TEMPLATE;
		damarisXMLModifyModel = damaris::model::ModifyModel(m_xml_config_object);
	}


public:
	Damaris_cfg(PDI::Context& ctx, PC_tree_t tree);

	const std::string& xml_config_object(void);

	PDI::Expression communicator() const;

	const std::unordered_map<std::string, damaris::model::DamarisVarXML>& datasets() const;
	const std::unordered_map<std::string, damaris::model::DamarisLayoutXML>& layouts() const;
	const std::unordered_map<std::string, damaris::model::DamarisParameterXML>& parameters() const;
	const damaris::model::DamarisParameterXML get_parameter_xml(std::string prm_name) const;
	const std::unordered_map<std::string, damaris::model::DamarisStoreXML>& storages() const;
	const std::unordered_map<std::string, damaris::model::DamarisGroupXML>& groups() const;

	const std::unordered_map<std::string, Desc_type>& descs() const;
	const std::unordered_map<std::string, Event_type>& events() const;
	const std::unordered_map<std::string, bool> descs_to_forward() const;
	const std::vector<std::string>& events_to_forward() const;
	const std::unordered_map<std::string, Dataset_Write_Info>& datasets_to_write() const;
	const std::unordered_map<std::string, std::pair<std::string, Desc_type>>& parameter_to_update() const;

	Dataset_Write_Info get_dataset_write_info(std::string data_name) const;
	std::pair<std::string, Desc_type> get_parameter_to_update_info(std::string data_name) const;

	std::list<std::string> get_after_write_events() const { return m_after_write_events; }

	bool is_there_after_write_events() const { return m_after_write_events.size(); }

	static std::string is_client_dataset_name() { return m_is_client_dataset_name; }

	static std::string client_comm_get_dataset_name() { return m_client_comm_get_dataset_name; }

	std::string init_on_event() const;
	std::string start_on_event() const;
	std::string stop_on_event() const;
	std::string end_iteration_on_event() const;
	PC_tree_t   resolve_config(PC_tree_t node, std::string section);
	std::string finalize_on_event() const;

	// const std::unordered_map<std::string, std::unordered_set<int>>& recover_var() const;

	// const std::unordered_map<std::string, std::set<std::tuple<PDI::Expression, PDI::Expression, std::string>>>& send_file() const;


	bool is_dataset_to_write(std::string data_name);
	bool is_parameter_to_update(std::string data_name);
	bool is_needed_metadata(std::string data_name);

	std::unordered_map<std::string, std::pair<std::string, std::string>> get_updatable_parameters(PDI::Context& ctx);

	void reset_parameter_depends_on(std::string prm_name);
	void reset_parameter_depends_on(std::vector<std::string> prm_list);

	void reset_all_parameters_depends_on();

	// ---------------------------------------------------------------
    // Signature assumed from usage in BatchedPdiForwardingStrategy / ImmediatePdiForwardingStrategy
    // (both call it as: int32_t available_blocks = m_config_ref.get_arch_domains_count();)
    // ---------------------------------------------------------------
    int32_t get_arch_domains_count() const;

    // ---------------------------------------------------------------
    // TO ADD -- byte size of the pdi_generic_channel layout
    // (e.g. <layout name="pdi_generic_layout" type="char" dimensions="65536"/>
    //  => 65536, since type="char" means 1 byte per element)
    // ---------------------------------------------------------------
    size_t get_generic_channel_layout_size() const;

    // ---------------------------------------------------------------
    // TO ADD -- reads <forwarding_mode value="batched|immediate"/> from
    // the PDI plugin's XML section. Defaults to Batched if the parameter
    // is absent (safe default: lower footprint on the global domains
    // budget, matches the mode already validated first).
    // ---------------------------------------------------------------
    damaris::interoperability::pdi::ForwardingMode get_pdi_forwarding_mode() const;

	// ---------------------------------------------------------------------
	// get_generic_channel_name()
	//
	// Reads <generic_channel_name value="..."/> from the PDI plugin's own
	// XML section (same section as <pdi_cfg_yaml_path>, cf. PdiPlugin::parseXML
	// for the exact DOM traversal pattern already used for that parameter).
	// Falls back to kDefaultGenericChannelName if the parameter is absent, so
	// existing configs that don't set it explicitly keep working unchanged.
	//
	// IMPORTANT: PdiPlugin's own equivalent accessor (server side) MUST parse
	// this exact same XML parameter -- never hardcode "pdi_generic_channel"
	// as a literal on either side, and never let the two sides drift apart
	// by using two different default values.
	// ---------------------------------------------------------------------
	std::string get_generic_channel_name() const;
}; // class Damaris_cfg

} // namespace damaris_pdi

#endif // DAMARIS_CFG_H_
