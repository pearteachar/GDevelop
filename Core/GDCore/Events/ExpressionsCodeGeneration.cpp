/** \file
 *  Game Develop
 *  2008-2013 Florian Rival (Florian.Rival@gmail.com)
 */
#include "ExpressionsCodeGeneration.h"
#include "GDCore/PlatformDefinition/Project.h"
#include "GDCore/PlatformDefinition/Layout.h"
#include "GDCore/PlatformDefinition/Platform.h"
#include "GDCore/PlatformDefinition/PlatformExtension.h"
#include "GDCore/IDE/MetadataProvider.h"
#include "GDCore/Events/ExpressionParser.h"
#include "GDCore/Events/EventsCodeGenerator.h"
#include "GDCore/Events/EventsCodeNameMangler.h"
#include "GDCore/Events/EventsCodeGenerationContext.h"
#include "GDCore/Events/ExpressionMetadata.h"
#include "GDCore/Events/ObjectMetadata.h"
#include "GDCore/Events/AutomatismMetadata.h"
#include "GDCore/CommonTools.h"

using namespace std;

namespace gd
{

CallbacksForGeneratingExpressionCode::CallbacksForGeneratingExpressionCode(string & plainExpression_,
                                                                           EventsCodeGenerator & codeGenerator_,
                                                                           EventsCodeGenerationContext & context_) :
    plainExpression(plainExpression_),
    codeGenerator(codeGenerator_),
    context(context_)
{

}

void CallbacksForGeneratingExpressionCode::OnConstantToken(string text)
{
    plainExpression += text;
};
void CallbacksForGeneratingExpressionCode::OnOperator(string text)
{
    plainExpression += text;
};
void CallbacksForGeneratingExpressionCode::OnNumber(string text)
{
    if (text.find_first_of(".e") == std::string::npos)
        text += ".0";

    plainExpression += text;
};

void CallbacksForGeneratingExpressionCode::OnStaticFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::ExpressionMetadata & expressionInfo)
{
    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);

    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::ExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }


    vector<string> parametersCode = codeGenerator.GenerateParametersCodes( parameters, expressionInfo.parameters, context);

    string parametersStr;
    for (unsigned int i = 0;i<parametersCode.size();++i)
    {
        if ( i != 0 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    plainExpression += expressionInfo.codeExtraInformation.functionCallName+"("+parametersStr+")";
};

void CallbacksForGeneratingExpressionCode::OnStaticFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::StrExpressionMetadata & expressionInfo)
{
    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);
    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::StrExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }

    //TODO : A bit of hack here..
    //Special case : Function without name is a string.
    if ( functionName.empty() )
    {
        if ( parameters.empty() ) return;
        plainExpression += codeGenerator.ConvertToStringExplicit(codeGenerator.ConvertToString(parameters[0].GetPlainString()));

        return;
    }

    //Prepare parameters
    vector<string> parametersCode = codeGenerator.GenerateParametersCodes(parameters, expressionInfo.parameters, context);
    string parametersStr;
    for (unsigned int i = 0;i<parametersCode.size();++i)
    {
        if ( i != 0 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    plainExpression += expressionInfo.codeExtraInformation.functionCallName+"("+parametersStr+")";
};

void CallbacksForGeneratingExpressionCode::OnObjectFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::ExpressionMetadata & expressionInfo)
{
    const gd::Project & project = codeGenerator.GetProject();
    const gd::Layout & scene = codeGenerator.GetLayout();

    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);
    if ( parameters.empty() ) return;

    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::ExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }

    //Prepare parameters
    vector<string> parametersCode = codeGenerator.GenerateParametersCodes(parameters, expressionInfo.parameters, context);
    string parametersStr;
    for (unsigned int i = 1;i<parametersCode.size();++i)
    {
        if ( i != 1 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    std::string output = "0";

    //Get object(s) concerned by function call
    std::vector<std::string> realObjects = codeGenerator. ExpandObjectsName(parameters[0].GetPlainString(), context);
    for (unsigned int i = 0;i<realObjects.size();++i)
    {
        context.ObjectsListNeeded(realObjects[i]);

        string objectType = gd::GetTypeOfObject(project, scene, realObjects[i]);
        const ObjectMetadata & objInfo = MetadataProvider::GetObjectMetadata(codeGenerator.GetPlatform(), objectType);

        //Build string to access the object
        codeGenerator.AddIncludeFile(objInfo.optionalIncludeFile);
        if ( context.GetCurrentObject() == realObjects[i]  && !context.GetCurrentObject().empty())
            output = codeGenerator.GenerateCurrentObjectFunctionCall(realObjects[i], objInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, context);
        else
            output = codeGenerator.GenerateNotPickedObjectFunctionCall(realObjects[i], objInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, output, context);
    }

    plainExpression += output;
};

void CallbacksForGeneratingExpressionCode::OnObjectFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::StrExpressionMetadata & expressionInfo)
{
    const gd::Project & project = codeGenerator.GetProject();
    const gd::Layout & scene = codeGenerator.GetLayout();

    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);
    if ( parameters.empty() ) return;

    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::StrExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }

    //Prepare parameters
    vector<string> parametersCode = codeGenerator.GenerateParametersCodes(parameters, expressionInfo.parameters, context);
    string parametersStr;
    for (unsigned int i = 1;i<parametersCode.size();++i)
    {
        if ( i != 1 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    //Get object(s) concerned by function call
    std::vector<std::string> realObjects = codeGenerator. ExpandObjectsName(parameters[0].GetPlainString(), context);

    std::string output = "\"\"";
    for (unsigned int i = 0;i<realObjects.size();++i)
    {
        context.ObjectsListNeeded(realObjects[i]);

        string objectType = gd::GetTypeOfObject(project, scene, realObjects[i]);
        const ObjectMetadata & objInfo = MetadataProvider::GetObjectMetadata(codeGenerator.GetPlatform(), objectType);

        //Build string to access the object
        codeGenerator.AddIncludeFile(objInfo.optionalIncludeFile);
        if ( context.GetCurrentObject() == realObjects[i]  && !context.GetCurrentObject().empty())
            output = codeGenerator.GenerateCurrentObjectFunctionCall(realObjects[i], objInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, context);
        else
            output = codeGenerator.GenerateNotPickedObjectFunctionCall(realObjects[i], objInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, output, context);
    }

    plainExpression += output;
};

void CallbacksForGeneratingExpressionCode::OnObjectAutomatismFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::ExpressionMetadata & expressionInfo)
{
    const gd::Project & project = codeGenerator.GetProject();
    const gd::Layout & scene = codeGenerator.GetLayout();

    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);
    if ( parameters.size() < 2 ) return;

    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::ExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }

    //Prepare parameters
    vector<string> parametersCode = codeGenerator.GenerateParametersCodes(parameters, expressionInfo.parameters, context);
    string parametersStr;
    for (unsigned int i = 2;i<parametersCode.size();++i)
    {
        if ( i != 2 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    //Get object(s) concerned by function call
    std::vector<std::string> realObjects = codeGenerator. ExpandObjectsName(parameters[0].GetPlainString(), context);

    std::string output = "0";
    for (unsigned int i = 0;i<realObjects.size();++i)
    {
        context.ObjectsListNeeded(realObjects[i]);

        //Cast the object if needed
        string automatismType = gd::GetTypeOfAutomatism(project, scene, parameters[1].GetPlainString());
        const AutomatismMetadata & autoInfo = MetadataProvider::GetAutomatismMetadata(codeGenerator.GetPlatform(), automatismType);

        //Build string to access the automatism
        codeGenerator.AddIncludeFile(autoInfo.optionalIncludeFile);
        if ( context.GetCurrentObject() == realObjects[i]  && !context.GetCurrentObject().empty() )
            output = codeGenerator.GenerateCurrentObjectAutomatismFunctionCall(realObjects[i], parameters[1].GetPlainString(), autoInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, context);
        else
            output = codeGenerator.GenerateNotPickedObjectAutomatismFunctionCall(realObjects[i], parameters[1].GetPlainString(), autoInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, output, context);
    }

    plainExpression += output;
};

void CallbacksForGeneratingExpressionCode::OnObjectAutomatismFunction(string functionName, const std::vector<gd::Expression> & parameters, const gd::StrExpressionMetadata & expressionInfo)
{
    const gd::Project & project = codeGenerator.GetProject();
    const gd::Layout & scene = codeGenerator.GetLayout();

    codeGenerator.AddIncludeFile(expressionInfo.codeExtraInformation.optionalIncludeFile);
    if ( parameters.size() < 2 ) return;

    //Launch custom code generator if needed
    if ( expressionInfo.codeExtraInformation.optionalCustomCodeGenerator != boost::shared_ptr<gd::StrExpressionMetadata::ExtraInformation::CustomCodeGenerator>() )
    { plainExpression += expressionInfo.codeExtraInformation.optionalCustomCodeGenerator->GenerateCode(parameters, codeGenerator, context); return; }

    //Prepare parameters
    vector<string> parametersCode = codeGenerator.GenerateParametersCodes(parameters, expressionInfo.parameters, context);
    string parametersStr;
    for (unsigned int i = 2;i<parametersCode.size();++i)
    {
        if ( i != 2 ) parametersStr += ", ";
        parametersStr += parametersCode[i];
    }

    //Get object(s) concerned by function call
    std::vector<std::string> realObjects = codeGenerator. ExpandObjectsName(parameters[0].GetPlainString(), context);

    std::string output = "\"\"";
    for (unsigned int i = 0;i<realObjects.size();++i)
    {
        context.ObjectsListNeeded(realObjects[i]);

        //Cast the object if needed
        string automatismType = gd::GetTypeOfAutomatism(project, scene, parameters[1].GetPlainString());
        const AutomatismMetadata & autoInfo = MetadataProvider::GetAutomatismMetadata(codeGenerator.GetPlatform(), automatismType);

        //Build string to access the automatism
        codeGenerator.AddIncludeFile(autoInfo.optionalIncludeFile);
        if ( context.GetCurrentObject() == realObjects[i]  && !context.GetCurrentObject().empty() )
            output = codeGenerator.GenerateCurrentObjectAutomatismFunctionCall(realObjects[i], parameters[1].GetPlainString(), autoInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, context);
        else
            output = codeGenerator.GenerateNotPickedObjectAutomatismFunctionCall(realObjects[i], parameters[1].GetPlainString(), autoInfo, expressionInfo.codeExtraInformation.functionCallName, parametersStr, output, context);
    }

    plainExpression += output;
};

bool CallbacksForGeneratingExpressionCode::OnSubMathExpression(const gd::Platform & platform, const gd::Project & project, const gd::Layout & layout, gd::Expression & expression)
{
    string newExpression;

    CallbacksForGeneratingExpressionCode callbacks(newExpression, codeGenerator, context);

    gd::ExpressionParser parser(expression.GetPlainString());
    if ( !parser.ParseMathExpression(platform, project, layout, callbacks) )
    {
        #if defined(GD_IDE_ONLY)
        firstErrorStr = callbacks.firstErrorStr;
        firstErrorPos = callbacks.firstErrorPos;
        #endif
        return false;
    }

    return true;
}

bool CallbacksForGeneratingExpressionCode::OnSubTextExpression(const gd::Platform & platform, const gd::Project & project, const gd::Layout & layout, gd::Expression & expression)
{
    string newExpression;

    CallbacksForGeneratingExpressionCode callbacks(newExpression, codeGenerator, context);

    gd::ExpressionParser parser(expression.GetPlainString());
    if ( !parser.ParseStringExpression(platform, project, layout, callbacks) )
    {
        #if defined(GD_IDE_ONLY)
        firstErrorStr = callbacks.firstErrorStr;
        firstErrorPos = callbacks.firstErrorPos;
        #endif
        return false;
    }

    return true;
}

}