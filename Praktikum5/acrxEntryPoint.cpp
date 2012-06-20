// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "TwoThreeTree.h"
#include <vector>
#include <algorithm>
#include <math.h>

#define PI 3.14159265358979323846

using namespace std;
using namespace Tree;

//-----------------------------------------------------------------------------
#define szRDS _RXST("CGCAD")

//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
//-----------------------------------------------------------------------------



// Typedef --------------------------------------------------------------------
typedef pair<AcGePoint3d, AcGePoint3d> Edge;

// Globale Variablen ----------------------------------------------------------
vector<AcGePoint3d> polyPoints;					// alle Punkte der Polyline p(0) bis p(n-1)
vector<TwoThreeTree> trees;						// zu jedem Punkt ein Tree
vector<Edge> outerEdges;
vector<Edge> innerEdges;

class CPraktikum5App : public AcRxArxApp {

																			public:
	CPraktikum5App () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// TODO: Load dependencies here

		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;

		// TODO: Add your initialization code here

		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		// TODO: Unload dependencies here

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}

	//-----------------------------------------------------------------------------
	// Los gehts!
	//-----------------------------------------------------------------------------
	
	static void CGCADPraktikum5triangularize(void){
		Greedy();
	}

	static void Greedy(){
			
			setPolyPoints();					// Es wird davon ausgegangen, dass tats�chlich eine Polyline existiert
			setTrees();
			setOuterAngles(polyPoints, trees);
			setOuterEdges();
			setInnerEdges();
			sortInnerEdges();
			createPolygons(polyPoints, trees);
		
	}

	static void setPolyPoints(){
		struct resbuf polylineFilter;
		polylineFilter.restype = 0;
		polylineFilter.resval.rstring = _T("LWPOLYLINE");
		polylineFilter.rbnext = NULL;
		
		AcCmColor color;
		color.setRGB(0,0,255);
		createLayer(_T("trianguliert"),color);

		ads_name ssname;
		long length = 0;
		int res = acedSSGet(_T("X"), NULL, NULL, &polylineFilter, ssname);
		if(res != RTNORM) return;
		acedSSLength(ssname, &length);
		for(int k = 0; k < length; k++){
			ads_name ent;
			AcDbObjectId id = AcDbObjectId::kNull;
			AcDbEntity* tmp;

			if(acedSSName(ssname,k,ent) != RTNORM) return;
			if(acdbGetObjectId(id,ent) != Acad::eOk) return; 
			if(acdbOpenAcDbEntity(tmp,id,AcDb::kForRead) != Acad::eOk) return;
			
			// gib mir die Polyline
			AcDbPolyline* polyline = (AcDbPolyline*)tmp;
			
			// global gesetzt
			//vector<AcGePoint3d> polyPoints;				// alle Punkte der Polyline p0-p(n-1)
			//vector<TwoThreeTree> trees;					// zu jedem Punkt ein Tree
						
			for(int i = 0; i < polyline->numVerts(); i++)	// gehe alle Punkte durch 
			{
				AcGePoint3d pt;
				polyline->getPointAt(i, pt);				// gib mir den punkt im index i
				bool exists = false;
				for(int j = 0; j < polyPoints.size(); j++) {
					if(polyPoints[j].x == pt.x && polyPoints[j].y == pt.y && polyPoints[j].z == pt.z) {
						exists = true;
					}
				}
				if(!exists){
					polyPoints.push_back(pt);					// und steck ihn in polyPoints
				//	TwoThreeTree tree;							
				//	tree.insertAngle(360,0);					// Tree init f�r jeden Punkt
				//	trees.push_back(tree);						// leg f�r jeden Punkt Tree mit 0�-360� an
				}
			}

			polyline->close();
		}
	}

	static void setTrees(){
		for(int i=0; i < polyPoints.size(); i++){
			TwoThreeTree tree;							
			tree.insertAngle(360,0);					// Tree init f�r jeden Punkt
			trees.push_back(tree);						// leg f�r jeden Punkt Tree mit 0�-360� an
		}
	}

	static void createLayer(const ACHAR* name, AcCmColor &color)
	{
		//database connect
		AcDbDatabase* pDB = acdbHostApplicationServices()->workingDatabase();

		//layer table
		AcDbLayerTable* pLayerTbl = NULL; 

		// Get the LayerTable for write because we will create a new entry 
		pDB->getSymbolTable(pLayerTbl,AcDb::kForWrite); 

		// Check if the layer is already there 
		if (!pLayerTbl->has(name)) // _T for char Unicode convert  
		{ 
			// Instantiate a new object and set its properties 
			AcDbLayerTableRecord *pLayerTblRcd = new AcDbLayerTableRecord; 
			pLayerTblRcd->setName(name); 
			pLayerTblRcd->setIsFrozen(0); // layer set to THAWED 
			pLayerTblRcd->setIsOff(0);    // layer set to ON 
			pLayerTblRcd->setIsLocked(0); // layer un-locked 
			pLayerTblRcd->setColor(color);
			// Now, add the new layer to its container 
			pLayerTbl->add(pLayerTblRcd); 

			// Close the container 
			pLayerTblRcd->close();
			pLayerTbl->close();

		}  
		else  
		{ 
			// If our layer is already there, just close the container and continue 
			pLayerTbl->close(); 
			acutPrintf(_T("\n"));
			acutPrintf(name);
			acutPrintf(_T(" already exists")); //output in the status line 
		}
	}

	static bool compareEdgesBySize(Edge edge1, Edge edge2)
	{
		if(edge1.first.distanceTo(edge1.second) <= edge2.first.distanceTo(edge2.second))
			return true;
		else
			return false;
	}

	static void setOuterEdges(){
		for(int i = 0; i < polyPoints.size(); i++)
		{
			for(int j = i + 1; j < polyPoints.size(); j++)
			{
				if(i + 1 == j || (i == 0 && j == polyPoints.size() - 1))
					outerEdges.push_back(Edge(polyPoints[i], polyPoints[j]));
				else
					continue;
			}
		}
	}
	
	static void setInnerEdges(){
		for(int i = 0; i < polyPoints.size(); i++)
		{
			for(int j = i + 1; j < polyPoints.size(); j++)
			{
				if(i + 1 == j || (i == 0 && j == polyPoints.size() - 1))
					continue;
				else
					innerEdges.push_back(Edge(polyPoints[i], polyPoints[j]));
			}
		}
	}

	static void sortInnerEdges(){
		std::sort(innerEdges.begin(), innerEdges.end(), compareEdgesBySize);
	}

	static void createPolygons(std::vector<AcGePoint3d> &polyPoints, std::vector<TwoThreeTree> &trees)
	{
		//std::vector<Edge> outerEdges;
		//std::vector<Edge> innerEdges;
		std::vector<Edge> newEdges;
		/*for(int i = 0; i < polyPoints.size(); i++)
		{
			for(int j = i + 1; j < polyPoints.size(); j++)
			{
				if(i + 1 == j || (i == 0 && j == polyPoints.size() - 1))
					outerEdges.push_back(Edge(polyPoints[i], polyPoints[j]));
				else
					innerEdges.push_back(Edge(polyPoints[i], polyPoints[j]));
			}
		}*/


		for(int i = 0; i < innerEdges.size() && newEdges.size() < polyPoints.size() - 3; i++)
		{	
			if(!isEdgeBlocked(innerEdges[i], polyPoints, trees))
			{
				appendEdge(innerEdges[i], newEdges, polyPoints, trees);					
			}
		}

		drawPolygons(newEdges, outerEdges);
	}

	static bool isEdgeBlocked(Edge &edge, std::vector<AcGePoint3d> &polyPoints, std::vector<TwoThreeTree> &trees)
	{
		for(int j = 0; j < polyPoints.size(); j++)
		{
			if(polyPoints[j] == edge.first)
			{
				int angle = (int)((atan2(edge.second.y - polyPoints[j].y,edge.second.x - polyPoints[j].x)) * 180 / PI);
				angle < 0 ? angle += 360: angle;

				int edge = trees[j].getEdgeForAngle(angle);
				acutPrintf(_T("polyPoints[i]: %d angle: %d \n"), j, angle);
				if(edge != 0)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		}
		return false;
	}

	static void appendEdge(Edge edge, std::vector<Edge> &edges, std::vector<AcGePoint3d> &polyPoints, std::vector<TwoThreeTree> &trees)
	{
		edges.push_back(edge);
		for(int i = 0; i < trees.size(); i++)
		{
			if(edge.first != polyPoints[i] && edge.second != polyPoints[i])
			{
				int angle[2];
				angle[0] = (int)(atan2(edge.first.y - polyPoints[i].y, edge.first.x - polyPoints[i].x) * 180 / PI);
				angle[1] = (int)(atan2(edge.second.y - polyPoints[i].y, edge.second.x - polyPoints[i].x) * 180 / PI);

				angle[0] < 0 ? angle[0] += 360: angle[0];
				angle[1] < 0 ? angle[1] += 360: angle[1];

				int tmp = angle[1];
				if(angle[1] < angle[0])
					tmp += 360;

				int diff = tmp - angle[0];
				if(diff > 180)
				{
					acutPrintf(_T("polyPoints[i]: %d startangle: %d endangle: %d\n"), i, angle[1], angle[0]);
					trees[i].setBlockingEdge(angle[1], angle[0], edges.size());
				}
				else
				{
					acutPrintf(_T("polyPoints[i]: %d startangle: %d endangle: %d\n"), i, angle[0], angle[1]);
					trees[i].setBlockingEdge(angle[0], angle[1], edges.size());
				}
			}
		}
	}

	static void drawPolygons(std::vector<Edge> &innerEdges, std::vector<Edge> &outerEdges)
	{
		{
			//database connect
			AcDbDatabase* pDB = acdbHostApplicationServices()->workingDatabase();

			//blocktable init
			AcDbBlockTable* pBlockTable = NULL;
			pDB->getSymbolTable(pBlockTable, AcDb::kForRead);

			//blocktable record init
			AcDbBlockTableRecord* pBlockTableRecord = NULL;
			pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForWrite);

			pBlockTable->close();

			/*				for(int i = 0; i < polyPoints.size(); i++)
			{
			AcDbFace* face;
			for(int j = 0; j < newEdges.size(); j++)
			{
			if(polyPoints[i] == newEdges[j].first)
			{
			face = new AcDbFace(newEdges[j].first, newEdges[j].second, polyPoints[i == 0 ? (polyPoints.size() - 1) : i - 1], TRUE, TRUE, TRUE);
			face->setLayer(_T("TRIANG"));
			pBlockTableRecord->appendAcDbEntity(face);
			face->close();
			}
			}
			}
			*/
			AcDbLine* line;
			for(int j = 0; j < innerEdges.size(); j++)
			{
				line = new AcDbLine(innerEdges[j].first, innerEdges[j].second);
				line->setLayer(_T("TRIANG"));
				pBlockTableRecord->appendAcDbEntity(line);
				line->close();
				ads_real result;
				acedGetReal(_T("Block"), &result);
			}
			pBlockTableRecord->close();

		}
	}

	static void setOuterAngles(std::vector<AcGePoint3d> &polyPoints, std::vector<TwoThreeTree> &trees){
		//----------------------------------------------------------------------------------------
		// bestimmt die anfangs und endwinkel eines knotens und f�gt sie zu trees hinzu
		//----------------------------------------------------------------------------------------

		for(int i = 0; i < polyPoints.size(); i++){
															
			int previous = i - 1;
			int next = i + 1;
			if(previous < 0)
				previous += polyPoints.size();
			if(next >= polyPoints.size())
				next = 0;

			int angle[2];
															// previous	= kante davor
															// next		= kante danach
															// angle[2]	= winkel von previous = angle[0] und von next = angle[1]

			angle[0] = getAngle(polyPoints, i, previous);	// angle[0] = start winkel
			angle[1] = getAngle(polyPoints, i, next);		// angle[1] = end winkel
			
															
			angle[0] < 0 ? angle[0] += 360: angle[0];		// atan2 liefert zahlen zwischen (-180) - (180)...
			angle[1] < 0 ? angle[1] += 360: angle[1];		// ... daher +=360 wenn negativ

			//acutPrintf(_T("polyPoints[%d]: startangle: %d endangle: %d\n"), i, angle[0], angle[1]);
			trees[i].setBlockingEdge(angle[0], angle[1], -1);
		}
		//acutPrintf(_T("________________________________"));
	}

	static int getAngle(std::vector<AcGePoint3d> &polyPoints, int& current, int& neighbour){
		 return (int)(atan2(polyPoints[neighbour].y - polyPoints[current].y, polyPoints[neighbour].x - polyPoints[current].x) * 180 / PI);
	}
} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CPraktikum5App)

	ACED_ARXCOMMAND_ENTRY_AUTO(CPraktikum5App, CGCADPraktikum5, triangularize, triangulieren, ACRX_CMD_TRANSPARENT, NULL)
