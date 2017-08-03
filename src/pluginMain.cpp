/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "boneToMeshCmd.h"
#include "boneToMeshNode.h"


#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MStatus.h>


const char* AUTHOR = "Ryan Porter";
const char* VERSION = "0.3.2";
const char* REQUIRED_API_VERSION = "Any";


MString BoneToMeshNode::NODE_NAME = "boneToMesh";
MTypeId BoneToMeshNode::NODE_ID = 0x00126b0f;

MString BoneToMeshCommand::COMMAND_NAME = "boneToMesh";


MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, AUTHOR, VERSION, REQUIRED_API_VERSION);

    status = fnPlugin.registerNode(
	    BoneToMeshNode::NODE_NAME,
        BoneToMeshNode::NODE_ID,
        BoneToMeshNode::creator,
        BoneToMeshNode::initialize,
		MPxNode::kDependNode
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerCommand(
        BoneToMeshCommand::COMMAND_NAME, 
        BoneToMeshCommand::creator, 
        BoneToMeshCommand::getSyntax
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}


MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, AUTHOR, VERSION, REQUIRED_API_VERSION);

    status = fnPlugin.deregisterNode(BoneToMeshNode::NODE_ID);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterCommand(BoneToMeshCommand::COMMAND_NAME);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    
    return MS::kSuccess;
}