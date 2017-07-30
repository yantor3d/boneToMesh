/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "boneToMesh.h"
#include "boneToMeshNode.h"

#include <algorithm>
#include <stdio.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFn.h>
#include <maya/MFnData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

// Input attributes
MObject BoneToMeshNode::inMesh_attr;
MObject BoneToMeshNode::boneMatrix_attr;
MObject BoneToMeshNode::directionMatrix_attr;
MObject BoneToMeshNode::boneLength_attr;
MObject BoneToMeshNode::subdivisionsAxis_attr;
MObject BoneToMeshNode::subdivisionsHeight_attr;
MObject BoneToMeshNode::direction_attr;

// Output attributes
MObject BoneToMeshNode::outMesh_attr;


const short X_AXIS = 0;
const short Y_AXIS = 1;
const short Z_AXIS = 2;
   

MStatus BoneToMeshNode::compute(const MPlug &plug, MDataBlock &dataBlock)
{
    MStatus status;

    if (plug != outMesh_attr) { 
        return MStatus::kUnknownParameter;
    }

    MObject inMesh = dataBlock.inputValue(inMesh_attr).data();

    MMatrix boneMatrix = MFnMatrixData(dataBlock.inputValue(boneMatrix_attr).data()).matrix();
    MMatrix directionMatrix = MFnMatrixData(dataBlock.inputValue(directionMatrix_attr).data()).matrix();

    double boneLength = dataBlock.inputValue(boneLength_attr).asDouble();
    short direction = dataBlock.inputValue(direction_attr).asShort();

    uint subdivisionsAxis = (uint) std::max(4, dataBlock.inputValue(subdivisionsAxis_attr).asLong());
    uint subdivisionsHeight = (uint) std::max(2, dataBlock.inputValue(subdivisionsHeight_attr).asLong());

    MDataHandle outMeshHandle = dataBlock.outputValue(outMesh_attr);

    MFnMeshData outMeshData;
    MObject outMesh = outMeshData.create(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (inMesh.isNull())
    {
        return MStatus::kFailure;
    } else {
        status = boneToMesh(
            inMesh, 
            boneMatrix, 
            directionMatrix,
            boneLength, 
            subdivisionsAxis, 
            subdivisionsHeight, 
            direction, 
            outMesh
        );
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (outMesh.isNull())
    {
        MGlobal::displayError("boneToMesh projection failed.");
        return MStatus::kFailure;
    } else {
        status = outMeshHandle.setMObject(outMesh);    
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    
    outMeshHandle.setClean();

    return MStatus::kSuccess;
}


MStatus BoneToMeshNode::initialize()
{
    MStatus status;

    MFnEnumAttribute enumAttr;
    MFnNumericAttribute numAttr;
    MFnTypedAttribute typedAttr;

    inMesh_attr = typedAttr.create("inMesh", "im", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    boneMatrix_attr = typedAttr.create("boneMatrix", "bm", MFnData::kMatrix, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    directionMatrix_attr = typedAttr.create("directionMatrix", "dm", MFnData::kMatrix, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    boneLength_attr = numAttr.create("boneLength", "len",  MFnNumericData::kDouble, 1.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numAttr.setKeyable(true);

    subdivisionsAxis_attr = numAttr.create("subdivisionsAxis", "sa", MFnNumericData::kLong, 0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numAttr.setDefault(8);
    numAttr.setMin(3);
    numAttr.setKeyable(true);

    subdivisionsHeight_attr = numAttr.create("subdivisionsHeight", "sh", MFnNumericData::kLong, 0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numAttr.setDefault(4);
    numAttr.setMin(1);
    numAttr.setKeyable(true);

    direction_attr = enumAttr.create("direction", "d", X_AXIS, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    enumAttr.setKeyable(true);
    enumAttr.addField("X", X_AXIS);
    enumAttr.addField("Y", Y_AXIS);
    enumAttr.addField("Z", Z_AXIS);

    outMesh_attr = typedAttr.create("outMesh", "om", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttr.setStorable(false);

    addAttribute(inMesh_attr);
    addAttribute(boneMatrix_attr);
    addAttribute(directionMatrix_attr);
    addAttribute(boneLength_attr);
    addAttribute(subdivisionsAxis_attr);
    addAttribute(subdivisionsHeight_attr);
    addAttribute(direction_attr);
    addAttribute(outMesh_attr);

    attributeAffects(inMesh_attr, outMesh_attr);
    attributeAffects(boneMatrix_attr, outMesh_attr);
    attributeAffects(directionMatrix_attr, outMesh_attr);
    attributeAffects(boneLength_attr, outMesh_attr);
    attributeAffects(subdivisionsAxis_attr, outMesh_attr);
    attributeAffects(subdivisionsHeight_attr, outMesh_attr);
    attributeAffects(direction_attr, outMesh_attr);

    return MStatus::kSuccess;
}


void* BoneToMeshNode::creator()
{
    return new BoneToMeshNode();
}
