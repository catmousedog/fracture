#include "SlangCompiler.hpp"

#include <slang-com-ptr.h>
#include <slang.h>
using Slang::ComPtr;

vector<uint32_t> compileComputeShader(
    string shaderName, string precisionType, string fractalName, string functionName, string filterName
)
{
    // once, at startup
    ComPtr<slang::IGlobalSession> global;
    slang::createGlobalSession(global.writeRef());

    slang::TargetDesc target{};
    target.format  = SLANG_SPIRV;
    target.profile = global->findProfile("spirv_1_5");

    slang::CompilerOptionEntry opt{};
    opt.name            = slang::CompilerOptionName::VulkanUseEntryPointName;
    opt.value.kind      = slang::CompilerOptionValueKind::Int;
    opt.value.intValue0 = 1;

    slang::PreprocessorMacroDesc macros[] = {{"REAL", precisionType.c_str()}};

    const char*        searchPaths[] = {"../shaders"};
    slang::SessionDesc sd{};
    sd.targets                  = &target;
    sd.targetCount              = 1;
    sd.searchPaths              = searchPaths;
    sd.searchPathCount          = 1;
    sd.compilerOptionEntries    = &opt;
    sd.compilerOptionEntryCount = 1;
    sd.preprocessorMacros       = macros;
    sd.preprocessorMacroCount   = 1;

    ComPtr<slang::ISession> session;
    global->createSession(sd, session.writeRef());

    // per (fractal, color) combination
    string glueSrc = "import " + shaderName +
                     ";\n"
                     "export struct Fractal : IFractal = " +
                     fractalName +
                     ";\n"
                     "export struct Function : IFunction = " +
                     functionName +
                     ";\n"
                     "export struct Color : IColor = " +
                     filterName + ";\n";

    ComPtr<slang::IBlob> diag;
    slang::IModule*      mod = session->loadModule(shaderName.c_str(), diag.writeRef());
    slang::IModule* glue = session->loadModuleFromSourceString("glue", "glue.slang", glueSrc.c_str(), diag.writeRef());

    ComPtr<slang::IEntryPoint> entry;
    mod->findEntryPointByName("compMain", entry.writeRef());

    slang::IComponentType*        parts[] = {mod, glue, entry};
    ComPtr<slang::IComponentType> composite, linked;
    session->createCompositeComponentType(parts, 3, composite.writeRef(), diag.writeRef());
    composite->link(linked.writeRef(), diag.writeRef());

    ComPtr<slang::IBlob> spirv;
    linked->getEntryPointCode(0, 0, spirv.writeRef(), diag.writeRef());

    auto* p = (const uint32_t*)spirv->getBufferPointer();
    return {p, p + spirv->getBufferSize() / 4};
}