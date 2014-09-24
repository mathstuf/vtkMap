/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapMarkerSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapMarkerSet.h"
#include "vtkTeardropSource.h"
#include <vtkActor.h>
#include <vtkDataArray.h>
#include <vtkDistanceToCamera.h>
#include <vtkIdTypeArray.h>
#include <vtkGlyph3D.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

double lat2y(double);

vtkStandardNewMacro(vtkMapMarkerSet)

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet()
{
  this->Renderer = NULL;
  this->MarkerPolyData = NULL;
  this->Mapper = NULL;
  this->Actor = NULL;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::~vtkMapMarkerSet()
{
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::SetRenderer(vtkRenderer *renderer)
{
  this->Renderer = renderer;
  this->MarkerPolyData = vtkPolyData::New();

  // Initialize rendering pipeline
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  this->MarkerPolyData->SetPoints(points.GetPointer());

  // Data array for marker color
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName("Colors");
  colors->SetNumberOfComponents(3);
  this->MarkerPolyData->GetPointData()->AddArray(colors.GetPointer());

  // Use DistanceToCamera filter to scale markers to constant screen size
  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(50.0);
  dFilter->SetRenderer(this->Renderer);
  dFilter->SetInputData(this->MarkerPolyData);

  // Instantiate marker glyph
  vtkNew<vtkTeardropSource> marker;
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(dFilter->GetOutputPort());
  glyph->SetSourceConnection(marker->GetOutputPort());
  glyph->ScalingOn();
  glyph->SetScaleFactor(1.0);
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetColorModeToColorByScalar();
  // Just gotta know this:
  glyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  glyph->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Colors");
  glyph->GeneratePointIdsOn();

  // Rotate the glyph to point downward (parallel to y axis)
  vtkNew<vtkTransform> transform;
  transform->RotateZ(90.0);
  glyph->SetSourceTransform(transform.GetPointer());

  // Setup mapper and actor
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(glyph->GetOutputPort());
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double Latitude, double Longitude)
{
  // Cannot add markers until renderer is set
  if (this->MarkerPolyData == NULL)
    {
    return -1;
    }

  vtkPoints *points = this->MarkerPolyData->GetPoints();
  vtkIdType id = points->GetNumberOfPoints();  // return value
  double x = Longitude;
  double y = lat2y(Latitude);
  points->InsertNextPoint(x, y, 0.0);

  unsigned char kwBlue[] = {0, 83, 155};
  vtkDataArray *data =
    this->MarkerPolyData->GetPointData()->GetArray("Colors");
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(data);
  colors->InsertNextTupleValue(kwBlue);

  this->MarkerPolyData->Modified();
  return id;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::RemoveMapMarkers()
{
   if (this->MarkerPolyData == NULL)
    {
    return;
    }

  vtkPoints *points = this->MarkerPolyData->GetPoints();
  points->Reset();

  vtkDataArray *data =
    this->MarkerPolyData->GetPointData()->GetArray("Colors");
  data->Reset();

  this->MarkerPolyData->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::
PickMarker(vtkRenderer *renderer, vtkPicker *picker, int displayCoords[2])
{
  vtkIdType markerId = -1;  // return value

  vtkPointPicker *pointPicker = vtkPointPicker::SafeDownCast(picker);
  if (pointPicker &&
      (pointPicker->Pick(displayCoords[0], displayCoords[1], 0.0, renderer)))
    {
    // std::cout << "Using tolerance " << pointPicker->GetTolerance() << std::endl;
    // vtkPoints *pickPoints = pointPicker->GetPickedPositions();
    // std::cout << "Picked " << pickPoints->GetNumberOfPoints() << " points "
    //           << std::endl;

    vtkIdType pointId = pointPicker->GetPointId();
    if (pointId > 0)
      {
      vtkDataArray *dataArray =
        pointPicker->GetDataSet()->GetPointData()->GetArray("InputPointIds");
      vtkIdTypeArray *glyphIdArray = vtkIdTypeArray::SafeDownCast(dataArray);
      if (glyphIdArray)
        {
        markerId = glyphIdArray->GetValue(pointId);
        //std::cout << "Point id " << pointId
        //          << " - Data " << markerId << std::endl;
        }

      }
    }

  return markerId;
}