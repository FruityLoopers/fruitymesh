/**

Copyright (c) 2014-2015 "M-Way Solutions GmbH"
FruityMesh - Bluetooth Low Energy mesh protocol [http://mwaysolutions.com/]

This file is part of FruityMesh

FruityMesh is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <adv_packets.h>
#include <AdvertisingController.h>
#include <AdvertisingModule.h>
#include <Logger.h>
#include <Utility.h>
#include <Storage.h>

extern "C"{
#include <app_error.h>
}

//This module allows a number of advertising messages to be configured.
//These will be broadcasted periodically

/*
TODO: Who's responsible for restoring the mesh-advertising packet? This module or the Node?
 * */


AdvertisingModule::AdvertisingModule(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
	: Module(moduleId, node, cm, name, storageSlot)
{
	//Register callbacks n' stuff

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(AdvertisingModuleConfiguration);

	//Start module configuration loading
	LoadModuleConfiguration();

	//Periodically broadcast a debug packet with some stats for debugging
	broadcastDebugPackets = false;
}

void AdvertisingModule::ConfigurationLoadedHandler()
{
	//Does basic testing on the loaded configuration
	Module::ConfigurationLoadedHandler();

	//Version migration can be added here
	if(configuration.moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config


	//Start the Module...
	logt("ADVMOD", "Config set");



}

void AdvertisingModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
	//Do stuff on timer...

}

void AdvertisingModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = 1;

	memset(configuration.messageData[0].messageData, 0, 31);

	advStructureFlags flags;
	advStructureName name;

	flags.len = SIZEOF_ADV_STRUCTURE_FLAGS-1;
	flags.type = BLE_GAP_AD_TYPE_FLAGS;
	flags.flags = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

	name.len = SIZEOF_ADV_STRUCTURE_NAME-1;
	name.type = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
	name.name[0] = 'A';
	name.name[1] = 'B';

	configuration.advertisingIntervalMs = 100;
	configuration.messageCount = 1;

	configuration.messageData[0].messageLength = 31;
	memcpy(configuration.messageData[0].messageData, &flags, SIZEOF_ADV_STRUCTURE_FLAGS);
	memcpy(configuration.messageData[0].messageData+SIZEOF_ADV_STRUCTURE_FLAGS, &name, SIZEOF_ADV_STRUCTURE_NAME);
}

void AdvertisingModule::NodeStateChangedHandler(discoveryState newState)
{
	if(newState == discoveryState::BACK_OFF || newState == discoveryState::DISCOVERY_OFF){
		//Activate our advertising

		//This is a small packet for debugging a node's state
		if(broadcastDebugPackets){
			u8 buffer[31];
			memset(buffer, 0, 31);

			advStructureFlags* flags = (advStructureFlags*)buffer;
			flags->len = SIZEOF_ADV_STRUCTURE_FLAGS-1;
			flags->type = BLE_GAP_AD_TYPE_FLAGS;
			flags->flags = BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

			advStructureManufacturer* manufacturer = (advStructureManufacturer*)(buffer+3);
			manufacturer->len = 26;
			manufacturer->type = BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
			manufacturer->companyIdentifier = 0x24D;

			AdvertisingModuleDebugMessage* msg = (AdvertisingModuleDebugMessage*)(buffer+7);

			msg->debugPacketIdentifier = 0xDE;
			msg->nodeId = node->persistentConfig.nodeId;
			msg->clusterId = node->clusterId;
			msg->clusterSize = node->clusterSize;
			msg->freeIn = cm->freeInConnections;
			msg->freeOut = cm->freeOutConnections;
			msg->connectionLossCounter = node->persistentConfig.connectionLossCounter;

			for(u8 i = 0; i<4; i++){
				if(cm->connections[i]){
					if(cm->connections[i]->reliableBuffersFree > 0){
						msg->txBufferFreeBitmask |= 1 << (i*2);
					}
					if(cm->connections[i]->unreliableBuffersFree > 0){
						msg->txBufferFreeBitmask |= 1 << (i*2+1);
					}
				}
			}

			if(cm->connections[0]->isConnected) msg->partner0 = cm->connections[0]->partnerId;
			if(cm->connections[1]->isConnected) msg->partner1 = cm->connections[1]->partnerId;
			if(cm->connections[2]->isConnected) msg->partner2 = cm->connections[2]->partnerId;
			if(cm->connections[3]->isConnected) msg->partner3 = cm->connections[3]->partnerId;

			msg->packetEnd = 0xDE;

			char strbuffer[200];
			Logger::getInstance().convertBufferToHexString(buffer, 31, strbuffer);

			logt("ADVMOD", "ADV set to %s", strbuffer);

			u32 err = sd_ble_gap_adv_data_set(buffer, 31, NULL, 0);
			APP_ERROR_CHECK(err);

			AdvertisingController::SetAdvertisingState(advState::ADV_STATE_HIGH);
		}
		else if(configuration.messageCount > 0){
			u32 err = sd_ble_gap_adv_data_set(configuration.messageData[0].messageData, configuration.messageData[0].messageLength, NULL, 0);
			APP_ERROR_CHECK(err);

			char buffer[200];
			Logger::getInstance().convertBufferToHexString((u8*)configuration.messageData[0].messageData, 31, buffer);

			logt("ADVMOD", "ADV set to %s", buffer);



			if(configuration.messageData[0].forceNonConnectable)
			{
				AdvertisingController::SetNonConnectable();
			}

			//Now, start advertising
			//TODO: Use values from config to advertise
			AdvertisingController::SetAdvertisingState(advState::ADV_STATE_HIGH);
		}

	} else if (newState == discoveryState::DISCOVERY) {
		//Do not trigger custom advertisings anymore, reset to node's advertising
		node->UpdateJoinMePacket(NULL);
	}
}

bool AdvertisingModule::TerminalCommandHandler(string commandName, vector<string> commandArgs)
{
	if(commandArgs.size() >= 2 && commandArgs[1] == moduleName)
	{
		if(commandName == "action")
		{
			if(commandArgs[1] != moduleName) return false;

			if(commandArgs[2] == "broadcast_debug")
			{
				broadcastDebugPackets = !broadcastDebugPackets;
				logt("ADVMOD", "Debug Packets are now set to %u", broadcastDebugPackets);

				return true;
			}
		}

	}

	//Must be called to allow the module to get and set the config
	return Module::TerminalCommandHandler(commandName, commandArgs);
}
