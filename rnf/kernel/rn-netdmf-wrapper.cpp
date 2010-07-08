#include <NetDMF.h>
#include <NetDMFDOM.h>
#include <NetDMFNode.h>
#include <NetDMFXmlNode.h>
#include <NetDMFPlatform.h>
#include <NetDMFEvent.h>
#include <NetDMFRoot.h>
#include <NetDMFScenario.h>
#include <NetDMFParameter.h>
#include <NetDMFDevice.h>
#include <NetDMFChannel.h>
#include <NetDMFConversation.h>
#include <NetDMFMovement.h>
#include <NetDMFArray.h>
#include <NetDMFTraffic.h>
#include <NetDMFAddressItem.h>
#include <libxml/tree.h>
#include <vector>
#include <sstream>

/**
 * @file
 * @brief NetDMF Wrappers
 * 
 * This file provides wrappers for functions
 * to enable C linkage.  These functions will perform most of the actions
 * and provide it to the C front-end.  Currently, there are *NO*
 * tie-ins with ROSS, i.e. we read the NetDMF but don't actually do
 * anything with it.  Almost all of this logic was stolen from
 * Payton Oliveri's python script NetDMFtoOpNet.py.
 */

/** The NetDMF Document Object Model, see NetDMF docs. */
static NetDMFDOM *dom = 0;
/** 
 * The root of the NetDMF file, i.e.
 * <NetDMF Version="2.0" xmlns:xi="http://www.w3.org/2003/XInclude">
 */
static NetDMFRoot *root = 0;

extern "C" char      g_rn_netdmf_config[];

extern "C" void parseScenarios();
/**
 * This function handles initialization of the NetDMF
 * description language.  The function currently doesn't do much except
 * demonstrate proper linkage.
 */
extern "C"
void
rnNetDMFInit()
{
  int retval;
  //  XdmfXmlNode  Parent, FirstChild, SecondChild;
  //  NetDMFNode node;

  dom = new NetDMFDOM();
  if (0 == dom) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  // Parse the XML File
  if (0 == strcmp("", g_rn_netdmf_config)) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }
  retval = dom->SetInputFileName(g_rn_netdmf_config);
  if (XDMF_SUCCESS != retval) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  retval = dom->Parse();
  if (XDMF_SUCCESS != retval) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  root = new NetDMFRoot();
  retval = root->SetDOM(dom);
  if (XDMF_SUCCESS != retval) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  parseScenarios();

#if 0
  retval = node.SetDOM(dom);
  if (XDMF_SUCCESS != retval) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  int elemIdx = 0;
  NetDMFXmlNode xmlNode = dom->FindElement("Node", elemIdx);
  if (0 == xmlNode) {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
    abort();
  }

  while (0 != xmlNode) {
    retval = node.SetElement(xmlNode);
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }
    
    retval = node.UpdateInformation();
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }
    
    std::string str1("router1");
    if (str1.compare(node.GetName()) != 0) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }
    
    if(5 != node.GetNodeId()) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }
    printf("Node %s had ID %d\n", node.GetName(), node.GetNodeId());
    
    elemIdx++;
    xmlNode = dom->FindElement("Node", elemIdx);
  }

  // Find the first element with TAG = Tag1
  Parent = dom->FindElement("Tag1");
  // Find the first (zero based) Tag2 below Parent
  FirstChild = dom->FindElement("Tag2", 0, Parent);
  //cout << "The Name of the First Child is <" << DOM->Get(FirstChild, "Name") << ">" << endl;
  // Find the second (zero based) Tag2 below Parent
  SecondChild = dom->FindElement("Tag2", 1, Parent);
  dom->Set(SecondChild, "Age", "10");
  dom->DeleteNode(FirstChild);
  printf("XML = \n***\n%s\n***\n", dom->Serialize(SecondChild));
  //cout << endl << "XML = " << endl << DOM->Serialize(Parent) << endl;
  delete dom;
  dom = 0;
#endif
}

NetDMFNode * getNodeWithAddress(std::string address)
{
  int totalScenarios = dom->FindNumberOfElements("Scenario");

  for (int i = 0; i < totalScenarios; i++) {
    NetDMFScenario scenarioItem;
    scenarioItem.SetDOM(dom);
    scenarioItem.SetElement(dom->FindElement("Scenario", i));
    scenarioItem.Update();

    int totalNodes = scenarioItem.GetNumberOfNodes();

    for (int j = 0; j < totalNodes; j++) {
      NetDMFNode *nodeItem = scenarioItem.GetNode(j);

      int totalDevices = nodeItem->GetNumberOfDevices();

      for (int k = 0; k < totalDevices; k++) {
	NetDMFDevice *deviceItem = nodeItem->GetDevice(k);

	int totalAddressItems = deviceItem->GetNumberOfAddressItems();

	for (int l = 0; l < totalAddressItems; l++) {
	  NetDMFAddressItem *addressItem = deviceItem->GetAddress(l);
	  addressItem->Update();
	  if (addressItem->GetAddresses(0,1) == address.c_str()) {
	    return nodeItem;
	  }
	}
      }
    }
    
    int totalPlatforms = scenarioItem.GetNumberOfPlatforms();

    for (int j = 0; j < totalPlatforms; j++) {
      NetDMFPlatform *platformItem = scenarioItem.GetPlatform(j);

      int totalNodes = platformItem->GetNumberOfNodes();

      for (int k = 0; k < totalNodes; k++) {
	NetDMFNode *nodeItem = platformItem->GetNode(k);

	int totalDevices = nodeItem->GetNumberOfDevices();

	for (int l = 0; l < totalDevices; l++) {
	  NetDMFDevice *deviceItem = nodeItem->GetDevice(l);

	  int totalAddressItems = deviceItem->GetNumberOfAddressItems();

	  for (int m = 0; m < totalAddressItems; m++) {
	    NetDMFAddressItem *addressItem = deviceItem->GetAddress(m);
	    addressItem->Update();
	    if (addressItem->GetAddresses(0,1) == address.c_str()) {
	      return nodeItem;
	    }
	  }
	}
      }
    }
  }

  return 0;
}

/**
 * Parse the NetDMFTraffic type, described below.
 *
 * The <Traffic> specifies the data that flows in a conversation. It
 * uses an XdmfDataItem to specify how many bytes/frames were transferred,
 * in each direction or lists the raw packets using <PacketItem>. If the
 * TrafficType is bin, a TrafficInterval is specified.
 @verbatim
 <Traffic
   TrafficType="Bin"
   Units="Bytes"
   Interval="0.1">
    <DataItem NumberType="Float" Dimensions="10" Format="XML">
      0.0 0.0 0.0 0.0 356.0 500.0 500.0 0.0 0.0 0.0
    </DataItem>
 </Traffic>
 @endverbatim
 */
void parseTraffics(NetDMFConversation *parent, int &index)
{
  int totalTraffics = parent->GetNumberOfTraffics();

  std::string starttime;
  if (dom->GetAttribute(parent->GetElement(), "StartTime")) {
    starttime = dom->GetAttribute(parent->GetElement(), "StartTime");
  }
  else {
    starttime = "0";
  }

  std::string srcaddress = parent->GetEndPointA();
  NetDMFNode *src = getNodeWithAddress(srcaddress);
  std::string destaddress = parent->GetEndPointB();
  NetDMFNode *dest = getNodeWithAddress(destaddress);

  for (int i = 0; i < totalTraffics; i++) {
    NetDMFTraffic *trafficItem = parent->GetTraffic(i);
    trafficItem->Update();
    XdmfArray *values = trafficItem->GetValues();
    XdmfInt64 valuesstring = values->GetNumberOfElements();

    for (int j = 0; j < valuesstring; j++) {
      if (0 != values->GetValueAsInt32(j)) {
	// Do some stuff with this non-zero traffic
	index++;
      }
    }
  }  
}

/**
 * Parse the NetDMFConversation type, described below.
 *
 * A NetDMFConversation  defines conversations between nodes. It is the
 * child of an event with EventType="Communication" and CommType="Conversation".
 * A conversation can have 0 or 1 PacketItems. If present, the PacketItem lists
 * the packets in the conversation. Additionally a Conversation can have 1 or 2
 * Traffic elements. These define the traffic in each direction.
 @verbatim
 <Event EventType="Comm" CommType="Conversation" StartTime="12345">
    <Conversation
        StartTime="12345"
        ConversationType="IPV4"
        EndPointA="10.11.102.23"
        EndPointB="10.11.104.23">
        <PacketItem
            NumberOfPackets="100"
            PacketType="Filter"
            Filter="src host 10.11.102.23 and dst host 10.11.104.23"
            Format="PCAP">
            test.pcap
         </PacketItem>
         <Traffic
            TrafficType="bin"
            Direction"AtoB"
            Units="bytes"
            Interval="0.1">
            <DataItem NumberType="Float" Dimensions="10" Format="XML">
                 0.0 0.0 0.0 0.0 356.0 500.0 500.0 0.0 0.0 0.0
            </DataItem>
         </Traffic>
     </Conversation>
 </Event>
 @endverbatim
 */
void parseCommunications(NetDMFEvent *parent)
{
  int totalConversations = parent->GetNumberOfConversations();

  int index = 0;

  for (int i = 0; i < totalConversations; i++) {
    NetDMFConversation *conversationItem = parent->GetConversation(i);

    parseTraffics(conversationItem, index);
  }
}

/**
 * Parse the NetDMFMovement type, described below.
 *
 * Movment for a node can be in the Scenario or Result section. MovementType can be :
 - Model
 - Path
 * If MovementType is Model, the ModelName and ModelData strings define a mobility
 * model and its parameters respectively to define the movement.

 * If MovementType is Path, PathType can be :
 - Grid : XYZ in grid coordinates
 - GeoSpatial : Lat, Lon, Height
 - GridVelocity : Grid + Velocity
 - GeoSpatialVelocity : GeoSpatial + velocity
 @verbatim
 <Movement
    MovementType="Model"
    ModelName="RandomWalk"
    ModelData="interval=1.0s maxspeed=0.5m/s"
 </Movement>
 <Movement 
    MovementType="Path"
    Interval="1.0"
    PathPositionType="GeoSpatial" >
    <DataItem  NumberType="Float" Dimensions="3 4" Format=XML>
       35.0  75.0  1.0
       35.15 75.0  1.0
       35.25 75.0  1.0
       35.35 75.0  1.0
    </DataItem>
 </Movement>
 @endverbatim
 */
void parseMovements(NetDMFEvent *parent)
{
  int totalMovements = parent->GetNumberOfMovements();

  XdmfFloat64 x, y;

  for (int i = 0; i < totalMovements; i++) {
    NetDMFMovement *movementItem = parent->GetMovement(i);
    movementItem->Update();
    std::string nodeidstring = movementItem->GetNodeId();

    XdmfArray *ar = movementItem->GetPathData()->GetArray();
    std::string dimensions = movementItem->GetPathData()->GetShapeAsString();
    std::istringstream input(dimensions);
    int dim1, dim2;
    input >> dim1;
    input >> dim2;

    if (dim2 == 4 || dim2 == 7) {
      x = ar->GetValueAsFloat64(2);
      y = ar->GetValueAsFloat64(1);
    }
    else {
      x = ar->GetValueAsFloat64(1);
      y = ar->GetValueAsFloat64(0);
    }
  }
}

/**
 * Parse the NetDMFEvent type, described below.
 *
 * An <Event> is the high level container for discrete events. In addition
 * to StartTime and EndTime (from the Unix epoch) an <Event>  has an
 * EventType attribute. EventType corresponds to the child element type of
 * the <Event>. Valid EventTypes are :
 * - Collection - Events that should be grouped
 * - Movement - Node Movement
 * - Communication - two or more nodes communicating

 * Communication Event Types can be further defined in two different ways, as
 * specified by CommunicationType. Valid CommunicationTypes are :
 * - Conversation - Communication between two endpoints
 * - EndPoint - Endpoints for communications
 * - PacketCapture - Packets captured at a specific node:device
 * - Application - A simulator-supported traffic generation application (e.g. ping)
 @verbatim
 <Result>
 <Event EventType="Collection" CollectionType="Temporal" 
 Name="Simple OLSR Simulation" StartTime="0.0" Endtime="50.0">
 <Event EventType="Collection" CollectionType="Temporal"
 StartTime="0.0" Endtime="1.0">
 <Event  EventType="Movement" 
    NodeId="2"
        <Movement 
            MovementType="Path"
            PathType="TimeLatLonHeight"
            PathLength="2">
        <DataItem  NumberType="Float" Dimensions="3 4" Format="XML">
    0.0        35.0   75.0  1.0
    1.0        35.15 75.0  1.0
        </DataItem>
        </Movement>
 </Event>
 <Event  EventType="Comm" CommType="EndPoints" EndPointType="IPV4">
 <AddressItem
    AddressType="IPV4" 
    NumberOfAddresses"4" 
    Format="XML" >
        10.11.102.23
        10.11.104.23
        192.168.0.1
        192.168.0.12
 </AddressItem>
 </Event>
 <Event EventType="Comm" CommType="Conversation">
 <Conversation
 ConversationType="IPV4"
 EndPointA="10.11.102.23"
 EndPointB="10.11.104.23">
 <PacketItem
    NumberOfPackets="100" 
    PacketType="Filter" 
    Filter="src host 10.11.102.23 and dst host 10.11.104.23" 
    Format="PCAP">
    test.pcap
 </PacketItem>
 <Traffic
    TrafficType="Bin"
    Units="Bytes"
    Interval="0.1">
 <DataItem NumberType="Float" Dimensions="10" Format="XML">
 0.0 0.0 0.0 0.0 356.0 500.0 500.0 0.0 0.0 0.0
 </DataItem>
 </Traffic>
 </Conversation>
 </Event>
 <Event EventType="Comm" CommType="PacketCapture">
   <Parameter Name="NodeId" Value="1"/>
   <Parameter Name="DeviceId" Value="0"/>
   <PacketItem Format="PCAP">
     test1_n1d0.pcap
   </PacketItem>
 </Event>
 </Event>
 <Event EventType="Collection" StartTime="1.0" Endtime="2.0">
    Events for time 1.0 2.0
 </Event>
 Etc.......
 </Event>
 </Result>
 @endverbatim
 */
void parseEvents(NetDMFScenario *parent)
{
  int totalEvents = parent->GetNumberOfEvents();

  for (int i = 0; i < totalEvents; i++) {
    NetDMFEvent *eventItem = parent->GetEvent(i);
    if (eventItem->GetEventType() == 1) {
      parseMovements(eventItem);
    }
    else if (eventItem->GetEventType() == 2) {
      parseCommunications(eventItem);
    }
    else {
      printf("Unknown event type: %d\n", i);
    }
  }
}

/**
 * Parse the NetDMFParameter type, described below.
 *
 * A NetDMFParameter contains a name and a value pair.
 * It's is only meaningful to the end application; NetDMF
 * does not interpret the data.
 @verbatim
 <Parameter Name="Wheels" Value="4"/>
 <Parameter Name="TopSpeed" Value="45"/>
 <Parameter Name="MaxConnections" Value="100000"/>
 <Parameter Name="Manufacturer" Value="Acme"/>
 @endverbatim
 */
void parseParameters(NetDMFNode *node, std::vector<NetDMFParameter *> &params)
{
  int totalParameters = node->GetNumberOfParameters();

  for (int i = 0; i < totalParameters; i++) {
    NetDMFParameter *param = node->GetParameter(i);
    params.push_back(param);
  }
}

/**
 * Parse the NetDMFDevice type, described below.
 *
 * Currently, there are two types of devices: Communications and Mobility.
 * Devices can contain other devices (a mobility device can have two communication
 * devices). Communication devices contain one or more protocol stacks. Devices
 * have parameters that are only of importance to that particular device.
 @verbatim
 <Device Name="HMMWV" DeviceType="Communication">
    <Parameter ... >
    <Parameter ... >
 </Device>


 <Device Name="RadioType1" DeviceType="Communication">
   <Parameter Name="DataRate" Value="300000" />
   <Parameter Name="Delay" Value="2.5" />
 </Device>

 <Device Name="HMMWV" DeviceType="Mobility">
    <Parameter Name="Wheels" Value="4" />
    <Parameter Name="TopSpeed" Value="20.0" />
    <Device Reference="/NetDMF/Device[@Name='RadioType1']" >
       <Stack Reference="/NetDMF/Stack[@Name='SimpleOlsr']" />
       <Stack Reference="/NetDMF/Stack[@Name='MyExperimentalProtocol']" />
    </Device>
 </Device>
 @endverbatim
 */
void parseDevices(NetDMFNode *node, std::vector<NetDMFDevice *> &dev)
{
  int totalDevices = node->GetNumberOfDevices();

  for (int i = 0; i < totalDevices; i++) {
    NetDMFDevice *device = node->GetDevice(i);
    dev.push_back(device);
  }
}

/**
 * Parse the NetDMFNode type, described below.
 *
 * A NetDMFNode  contains a single network node.
 @verbatim
 <Node Name="Person1" NodeId="1">
    <Device ... >
    <Stack... >
 </Node>
 @endverbatim
 */
void parseNodes(NetDMFElement *elmt)
{
  if (NetDMFPlatform *parent = dynamic_cast<NetDMFPlatform*>(elmt)) {
    int totalNodes = parent->GetNumberOfNodes();

    for (int i = 0; i < totalNodes; i++) {
       NetDMFNode *nodeItem = parent->GetNode(i);

      std::vector<NetDMFDevice *> devices;
      std::vector<NetDMFParameter *> params;
      parseDevices(nodeItem, devices);
      parseParameters(nodeItem, params);

      if (devices.size() == 0) {
	printf("Ignoring Node with no devices: %s, %d", nodeItem->GetName(),
	       nodeItem->GetNodeId());
	continue;
      }

      if (devices.size() > 1) {
	for (int j = 1; j < devices.size(); j++) {
	  printf("Ignoring Device named: %s", (devices[j])->GetName());
	}
      }

      for (int j = 0; j < params.size(); j++) {
	if (params[j]->GetName() == "IPv4MulticastMembership") {
	  // Possibly do something in ROSS
	}
      }
    }
  }
  else if (NetDMFScenario *parent = dynamic_cast<NetDMFScenario*>(elmt)) {
    int totalNodes = parent->GetNumberOfNodes();

    for (int i = 0; i < totalNodes; i++) {
       NetDMFNode *nodeItem = parent->GetNode(i);

      std::vector<NetDMFDevice *> devices;
      std::vector<NetDMFParameter *> params;
      parseDevices(nodeItem, devices);
      parseParameters(nodeItem, params);

      if (devices.size() == 0) {
	printf("Ignoring Node with no devices: %s, %d", nodeItem->GetName(),
	       nodeItem->GetNodeId());
	continue;
      }

      if (devices.size() > 1) {
	for (int j = 1; j < devices.size(); j++) {
	  printf("Ignoring Device named: %s", (devices[j])->GetName());
	}
      }

      for (int j = 0; j < params.size(); j++) {
	if (params[j]->GetName() == "IPv4MulticastMembership") {
	  // Possibly do something in ROSS
	}
      }
    }
  }
  else {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
  }
}

/**
 * Parse the NetDMFChannel type, described below.
 *
 * In a wired or wireless network, a channel specifies physical or logical
 * connectivity to a particular network segment, such as a shared Ethernet
 * or 802.11 network. The NetDMFChannel  element contains a list of network
 * devices that are considered to be 'connected' (either physically or
 * logically). The network devices are specified by an XML reference to the
 * definition within a NetDMFNode.
 @verbatim
 <Device Name="Cat6" DeviceType="Communication">
    <Parameter Name="MaxRate" Value="100E06" />
 </Device>
 <Channel Name="Cables" ChannelType="Duplex">
    <Device Reference="/NetDMF/Device[@Name='Cat6']" />
      /NetDMF/Scenario/Node[3]/Device
      /NetDMF/Scenario/Node[4]/Device[2]
      /NetDMF/Scenario/Node[@Name='Server']/Device[1]
 </Channel>
 @endverbatim
 */
void parseChannels(NetDMFElement *elmt)
{
  std::vector<NetDMFDevice *> devices;
  std::vector<NetDMFNode *> nodes;

  if (NetDMFPlatform *parent = dynamic_cast<NetDMFPlatform*>(elmt)) {
    int totalChannels = parent->GetNumberOfChannels();

    std::string srcstring;
    std::string deststring;

    for (int i = 0; i < totalChannels; i++) {
      NetDMFChannel *channelItem = parent->GetChannel(i);
      channelItem->Update();

      int totalDevices = channelItem->GetNumberOfDeviceIds();

      for (int j = 0; j < totalDevices; j++) {
	std::string deviceId = channelItem->GetDeviceIds(j);
	NetDMFXmlNode channelDevice = dom->FindElementByPath(deviceId.c_str());
	NetDMFDevice *deviceItem = channelItem->GetAttachedDevice(j);
	devices.push_back(deviceItem);
	std::string channelDeviceName = dom->GetAttribute(channelDevice, "Name");

	std::string xpath = dom->GetPath(channelDevice);
	int dnum = xpath.find("Device") - 1;
	xpath = xpath.substr(0, dnum);
	NetDMFXmlNode ele = dom->FindElementByPath(xpath.c_str());

	int totalNodes = parent->GetNumberOfNodes();
	for (int k = 0; k < totalNodes; k++) {
	  NetDMFNode *nodeItem = parent->GetNode(k);
	  std::string nodexpath = dom->GetPath(nodeItem->GetElement());
	  if (nodexpath == xpath) {
	    nodes.push_back(nodeItem);
	    if (j == 0) {
	      srcstring = nodeItem->GetName() + nodeItem->GetNodeId();
	    }
	    else {
	      deststring = nodeItem->GetName() + nodeItem->GetNodeId();
	    }
	  }
	}
      }
    }
  }
  else if (NetDMFScenario *parent = dynamic_cast<NetDMFScenario*>(elmt)) {
    int totalChannels = parent->GetNumberOfChannels();

    std::string srcstring;
    std::string deststring;

    for (int i = 0; i < totalChannels; i++) {
      NetDMFChannel *channelItem = parent->GetChannel(i);
      channelItem->Update();

      int totalDevices = channelItem->GetNumberOfDeviceIds();

      for (int j = 0; j < totalDevices; j++) {
	std::string deviceId = channelItem->GetDeviceIds(j);
	NetDMFXmlNode channelDevice = dom->FindElementByPath(deviceId.c_str());
	NetDMFDevice *deviceItem = channelItem->GetAttachedDevice(j);
	devices.push_back(deviceItem);
	std::string channelDeviceName = dom->GetAttribute(channelDevice, "Name");

	std::string xpath = dom->GetPath(channelDevice);
	int dnum = xpath.find("Device") - 1;
	xpath = xpath.substr(0, dnum);
	NetDMFXmlNode ele = dom->FindElementByPath(xpath.c_str());

	int totalNodes = parent->GetNumberOfNodes();
	for (int k = 0; k < totalNodes; k++) {
	  NetDMFNode *nodeItem = parent->GetNode(k);
	  std::string nodexpath = dom->GetPath(nodeItem->GetElement());
	  if (nodexpath == xpath) {
	    nodes.push_back(nodeItem);
	    if (j == 0) {
	      srcstring = nodeItem->GetName() + nodeItem->GetNodeId();
	    }
	    else {
	      deststring = nodeItem->GetName() + nodeItem->GetNodeId();
	    }
	  }
	}

	int totalPlatforms = parent->GetNumberOfPlatforms();
	for (int k = 0; k < totalPlatforms; k++) {
	  NetDMFPlatform *platformItem = parent->GetPlatform(k);
	  totalNodes = platformItem->GetNumberOfNodes();
	  for (int l = 0; l < totalNodes; l++) {
	    NetDMFNode *nodeItem = platformItem->GetNode(l);
	    std::string nodexpath = dom->GetPath(nodeItem->GetElement());
	    if (nodexpath == xpath) {
	      nodes.push_back(nodeItem);
	      if (j == 0) {
		srcstring = nodeItem->GetName() + nodeItem->GetNodeId();
	      }
	      else {
		deststring = nodeItem->GetName() + nodeItem->GetNodeId();
	      }
	    }
	  }
	}
      }
    }
  }
  else {
    printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
  }
}

/**
 * Parse the NetDMFPlatform type, described below.
 *
 * A NetDMFPlatform  contains a "platform" definition which is a collection
 * of nodes. It has many of the properties of a node.
 @verbatim
 <Platform Name="Rack1">
    <Node/>
    <Node/>
    <Node/>
    <Parameter/>
 </Platform>
 @endverbatim
 */
void parsePlatforms(NetDMFScenario *scenario)
{
  int retval;
  int totalPlatforms = scenario->GetNumberOfPlatforms();

  NetDMFPlatform *platform;

  for (int i = 0; i < totalPlatforms; i++) {
    platform = scenario->GetPlatform(i);

    std::string idstring = dom->GetPath(platform->GetElement());

    int index0 = idstring.find("Platform");
    // We only want [index0:len(idstring)]
    idstring = idstring.substr(index0);
    if (idstring.find("[") == std::string::npos) {
      // It's not in the string, set idstring to 1
      idstring = "1";
    }
    else {
      int index1 = idstring.find("[");
      int index2 = idstring.find("]");
      idstring = idstring.substr(index1, index2 - index1);
    }

    parseNodes(platform);

    parseChannels(platform);
  }
}

/**
 * Parse the NetDMFScenario type, described below.
 * 
 * A NetDMFScenario contains a "scenario" suitable for setting up a Network Simulation.
 @verbatim
 <Scenario Name="Simuation 1 Scenario">
    <Protocol/>
    <Node/>
    <Event/>
    <Parameter/>
    <Channel/>
    <Platform/>
 </Scenario>
 @endverbatim
 */
extern "C"
void parseScenarios()
{
  int retval;
  int totalScenarios = dom->FindNumberOfElements("Scenario");

  NetDMFScenario *scenario;
  for (int i = 0; i < totalScenarios; i++) {
    scenario = new NetDMFScenario();

    retval = scenario->SetDOM(dom);
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }

    retval = scenario->SetElement(dom->FindElement("Scenario", i));
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }

    retval = scenario->Update();
    /**
     * @bug There's a bug in NetDMF that allows the Update function
     * to run off the end w/o returning XMD_SUCCESS
     
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }
    */
    parsePlatforms(scenario);
    
    parseNodes(scenario);

    parseChannels(scenario);

    parseEvents(scenario);
  }
}
