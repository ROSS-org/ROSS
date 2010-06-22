#include <NetDMF.h>
#include <NetDMFDOM.h>
#include <NetDMFNode.h>

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
  //XdmfDOM    *DOM = new XdmfDOM();
  dom = new NetDMFDOM();
  if (dom == 0) {
    printf("We have a problem\n");
    exit(-1);
  }
  XdmfXmlNode  Parent, FirstChild, SecondChild;
  NetDMFNode foo;

  // Parse the XML File
  dom->SetInputFileName("MyFile.xml");
  retval = dom->Parse();
  if (retval != XDMF_SUCCESS) {
    printf("We have a problem\n");
    exit(-1);
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
