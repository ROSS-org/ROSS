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
#include <libxml/tree.h>
#include <vector>

/**
 * @file
 * @brief NetDMF Wrappers
 * 
 * This file provides wrappers for functions
 * to enable C linkage.  These functions will perform most of the actions
 * and provide it to the C front-end.
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
 * This function creates and setups the
 * g_rn_machines global data structure of nodes.  Non-functional atm.
 */
extern "C"
void
rnNetDMFTopology()
{
}

void parseParameters(NetDMFNode *node, std::vector<NetDMFParameter *>params)
{
  int totalParameters = node->GetNumberOfParameters();

  for (int i = 0; i < totalParameters; i++) {
    NetDMFParameter *param = node->GetParameter(i);
    params.push_back(param);
  }
}

void parseDevices(NetDMFNode *node, std::vector<NetDMFDevice *>dev)
{
  int totalDevices = node->GetNumberOfDevices();

  for (int i = 0; i < totalDevices; i++) {
    NetDMFDevice *device = node->GetDevice(i);
    dev.push_back(device);
  }
}

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

    // Parse Nodes
    // Parse Channels
  }
}

/**
 * Parse the scenarios in the NetDMF file.  Logic stolen from Payton's
 * python script NetDMFtoOpNet.py
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
