/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "boneToMesh.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

#include <maya/MFloatArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

const short FILL_NONE     = 0;
const short FILL_SHORTEST = 1;
const short FILL_LONGEST  = 2;
const short FILL_AVERAGE  = 3;
const short FILL_RADIUS   = 4;

MStatus boneToMesh(
    const MObject &inMesh, 
    const MObject &components,
    const MMatrix &boneMatrix, 
    const MMatrix &directionMatrix, 
    BoneToMeshParams &params,
    MObject &outMesh
) {
    MStatus status;

    BoneToMeshProjection proj;

    proj.boneMatrix = boneMatrix;
    proj.directionMatrix = directionMatrix;

    switch (params.direction)
    {
        case 0: // X axis
            proj.directionVector = MVector::xAxis;
            proj.projectionVector = MVector::yAxis;
            proj.longAxis = MVector::kXaxis;
        break;

        case 1: // Y axis
            proj.directionVector = MVector::yAxis;
            proj.projectionVector = MVector::zAxis;
            proj.longAxis = MVector::kYaxis;
        break;

        case 2: // Z axis 
            proj.directionVector = MVector::zAxis;
            proj.projectionVector = MVector::xAxis;
            proj.longAxis = MVector::kZaxis;
        break;
    }

    MVector dVec(proj.directionVector);
    dVec *= params.boneLength;
    dVec *= proj.boneMatrix;

    proj.directionVector = MFloatVector(dVec);
    proj.startPoint = MFloatPoint(MPoint::origin * proj.boneMatrix);
    proj.maxVertices = params.subdivisionsY * params.subdivisionsX;

    projectionVectors(params, proj);
    projectBoneToMesh(inMesh, components, params, proj);
    fillPartialLoops(params, proj);
    createMesh(params, proj, outMesh);

    return MStatus::kSuccess;
}

MStatus projectionVectors(BoneToMeshParams &params, BoneToMeshProjection &proj)
{
    MStatus status;

    proj.raySources.resize(params.subdivisionsY);
    proj.rayDirections.resize(proj.maxVertices);

   for (uint sh = 0; sh < params.subdivisionsY; sh++)
    {
        float t = float(sh) / float(params.subdivisionsY - 1);
        MFloatPoint raySource(proj.startPoint + (proj.directionVector * t));

        proj.raySources[sh] = raySource; 

        MTransformationMatrix dxMatrix(proj.directionMatrix);
        dxMatrix.setTranslation(MVector(raySource), MSpace::kWorld);

        MMatrix dMatrix = dxMatrix.asMatrix();

        for (uint sa = 0; sa < params.subdivisionsX; sa++)
        {
            double a = (2.0 * M_PI) * (float(sa) / float(params.subdivisionsX));
            
            uint idx = (sh * params.subdivisionsX) + sa;
            MVector ray(proj.projectionVector);
            ray = ray.rotateBy(proj.longAxis, a);
            ray *= dMatrix;

            proj.rayDirections[idx] = MFloatVector(ray);
        }
    }
    
    return MStatus::kSuccess;
}


MStatus projectBoneToMesh(
    const MObject &inMesh, 
    const MObject &components, 
    BoneToMeshParams &params, 
    BoneToMeshProjection &proj
) {
    MStatus status;

    MFnMesh inMeshFn(inMesh);

    MMeshIsectAccelParams accelParams = inMeshFn.autoUniformGridParams();

    bool useComponents = !components.isNull();

    MIntArray faceIds;
    MIntArray* faceIds_ptr = NULL;

    if (useComponents)
    {
        MFnSingleIndexedComponent fnComponents(components);

        int numComponents = fnComponents.elementCount();
        faceIds.setLength(numComponents);

        for (int i = 0; i < numComponents; i++)
        {
            faceIds[i] = fnComponents.element(i);
        }

        faceIds_ptr = &faceIds;
    }

    float tolerance = (float) 1e-6;
    float maxParams = (float) params.maxDistance; 

    proj.indices.resize(proj.maxVertices, -1);
    proj.points.resize(proj.maxVertices);

    for (uint sh = 0; sh < params.subdivisionsY; sh++)
    {
        for (uint sa = 0; sa < params.subdivisionsX; sa++)
        {
            uint idx = (sh * params.subdivisionsX) + sa;
            MFloatPointArray hitPoints;

            bool hits = inMeshFn.allIntersections(
                proj.raySources[sh], 
                proj.rayDirections[idx],
                faceIds_ptr,    
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
                proj.indices[idx] = proj.vertexIndex++;
                proj.points[idx] = MFloatPoint(hitPoints[0]);
            }

            CHECK_MSTATUS(status);
        }
    }

    return MStatus::kSuccess;
}

MStatus fillPartialLoops(BoneToMeshParams &params, BoneToMeshProjection &proj)
{
    MStatus status; 

    if (params.fillPartialLoopsMethod != FILL_NONE)
    {
        // Fill in missing points.
        for (uint sh = 0; sh < params.subdivisionsY; sh++)
        {
            int numHits = 0;

            float rayLength = params.fillPartialLoopsMethod == FILL_SHORTEST ? FLT_MAX : 0.0f;

            for (uint sa = 0; sa < params.subdivisionsX; sa++)
            {
                uint idx = (sh * params.subdivisionsX) + sa;

                if (proj.indices[idx] != -1) 
                {
                    switch (params.fillPartialLoopsMethod)
                    {
                        case FILL_SHORTEST: 
                            rayLength = std::min(rayLength, (proj.points[idx] - proj.raySources[sh]).length());
                            break;
                        case FILL_LONGEST: 
                            rayLength = std::max(rayLength, (proj.points[idx] - proj.raySources[sh]).length());
                            break;
                        case FILL_AVERAGE: 
                            rayLength += (proj.points[idx] - proj.raySources[sh]).length();
                            break;
                    }
                    
                    numHits++;
                }
            }

            if (numHits == 0) 
            {
                continue; 
            }

            switch(params.fillPartialLoopsMethod)
            {
                case FILL_AVERAGE: rayLength /= float(numHits); break;
                case FILL_RADIUS:  rayLength = (float) params.radius;   break;
            }

            for (uint sa = 0; sa < params.subdivisionsX; sa++)
            {
                uint idx = (sh * params.subdivisionsX) + sa;

                if (proj.indices[idx] == -1) 
                {
                    proj.indices[idx] = proj.vertexIndex++;
                    proj.points[idx] = proj.raySources[sh] + (proj.rayDirections[idx] * rayLength);
                }
            }
        }

        // Reset the vertex indices
        proj.vertexIndex = 0;

        for (uint sh = 0; sh < params.subdivisionsY; sh++)
        {
            for (uint sa = 0; sa < params.subdivisionsX; sa++)
            {
                uint idx = (sh * params.subdivisionsX) + sa;

                if (proj.indices[idx] != -1) 
                {
                    proj.indices[idx] = proj.vertexIndex++; 
                }
            }
        }
    }

    return MStatus::kSuccess;
}


MStatus createMesh(BoneToMeshParams &params, BoneToMeshProjection &proj, MObject &outMesh) 
{
    MStatus status;   

    MFloatPointArray vertexArray(proj.maxVertices);
    MIntArray polygonCounts(proj.maxVertices, 4);
    MIntArray polygonConnects(proj.maxVertices * 4, -1);

    int numVertices = 0;
    int numPolygons = 0;

    // Face order - clockwise vs counter-clockwise
    int cw = (int) params.boneLength >= 0;
    int cc = (int) params.boneLength < 0;

    for (uint sh = 0; sh < params.subdivisionsY; sh++)
    {
        for (uint sa = 0; sa < params.subdivisionsX; sa++)
        {
            uint idx = (sh * params.subdivisionsX) + sa;

            if (proj.indices[idx] != -1) 
            { 
                vertexArray.set(proj.points[idx], (uint) numVertices);
                numVertices++;
            } 
        }
    }           

    for (uint sh = 0; sh < params.subdivisionsY - 1; sh++)
    {
        for (uint sa = 0; sa < params.subdivisionsX; sa++)
        {
            uint na = (sa + 1) % (params.subdivisionsX);

            uint idx0 = (sh * params.subdivisionsX) + sa;
            uint idx1 = (sh * params.subdivisionsX) + na;
            uint idx2 = ((sh + 1) * params.subdivisionsX) + sa;
            uint idx3 = ((sh + 1) * params.subdivisionsX) + na;

            int vtx0 = proj.indices[idx0];
            int vtx1 = proj.indices[idx1];
            int vtx2 = proj.indices[idx2];
            int vtx3 = proj.indices[idx3];           

            if (vtx0 == -1 || vtx1 == -1 || vtx2 == -1 || vtx3 == -1) { continue; }        

            // (vtx# * clockwise) + (vtx# * counter-clockwise) 
            // to avoid the if branch
            polygonConnects[(numPolygons * 4) + 0] = (vtx0 * cw) + (vtx0 * cc);
            polygonConnects[(numPolygons * 4) + 1] = (vtx1 * cw) + (vtx2 * cc);
            polygonConnects[(numPolygons * 4) + 2] = (vtx3 * cw) + (vtx3 * cc);
            polygonConnects[(numPolygons * 4) + 3] = (vtx2 * cw) + (vtx1 * cc);

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
