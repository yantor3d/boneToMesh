/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR_3D_BONE_TO_MESH_H
#define YANTOR_3D_BONE_TO_MESH_H

#include "boneToMesh.h"

#include <cmath>
#include <stdio.h>
#include <vector>

#include <maya/MFloatArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

MStatus boneToMesh(
    const MObject &inMesh, 
    const MMatrix &boneMatrix, 
    const MMatrix &directionMatrix, 
    const double boneLength, 
    const uint subdivisionsAxis, 
    const uint subdivisionsHeight, 
    const short direction,
    MObject &outMesh
) {
    MStatus status;

    MFnMesh inMeshFn(inMesh);

    MMeshIsectAccelParams accelParams = inMeshFn.autoUniformGridParams();

    MVector directionVector;
    MVector projectionVector;
    MVector::Axis longAxis;

    switch (direction)
    {
        case 0: // X axis
            directionVector = MVector::xAxis;
            projectionVector = MVector::yAxis;
            longAxis = MVector::kXaxis;
        break;

        case 1: // Y axis
            directionVector = MVector::yAxis;
            projectionVector = MVector::zAxis;
            longAxis = MVector::kYaxis;
        break;

        case 2: // Z axis 
            directionVector = MVector::zAxis;
            projectionVector = MVector::xAxis;
            longAxis = MVector::kZaxis;
        break;
    }

    directionVector *= boneLength;
    directionVector *= boneMatrix;

    MPoint startPoint = MPoint::origin * boneMatrix;

    MFloatPoint raySource; 
    MFloatVector rayDirection;

    float maxParams = (float) 9001.0;
    float tolerance = (float) 1e-6;

    uint numPoints = subdivisionsAxis * subdivisionsHeight;

    std::vector<int> indices(numPoints, -1);
    std::vector<MFloatPoint> points(numPoints);
    
    int vertexIndex = 0;

    for (uint sh = 0; sh < subdivisionsHeight; sh++)
    {
        double t = float(sh) / float(subdivisionsHeight - 1);
        raySource = MFloatPoint(startPoint + (directionVector * t));

        MTransformationMatrix dxMatrix(directionMatrix);
        dxMatrix.setTranslation(MVector(raySource), MSpace::kWorld);

        MMatrix dMatrix = dxMatrix.asMatrix();

        for (uint sa = 0; sa < subdivisionsAxis; sa++)
        {
            double a = (2.0 * M_PI) * (float(sa) / float(subdivisionsAxis - 1));
            
            rayDirection = MFloatVector(projectionVector.rotateBy(longAxis, a) * dMatrix);
            MFloatPointArray hitPoints;

            bool hits = inMeshFn.allIntersections(
                raySource, 
                rayDirection,
                NULL,           // face Ids
                NULL,           // tri Ids
                true,           // sort ids 
                MSpace::kObject,// space
                maxParams,
                false,          // test both directions
                &accelParams,   // acceleration parameters
                true,           // sort hits
                hitPoints,      
                NULL,           // hit ray params
                NULL,           // hit faces
                NULL,           // hit triangles
                NULL,           // hit barycentric coordinates 
                NULL,           // hit barycentric coordinates 
                tolerance,
                &status
            );

            if (hits)
            {
                uint idx = (sh * subdivisionsAxis) + sa;
                indices[idx] = vertexIndex++;
                points[idx] = MFloatPoint(hitPoints[0]);
            }

            CHECK_MSTATUS(status);
        }
    } 

    int maxVertices = subdivisionsAxis * subdivisionsHeight;
    int maxPolygons = subdivisionsAxis * subdivisionsHeight;

    MFloatPointArray vertexArray(maxVertices);
    MIntArray polygonCounts(maxPolygons, 4);
    MIntArray polygonConnects(maxPolygons * 4);

    int numVertices = 0;
    int numPolygons = 0;

    for (uint sh = 0; sh < subdivisionsHeight; sh++)
    {
        for (uint sa = 0; sa < subdivisionsAxis; sa++)
        {
            uint idx = (sh * subdivisionsAxis) + sa;

            if (indices[idx] != -1) 
            { 
                vertexArray.set(points[idx], (uint) numVertices);
                numVertices++;
            } 
        }
    }           

    for (uint sh = 0; sh < subdivisionsHeight - 1; sh++)
    {
        for (uint sa = 0; sa < subdivisionsAxis; sa++)
        {
            uint na = (sa + 1) % (subdivisionsAxis - 1);

            uint idx0 = (sh * subdivisionsAxis) + sa;
            uint idx1 = (sh * subdivisionsAxis) + na;
            uint idx2 = ((sh + 1) * subdivisionsAxis) + sa;
            uint idx3 = ((sh + 1) * subdivisionsAxis) + na;

            int vtx0 = indices[idx0];
            int vtx1 = indices[idx1];
            int vtx2 = indices[idx2];
            int vtx3 = indices[idx3];           

            if (vtx0 == -1 || vtx1 == -1 || vtx2 == -1 || vtx3 == -1) { continue; }        

            polygonConnects[(numPolygons * 4) + 0] = vtx1;
            polygonConnects[(numPolygons * 4) + 1] = vtx0;
            polygonConnects[(numPolygons * 4) + 2] = vtx2;
            polygonConnects[(numPolygons * 4) + 3] = vtx3;

            numPolygons++; 
        }
    }

    vertexArray.setLength(numVertices);
    polygonCounts.setLength(numPolygons);
    polygonConnects.setLength(numPolygons * 4);

    MFnMesh outMeshFn;

    outMeshFn.create(
        numVertices,
        numPolygons,
        vertexArray,
        polygonCounts,
        polygonConnects,
        outMesh,
        &status
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


#endif 