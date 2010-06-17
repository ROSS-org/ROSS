#include <NetDMF.h>
#include <NetDMFDOM.h>

/** 
 * @file rn-netdmf-wrapper.cpp  This file provides wrappers for functions
 * to enable C linkage.  It provides little else.
 */

/**
 * @fn rnNetDMFInit  This function handles initialization of the NetDMF
 * description language.  The function currently doesn't do much except
 * demonstrate proper linkage.
 */
extern "C"
void
rnNetDMFInit()
{
  
  XdmfDOM    *DOM = new XdmfDOM();
  XdmfXmlNode  Parent, FirstChild, SecondChild;

  // Parse the XML File
  DOM->SetInputFileName("MyFile.xml");
  DOM->Parse();
  // Find the first element with TAG = Tag1
  Parent = DOM->FindElement("Tag1");
  // Find the first (zero based) Tag2 below Parent
  FirstChild = DOM->FindElement("Tag2", 0, Parent);
  //cout << "The Name of the First Child is <" << DOM->Get(FirstChild, "Name") << ">" << endl;
  // Find the second (zero based) Tag2 below Parent
  SecondChild = DOM->FindElement("Tag2", 1, Parent);
  DOM->Set(SecondChild, "Age", "10");
  DOM->DeleteNode(FirstChild);
  //cout << endl << "XML = " << endl << DOM->Serialize(Parent) << endl;
}

/**
 * @fn rnNetDMFTopology  This function creates and setups the
 * g_rn_machines global data structure of nodes.  Non-functional atm.
 */
extern "C"
void
rnNetDMFTopology()
{
}
