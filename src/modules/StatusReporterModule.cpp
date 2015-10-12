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

#include <Logger.h>
#include <StatusReporterModule.h>
#include <Utility.h>
#include <Storage.h>
#include <Node.h>
#include <Config.h>
#include <stdlib.h>

extern "C"{
#include <app_error.h>
}

StatusReporterModule::StatusReporterModule(u16 moduleId, Node* node, ConnectionManager* cm, const char* name, u16 storageSlot)
	: Module(moduleId, node, cm, name, storageSlot)
{
	//Register callbacks n' stuff
	Logger::getInstance().enableTag("STATUSMOD");

	//Save configuration to base class variables
	//sizeof configuration must be a multiple of 4 bytes
	configurationPointer = &configuration;
	configurationLength = sizeof(StatusReporterModuleConfiguration);

	lastConnectionReportingTimer = 0;
	lastStatusReportingTimer = 0;

	//Start module configuration loading
	LoadModuleConfiguration();
}

void StatusReporterModule::ConfigurationLoadedHandler()
{
	//Does basic testing on the loaded configuration
	Module::ConfigurationLoadedHandler();

	//Version migration can be added here
	if(configuration.moduleVersion == 1){/* ... */};

	//Do additional initialization upon loading the config


	//Start the Module...

}



void StatusReporterModule::TimerEventHandler(u16 passedTime, u32 appTimer)
{
	//Every reporting interval, the node should send its connections
	if(configuration.connectionReportingIntervalMs != 0 && node->appTimerMs - lastConnectionReportingTimer > configuration.connectionReportingIntervalMs)
	{
		//Send connection info
		SendConnectionInformation(NODE_ID_BROADCAST);

		lastConnectionReportingTimer = node->appTimerMs;
	}

	//Every reporting interval, the node should send its status
	if(configuration.statusReportingIntervalMs != 0 && node->appTimerMs - lastStatusReportingTimer > configuration.statusReportingIntervalMs)
	{
		//Send status
		SendStatusInformation(NODE_ID_BROADCAST);

		lastStatusReportingTimer = node->appTimerMs;
	}


}

void StatusReporterModule::ResetToDefaultConfiguration()
{
	//Set default configuration values
	configuration.moduleId = moduleId;
	configuration.moduleActive = true;
	configuration.moduleVersion = 1;

	lastConnectionReportingTimer = 0;
	lastStatusReportingTimer = 0;

	configuration.statusReportingIntervalMs = 0;
	configuration.connectionReportingIntervalMs = 30 * 1000;
	configuration.connectionRSSISamplingMode = RSSISampingModes::RSSI_SAMLING_HIGH;
	configuration.advertisingRSSISamplingMode = RSSISampingModes::RSSI_SAMLING_HIGH;

	//Set additional config values...

}

bool StatusReporterModule::TerminalCommandHandler(string commandName, vector<string> commandArgs)
{
	//Get the status information of the plugged in node
	if(commandName == "uart_get_plugged_in")
	{
		uart("STATUSMOD", "{\"module\":%d, \"type\":\"response\", \"msgType\":\"plugged_in\", \"nodeId\":%u, \"chipIdA\":%u, \"chipIdB\":%u}\n", moduleId, node->persistentConfig.nodeId, NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1]);

		return true;
	}
	else if(commandName == "rssistart")
	{
		for (int i = 0; i < Config->meshMaxConnections; i++)
		{
			StartConnectionRSSIMeasurement(cm->connections[i]);
		}

		return true;
	}
	else if(commandName == "rssistop")
	{
		for (int i = 0; i < Config->meshMaxConnections; i++)
		{
			StopConnectionRSSIMeasurement(cm->connections[i]);
		}

		return true;
	}

	//React on commands, return true if handled, false otherwise
	if(commandArgs.size() >= 2 && commandArgs[1] == moduleName)
	{
		if(commandName == "uart_module_trigger_action" || commandName == "action")
		{
			//Rewrite "this" to our own node id, this will actually build the packet
			//But reroute it to our own node
			nodeID destinationNode = (commandArgs[0] == "this") ? node->persistentConfig.nodeId : atoi(commandArgs[0].c_str());

			//E.g. UART_MODULE_TRIGGER_ACTION 635 STATUS led on
			if(commandArgs.size() == 4 && commandArgs[2] == "led")
			{
				connPacketModuleAction packet;
				packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
				packet.header.sender = node->persistentConfig.nodeId;
				packet.header.receiver = destinationNode;

				packet.moduleId = moduleId;
				packet.actionType = StatusModuleTriggerActionMessages::SET_LED_MESSAGE;
				packet.data[0] = commandArgs[3] == "on" ? 1: 0;


				cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE_ACTION + 1, true);

				return true;
			}
			else if(commandArgs.size() == 3 && commandArgs[2] == "get_status")
			{
				RequestStatusInformation(destinationNode);

				return true;
			}
			else if(commandArgs.size() == 3 && commandArgs[2] == "get_connections")
			{
				connPacketModuleAction packet;
				packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
				packet.header.sender = node->persistentConfig.nodeId;
				packet.header.receiver = destinationNode;

				packet.moduleId = moduleId;
				packet.actionType = StatusModuleTriggerActionMessages::GET_CONNECTIONS_MESSAGE;


				cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE_ACTION + 1, true);

				return true;
			}
		}
	}

	//Must be called to allow the module to get and set the config
	return Module::TerminalCommandHandler(commandName, commandArgs);
}

void StatusReporterModule::ConnectionPacketReceivedEventHandler(connectionPacket* inPacket, Connection* connection, connPacketHeader* packetHeader, u16 dataLength)
{
	//Must call superclass for handling
	Module::ConnectionPacketReceivedEventHandler(inPacket, connection, packetHeader, dataLength);

	if(packetHeader->messageType == MESSAGE_TYPE_MODULE_TRIGGER_ACTION){
		connPacketModuleAction* packet = (connPacketModuleAction*)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId){
			//It's a LED message
			if(packet->actionType == StatusModuleTriggerActionMessages::SET_LED_MESSAGE){
				if(packet->data[0])
				{
					//Switch LED on
					node->currentLedMode = Node::ledMode::LED_MODE_OFF;

					node->LedRed->On();
					node->LedGreen->On();
					node->LedBlue->On();
				}
				else
				{
					//Switch LEDs back to connection signaling
					node->currentLedMode = Node::ledMode::LED_MODE_CONNECTIONS;
				}
			}
			//We were queried for our status
			else if(packet->actionType == StatusModuleTriggerActionMessages::GET_STATUS_MESSAGE)
			{
				SendStatusInformation(packet->header.sender);

			}
			//We were queried for our connections
			else if(packet->actionType == StatusModuleTriggerActionMessages::GET_CONNECTIONS_MESSAGE)
			{
				StatusReporterModule::SendConnectionInformation(packetHeader->sender);
			}
		}
	}

	//Parse Module responses
	if(packetHeader->messageType == MESSAGE_TYPE_MODULE_ACTION_RESPONSE){

		connPacketModuleAction* packet = (connPacketModuleAction*)packetHeader;

		//Check if our module is meant and we should trigger an action
		if(packet->moduleId == moduleId)
		{
			//Somebody reported its connections back
			if(packet->actionType == StatusModuleActionResponseMessages::CONNECTIONS_MESSAGE)
			{
				StatusReporterModuleConnectionsMessage* packetData = (StatusReporterModuleConnectionsMessage*) (packet->data);
				uart("STATUSMOD", "{\"module\":%d, \"type\":\"response\", \"msgType\":\"connections\", \"nodeId\":%d, \"partners\":[%d,%d,%d,%d], \"rssiValues\":[%d,%d,%d,%d]}\n", moduleId, packet->header.sender, packetData->partner1, packetData->partner2, packetData->partner3, packetData->partner4, packetData->rssi1, packetData->rssi2, packetData->rssi3, packetData->rssi4);
			}
			else if(packet->actionType == StatusModuleActionResponseMessages::STATUS_MESSAGE)
			{
				//Print packet to console
				StatusReporterModuleStatusMessage* data = (StatusReporterModuleStatusMessage*) (packet->data);
				uart("STATUSMOD", "{\"module\":%d, \"type\":\"response\", \"msgType\":\"status\", \"nodeId\":%u, \"chipIdA\":%u, \"chipIdB\":%u, \"clusterId\":%u, \"clusterSize\":%d, \"freeIn\":%u, \"freeOut\":%u, \"addr\":\"%02X:%02X:%02X:%02X:%02X:%02X\"}\n", moduleId, packet->header.sender, data->chipIdA, data->chipIdB, data->clusterId, data->clusterSize, data->freeIn, data->freeOut, data->accessAddress.addr[5], data->accessAddress.addr[4], data->accessAddress.addr[3], data->accessAddress.addr[2], data->accessAddress.addr[1], data->accessAddress.addr[0]);
			}
		}
	}
}

void StatusReporterModule::RequestStatusInformation(nodeID targetNode)
{
	connPacketModuleAction packet;
	packet.header.messageType = MESSAGE_TYPE_MODULE_TRIGGER_ACTION;
	packet.header.sender = node->persistentConfig.nodeId;
	packet.header.receiver = targetNode;

	packet.moduleId = moduleId;
	packet.actionType = StatusModuleTriggerActionMessages::GET_STATUS_MESSAGE;

	logt("STATUS", "Requesting status");


	cm->SendMessageToReceiver(NULL, (u8*)&packet, SIZEOF_CONN_PACKET_MODULE_ACTION, true);
}

//This method sends the node's status over the network
void StatusReporterModule::SendStatusInformation(nodeID toNode)
{
	logt("STATUS", "Sending back info");

	u8 buffer[SIZEOF_CONN_PACKET_MODULE_ACTION + SIZEOF_STATUS_REPORTER_MODULE_STATUS_MESSAGE];
	connPacketModuleAction* packet = (connPacketModuleAction*) buffer;
	StatusReporterModuleStatusMessage* data = (StatusReporterModuleStatusMessage*) packet->data;

	packet->header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
	packet->header.sender = node->persistentConfig.nodeId;
	packet->header.receiver = toNode;

	packet->moduleId = moduleId;
	packet->actionType = StatusModuleActionResponseMessages::STATUS_MESSAGE;

	data->chipIdA = NRF_FICR->DEVICEID[0];
	data->chipIdB = NRF_FICR->DEVICEID[1];
	data->clusterId = node->clusterId;
	data->clusterSize = node->clusterSize;
	sd_ble_gap_address_get(&data->accessAddress);
	data->freeIn = cm->freeInConnections;
	data->freeOut = cm->freeOutConnections;


	cm->SendMessageToReceiver(NULL, (u8*)packet, SIZEOF_CONN_PACKET_MODULE_ACTION + SIZEOF_STATUS_REPORTER_MODULE_STATUS_MESSAGE, true);
}

//This method sends information about the current connections over the network
void StatusReporterModule::SendConnectionInformation(nodeID toNode)
{
	//Build response and send
	u16 packetSize = SIZEOF_CONN_PACKET_MODULE_ACTION + SIZEOF_STATUS_REPORTER_MODULE_CONNECTIONS_MESSAGE;
	u8 buffer[packetSize];
	connPacketModuleAction* outPacket = (connPacketModuleAction*)buffer;

	outPacket->header.messageType = MESSAGE_TYPE_MODULE_ACTION_RESPONSE;
	outPacket->header.receiver = NODE_ID_BROADCAST;
	outPacket->header.sender = node->persistentConfig.nodeId;

	outPacket->moduleId = moduleId;
	outPacket->actionType = StatusModuleActionResponseMessages::CONNECTIONS_MESSAGE;

	StatusReporterModuleConnectionsMessage* outPacketData = (StatusReporterModuleConnectionsMessage*)(outPacket->data);

	outPacketData->partner1 = cm->connections[0]->partnerId;
	outPacketData->partner2 = cm->connections[1]->partnerId;
	outPacketData->partner3 = cm->connections[2]->partnerId;
	outPacketData->partner4 = cm->connections[3]->partnerId;

	outPacketData->rssi1 = cm->connections[0]->GetAverageRSSI();
	outPacketData->rssi2 = cm->connections[1]->GetAverageRSSI();
	outPacketData->rssi3 = cm->connections[2]->GetAverageRSSI();
	outPacketData->rssi4 = cm->connections[3]->GetAverageRSSI();


	cm->SendMessageToReceiver(NULL, buffer, packetSize, true);
}

void StatusReporterModule::StartConnectionRSSIMeasurement(Connection* connection){
	u32 err = 0;

	if (connection->isConnected)
	{
		//Reset old values
		connection->rssiSamplesNum = 0;
		connection->rssiSamplesSum = 0;

		err = sd_ble_gap_rssi_start(connection->connectionHandle, 0, 0);
		APP_ERROR_CHECK(err);

		logt("STATUSMOD", "RSSI measurement started for connection %u with code %u", connection->connectionId, err);
	}
}

void StatusReporterModule::StopConnectionRSSIMeasurement(Connection* connection){
	u32 err = 0;

	if (connection->isConnected)
	{
		err = sd_ble_gap_rssi_stop(connection->connectionHandle);
		APP_ERROR_CHECK(err);

		logt("STATUSMOD", "RSSI measurement stopped for connection %u with code %u", connection->connectionId, err);
	}
}


//This handler receives all ble events and can act on them
void StatusReporterModule::BleEventHandler(ble_evt_t* bleEvent){

	//New RSSI measurement for connection received
	if(bleEvent->header.evt_id == BLE_GAP_EVT_RSSI_CHANGED)
	{
		Connection* connection = cm->GetConnectionFromHandle(bleEvent->evt.gap_evt.conn_handle);
		i8 rssi = bleEvent->evt.gap_evt.params.rssi_changed.rssi;

		connection->rssiSamplesNum++;
		connection->rssiSamplesSum += rssi;

		if(connection->rssiSamplesNum > 50){
			connection->rssiAverage = connection->rssiSamplesSum / connection->rssiSamplesNum;

			connection->rssiSamplesNum = 0;
			connection->rssiSamplesSum = 0;

			//logt("STATUSMOD", "New RSSI average %d", connection->rssiAverage);
		}


	}
}
;

void StatusReporterModule::MeshConnectionChangedHandler(Connection* connection)
{
	//New connection has just been made
	if(connection->handshakeDone){
		//TODO: Implement low and medium rssi sampling with timer handler
		//TODO: disable and enable rssi sampling on existing connections
		if(configuration.connectionRSSISamplingMode == RSSISampingModes::RSSI_SAMLING_HIGH){
			StartConnectionRSSIMeasurement(connection);
		}
	}
}
