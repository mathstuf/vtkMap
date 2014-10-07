/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOsmLayer.h"

#include "vtkMercator.h"

#include <vtkObjectFactory.h>

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkOsmLayer)

//----------------------------------------------------------------------------
struct sortTiles
{
  inline bool operator() (vtkMapTile* tile1,  vtkMapTile* tile2)
  {
    return (tile1->GetBin() < tile2->GetBin());
  }
};

//----------------------------------------------------------------------------
vtkOsmLayer::vtkOsmLayer() : vtkLayer()
{
}

//----------------------------------------------------------------------------
vtkOsmLayer::~vtkOsmLayer()
{
}

//----------------------------------------------------------------------------
void vtkOsmLayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsmLayer::Update()
{
  this->AddTiles();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::RemoveTiles()
{
  // TODO
  if (!this->Renderer)
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTiles()
{
  if (!this->Renderer)
    {
    return;
    }

  double focusDisplayPoint[3], bottomLeft[4], topRight[4];
  int width, height, llx, lly;

  this->Renderer->SetWorldPoint(0.0, 0.0, 0.0, 1.0);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(focusDisplayPoint);

  this->Renderer->GetTiledSizeAndOrigin(&width, &height, &llx, &lly);
  this->Renderer->SetDisplayPoint(llx, lly, focusDisplayPoint[2]);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(bottomLeft);

  if (bottomLeft[3] != 0.0)
    {
    bottomLeft[0] /= bottomLeft[3];
    bottomLeft[1] /= bottomLeft[3];
    bottomLeft[2] /= bottomLeft[3];
    }

  //std::cerr << "Before bottomLeft " << bottomLeft[0] << " " << bottomLeft[1] << std::endl;

  bottomLeft[0] = std::max(bottomLeft[0], -180.0);
  bottomLeft[0] = std::min(bottomLeft[0],  180.0);
  bottomLeft[1] = std::max(bottomLeft[1], -180.0);
  bottomLeft[1] = std::min(bottomLeft[1],  180.0);

  this->Renderer->SetDisplayPoint(llx + width, lly + height, focusDisplayPoint[2]);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(topRight);

  if (topRight[3] != 0.0)
    {
    topRight[0] /= topRight[3];
    topRight[1] /= topRight[3];
    topRight[2] /= topRight[3];
    }

  topRight[0] = std::max(topRight[0], -180.0);
  topRight[0] = std::min(topRight[0],  180.0);
  topRight[1] = std::max(topRight[1], -180.0);
  topRight[1] = std::min(topRight[1],  180.0);

  int tile1x = vtkMercator::long2tilex(bottomLeft[0], this->Map->GetTileZoom());
  int tile2x = vtkMercator::long2tilex(topRight[0], this->Map->GetTileZoom());

  int tile1y = vtkMercator::lat2tiley(
                 vtkMercator::y2lat(bottomLeft[1]), this->Map->GetTileZoom());
  int tile2y = vtkMercator::lat2tiley(
                 vtkMercator::y2lat(topRight[1]), this->Map->GetTileZoom());

  //std::cerr << "tile1y " << tile1y << " " << tile2y << std::endl;

  if (tile2y > tile1y)
    {
    int temp = tile1y;
    tile1y = tile2y;
    tile2y = temp;
    }

  //std::cerr << "Before bottomLeft " << bottomLeft[0] << " " << bottomLeft[1] << std::endl;
  //std::cerr << "Before topRight " << topRight[0] << " " << topRight[1] << std::endl;

  /// Clamp tilex and tiley
  tile1x = std::max(tile1x, 0);
  tile1x = std::min(static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1, tile1x);
  tile2x = std::max(tile2x, 0);
  tile2x = std::min(static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1, tile2x);

  tile1y = std::max(tile1y, 0);
  tile1y = std::min(static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1, tile1y);
  tile2y = std::max(tile2y, 0);
  tile2y = std::min(static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1, tile2y);

  int noOfTilesX = std::max(1, static_cast<int>(pow(2, this->Map->GetTileZoom())));
  int noOfTilesY = std::max(1, static_cast<int>(pow(2, this->Map->GetTileZoom())));

  double lonPerTile = 360.0 / noOfTilesX;
  double latPerTile = 360.0 / noOfTilesY;

  //std::cerr << "llx " << llx << " lly " << lly << " " << height << std::endl;
  //std::cerr << "tile1y " << tile1y << " " << tile2y << std::endl;

  //std::cerr << "tile1x " << tile1x << " tile2x " << tile2x << std::endl;
  //std::cerr << "tile1y " << tile1y << " tile2y " << tile2y << std::endl;

  int xIndex, yIndex;
  for (int i = tile1x; i <= tile2x; ++i)
    {
    for (int j = tile2y; j <= tile1y; ++j)
      {
      xIndex = i;
      yIndex = static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1 - j;

      vtkMapTile* tile = this->GetCachedTile(this->Map->GetTileZoom(), xIndex, yIndex);
      if (!tile)
        {
        tile = vtkMapTile::New();
        double llx = -180.0 + xIndex * lonPerTile;
        double lly = -180.0 + yIndex * latPerTile;
        double urx = -180.0 + (xIndex + 1) * lonPerTile;
        double ury = -180.0 + (yIndex + 1) * latPerTile;

        tile->SetCorners(llx, lly, urx, ury);

        std::ostringstream oss;
        oss << this->Map->GetTileZoom();
        std::string zoom = oss.str();
        oss.str("");

        oss << i;
        std::string row = oss.str();
        oss.str("");

        oss << (static_cast<int>(pow(2, this->Map->GetTileZoom())) - 1 - yIndex);
        std::string col = oss.str();
        oss.str("");

        // Set tile texture source
        oss << zoom << row << col;
        tile->SetImageKey(oss.str());
        tile->SetImageSource("http://tile.openstreetmap.org/" + zoom + "/" + row +
                             "/" + col + ".png");
        tile->Init(this->Map->GetCacheDirectory());
        this->AddTileToCache(this->Map->GetTileZoom(), xIndex, yIndex, tile);
      }
    this->NewPendingTiles.push_back(tile);
    tile->SetVisible(true);
    }
  }

  if (this->NewPendingTiles.size() > 0)
    {
    std::vector<vtkActor*>::iterator itr = this->CachedActors.begin();
    for (itr; itr != this->CachedActors.end(); ++itr)
      {
      this->Renderer->RemoveActor(*itr);
      }

    vtkPropCollection* props = this->Renderer->GetViewProps();

    props->InitTraversal();
    vtkProp* prop = props->GetNextProp();
    std::vector<vtkProp*> otherProps;
    while (prop)
      {
      otherProps.push_back(prop);
      prop = props->GetNextProp();
      }

    this->Renderer->RemoveAllViewProps();

    std::sort(this->NewPendingTiles.begin(), this->NewPendingTiles.end(),
              sortTiles());

    for (int i = 0; i < this->NewPendingTiles.size(); ++i)
      {
      // Add tile to the renderer
      this->Renderer->AddActor(this->NewPendingTiles[i]->GetActor());
      }

    std::vector<vtkProp*>::iterator itr2 = otherProps.begin();
    while (itr2 != otherProps.end())
      {
      this->Renderer->AddViewProp(*itr2);
      ++itr2;
      }

    this->NewPendingTiles.clear();
    }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTileToCache(int zoom, int x, int y, vtkMapTile* tile)
{
  this->CachedTiles[zoom][x][y] = tile;
  this->CachedActors.push_back(tile->GetActor());
}

//----------------------------------------------------------------------------
vtkMapTile *vtkOsmLayer::GetCachedTile(int zoom, int x, int y)
{
  if (this->CachedTiles.find(zoom) == this->CachedTiles.end() &&
      this->CachedTiles[zoom].find(x) == this->CachedTiles[zoom].end() &&
      this->CachedTiles[zoom][x].find(y) == this->CachedTiles[zoom][x].end())
    {
    return NULL;
    }

  return this->CachedTiles[zoom][x][y];
}