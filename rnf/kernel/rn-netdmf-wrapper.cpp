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
#include <libxml/tree.h>
#include <vector>

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
  retval = dom->SetInputFileName("NodeTest1.xmn");
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
void parseParameters(NetDMFNode *node, std::vector<NetDMFParameter *>params)
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
void parseDevices(NetDMFNode *node, std::vector<NetDMFDevice *>dev)
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
  }
  else {
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
void parseChannels(NetDMFElement *elmt, std::vector<NetDMFDevice *>&devices)
{
  if (NetDMFPlatform *parent = dynamic_cast<NetDMFPlatform*>(elmt)) {
    int totalChannels = parent->GetNumberOfChannels();

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
	int k = xpath.find("Device") - 1;
	xpath = xpath.substr(0, k);
	NetDMFXmlNode ele = dom->FindElementByPath(xpath.c_str());

	// STOP HERE FOR THE DAY
      }
    }
  }
  else if (NetDMFScenario *parent = dynamic_cast<NetDMFScenario*>(elmt)) {
  }
  else {
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

    // Parse Channels
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
    if (XDMF_SUCCESS != retval) {
      printf("%s:%d:We have a problem\n", __FILE__, __LINE__);
      abort();
    }

    parsePlatforms(scenario);
    
    parseNodes(scenario);

    /* Parse Channels */
    /* Parse Events */
  }
}
