#include <NetDMF.h>
#include <NetDMFDOM.h>
#include <NetDMFNode.h>
#include <NetDMFXmlNode.h>

/**
 * @file
 * @brief NetDMF Wrappers
 * 
 * This file provides wrappers for functions
 * to enable C linkage.  These functions will perform most of the actions
 * and provide it to the C front-end.
 */

static NetDMFDOM *dom = 0;

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
  XdmfXmlNode  Parent, FirstChild, SecondChild;
  NetDMFNode node;

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
