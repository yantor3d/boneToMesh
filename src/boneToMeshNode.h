/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR_3D_BONE_TO_MESH_NODE_H
#define YANTOR_3D_BONE_TO_MESH_NODE_H

#include <maya/MDataBlock.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>

class BoneToMeshNode : public MPxNode
{
public:
    static  void*       creator();
    static  MStatus     initialize();
    
    virtual MStatus     compute(const MPlug &plug, MDataBlock &dataBlock);

private:
    virtual MObject     unpackComponentList(MObject &componentList);

public:
    static MString      NODE_NAME;
    static MTypeId      NODE_ID;

private:
    static MObject      boneLength_attr;
    static MObject      boneMatrix_attr;
    static MObject      components_attr;
    static MObject      direction_attr;
    static MObject      directionMatrix_attr;
    static MObject      fillPartialLoops_attr;
    static MObject      inMesh_attr;
    static MObject      maxDistance_attr;
    static MObject      subdivisionsAxis_attr;
    static MObject      subdivisionsHeight_attr;
    static MObject      radius_attr;
    static MObject      useMaxDistance_attr;

    static MObject      outMesh_attr;
};

#endif