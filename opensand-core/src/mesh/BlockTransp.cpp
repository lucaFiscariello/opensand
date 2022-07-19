/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file BlockTransp.cpp
 * @brief
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#include "BlockTransp.h"

#include "OpenSandModelConf.h"
#include <opensand_rt/MessageEvent.h>

constexpr uint8_t DATA_IN_GW_ID = 8;
constexpr uint8_t CTRL_IN_GW_ID = 4;

/**
 * @brief Returns whether the message should be sent to gateway or terminals
 */
Component getDestinationType(bool mesh_mode, tal_id_t src_id, tal_id_t dest_id, std::shared_ptr<OutputLog> log)
{
	Component dest;
	if (mesh_mode)
	{
		if (dest_id == BROADCAST_TAL_ID)
		{
			dest = Component::terminal;
		}
		else
		{
			dest = OpenSandModelConf::Get()->getEntityType(dest_id);
			if (dest == Component::unknown || dest == Component::satellite)
			{
				LOG(log, LEVEL_ERROR, "The type of the dest entity %d is %s", dest_id, getComponentName(dest).c_str());
				return Component::unknown;
			}
		}
	}
	else // star mode
	{
		Component src = OpenSandModelConf::Get()->getEntityType(src_id);
		if (src == Component::gateway)
		{
			dest = Component::terminal;
		}
		else if (src == Component::terminal)
		{
			dest = Component::gateway;
		}
		else
		{
			LOG(log, LEVEL_ERROR, "The type of the src entity %d is %s", src_id, getComponentName(src).c_str());
			return Component::unknown;
		}
	}
	return dest;
}

BlockTransp::BlockTransp(const std::string &name, TranspConfig transp_config):
    Block(name),
    entity_id{transp_config.entity_id},
    isl_enabled{transp_config.isl_enabled} {}

bool BlockTransp::onInit()
{
	const auto conf = OpenSandModelConf::Get();
	auto downward = dynamic_cast<Downward *>(this->downward);
	auto upward = dynamic_cast<Upward *>(this->upward);

	std::unordered_map<SpotComponentPair, tal_id_t> routes;
	std::unordered_map<tal_id_t, spot_id_t> spot_by_entity;
	for (auto &&spot: conf->getSpotsTopology())
	{
		const SpotTopology &topo = spot.second;

		spot_by_entity[topo.gw_id] = topo.spot_id;
		for (tal_id_t tal_id: topo.st_ids)
		{
			spot_by_entity[tal_id] = topo.spot_id;
		}

		routes[{topo.spot_id, Component::gateway}] = topo.sat_id_gw;
		routes[{topo.spot_id, Component::terminal}] = topo.sat_id_st;

		// Check that ISL are enabled when they should be
		if (topo.sat_id_gw != topo.sat_id_st &&
		    (topo.sat_id_gw == entity_id || topo.sat_id_st == entity_id) &&
		    !isl_enabled)
		{
			LOG(log_init, LEVEL_ERROR,
			    "The gateway of the spot %d is connected to sat %d and the "
			    "terminals are connected to sat %d, but no ISL is configured on sat %d",
			    topo.spot_id, topo.sat_id_gw, topo.sat_id_st, entity_id);
			return false;
		}
	}
	upward->routes = routes;
	downward->routes = routes;
	upward->spot_by_entity = spot_by_entity;
	downward->spot_by_entity = spot_by_entity;
	return true;
}

BlockTransp::Upward::Upward(const std::string &name, TranspConfig transp_config):
    RtUpwardMux(name),
    entity_id{transp_config.entity_id}
{
	mesh_mode = OpenSandModelConf::Get()->isMeshArchitecture();
}

bool BlockTransp::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);
	switch (to_enum<InternalMessageType>(msg_event->getMessageType()))
	{
		// sent by SatCarrier
		case InternalMessageType::unknown:
		case InternalMessageType::sig:
		case InternalMessageType::encap_data:
		{
			auto frame = static_cast<DvbFrame *>(msg_event->getData());
			return handleDvbFrame(std::unique_ptr<DvbFrame>(frame));
		}
		// sent by Encap
		case InternalMessageType::decap_data:
		{
			auto burst = static_cast<NetBurst *>(msg_event->getData());
			return handleNetBurst(std::unique_ptr<NetBurst>(burst));
		}
		case InternalMessageType::link_up:
			// ignore
			return true;
		default:
			LOG(log_receive, LEVEL_ERROR,
			    "Unexpected message type received: %d",
			    msg_event->getMessageType());
			return false;
	}
}

bool BlockTransp::Upward::handleDvbFrame(std::unique_ptr<DvbFrame> frame)
{
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const uint8_t id = carrier_id % 10;
	const auto msg_type = id >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const log_level_t log_level = id >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const Component dest = (id == CTRL_IN_GW_ID || id == DATA_IN_GW_ID) ? Component::terminal : Component::gateway;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		return sendToOppositeChannel(std::move(frame), msg_type);
	}
	else
	{
		// send by ISL
		return sendToUpperBlock(std::move(frame), msg_type);
	}
}

bool BlockTransp::Upward::handleNetBurst(std::unique_ptr<NetBurst> burst)
{
	if (burst->empty())
		return true;

	NetPacket &msg = *burst->front();

	const auto dest_id = msg.getDstTalId();
	const auto src_id = msg.getSrcTalId();
	const spot_id_t spot_id = spot_by_entity[src_id];
	LOG(log_receive, LEVEL_INFO, "Received a NetBurst (%d->%d, spot_id %d)", src_id, dest_id, spot_id);

	Component dest = getDestinationType(mesh_mode, src_id, dest_id, log_receive);
	if (dest == Component::unknown)
		return false;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		return sendToOppositeChannel(std::move(burst), InternalMessageType::decap_data);
	}
	else
	{
		// send by ISL
		return sendToUpperBlock(std::move(burst), InternalMessageType::decap_data);
	}
}

BlockTransp::Downward::Downward(const std::string &name, TranspConfig transp_config):
    RtDownwardDemux<SpotComponentPair>(name),
    entity_id{transp_config.entity_id}
{
	mesh_mode = OpenSandModelConf::Get()->isMeshArchitecture();
}

bool BlockTransp::Downward::onEvent(const RtEvent *const event)
{
	if (event->getType() != EventType::Message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);

	switch (to_enum<InternalMessageType>(msg_event->getMessageType()))
	{
		// sent by SatCarrier
		case InternalMessageType::unknown:
		case InternalMessageType::sig:
		case InternalMessageType::encap_data:
		{
			auto frame = static_cast<DvbFrame *>(msg_event->getData());
			return handleDvbFrame(std::unique_ptr<DvbFrame>(frame));
		}
		// sent by Encap
		case InternalMessageType::decap_data:
		{
			auto burst = static_cast<NetBurst *>(msg_event->getData());
			return handleNetBurst(std::unique_ptr<NetBurst>(burst));
		}
		case InternalMessageType::link_up:
			// ignore
			return true;
		default:
			LOG(log_receive, LEVEL_ERROR,
			    "Unexpected message type received: %d",
			    msg_event->getMessageType());
			return false;
	}
}

bool BlockTransp::Downward::handleDvbFrame(std::unique_ptr<DvbFrame> frame)
{
	const spot_id_t spot_id = frame->getSpot();
	const uint8_t carrier_id = frame->getCarrierId();
	const uint8_t id = carrier_id % 10;
	const auto msg_type = id >= 6 ? InternalMessageType::encap_data : InternalMessageType::sig;
	const log_level_t log_level = id >= 6 ? LEVEL_INFO : LEVEL_DEBUG;
	LOG(log_receive, log_level, "Received a DvbFrame (spot_id %d, carrier id %d, msg type %d)",
	    spot_id, carrier_id, frame->getMessageType());

	const Component dest = (id == CTRL_IN_GW_ID || id == DATA_IN_GW_ID) ? Component::terminal : Component::gateway;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		if (id % 2 != 0)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "Received a message from an output carried id (%d)", carrier_id);
			return false;
		}

		// add one to the input carrier id to get the corresponding output carrier id
		frame->setCarrierId(carrier_id + 1);
		return sendToLowerBlock({spot_id, dest}, std::move(frame), msg_type);
	}
	else
	{
		// send by ISL
		return sendToOppositeChannel(std::move(frame), msg_type);
	}
}

bool BlockTransp::Downward::handleNetBurst(std::unique_ptr<NetBurst> burst)
{
	if (burst->empty())
		return true;

	NetPacket &msg = *burst->front();

	const auto dest_id = msg.getDstTalId();
	const auto src_id = msg.getSrcTalId();
	const spot_id_t spot_id = spot_by_entity[src_id];
	LOG(log_receive, LEVEL_INFO, "Received a NetBurst (%d->%d, spot_id %d)", src_id, dest_id, spot_id);

	Component dest = getDestinationType(mesh_mode, src_id, dest_id, log_receive);
	if (dest == Component::unknown)
		return false;

	const auto dest_sat_id_it = routes.find({spot_id, dest});
	if (dest_sat_id_it == routes.end())
	{
		LOG(log_receive, LEVEL_ERROR, "No route found for %s in spot %d",
		    dest == Component::gateway ? "GW" : "ST", spot_id);
		return false;
	}
	const tal_id_t dest_sat_id = dest_sat_id_it->second;

	if (dest_sat_id == entity_id)
	{
		return sendToLowerBlock({spot_id, dest}, std::move(burst), InternalMessageType::decap_data);
	}
	else
	{
		// send by ISL
		return sendToOppositeChannel(std::move(burst), InternalMessageType::decap_data);
	}
}
