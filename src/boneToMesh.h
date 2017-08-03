/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR_3D_BONE_TO_MESH_H
#define YANTOR_3D_BONE_TO_MESH_H

#include <cfloat>
#include <vector>

#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MVector.h>

struct BoneToMeshParams
{
    float  maxDistance            = FLT_MAX;
    float  boneLength             = 1.0;
    uint   subdivisionsX          = 8;
    uint   subdivisionsY          = 4;
    int    direction              = 0;
    int    fillPartialLoopsMethod = 0;
    float  radius                 = 1.0;
};

struct BoneToMeshProjection
{
    MMatrix       boneMatrix;
    MMatrix       directionMatrix;
    MFloatVector  directionVector;
    MFloatVector  projectionVector;
    MFloatPoint   startPoint;

    MVector::Axis longAxis;

    std::vector<MFloatPoint>  raySources;
    std::vector<MFloatVector> rayDirections;

    std::vector<int>          indices;
    std::vector<MFloatPoint>  points;

    int vertexIndex = 0;
    int maxVertices = 0;
    int maxPolygons = 0;
};

MStatus boneToMesh(
    const MObject &inMesh, 
    const MObject &components,
    const MMatrix &boneMatrix, 
    const MMatrix &directionMatrix, 
    BoneToMeshParams &params,
    MObject &outMesh
);

MStatus projectionVectors(BoneToMeshParams &params, BoneToMeshProjection &proj);
MStatus projectBoneToMesh(const MObject &inMesh, const MObject &components, BoneToMeshParams &params, BoneToMeshProjection &proj);
MStatus fillPartialLoops(BoneToMeshParams &params, BoneToMeshProjection &proj);
MStatus createMesh(BoneToMeshParams &params, BoneToMeshProjection &proj, MObject &outMesh);

#endif 