/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef YANTOR_3D_BONE_TO_MESH_CMD_H
#define YANTOR_3D_BONE_TO_MESH_CMD_H

#include "boneToMesh.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

class BoneToMeshCommand : public MPxCommand
{
public:
                        BoneToMeshCommand();
    virtual             ~BoneToMeshCommand();

    static void*        creator();

    static MSyntax      getSyntax();
    virtual MStatus     parseArguments(MArgDatabase &argsData);
    virtual MStatus     validateArguments();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual bool        isUndoable() const { return true; }
    virtual bool        hasSyntax()  const { return true; }

private:
    virtual void        help();

public:
    static MString      COMMAND_NAME;

private:
    MDagPath            inMesh;
    MObject             components;

    MString             axis;
    MObject             boneObj;

    BoneToMeshParams    params;

    bool                constructionHistory = false;
    bool                showHelp = false;
    bool                useMaxDistance = false;
    bool                useWorldDirection = false;

    MObject             undoCreatedMesh;
    MObject             undoCreatedNode;
};

#endif
