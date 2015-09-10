/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file SatGw.cpp
 * @brief This bloc implements a satellite spots
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "SatGw.h"
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include "ForwardSchedulingS2.h"
#include "DvbRcsFrame.h"

#include <opensand_output/Output.h>

#include <stdlib.h>

// TODO size per fifo ?
// TODO to not do...: do not create all fifo in the regenerative case
SatGw::SatGw(tal_id_t gw_id,
             spot_id_t spot_id,
             uint8_t log_id,
             uint8_t ctrl_id,
             uint8_t data_in_st_id,
             uint8_t data_in_gw_id,
             uint8_t data_out_st_id,
             uint8_t data_out_gw_id,
             size_t fifo_size):
	gw_id(gw_id),
	spot_id(spot_id),
	data_in_st_id(data_in_st_id),
	data_in_gw_id(data_in_gw_id),
	complete_st_dvb_frames(),
	complete_gw_dvb_frames(),
	st_scheduling(NULL),
	gw_scheduling(NULL),
	l2_from_st_bytes(0),
	l2_from_gw_bytes(0),
	gw_mutex("GW"),
	probe_sat_output_gw_queue_size(),
	probe_sat_output_gw_queue_size_kb(),
	probe_sat_output_st_queue_size(),
	probe_sat_output_st_queue_size_kb(),
	probe_sat_l2_from_st(),
	probe_sat_l2_to_st(),
	probe_sat_l2_from_gw(),
	probe_sat_l2_to_gw()
{
	// initialize MAC FIFOs
#define SIG_FIFO_SIZE 1000
	this->logon_fifo = new DvbFifo(log_id, SIG_FIFO_SIZE, "logon_fifo");
	this->control_fifo = new DvbFifo(ctrl_id, SIG_FIFO_SIZE, "control_fifo");
	this->data_out_st_fifo = new DvbFifo(data_out_st_id, fifo_size,
	                                     "data_out_st");
	this->data_out_gw_fifo = new DvbFifo(data_out_gw_id, fifo_size,
	                                     "data_out_gw");
	// Output Log
	this->log_init = Output::registerLog(LEVEL_WARNING, 
	                                     "Dvb.init.spot_%d.gw_%d",
	                                     this->spot_id, this->gw_id);
	this->log_receive = Output::registerLog(LEVEL_WARNING, 
	                                     "Dvb.receive.spot_%d.gw_%d",
	                                     this->spot_id, this->gw_id);
	this->input_sts = new StFmtSimuList();
	this->output_sts = new StFmtSimuList();
}

SatGw::~SatGw()
{
	this->complete_st_dvb_frames.clear();
	this->complete_gw_dvb_frames.clear();

	// remove scheduling (only for regenerative satellite)
	if(st_scheduling)
		delete this->st_scheduling;
	if(gw_scheduling)
		delete this->gw_scheduling;

	delete this->logon_fifo;
	delete this->control_fifo;
	delete this->data_out_st_fifo;
	delete this->data_out_gw_fifo;
}

bool SatGw::initScheduling(time_ms_t fwd_timer_ms,
                           const EncapPlugin::EncapPacketHandler *pkt_hdl,
                           const TerminalCategoryDama *const st_category,
                           const TerminalCategoryDama *const gw_category)
{
	fifos_t st_fifos;
	st_fifos[this->data_out_st_fifo->getCarrierId()] = this->data_out_st_fifo;
	fifos_t gw_fifos;
	gw_fifos[this->data_out_gw_fifo->getCarrierId()] = this->data_out_gw_fifo;
	this->st_scheduling = new ForwardSchedulingS2(fwd_timer_ms,
	                                              pkt_hdl,
	                                              st_fifos,
	                                              this->output_sts->getListSts(),
	                                              this->output_modcod_def,
	                                              st_category,
	                                              this->spot_id,
	                                              false,
	                                              this->gw_id,
	                                              "ST");
	if(!this->st_scheduling)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "cannot create down ST scheduling for spot %u\n",
		    this->spot_id);
		return false;
	}
	this->gw_scheduling = new ForwardSchedulingS2(fwd_timer_ms,
	                                              pkt_hdl,
	                                              gw_fifos,
	                                              this->output_sts->getListSts(),
	                                              this->output_modcod_def,
	                                              gw_category,
	                                              this->spot_id,
	                                              false,
	                                              this->gw_id,
	                                              "GW");
	if(!this->gw_scheduling)
	{
		LOG(this->log_init, LEVEL_ERROR, 
		    "cannot create down GW scheduling for spot %u\n",
		    this->spot_id);
		return false;
	}
	return true;
}


bool SatGw::initModcodSimu(void)
{
	// Retrieve the value of the ‘enable’ parameter for the physical layer
	if(!this->initFmt())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize fmt\n");
		return false;
	}
	
	if(!this->initModcodSimuFile(RETURN_UP_MODCOD_TIME_SERIES,
	                             this->gw_id, this->spot_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the modcod part of the "
		    "initialisation\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS,
	                            &this->input_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the modcod part of the "
		    "initialisation\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->output_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the modcod part of the "
		    "initialisation\n");
		return false;
	}
	
	if(!this->goFirstScenarioStep())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize downlink MODCOD IDs\n");
		return false;
	}
	
	if(!this->addTerminal(this->gw_id))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to register simulated GW with MAC "
		    "ID %u\n", this->gw_id);
		return false;
	}
	

	return true;

}

void SatGw::initScenarioTimer(event_id_t sce_timer)
{
	if(!this->with_phy_layer)
	{
		this->scenario_timer = sce_timer;
	}
}

bool SatGw::initProbes()
{
	Probe<int> *probe_output_st;
	Probe<int> *probe_output_st_kb;
	Probe<int> *probe_l2_to_st;
	Probe<int> *probe_l2_from_st;
	Probe<int> *probe_l2_to_gw;
	Probe<int> *probe_l2_from_gw;
	Probe<int> *probe_output_gw;
	Probe<int> *probe_output_gw_kb;


	probe_output_st = Output::registerProbe<int>(
			"Packets", false, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Delay buffer size.Output_ST",
			this->spot_id, this->gw_id);
	this->probe_sat_output_st_queue_size.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_output_st));

	probe_output_st_kb = Output::registerProbe<int>(
			"Kbits", false, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Delay buffer size.Output_ST_kb",
			this->spot_id, this->gw_id);
	this->probe_sat_output_st_queue_size_kb.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_output_st_kb));

	probe_l2_to_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Throughputs.L2_to_ST", this->spot_id, this->gw_id);
	this->probe_sat_l2_to_st.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id, probe_l2_to_st));

	probe_l2_from_st = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Throughputs.L2_from_ST", this->spot_id, this->gw_id);
	this->probe_sat_l2_from_st.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_l2_from_st));


	probe_l2_to_gw = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Throughputs.L2_to_GW",
			this->spot_id, this->gw_id);
	this->probe_sat_l2_to_gw.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_l2_to_gw));
	probe_l2_from_gw = Output::registerProbe<int>(
			"Kbits/s", true, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Throughputs.L2_from_GW",
			this->spot_id, this->gw_id);
	this->probe_sat_l2_from_gw.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_l2_from_gw));
	probe_output_gw = Output::registerProbe<int>(
			"Packets", false, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Delay buffer size.Output_GW",
			this->spot_id, this->gw_id);
	this->probe_sat_output_gw_queue_size.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_output_gw));

	probe_output_gw_kb = Output::registerProbe<int>(
			"Kbits", false, SAMPLE_LAST,
			"Spot_%d.Gw_%d.Delay buffer size.Output_GW_kb",
			this->spot_id, this->gw_id);
	this->probe_sat_output_gw_queue_size_kb.insert(
			std::pair<unsigned int, Probe<int> *>(this->gw_id,
			                                      probe_output_gw_kb));

	return true;
}

bool SatGw::schedule(const time_sf_t current_superframe_sf,
                     time_ms_t current_time)
{
	// not used by scheduling here
	uint32_t remaining_allocation = 0;

	if(!this->st_scheduling || !this->gw_scheduling)
	{
		return false;
	}

	if(!this->st_scheduling->schedule(current_superframe_sf,
	                                  current_time,
	                                  &this->complete_st_dvb_frames,
	                                  remaining_allocation))
	{
		return false;
	}
	if(!this->gw_scheduling->schedule(current_superframe_sf,
	                                  current_time,
	                                  &this->complete_gw_dvb_frames,
	                                  remaining_allocation))
	{
		return false;
	}
	return true;
}


bool SatGw::addTerminal(tal_id_t tal_id)
{
	// check for column in FMT simulation list
	if(!this->addInputTerminal(tal_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"failed to register simulated ST with MAC "
				"ID %u\n", tal_id);
		return false;
	}
	if(!this->addOutputTerminal(tal_id))
	{
		LOG(this->log_receive, LEVEL_ERROR,
				"failed to register simulated ST with MAC "
				"ID %u\n", tal_id);
		return false;
	}

	return true;
}

bool SatGw::updateFmt(DvbFrame *dvb_frame,
                      EncapPlugin::EncapPacketHandler *pkt_hdl)
{
	DvbRcsFrame *frame = dvb_frame->operator DvbRcsFrame*();

	if(this->with_phy_layer)
	{
		tal_id_t src_tal_id;
		// decode the first packet in frame to be able to get source terminal ID
		if(!pkt_hdl->getSrc(frame->getPayload(), src_tal_id))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "unable to read source terminal ID in "
			    "frame, won't be able to update C/N "
			    "value\n");
		}
		else
		{
			double cn = frame->getCn();
			LOG(this->log_receive, LEVEL_INFO,
			    "Uplink CNI for terminal %u = %f\n",
			    src_tal_id, cn);
			this->setRequiredCniInput(src_tal_id, cn);
		}
	}

	return true;
}


bool SatGw::handleSac(DvbFrame *dvb_frame)
{
	double cni;
	// handle SAC here to get the uplink ACM parameters
	Sac *sac = (Sac *)dvb_frame;
	tal_id_t tal_id = sac->getTerminalId();
	LOG(this->log_receive, LEVEL_INFO,
	    "Get SAC from ST%u, with C/N0 = %.2f\n",
	    tal_id, sac->getCni());

	this->setRequiredCniOutput(tal_id, sac->getCni());

	// update ACM parameters with uplink value, thus the GW will
	// known uplink C/N and thus update uplink MODCOD used in TTP
	cni = this->getRequiredCniInput(tal_id);
	sac->setAcm(cni);
	
	// TODO we won't update ACM parameters if we did not receive
	// traffic from this terminal, GW will have a wrong value...
	return true;
}

bool SatGw::updateProbes(time_ms_t stats_period_ms)
{
	// Queue sizes
	mac_fifo_stat_context_t output_gw_fifo_stat;
	mac_fifo_stat_context_t output_st_fifo_stat;
	this->data_out_st_fifo->getStatsCxt(output_st_fifo_stat);

	this->probe_sat_output_st_queue_size[this->gw_id]->put(
			output_st_fifo_stat.current_pkt_nbr);
	this->probe_sat_output_st_queue_size_kb[this->gw_id]->put(
			((int) output_st_fifo_stat.current_length_bytes * 8 / 1000));

	// Throughputs
	// L2 from ST
	this->probe_sat_l2_from_st[this->gw_id]->put(
			this->getL2FromSt() * 8 / stats_period_ms);

	// L2 to ST
	this->probe_sat_l2_to_st[this->gw_id]->put(
			((int) output_st_fifo_stat.out_length_bytes * 8 /
			 stats_period_ms));

	// L2 from GW
	this->probe_sat_l2_from_gw[this->gw_id]->put(
			this->getL2FromGw() * 8 / stats_period_ms);

	// L2 to GW
	data_out_gw_fifo->getStatsCxt(output_gw_fifo_stat);
	this->probe_sat_l2_to_gw[this->gw_id]->put(
			((int) output_gw_fifo_stat.out_length_bytes * 8 /
			 stats_period_ms));

	// Queue sizes
	this->probe_sat_output_gw_queue_size[this->gw_id]->put(
			output_gw_fifo_stat.current_pkt_nbr);
	this->probe_sat_output_gw_queue_size_kb[this->gw_id]->put(
			((int) output_gw_fifo_stat.current_length_bytes * 8 / 1000));

	return true;
}


uint16_t SatGw::getGwId(void) const
{
	return this->gw_id;
}

uint8_t SatGw::getDataInStId(void) const
{
	return this->data_in_st_id;
}

uint8_t SatGw::getDataInGwId(void) const
{
	return this->data_in_gw_id;
}

DvbFifo *SatGw::getDataOutStFifo(void) const
{
	return this->data_out_st_fifo;
}

DvbFifo *SatGw::getDataOutGwFifo(void) const
{
	return this->data_out_gw_fifo;
}

DvbFifo *SatGw::getControlFifo(void) const
{
	return this->control_fifo;
}

uint8_t SatGw::getControlCarrierId(void) const
{
	return this->control_fifo->getCarrierId();
}

DvbFifo *SatGw::getLogonFifo(void) const
{
	return this->logon_fifo;
}

list<DvbFrame *> &SatGw::getCompleteStDvbFrames(void)
{
	return this->complete_st_dvb_frames;
}

list<DvbFrame *> &SatGw::getCompleteGwDvbFrames(void)
{
	return this->complete_gw_dvb_frames;
}

void SatGw::updateL2FromSt(vol_bytes_t bytes)
{
	RtLock lock(this->gw_mutex);
	this->l2_from_st_bytes += bytes;
}

void SatGw::updateL2FromGw(vol_bytes_t bytes)
{
	RtLock lock(this->gw_mutex);
	this->l2_from_gw_bytes += bytes;
}

vol_bytes_t SatGw::getL2FromSt(void)
{
	RtLock lock(this->gw_mutex);
	vol_bytes_t val = this->l2_from_st_bytes;
	this->l2_from_st_bytes = 0;
	return val;
}

vol_bytes_t SatGw::getL2FromGw(void)
{
	RtLock lock(this->gw_mutex);
	vol_bytes_t val = this->l2_from_gw_bytes;
	this->l2_from_gw_bytes = 0;
	return val;
}

bool SatGw::goFirstScenarioStep()
{
	return this->fmt_simu.goFirstScenarioStep();
}

bool SatGw::goNextScenarioStep(double &duration)
{
	return this->fmt_simu.goNextScenarioStep(duration);
}

spot_id_t SatGw::getSpotId(void)
{
	return this->spot_id;
}

FmtDefinitionTable* SatGw::getOutputModcodDef(void)
{
	return this->output_modcod_def;
}

FmtDefinitionTable* SatGw::getInputModcodDef(void)
{
	return this->input_modcod_def;
}

event_id_t SatGw::getScenarioTimer(void)
{
	return this->scenario_timer;
}

void SatGw::print(void)
{
	DFLTLOG(LEVEL_ERROR, "gw_id = %d, spot_id = %d\n",
	        this->gw_id, this->spot_id);
}

