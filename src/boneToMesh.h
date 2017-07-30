/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR_3D_BONE_TO_MESH_H
#define YANTOR_3D_BONE_TO_MESH_H

#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

MStatus boneToMesh(
    const MObject &inMesh, 
    const MMatrix &boneMatrix, 
    const MMatrix &directionMatrix, 
    const double boneLength, 
    const uint subdivisionsAxis, 
    const uint subdivisionsHeight, 
    const short direction,
    MObject &outMesh
);

#endif 