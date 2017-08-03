/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "boneToMesh.h"
#include "boneToMeshCmd.h"

#include <algorithm>
#include <cfloat>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagModifier.h>
#include <maya/MDGModifier.h>
#include <maya/MFn.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>


#define RETURN_IF_ERROR(s) if (!s) { return s; }

const char* AXIS_FLAG = "-a";
const char* AXIS_LONG = "-axis"; 

const char* BONE_FLAG = "-b";
const char* BONE_LONG = "-bone";

const char* CONSTRUCTION_HISTORY_FLAG = "-ch";
const char* CONSTRUCTION_HISTORY_LONG = "-constructionHistory";

const char* FILL_PARTIAL_LOOPS_FLAG = "-fp";
const char* FILL_PARTIAL_LOOPS_LONG = "-fillPartialLoops";

const char* HELP_FLAG = "-h";
const char* HELP_LONG = "-help";

const char* LENGTH_FLAG = "-l";
const char* LENGTH_LONG = "-length";

const char* MAX_DISTANCE_FLAG = "-md";
const char* MAX_DISTANCE_LONG = "-maxDistance";

const char* RADIUS_FLAG = "-r";
const char* RADIUS_LONG = "-radius";

const char* SUBDIVISIONS_X_FLAG = "-sx";
const char* SUBDIVISIONS_X_LONG = "-subdivisionsX";

const char* SUBDIVISIONS_Y_FLAG = "-sy";
const char* SUBDIVISIONS_Y_LONG = "-subdivisionsY";

const char* WORLD_SPACE_FLAG = "-w";
const char* WORLD_SPACE_LONG = "-world";


BoneToMeshCommand::BoneToMeshCommand()
{

}


BoneToMeshCommand::~BoneToMeshCommand()
{

}


void* BoneToMeshCommand::creator()
{
    return new BoneToMeshCommand();
}


void BoneToMeshCommand::help()
{
    MString helpMessage(
        "\nboneToMesh\n"
        "\n"
        "Creates a cylindrical mesh around the specified bone and projects it outward onto the selected mesh.\n"
        "\n"
        "FLAGS\n"
        "Long Name            Short Name   Argument Type(s)    Description\n"
        "-axis                -a           string              Long axis of the bone. Accepted values are \"x\", \"y\", or \"z\".\n"
        "-bone                -b           string              Transform at the base of the \"bone\".\n"
        "-constructionHistory -ch          boolean             Toggles construction history on/off.\n"
        "-fillPartialLoops    -fp          string              Method by which partial loops have their missing points filled\n"
        "                                                      Accepted values are 0 - \"none\", 1 - \"shortest\", 2 - \"longest\", 3 - \"average\", or 4 - \"radius\".\n"
        "-length              -l           double              Length of the bone.\n"
        "-maxDistance         -md          double              Maximum distance from the bone an intersection with the mesh may occur.\n"
        "-radius              -r           double              Distance from the bone of filled in points if -fillPartialLoops is set to \"radius\".\n"
        "-subdivisionsX       -sx          int                 Specifies the number of subdivisions around the bone.\n"
        "-subdivisionsY       -sy          int                 Specifies the number of subdivisions along the bone.\n"
        "-world               -w           boolean             Toggles the axis between world and local.\n"
    );

    MGlobal::displayInfo(helpMessage);
}


MStatus BoneToMeshCommand::parseArguments(MArgDatabase &argsData)
{
    MStatus status;

    // -help flag
    if (argsData.isFlagSet(HELP_FLAG))
    {
        this->showHelp = true;
        return MStatus::kSuccess;
    } else {
        this->showHelp = false;
    }

    // selected mesh
    {
        MSelectionList selection;
        argsData.getObjects(selection);

        if (selection.isEmpty())
        {
            MGlobal::displayError("Must select a mesh.");
            return MStatus::kFailure;
        }

        selection.getDagPath(0, this->inMesh, this->components);
    }

    double tmp = 0.0;
    
    // -axis flag
    if (argsData.isFlagSet(AXIS_FLAG))
    {
        status = argsData.getFlagArgument(AXIS_FLAG, 0, this->axis);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        this->axis = "x";
    }

    // -bone flag
    if (argsData.isFlagSet(BONE_FLAG))
    {
        MSelectionList selection;
        MString objectName;

        status = argsData.getFlagArgument(BONE_FLAG, 0, objectName);        
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = selection.add(objectName);

        if (status)
        {
            status = selection.getDependNode(0, this->boneObj);    
            RETURN_IF_ERROR(status);
        } else {
            MString errorMsg("Object '^1s does not exist.");
            errorMsg.format(errorMsg, objectName);
            MGlobal::displayError(errorMsg);
            return status;
        }
    } else {
        MGlobal::displayError("The -bone/-b flag is required.");
        return MStatus::kFailure;
    }

    // -constructionHistory flag
    if (argsData.isFlagSet(CONSTRUCTION_HISTORY_FLAG))
    {
        status = argsData.getFlagArgument(CONSTRUCTION_HISTORY_FLAG, 0, this->constructionHistory);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }  

    // -fillPartialLoops flag
    if (argsData.isFlagSet(FILL_PARTIAL_LOOPS_FLAG))
    {
        status = argsData.getFlagArgument(FILL_PARTIAL_LOOPS_FLAG, 0, params.fillPartialLoopsMethod);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (params.fillPartialLoopsMethod < 0) { params.fillPartialLoopsMethod = 0; }
        if (params.fillPartialLoopsMethod > 4) { params.fillPartialLoopsMethod = 4; }
    }

    // -length flag
    if (argsData.isFlagSet(LENGTH_FLAG))
    {
        status = argsData.getFlagArgument(LENGTH_FLAG, 0, tmp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        params.boneLength = (float) tmp;
    }

    // -maxDistance flag
    if (argsData.isFlagSet(MAX_DISTANCE_FLAG))
    {
        this->useMaxDistance = true;

        status = argsData.getFlagArgument(MAX_DISTANCE_FLAG, 0, tmp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        params.maxDistance = (float) tmp;
    }

    // -radius flag
    if (argsData.isFlagSet(RADIUS_FLAG))
    {
        status = argsData.getFlagArgument(RADIUS_FLAG, 0, tmp);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        params.radius = (float) tmp;
    }

    // -subdivisionsX (axis) flag
    if (argsData.isFlagSet(SUBDIVISIONS_X_FLAG))
    {
        status = argsData.getFlagArgument(SUBDIVISIONS_X_FLAG, 0, params.subdivisionsX);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // -subdivisionsY (height) flag 
    if (argsData.isFlagSet(SUBDIVISIONS_Y_FLAG))
    {
        status = argsData.getFlagArgument(SUBDIVISIONS_Y_FLAG, 0, params.subdivisionsY);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // -world flag
    if (argsData.isFlagSet(WORLD_SPACE_FLAG))
    {
        status = argsData.getFlagArgument(WORLD_SPACE_FLAG, 0, this->useWorldDirection);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        this->useWorldDirection = false;
    }


    return MStatus::kSuccess;
}


MStatus BoneToMeshCommand::validateArguments()
{
    MStatus status;

    if (
        this->axis != "x" &&
        this->axis != "y" &&
        this->axis != "z"
    ) {
        MGlobal::displayError("-axis/-a flag must be set to \"x\", \"y\", or \"z\".");
        return MStatus::kFailure;
    }
    
    if (this->inMesh.hasFn(MFn::kMesh))
    {
        if (this->inMesh.node().hasFn(MFn::kTransform))
        {
            this->inMesh.extendToShapeDirectlyBelow(0);
        }
    } else {
        MGlobal::displayError("Must select a mesh.");
        return MStatus::kFailure;
    }

    if (!this->boneObj.hasFn(MFn::kTransform)) {
        MGlobal::displayError("The -bone/-b flag expects a transform.");
        return MStatus::kFailure;
    }

    if (params.subdivisionsX < 3) {
        MGlobal::displayError("The -subdivisionsX/-sx flag must be at least 3.");
        return MStatus::kFailure;
    }

    if (params.subdivisionsY < 1) {
        MGlobal::displayError("The -subdivisionsY/-sy flag must be at least 1.");
        return MStatus::kFailure;
    }

    return MStatus::kSuccess;
}


MSyntax BoneToMeshCommand::getSyntax()
{
    MSyntax syntax;

    syntax.addFlag(AXIS_FLAG, AXIS_LONG, MSyntax::kString);
    syntax.addFlag(BONE_FLAG, BONE_LONG, MSyntax::kString);
    syntax.addFlag(CONSTRUCTION_HISTORY_FLAG, CONSTRUCTION_HISTORY_LONG, MSyntax::kBoolean);
    syntax.addFlag(FILL_PARTIAL_LOOPS_FLAG, FILL_PARTIAL_LOOPS_LONG, MSyntax::kLong);
    syntax.addFlag(HELP_FLAG, HELP_LONG, MSyntax::kBoolean);
    syntax.addFlag(LENGTH_FLAG, LENGTH_LONG, MSyntax::kDouble);
    syntax.addFlag(MAX_DISTANCE_FLAG, MAX_DISTANCE_LONG, MSyntax::kDouble);
    syntax.addFlag(RADIUS_FLAG, RADIUS_LONG, MSyntax::kDouble);
    syntax.addFlag(SUBDIVISIONS_X_FLAG, SUBDIVISIONS_X_LONG, MSyntax::kLong);
    syntax.addFlag(SUBDIVISIONS_Y_FLAG, SUBDIVISIONS_Y_LONG, MSyntax::kLong);
    syntax.addFlag(WORLD_SPACE_FLAG, WORLD_SPACE_LONG, MSyntax::kBoolean);

    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 1, 1);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}


MStatus BoneToMeshCommand::doIt(const MArgList& argList)
{
    MStatus status;

    MArgDatabase argsData(syntax(), argList, &status);

    status = this->parseArguments(argsData);        
    RETURN_IF_ERROR(status);

    if (this->showHelp)
    {
        help();
        return MStatus::kSuccess;
    }

    status = this->validateArguments();
    RETURN_IF_ERROR(status);

    return this->redoIt();
}


MStatus BoneToMeshCommand::redoIt()
{
    MStatus status;

    if (this->axis == "x")      { params.direction = 0; }
    else if (this->axis == "y") { params.direction = 1; }
    else if (this->axis == "z") { params.direction = 2; }

    MFnTransform fnXform(this->boneObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MMatrix boneMatrix = fnXform.transformation().asMatrix();
    MMatrix directionMatrix = this->useWorldDirection ? MMatrix::identity : MMatrix(boneMatrix);

    MDagModifier dagMod;

    MObject newMeshParent;
    MObject newMesh = dagMod.createNode("transform", MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);
    
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPath parentTransform;
    MDagPath::getAPathTo(newMesh, parentTransform);

    MObject inMeshObj = this->inMesh.node();

    BoneToMeshParams params;
    
    status = boneToMesh(
        inMeshObj,
        this->components,
        boneMatrix,
        directionMatrix,
        params,
        newMesh
    );

    RETURN_IF_ERROR(status);

    MGlobal::executeCommand("sets -e -forceElement initialShadingGroup " + parentTransform.partialPathName());

    newMeshParent = MObject(parentTransform.node());
    parentTransform.extendToShape();
    newMesh = MObject(parentTransform.node());
    parentTransform.pop();

    MString boneName = MFnDagNode(this->boneObj).name();

    dagMod.renameNode(newMeshParent, boneName + "_Mesh");
    dagMod.renameNode(newMesh, boneName + "_MeshShape");
    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    this->appendToResult(parentTransform.partialPathName());

    if (constructionHistory)
    {
        MDGModifier dgMod;

        MObject newNode = dgMod.createNode("boneToMesh", &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnDagNode        fnInMesh(this->inMesh);
        MFnDependencyNode fnBone(this->boneObj);
        MFnDependencyNode fnNode(newNode);
        MFnDependencyNode fnNewMesh(newMesh);

        if (!this->components.isNull())
        {
            MFnComponentListData fnComponentList;
            MObject componentList = fnComponentList.create();

            MItMeshPolygon itPoly(this->inMesh, this->components);

            while (!itPoly.isDone())
            {
                MObject c = itPoly.currentItem();
                status = fnComponentList.add(c);
                CHECK_MSTATUS_AND_RETURN_IT(status);

                itPoly.next();
            }

            MPlug node_componentsPlug = fnNode.findPlug("components", false);
            status = node_componentsPlug.setMObject(componentList);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        MPlug inMesh_worldMeshPlug     = fnInMesh.findPlug("worldMesh", false, &status).elementByLogicalIndex(0);
        MPlug bone_worldMatrixPlug     = fnBone.findPlug("worldMatrix", false, &status).elementByLogicalIndex(0);
        
        MPlug node_boneLengthPlug      = fnNode.findPlug("boneLength", false);
        MPlug node_boneMatrixPlug      = fnNode.findPlug("boneMatrix", false);
        MPlug node_directionPlug       = fnNode.findPlug("direction", false);
        MPlug node_directionMatrixPlug = fnNode.findPlug("directionMatrix", false);
        MPlug node_inMeshPlug          = fnNode.findPlug("inMesh", false);
        MPlug node_maxDistancePlug     = fnNode.findPlug("maxDistance", false);
        MPlug node_outMeshPlug         = fnNode.findPlug("outMesh", false);
        MPlug node_subdivisionsXPlug   = fnNode.findPlug("subdivisionsAxis", false);
        MPlug node_subdivisionsYPlug   = fnNode.findPlug("subdivisionsHeight", false);
        MPlug node_useMaxDistancePlug  = fnNode.findPlug("useMaxDistance", false);

        MPlug newMesh_inMeshPlug       = fnNewMesh.findPlug("inMesh", false, &status);

        if (this->useMaxDistance)
        {
            status = node_useMaxDistancePlug.setBool(true);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            status = node_maxDistancePlug.setDouble((double) params.maxDistance);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        dgMod.connect(inMesh_worldMeshPlug, node_inMeshPlug);
        dgMod.connect(bone_worldMatrixPlug, node_boneMatrixPlug);

        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MFnMatrixData fnMatrixData;
        MObject directionMatrixData = fnMatrixData.create(directionMatrix);
        node_directionMatrixPlug.setMObject(directionMatrixData);

        node_boneLengthPlug.setDouble((double) params.boneLength);
        status = node_subdivisionsXPlug.setInt(params.subdivisionsX);
        status = node_subdivisionsYPlug.setInt(params.subdivisionsY);
        node_directionPlug.setShort(params.direction);

        status = dgMod.connect(node_outMeshPlug, newMesh_inMeshPlug);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN_IT(status);

        this->appendToResult(fnNode.name());

        this->undoCreatedNode = newNode;
    }

    this->undoCreatedMesh = newMeshParent;

    return MStatus::kSuccess;
}


MStatus BoneToMeshCommand::undoIt()
{
    MStatus status;

    bool createdMesh = !undoCreatedMesh.isNull();
    bool createdNode = !undoCreatedNode.isNull();

    if (createdNode && createdMesh)
    {
        MString deleteCmd("delete ^1s ^2s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedNode).name(),
            MFnDependencyNode(undoCreatedMesh).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else if (createdNode) { 
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString deleteCmd("delete ^1s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedNode).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else if (createdMesh) {
        MString deleteCmd("delete ^1s");
        deleteCmd.format(
            deleteCmd,
            MFnDependencyNode(undoCreatedMesh).name()
        );

        status = MGlobal::executeCommand(deleteCmd);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return MStatus::kSuccess;
}
