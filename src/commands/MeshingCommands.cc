/***
DEVSIM
Copyright 2013 Devsim LLC

This file is part of DEVSIM.

DEVSIM is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3.

DEVSIM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with DEVSIM.  If not, see <http://www.gnu.org/licenses/>.
***/

#include "MeshingCommands.hh"
#include "dsCommand.hh"
#include "CommandHandler.hh"
#include "Validate.hh"
#include "GlobalData.hh"
#include "CheckFunctions.hh"
#include "MeshKeeper.hh"
#include "Mesh1d.hh"
#include "Mesh2d.hh"
#include "DevsimReader.hh"
#include "DevsimWriter.hh"
#include "DevsimRestartWriter.hh"
#include "FloodsWriter.hh"
#include "VTKWriter.hh"
#include "TecplotWriter.hh"
#include "dsAssert.hh"
#include "GmshReader.hh"
#include "GmshLoader.hh"

#include <sstream>

using namespace dsValidate;

namespace dsCommand {

void 
create1dMeshCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] =
    {
        {"mesh", "",   dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshCannotExist},
        {NULL,   NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();

    const std::string &meshName = data.GetStringOption("mesh");

    dsMesh::MeshPtr mp = NULL;
    if (commandName == "create_1d_mesh")
    {
        mp = new dsMesh::Mesh1d(meshName);
    }
    else if (commandName == "create_2d_mesh")
    {
        mp = new dsMesh::Mesh2d(meshName);
    }
    else
    {
        dsAssert(0, "UNEXPECTED");
    }
    mdata.AddMesh(mp);
    data.SetEmptyResult();
}

void 
finalizeMeshCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh", "",   dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {NULL,   NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();

    const std::string &meshName = data.GetStringOption("mesh");

    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    {
        bool ret = mp->Finalize(errorString);
        if (!ret)
        {
          data.SetErrorResult(errorString);
          return;
        }
    }
    data.SetEmptyResult();
}

void 
add1dMeshLineCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"tag",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL},
        {"pos",    "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::REQUIRED, NULL},
/// this will default to plus spacing
        {"ns",     "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"ps",     "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::REQUIRED, mustBePositive},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &tagName  = data.GetStringOption("tag");
    double  pos = data.GetDoubleOption("pos");
    double  ns  = data.GetDoubleOption("ns");
    double  ps  = data.GetDoubleOption("ps");

    if (ns <= 0.0)
    {
        ns = ps;
    }

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh1dPtr m1dp = dynamic_cast<dsMesh::Mesh1dPtr>(mp);
    if (!m1dp)
    {
      std::ostringstream os;
      os << meshName << " is not a 1D mesh\n";
      data.SetErrorResult(os.str());
      return;
    }
    else
    {
        if (!tagName.empty())
        {
            m1dp->AddLine(dsMesh::MeshLine1d(pos, ps, ns, tagName));
            data.SetEmptyResult();
        }
        else
        {
            m1dp->AddLine(dsMesh::MeshLine1d(pos, ps, ns));
            data.SetEmptyResult();
        }
    }
}

void 
add2dMeshLineCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"pos",    "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::REQUIRED, NULL},
/// this will default to plus spacing
        {"ns",     "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"ps",     "0", dsGetArgs::Types::FLOAT, dsGetArgs::Types::REQUIRED, mustBePositive},
        {"dir",    "",  dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, NULL},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &dirName  = data.GetStringOption("dir");
    double  pos = data.GetDoubleOption("pos");
    double  ns  = data.GetDoubleOption("ns");
    double  ps  = data.GetDoubleOption("ps");

    if (ns <= 0.0)
    {
        ns = ps;
    }

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh2dPtr m2dp = dynamic_cast<dsMesh::Mesh2dPtr>(mp);
    if (!m2dp)
    {
      std::ostringstream os;
      os << meshName << " is not a 2D mesh\n";
      data.SetErrorResult(os.str());
      return;
    }
    else
    {
        if (dirName == "x")
        {
            dsMesh::MeshLine2dPtr mlp(new dsMesh::MeshLine2d(pos, ps, ns));
            m2dp->AddLine(dsMesh::XDIR, mlp);
            data.SetEmptyResult();
        }
        else if (dirName == "y")
        {
            dsMesh::MeshLine2dPtr mlp(new dsMesh::MeshLine2d(pos, ps, ns));
            m2dp->AddLine(dsMesh::YDIR, mlp);
            data.SetEmptyResult();
        }
        else
        {
            std::ostringstream os;
            os << dirName << " is not a valid mesh direction\n";
            data.SetErrorResult(os.str());
            return;
        }
    }

}

void 
add1dInterfaceCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"name",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"tag",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &name = data.GetStringOption("name");
    const std::string &tagName  = data.GetStringOption("tag");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh1dPtr m1dp = dynamic_cast<dsMesh::Mesh1dPtr>(mp);
    if (!m1dp)
    {
        std::ostringstream os;
        os << meshName << " is not a 1D mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
        if (commandName == "add_1d_interface")
        {
            m1dp->AddInterface(dsMesh::MeshInterface1d(name, tagName));
            data.SetEmptyResult();
        }
        else if (commandName == "add_1d_contact")
        {
            m1dp->AddContact(dsMesh::MeshContact1d(name, tagName));
            data.SetEmptyResult();
        }
    }
}

void 
add2dInterfaceCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"name",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region0",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region1",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"xl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"xh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"bloat", "1.0e-10",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &name = data.GetStringOption("name");
    const std::string &region0Name  = data.GetStringOption("region0");
    const std::string &region1Name  = data.GetStringOption("region1");
    const double xl = data.GetDoubleOption("xl");
    const double xh = data.GetDoubleOption("xh");
    const double yl = data.GetDoubleOption("yl");
    const double yh = data.GetDoubleOption("yh");
    const double bl = data.GetDoubleOption("bloat");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh2dPtr m2dp = dynamic_cast<dsMesh::Mesh2dPtr>(mp);
    if (!m2dp)
    {
        std::ostringstream os;
        os << meshName << " is not a 2D mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
        if (region0Name == region1Name)
        {
            std::ostringstream os;
            os << region0Name << " cannot be specified for both region0 and region1\n";
            data.SetErrorResult(os.str());
            return;
        }
        else
        {
            dsMesh::MeshInterface2dPtr ip(new dsMesh::MeshInterface2d(name, region0Name, region1Name)); 
            m2dp->AddInterface(ip);
            dsMesh::BoundingBox bb(xl, xh, yl, yh, bl);
            ip->AddBoundingBox(bb);
            data.SetEmptyResult();
        }
    }   
}

void 
add2dContactCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"name",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region",  "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"xl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"xh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"bloat", "1.0e-10",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &name = data.GetStringOption("name");
    const std::string &regionName  = data.GetStringOption("region");
    const double xl = data.GetDoubleOption("xl");
    const double xh = data.GetDoubleOption("xh");
    const double yl = data.GetDoubleOption("yl");
    const double yh = data.GetDoubleOption("yh");
    const double bl = data.GetDoubleOption("bloat");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh2dPtr m2dp = dynamic_cast<dsMesh::Mesh2dPtr>(mp);
    if (!m2dp)
    {
        std::ostringstream os;
        os << meshName << " is not a 2D mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
        dsMesh::MeshContact2dPtr cp(new dsMesh::MeshContact2d(name, regionName)); 
        m2dp->AddContact(cp);
        dsMesh::BoundingBox bb(xl, xh, yl, yh, bl);
        cp->AddBoundingBox(bb);
        data.SetEmptyResult();
    }
}

void 
add1dRegionCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"region",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"material", "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"tag1",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"tag2",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,  NULL,    dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &regionName = data.GetStringOption("region");
    const std::string &materialName = data.GetStringOption("material");
    const std::string &tagName1 = data.GetStringOption("tag1");
    const std::string &tagName2 = data.GetStringOption("tag2");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh1dPtr m1dp = dynamic_cast<dsMesh::Mesh1dPtr>(mp);
    if (!m1dp)
    {
        std::ostringstream os;
        os << meshName << " is not a 1D mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
        m1dp->AddRegion(dsMesh::MeshRegion1d(regionName, materialName, tagName1, tagName2));
        data.SetEmptyResult();
    }
}

void 
add2dRegionCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"region",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"material", "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"xl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"xh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yl", "-MAXDOUBLE", dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"yh", "MAXDOUBLE",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {"bloat", "1.0e-10",  dsGetArgs::Types::FLOAT, dsGetArgs::Types::OPTIONAL, NULL},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &regionName = data.GetStringOption("region");
    const std::string &materialName = data.GetStringOption("material");
    const double xl = data.GetDoubleOption("xl");
    const double xh = data.GetDoubleOption("xh");
    const double yl = data.GetDoubleOption("yl");
    const double yh = data.GetDoubleOption("yh");
    const double bl = data.GetDoubleOption("bloat");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::Mesh2dPtr m2dp = dynamic_cast<dsMesh::Mesh2dPtr>(mp);
    if (!m2dp)
    {
        std::ostringstream os;
        os << meshName << " is not a 2D mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
        dsMesh::MeshRegion2dPtr rp(new dsMesh::MeshRegion2d(regionName, materialName)); 
        m2dp->AddRegion(rp);
        dsMesh::BoundingBox bb(xl, xh, yl, yh, bl);
        rp->AddBoundingBox(bb);
        data.SetEmptyResult();
    }
}

void 
createDeviceCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"mesh",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustBeFinalized},
        {"device",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, mustNotBeValidDevice},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &deviceName = data.GetStringOption("device");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);

    bool ret = mp->Instantiate(deviceName, errorString);
    if (!ret)
    {
      data.SetErrorResult(errorString);
      return;
    }
    else
    {
      data.SetEmptyResult();
    }
}

void 
loadDevicesCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
// TODO:  "specify list of solutions to read/write"
// TODO:  "delete device if there is a problem"
// TODO:  "specify option to only load the mesh creator, but don't instantiate"
    dsGetArgs::Option option[] = {
        {"file",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, NULL},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &fileName = data.GetStringOption("file");

    bool ret = dsDevsimParse::LoadMeshes(fileName, errorString);
    if (!ret)
    {
      data.SetErrorResult(errorString);
      return;
    }
    else
    {
      data.SetEmptyResult();
    }
}

void 
writeDevicesCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] = {
        {"file",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, NULL},
        {"device",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL},
        {"type",     "", dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL},
        {NULL,  NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &fileName = data.GetStringOption("file");
    const std::string &device   = data.GetStringOption("device");
    const std::string &type = data.GetStringOption("type");

    std::auto_ptr<MeshWriter> mw;

    if (type.empty() || (type == "devsim"))
    {
        mw = std::auto_ptr<MeshWriter>(new DevsimRestartWriter());
    }
    else if (type == "devsim_data")
    {
        mw = std::auto_ptr<MeshWriter>(new DevsimWriter());
    }
    else if (type == "floops")
    {
        mw = std::auto_ptr<MeshWriter>(new FloodsWriter());
    }
    else if (type == "vtk")
    {
        mw = std::auto_ptr<MeshWriter>(new VTKWriter());
    }
    else if (type == "tecplot")
    {
        mw = std::auto_ptr<MeshWriter>(new TecplotWriter());
    }
    else
    {
        errorString += "type: " + type + " is not a valid type.  Please select from \"devsim\", \"devsim_data\", \"floops\", \"vtk\", or \"tecplot\".\n";
        data.SetErrorResult(errorString);
        return;
    }


    bool ret = true;
    if (device.empty())
    {
        ret = mw->WriteMeshes(fileName, errorString);
    }
    else
    {
        ret = mw->WriteMesh(device, fileName, errorString);
    }

    if (!ret)
    {
      data.SetErrorResult(errorString);
      return;
    }
    else
    {
      data.SetEmptyResult();
    }
}

void 
createGmshMeshCmd(CommandHandler &data)
{
    std::string errorString;

//    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    dsGetArgs::Option option[] =
    {
        {"mesh", "",   dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshCannotExist},
        {"file", "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,   NULL, dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL, NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &fileName = data.GetStringOption("file");
    const std::string &meshName = data.GetStringOption("mesh");

    bool ret = dsGmshParse::LoadMeshes(fileName, meshName, errorString);
    if (!ret)
    {
      data.SetErrorResult(errorString);
      return;
    }
    else
    {
      data.SetEmptyResult();
    }
}

void 
addGmshInterfaceCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"name",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"gmsh_name",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region0",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region1",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &name = data.GetStringOption("name");
    const std::string &gmshName  = data.GetStringOption("gmsh_name");
    const std::string &regionName0  = data.GetStringOption("region0");
    const std::string &regionName1  = data.GetStringOption("region1");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::GmshLoaderPtr gmp = dynamic_cast<dsMesh::GmshLoaderPtr>(mp);
    if (!gmp)
    {
        std::ostringstream os;
        os << meshName << " is not a gmsh mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
      gmp->MapPhysicalNameToInterface(gmshName, name, regionName0, regionName1);
      data.SetEmptyResult();
    }
}

void 
addGmshContactCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"name",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"gmsh_name",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"region",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &name = data.GetStringOption("name");
    const std::string &gmshName  = data.GetStringOption("gmsh_name");
    const std::string &regionName  = data.GetStringOption("region");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::GmshLoaderPtr gmp = dynamic_cast<dsMesh::GmshLoaderPtr>(mp);
    if (!gmp)
    {
        std::ostringstream os;
        os << meshName << " is not a gmsh mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
      gmp->MapPhysicalNameToContact(gmshName, name, regionName);
      data.SetEmptyResult();
    }
}

void 
addGmshRegionCmd(CommandHandler &data)
{
    std::string errorString;

    const std::string commandName = data.GetCommandName();

    using namespace dsGetArgs;
    
    dsGetArgs::Option option[] = {
        {"mesh",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, meshMustNotBeFinalized},
        {"region",   "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"gmsh_name",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {"material",    "", dsGetArgs::Types::STRING, dsGetArgs::Types::REQUIRED, stringCannotBeEmpty},
        {NULL,  NULL,  dsGetArgs::Types::STRING, dsGetArgs::Types::OPTIONAL,  NULL}
    };

    dsGetArgs::switchList switches = NULL;


    bool error = data.processOptions(option, switches, errorString);

    if (error)
    {
        data.SetErrorResult(errorString);
        return;
    }

    const std::string &meshName = data.GetStringOption("mesh");
    const std::string &gmshName  = data.GetStringOption("gmsh_name");
    const std::string &regionName  = data.GetStringOption("region");
    const std::string &materialName  = data.GetStringOption("material");

    dsMesh::MeshKeeper &mdata = dsMesh::MeshKeeper::GetInstance();
    dsMesh::MeshPtr mp = mdata.GetMesh(meshName);
    dsMesh::GmshLoaderPtr gmp = dynamic_cast<dsMesh::GmshLoaderPtr>(mp);
    if (!gmp)
    {
        std::ostringstream os;
        os << meshName << " is not a gmsh mesh\n";
        data.SetErrorResult(os.str());
        return;
    }
    else
    {
      gmp->MapPhysicalNameToRegion(gmshName, regionName, materialName);
      data.SetEmptyResult();
    }
}

Commands MeshingCommands[] = {
    {"create_1d_mesh",    create1dMeshCmd},
    {"finalize_mesh",     finalizeMeshCmd},
    {"add_1d_mesh_line",  add1dMeshLineCmd},
    {"add_1d_interface",  add1dInterfaceCmd},
    {"add_1d_contact",    add1dInterfaceCmd},
    {"add_1d_region",     add1dRegionCmd},
    {"create_2d_mesh",    create1dMeshCmd},
    {"add_2d_mesh_line",  add2dMeshLineCmd},
    {"add_2d_region",     add2dRegionCmd},
    {"add_2d_interface",  add2dInterfaceCmd},
    {"add_2d_contact",    add2dContactCmd},
    {"create_device",  createDeviceCmd},
    {"load_devices",   loadDevicesCmd},
    {"write_devices",  writeDevicesCmd},
    {"create_gmsh_mesh", createGmshMeshCmd},
    {"add_gmsh_contact", addGmshContactCmd},
    {"add_gmsh_interface", addGmshInterfaceCmd},
    {"add_gmsh_region", addGmshRegionCmd},
    {NULL, NULL}
};
}

// TODO:  "get_mesh_list"
// TODO:  "Remove mesh after loaded by default.  Provide option for copying or rename device on read."

