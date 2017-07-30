/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "boneToMesh.h"
#include "boneToMeshNode.h"

#include <algorithm>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFn.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnSingleIndexedComponent.h>
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
MObject BoneToMeshNode::components_attr;
MObject BoneToMeshNode::boneMatrix_attr;
MObject BoneToMeshNode::directionMatrix_attr;
MObject BoneToMeshNode::boneLength_attr;
MObject BoneToMeshNode::subdivisionsAxis_attr;
MObject BoneToMeshNode::subdivisionsHeight_attr;
MObject BoneToMeshNode::direction_attr;
MObject BoneToMeshNode::fillPartialLoops_attr;
MObject BoneToMeshNode::radius_attr;

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

    MObject inMesh             = dataBlock.inputValue(inMesh_attr).data();
    double  boneLength         = dataBlock.inputValue(boneLength_attr).asDouble();
    MMatrix boneMatrix         = MFnMatrixData(dataBlock.inputValue(boneMatrix_attr).data()).matrix();
    MObject componentsList     = dataBlock.inputValue(components_attr).data();
    short   direction          = dataBlock.inputValue(direction_attr).asShort();
    MMatrix directionMatrix    = MFnMatrixData(dataBlock.inputValue(directionMatrix_attr).data()).matrix();
    short   fillPartialLoops   = dataBlock.inputValue(fillPartialLoops_attr).asShort();
    double  radius             = dataBlock.inputValue(radius_attr).asDouble();
    uint    subdivisionsAxis   = (uint) std::max(4, dataBlock.inputValue(subdivisionsAxis_attr).asLong());
    uint    subdivisionsHeight = (uint) std::max(2, dataBlock.inputValue(subdivisionsHeight_attr).asLong());
    MObject components = this->unpackComponentList(componentsList);

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
            components,
            boneMatrix, 
            directionMatrix,
            boneLength, 
            subdivisionsAxis, 
            subdivisionsHeight, 
            direction, 
            fillPartialLoops,
            (float) radius,
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


MObject BoneToMeshNode::unpackComponentList(MObject &componentList)
{
    MObject components;

    if (!componentList.isNull())
    {
        MFnComponentListData fnComponentList(componentList);
        MFnSingleIndexedComponent fnComponents;
        components = fnComponents.create(MFn::kMeshPolygonComponent);

        uint numComponents = fnComponentList.length();

        for (uint i = 0; i < numComponents; i++)
        {
            MObject c = fnComponentList[i];

            if (c.apiType() == MFn::kMeshPolygonComponent)
            {
                MFnSingleIndexedComponent fnComponent(c);
                int n = fnComponent.elementCount();

                for (int j = 0; j < n; j++)
                {
                    fnComponents.addElement(fnComponent.element(j));
                }
            }
        }
    }

    return components;
}


MStatus BoneToMeshNode::initialize()
{
    MStatus status;

    MFnEnumAttribute enumAttr;
    MFnNumericAttribute numAttr;
    MFnTypedAttribute typedAttr;

    inMesh_attr = typedAttr.create("inMesh", "im", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    components_attr = typedAttr.create("components", "c", MFnData::kComponentList, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    boneMatrix_attr = typedAttr.create("boneMatrix", "bm", MFnData::kMatrix, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    directionMatrix_attr = typedAttr.create("directionMatrix", "dm", MFnData::kMatrix, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    boneLength_attr = numAttr.create("boneLength", "len",  MFnNumericData::kDouble, 1.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numAttr.setKeyable(true);
    numAttr.setKeyable(true);

    direction_attr = enumAttr.create("direction", "d", X_AXIS, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    enumAttr.setKeyable(true);
    enumAttr.addField("X", X_AXIS);
    enumAttr.addField("Y", Y_AXIS);
    enumAttr.addField("Z", Z_AXIS);

    fillPartialLoops_attr = enumAttr.create("fillPartialLoops", "fp", 3, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    enumAttr.addField("No Fill",  0),
    enumAttr.addField("Shortest", 1);
    enumAttr.addField("Longest",  2);
    enumAttr.addField("Average",  3);
    enumAttr.addField("Radius",   4);
    enumAttr.setKeyable(true);

    radius_attr = numAttr.create("radius", "r", MFnNumericData::kDouble, 1.0, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numAttr.setMin(0.0);
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

    outMesh_attr = typedAttr.create("outMesh", "om", MFnData::kMesh, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttr.setStorable(false);

    addAttribute(inMesh_attr);
    addAttribute(boneLength_attr);
    addAttribute(boneMatrix_attr);
    addAttribute(components_attr);
    addAttribute(direction_attr);
    addAttribute(directionMatrix_attr);
    addAttribute(fillPartialLoops_attr);
    addAttribute(radius_attr);
    addAttribute(subdivisionsAxis_attr);
    addAttribute(subdivisionsHeight_attr);
    addAttribute(outMesh_attr);

    attributeAffects(inMesh_attr, outMesh_attr);
    attributeAffects(boneMatrix_attr, outMesh_attr);
    attributeAffects(boneLength_attr, outMesh_attr);
    attributeAffects(components_attr, outMesh_attr);
    attributeAffects(fillPartialLoops_attr, outMesh_attr);
    attributeAffects(direction_attr, outMesh_attr);
    attributeAffects(directionMatrix_attr, outMesh_attr);
    attributeAffects(radius_attr, outMesh_attr);
    attributeAffects(subdivisionsAxis_attr, outMesh_attr);
    attributeAffects(subdivisionsHeight_attr, outMesh_attr);

    return MStatus::kSuccess;
}


void* BoneToMeshNode::creator()
{
    return new BoneToMeshNode();
}
