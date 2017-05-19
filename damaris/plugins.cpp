#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#include "Damaris.h"
#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include "damaris/data/Block.hpp"
#include "damaris/data/Layout.hpp"
#include "damaris/env/Environment.hpp"
#include "damaris/buffer/DataSpace.hpp"
#include "damaris/buffer/Buffer.hpp"
#include "damaris/model/Model.hpp"

#include "vtkSmartPointer.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkTree.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "vtkGraphicsFactory.h"
#include "vtkRenderWindow.h"
#include "vtkGraphMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkGraphLayout.h"

using namespace damaris;
//using namespace boost;


int last_render_itr = 0;
void analyze_data(int **data, int num_clients, int *num_items);
void create_treemap(int num_clients, int total_items, int nkp);

/*
 * Set up as a Damaris bcast event type
 * if one client sends, all servers will perform this
 * can do collective among all damaris servers with this
 *
 * for now set as a group event - all clients must initiate event for server to execute
 */
extern "C"{
void aggregate_data(const std::string& event, int32_t src, int32_t iteration, const char* args)
{
    int clients_per_node = Environment::ClientsPerNode();
    int total_clients = Environment::CountTotalClients();
    int64_t start_idx, end_idx = 0;
    DataSpace<Buffer> ds;
    //std::cout << "Step " << iteration << std::endl;

    // determine when to render a new vis
    // for now, find percent difference between max and min of the switch LPs 
    shared_ptr<Variable> rem_sends_gvt = VariableManager::Search("ross/gvt_inst/lps/network_sends");
    const model::Type& var_type = rem_sends_gvt->GetLayout()->GetType();
    int *agg_data[clients_per_node];
    int num_items[clients_per_node];
    int total_items = 0;

    if (!rem_sends_gvt)
        printf("rem_sends_gvt is NULL!!\n");

    // TODO don't assume 1 damaris server per node
    // TODO client ids for multiple nodes?
    if (iteration > 0)
    {
        for (int i = 0; i < clients_per_node; i++)
        {
            shared_ptr<Block> cur_block = rem_sends_gvt->GetBlock(i, iteration, 0);
            if (cur_block)
            {
                start_idx = cur_block->GetStartIndex(0);
                end_idx = cur_block->GetEndIndex(0);
                ds = cur_block->GetDataSpace();
                if (var_type == model::Type::int_ || var_type == model::Type::integer)
                {
                    agg_data[i] = (int*) ds.GetData();
                    num_items[i] = cur_block->NbrOfItems();
                    total_items += num_items[i];
                }

            }
        }

        analyze_data(agg_data, clients_per_node, num_items);
    }

}
}

void analyze_data(int **data, int num_clients, int *num_items)
{
    int min = INT_MAX, max = 0;
    double per_diff = 0;

    for (int i = 0; i < num_clients; i++)
    {
        for (int j = 0; j < num_items[i]; j++)
        {
            if (data[i][j] < min)
                min = data[i][j];
            if (data[i][j] > max)
                max = data[i][j];
        }
    }

    per_diff = (max - min)/((max + min)/2.0);

    if (per_diff > .2)
    {
        create_treemap(num_clients, num_items[0] * num_clients, 16);

    }
}

int img_num = 0;
void create_treemap(int num_clients, int total_items, int nkp)
{
    std::ostringstream tmp;
    tmp << "screenshot-" << img_num << ".png";
    std::string filename = tmp.str();
    img_num++;

    //vtkSmartPointer<vtkGraphicsFactory> gf = vtkSmartPointer<vtkGraphicsFactory>::New();
    //gf->SetOffScreenOnlyMode(1);
    //gf->SetUseMesaClasses(1);

    vtkSmartPointer<vtkMutableDirectedGraph> g = vtkSmartPointer<vtkMutableDirectedGraph>::New();
    vtkSmartPointer<vtkTree> tree = vtkSmartPointer<vtkTree>::New();

    //TODO move this tree set up to some init function; don't want to keep recreating this
    vtkIdType root = g->AddVertex();
    vtkIdType pe_nodes[num_clients];
    vtkIdType kp_nodes[nkp];
    vtkIdType lp_nodes[total_items];

    for (int i = 0; i < num_clients; i++)
    {
        pe_nodes[i] = g->AddVertex();
        g->AddEdge(root, pe_nodes[i]);
        for (int j = 0; j < nkp; j++)
        {
            kp_nodes[j] = g->AddVertex();
            g->AddEdge(pe_nodes[i], kp_nodes[j]);
            for (int k = 0; k < total_items; k++)
            {
                lp_nodes[k] = g->AddVertex();
                g->AddEdge(kp_nodes[j], lp_nodes[k]);
            }
        }
    }

    bool success = tree->CheckedShallowCopy(g);
    vtkSmartPointer<vtkTreeLayoutStrategy> strategy = vtkSmartPointer<vtkTreeLayoutStrategy>::New();
    vtkSmartPointer<vtkGraphLayout> layout = vtkSmartPointer<vtkGraphLayout>::New();
    layout->SetInputData(tree);
    layout->SetLayoutStrategy(strategy);

    vtkSmartPointer<vtkGraphMapper> mapper = vtkSmartPointer<vtkGraphMapper>::New();
    mapper->SetInputConnection(layout->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    renderer->SetBackground(1,1,1);
    vtkSmartPointer<vtkRenderWindow> rw = vtkSmartPointer<vtkRenderWindow>::New();
    rw->AddRenderer(renderer);
    rw->SetAlphaBitPlanes(1);
    rw->SetSize(300,300);
    rw->SetUseOffScreenBuffers(1);
    rw->OffScreenRenderingOn();

    //vtkSmartPointer<vtkRenderWindowInteractor> rwi = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    //rwi->SetRenderWindow(rw);


    rw->Render();

    //vtkSmartPointer<vtkGraphLayoutView> tree_view = vtkSmartPointer<vtkGraphLayoutView>::New();
    //tree_view->AddRepresentationFromInput(tree);
    //tree_view->SetLayoutStrategyToTree();
//    rw = tree_view->GetRenderWindow();
//    rw->SetOffScreenRendering(1);
//    tree_view->SetRenderWindow(rw);
    //tree_view->ResetCamera();
    //tree_view->Render();
    //tree_view->GetInteractor()->Start();

    vtkSmartPointer<vtkWindowToImageFilter> wi = vtkSmartPointer<vtkWindowToImageFilter>::New();
    wi->SetInput(rw);
    wi->SetMagnification(3);
    wi->SetInputBufferTypeToRGBA();
    wi->ReadFrontBufferOff();
    wi->Update();

    vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(filename.c_str());
    writer->SetInputConnection(wi->GetOutputPort());
    writer->Write();

    //rw->Render();
    //renderer->ResetCamera();
    //rw->Render();
    //rwi->Start();

/*
    gvt_lp_source = vtkDelimitedTextReader()
    gvt_lp_source.SetFieldDelimiterCharacters(",")
    gvt_lp_source.SetHaveHeaders(True)
#gvt_lp_source.SetMaxRecords(1151)
    gvt_lp_source.SetDetectNumericColumns(True)
    gvt_lp_source.SetFileName(file_dir + "/ross-stats-gvt-lps-samp.csv")
    gvt_lp_source.SetOutputPedigreeIds(True)
    gvt_lp_source.SetPedigreeIdArrayName("LP_ID")

# get lp event data
    lp_events = vtkDelimitedTextReader()
    lp_events.SetFieldDelimiterCharacters(",")
    lp_events.SetHaveHeaders(True)
    lp_events.SetDetectNumericColumns(True)
#lp_events.SetMaxRecords(10)
    lp_events.SetFileName(file_dir + "/ross-stats-comm-pruned.csv")
    lp_events.SetOutputPedigreeIds(True)
#lp_events.SetPedigreeIdArrayName("dest_lp")
    lp_events.Update()

    lp_thresh = vtkThresholdTable()
    lp_thresh.SetInputConnection(lp_events.GetOutputPort())
    lp_thresh.SetInputArrayToProcess(0,0,0,6, "num_msgs")
    lp_thresh.SetMode(1) # 1 = Accept Greater than
    lp_thresh.SetMinValue(vtkVariant(100))
    lp_thresh.Update()

#reader1 = vtkXMLTreeReader()
#reader1.SetFileName(file_dir + "/ross-stats-comm.xml")
#reader1.SetEdgePedigreeIdArrayName("tree edge")
#reader1.GenerateVertexPedigreeIdsOn();
#reader1.SetVertexPedigreeIdArrayName("LP_ID");
#reader1.Update()

#numeric = vtkStringToNumeric()
#numeric.SetInputConnection(reader1.GetOutputPort())
#numeric.Update()

    gvt_lp_ttt = vtkTableToTreeFilter()
    gvt_lp_ttt.SetInputConnection(gvt_lp_source.GetOutputPort())

#lp_ev_ttt = vtkTableToTreeFilter()
#lp_ev_ttt.DebugOn()
#lp_ev_ttt.SetInputConnection(lp_events.GetOutputPort())
#
#group_lp_ev = vtkGroupLeafVertices()
#group_lp_ev.DebugOn()
#group_lp_ev.SetInputConnection(reader1.GetOutputPort())
#group_lp_ev.SetInputArrayToProcess(0, 0, 0, VERTICES, "LP_ID")
#group_lp_ev.SetInputArrayToProcess(1, 0, 0, VERTICES, "dest_lp")
#group_lp_ev.SetGroupDomain("LP_ID")
#print(group_lp_ev.GetInputArrayInformation(0))
#print(group_lp_ev.GetInputArrayInformation(1))
#print(group_lp_ev.GetGroupDomain())
#group_lp_ev.Update()

#group_lp_tlf = vtkTreeLevelsFilter()
#group_lp_tlf.SetInputConnection(group_lp_ev.GetOutputPort())

    lpev_ttg = vtkTableToGraph()
    lpev_ttg.SetInputConnection(lp_thresh.GetOutputPort())
    lpev_ttg.AddLinkVertex("src_lp", "LP_ID", False)
    lpev_ttg.AddLinkVertex("dest_lp", "LP_ID", False)
#lpev_ttg.AddLinkVertex("num_msgs", "num_msgs", True)
    lpev_ttg.AddLinkEdge("src_lp", "dest_lp")
    lpev_ttg.SetDirected(True)
    lpev_ttg.Update()

    lpev_rem = vtkRemoveIsolatedVertices()
    lpev_rem.SetInputConnection(lpev_ttg.GetOutputPort())

#print(lp_events.GetOutput())
##lpev_ttg.GetOutput().GetEdgeData().AddArray(lp_events.GetOutput())
#edges = lpev_ttg.GetOutput().GetEdgeData()
#print(lpev_ttg.GetOutput().GetEdgeData())
#edges.SetActiveScalars("num_msgs")
#print(edges)
#print(lpev_ttg)

    group_pes = vtkGroupLeafVertices()
    group_pes.SetInputConnection(gvt_lp_ttt.GetOutputPort())
    group_pes.SetInputArrayToProcess(0, 0, 0, VERTICES, "PE_ID")
    group_pes.SetInputArrayToProcess(1, 0, 0, VERTICES, "LP_ID")

    group_kps = vtkGroupLeafVertices()
    group_kps.SetInputConnection(group_pes.GetOutputPort())
    group_kps.SetInputArrayToProcess(0, 0, 0, VERTICES, "KP_ID")
    group_kps.SetInputArrayToProcess(1, 0, 0, VERTICES, "LP_ID")

    ev_rb_agg = vtkTreeFieldAggregator()
    ev_rb_agg.SetInputConnection(group_kps.GetOutputPort())
    ev_rb_agg.SetLeafVertexUnitSize(False)
    ev_rb_agg.SetField("remote_sends")
    ev_rb_agg.Update()

    edges = vtkExtractEdges()
    edges.SetInputData(lpev_ttg.GetOutput())
    edges.Update()
    mapper = vtkPolyDataMapper()
    mapper.SetInputData(edges.GetOutput())
    actor = vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetOpacity(0.9)

    view = vtkTreeMapView()
#view = vtkTreeRingView()
#view.SetRepresentationFromInputConnection(ev_rb_agg.GetOutputPort());
    view.SetTreeFromInputConnection(ev_rb_agg.GetOutputPort())
    view.SetGraphFromInputConnection(lpev_ttg.GetOutputPort())
    view.SetLayoutStrategyToSquarify();
    view.SetAreaColorArrayName("level")
    view.SetAreaLabelVisibility(True);
    view.SetAreaLabelArrayName("LP_ID");
    view.SetAreaSizeArrayName("remote_sends");
    view.SetBundlingStrength(.9)
#view.SetEdgeLabelVisibility(True)
#view.SetEdgeLabelArrayName("num_msgs")
    view.Update()
    view.SetColorEdges(True)
    view.SetEdgeColorArrayName("num_msgs")
#view.SetEdgeScalarBarVisibility(True)
#view.SetEdgeColorArrayName("edge")

#view1 = vtkGraphLayoutView()
##view1.AddRepresentationFromInput(comm_graph)
#view1.AddRepresentationFromInputConnection(lpev_ttg.GetOutputPort())
##view1.SetVertexLabelArrayName("PE_ID")
##view1.SetVertexLabelArrayName("KP_ID")
#view1.SetVertexLabelArrayName("LP_ID")
#view1.SetVertexLabelVisibility(True)
##view1.SetLayoutStrategyToTree()
#view1.SetVertexLabelFontSize(20)
##view1.SetEdgeLabelVisibility(True)

#view.SetLayoutStrategyToSquarify();
#view.SetAreaSizeArrayName("");
#view.SetAreaColorArrayName("level");
#view.SetAreaHoverArrayName("KP_ID");

# Apply a theme to the views
    theme = vtkViewTheme.CreateMellowTheme()
    view.ApplyViewTheme(theme)
#view1.ApplyViewTheme(theme)
    theme.FastDelete()

#renderer = vtkRenderer()
#renWin = vtkRenderWindow()
#renWin.AddRenderer(renderer)
#
#iren = vtkRenderWindowInteractor()
#iren.SetRenderWindow(renWin)
#
#renWin.Render()
#iren.Start()
    renderer = view.GetRenderer()
    renderer.AddActor(actor)
    view.Update()

    view.ResetCamera()
    view.Render()
#
##view1.ResetCamera()
##view1.Render()
#
    view.GetInteractor().Start()
*/    
}

