#include <NetDMF.h>
#include <NetDMFDOM.h>

/** 
 * @file rn-netdmf.c  This file contains all NetDMF support.  We'll try and
 * keep this file as similar as possible to the XML support.
 */

/**
 * @fn rn_netdmf_init  This function handles initialization of the NetDMF
 * description language.
 */
void
rn_netdmf_init()
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
 * @fn rn_netdmf_topology  This function creates and setups the
 * g_rn_machines global data structure of nodes.
 */
void
rn_netdmf_topology()
{
}
