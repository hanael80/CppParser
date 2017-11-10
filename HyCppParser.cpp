
#include "PCH.h"
#include <stdio.h>
#pragma warning(disable:4996)
#include "HyCppParser.h"
#include "HyCppParserConsts.h"
#include "HyCppPreprocessor.h"
#include <set>
#include <vector>


//#define USE_ORIGINAL_VALUE


void Class::GetVirtualFunctions( const ParsingResult& parsingResult, std::map< std::string, const ClassMemberFunction* >& mapFunction ) const
{
	for ( const ClassMemberFunction& memberFunction : m_memberFunctions )
	{
		if ( memberFunction.m_strAttribute.find( "virtual" ) == std::string::npos) continue;
		if ( mapFunction.find( memberFunction.m_name ) != mapFunction.end()) continue;

		mapFunction[ memberFunction.m_name ] = &memberFunction;
	}

	for ( const ParentClass& parentClassInfo : m_listParentClass )
	{
		std::map< std::string, Class >::const_iterator itrClass = parsingResult.m_classes.find( parentClassInfo.m_name.GetFullValue() );
		if ( itrClass == parsingResult.m_classes.end() ) continue;

		const Class& parentClass = itrClass->second;
		parentClass.GetVirtualFunctions( parsingResult, mapFunction );
	}
}

ClassMemberFunction* Class::FindMemberFunction( const std::string& functionName )
{
	for ( ClassMemberFunction& memberFunction : m_memberFunctions )
		if ( memberFunction.m_name == functionName )
			return &memberFunction;

	return NULL;
}

typedef std::list< VarType > VarTypeList;
typedef std::list< std::string > VariableAttributes;

ParsingResult*                               g_parsingResult = NULL;
std::list< FunctionParam >                   g_functionParams;
std::string                                  g_strClassAccess;
std::string                                  g_strClassMemberAccess;
std::list< Variable >                        g_listVariable;
std::list< Namespace* >                      g_listNamespace;
std::list< VariableAttributes >              g_listVariableAttributes;
std::string                                  g_strVariableScopeAttribute;
VarTypeList                                  g_variableTypes;
std::list< HyUInt32 >                        g_listVariableTypeCount;
TokenString                                  g_functionName;
std::string                                  g_strFunctionParameterName;
std::string                                  g_functionAttribute;
std::string                                  g_strOperatorType;
std::list< HyUInt32 >                        g_functionParameterCount;
std::list< Class* >                          g_listClass;
std::list< int >                             g_listFunctionCallParamCount;
VarTypeList                                  g_listTemplateType;
std::list< std::string >                     g_listTypeDefName;
std::list< Function >                        g_functionTypeDefList;
std::list< VarTypeList >                     g_listSpecifiedValues;
std::map< std::string, VarType >             g_mapTypeDef;
std::list< ParentClass >                     g_listParentClass;
std::list< VarTypeList >                     g_listVariableTypeList;
std::list< bool >                            g_listClassBodyExistance;
HyInt32                                      g_unnamedNamespaceIdx = 0;
std::list< std::pair< TokenPos, TokenPos > > g_ranges;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	생성자
///
/// @param	flag	플래그
/// @param	name	이름
////////////////////////////////////////////////////////////////////////////////////////////////////
HyCppParser::HyCppParser(
	      HyUInt32  flag,
	const HyString& name)
:
HyGrammarParser( name )
{
	Init( flag );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	현재 클래스를 반환한다.
///
/// @return	현재 클래스
////////////////////////////////////////////////////////////////////////////////////////////////////
static Class* _GetCurClass()
{
	return g_listClass.back();
}

std::string ConvertByTypeDef( const std::string& strString, const std::map< std::string, VarType >& mapTypeDef )
{
#ifdef USE_ORIGINAL_VALUE
	return strString;
#else
	char szTarget[ 256 ];
	strcpy_s( szTarget, sizeof( szTarget ), strString.c_str() );

	bool bConverted;

	do
	{
		bConverted = false;

		std::map<std::string, VarType>::const_reverse_iterator itrTypeDef = mapTypeDef.rbegin();

		for ( ; itrTypeDef != mapTypeDef.rend(); ++itrTypeDef )
		{
			const std::string& strSource = itrTypeDef->first;
			const VarType& target = itrTypeDef->second;

			char* const p = strstr(szTarget, strSource.c_str());
			if ( p == NULL )
				continue;

			if ( p != szTarget && p[ -1 ] != ' ' )
				continue;

			if ( p[ strSource.length() ] != 0 && p[ strSource.length() ] != ' ' )
				continue;

			char szBuf[ 256 ];
			strcpy_s( szBuf, sizeof( szBuf ), p + strSource.length() );
			*p = 0;
			strcat_s( szTarget, sizeof( szTarget ), target.GetFullValue().c_str() );
			strcat_s( szTarget, sizeof( szTarget ), szBuf );
			bConverted = true;
			break;
		}
	} while ( bConverted );

	return szTarget;
#endif
}


std::string ConvertByTypeDef(const std::string& strString)
{
#ifdef USE_ORIGINAL_VALUE
	return strString;
#else
	return ConvertByTypeDef(strString, g_parsingResult->m_typeDefs);
#endif
}


VarType ConvertByTypeDef( VarType varType, const std::map< std::string, VarType >& mapTypeDef )
{
#ifndef USE_ORIGINAL_VALUE
	varType.m_value = ConvertByTypeDef( varType.m_value, mapTypeDef );

	std::list< VarType >::iterator itrTemplateParam = varType.m_templateParamList.begin();
	for ( ; itrTemplateParam != varType.m_templateParamList.end(); ++itrTemplateParam )
	{
		VarType& templateParam = *itrTemplateParam;
		templateParam = ConvertByTypeDef( templateParam, mapTypeDef );
	}
#endif

	return varType;
}


VarType ConvertByTypeDef( const VarType& varType )
{
#ifdef USE_ORIGINAL_VALUE
	return varType;
#else
	return ConvertByTypeDef( varType, g_parsingResult->m_typeDefs );
#endif
}


void FirstLevelStatement_TypeDefinition( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	std::map<std::string, VarType>::const_iterator itr = g_mapTypeDef.begin();

	for ( ; itr != g_mapTypeDef.end(); ++itr )
	{
		const VarType& target = itr->second;
		g_listNamespace.back()->m_typeDefs[ itr->first ] = target;

		std::string strSource;
		if ( g_listNamespace.back()->m_fullName.empty() )
			strSource = itr->first;
		else
			strSource = g_listNamespace.back()->m_fullName + "::" + itr->first;

		g_parsingResult->m_typeDefs[ strSource ] = target;
		g_parsingResult->m_reverseTypeDefs[ target.GetFullValue() ] = strSource;
	}

	g_mapTypeDef.clear();
}


void NamespaceArea( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listNamespace.pop_back();
}


void NamespaceName( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	Namespace* parentNamespace = g_listNamespace.back();
	if ( !parentNamespace )
	{
		LOG( HyLog::Fatal, "parentNamespace == NULL" );
		return;
	}

	const std::string& namespaceName = vecToken[ 0 ]->m_value;
	Namespace& curNamespace = parentNamespace->m_subNamespaces[ namespaceName ];
	curNamespace.m_name = namespaceName;

	if ( parentNamespace->m_name.empty() )
		curNamespace.m_fullName = namespaceName;
	else
		curNamespace.m_fullName = parentNamespace->m_name + "::" + namespaceName;

	g_listNamespace.push_back( &curNamespace );
}

void UnnamedNamespace( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	Namespace* const pParentNamespace = g_listNamespace.back();
	if(pParentNamespace == NULL)
	{
		LOG(HyLog::Fatal, "pParentNamespace == NULL");
		return;
	}

	HyChar namespaceName[ 256 ];
	sprintf( namespaceName, "unnamedNamespace%d", ++g_unnamedNamespaceIdx );

	Namespace& curNamespace = pParentNamespace->m_subNamespaces[ namespaceName ];
	curNamespace.m_name = namespaceName;

	if(pParentNamespace->m_name.empty())
		curNamespace.m_fullName = namespaceName;
	else
		curNamespace.m_fullName = pParentNamespace->m_name + "::" + namespaceName;

	g_listNamespace.push_back(&curNamespace);
}

void TypeDefinitionBody_VariableType( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();

	std::list< std::string >::const_iterator itrTypeDefName = g_listTypeDefName.begin();

	for ( ; itrTypeDefName != g_listTypeDefName.end(); ++itrTypeDefName )
	{
		const std::string& strTypeDefName = *itrTypeDefName;

		const VarType target = ConvertByTypeDef( varType );
		g_mapTypeDef[ strTypeDefName ] = target;

		const std::map< std::string, Class >::const_iterator itrClass = g_parsingResult->m_classes.find( target.m_value );
		if ( itrClass == g_parsingResult->m_classes.end() ) continue;

		const Class& templateClass = itrClass->second;

		if ( templateClass.m_listTemplateType.empty() ) continue;

		if ( templateClass.m_listTemplateType.size() != target.m_templateParamList.size() )
		{
			LOG( HyLog::Fatal, "pClass->m_listTemplateType.size() != target.m_listTemplateParam.size()" );
			return;
		}

		std::map< std::string, VarType > mapTypeDef;

		VarTypeList::const_iterator itrClassTemplateType = templateClass.m_listTemplateType.begin();
		VarTypeList::const_iterator itrTargetTemplateType = target.m_templateParamList.begin();

		while ( itrClassTemplateType != templateClass.m_listTemplateType.end() )
		{
			mapTypeDef[ itrClassTemplateType->GetFullValue() ] = *itrTargetTemplateType;

			++itrClassTemplateType;
			++itrTargetTemplateType;
		}

		VarType className = templateClass.m_name;
		className.m_templateParamList = target.m_templateParamList;
		Class& newClass = g_parsingResult->m_classes[ className.GetFullValue() ];
		templateClass.m_pNamespace->m_classes[ className.GetFullValue() ] = &newClass;

		newClass.m_pNamespace = templateClass.m_pNamespace;
		newClass.m_name = className;
		if(templateClass.m_pNamespace->m_fullName.empty())
			newClass.m_strFullName = newClass.m_name.GetFullValue();
		else
			newClass.m_strFullName = newClass.m_pNamespace->m_fullName + "::" + newClass.m_name.GetFullValue();
		newClass.m_mapTypeDef = templateClass.m_mapTypeDef;
		newClass.m_position = templateClass.m_position;

		std::list< ClassMemberVariable >::const_iterator itrMemberVariable = templateClass.m_listMemberVariable.begin();

		for (; itrMemberVariable != templateClass.m_listMemberVariable.end(); ++itrMemberVariable )
		{
			const ClassMemberVariable& memberVariable = *itrMemberVariable;

			ClassMemberVariable newMemberVariable = memberVariable;
			newMemberVariable.m_type = ConvertByTypeDef( memberVariable.m_type, mapTypeDef );
			newClass.m_listMemberVariable.push_back( newMemberVariable );
		}

		std::list< ClassMemberFunction >::const_iterator itrMemberFunction = templateClass.m_memberFunctions.begin();
		for (; itrMemberFunction != templateClass.m_memberFunctions.end(); ++itrMemberFunction )
		{
			const ClassMemberFunction& memberFunction = *itrMemberFunction;

			ClassMemberFunction newMemberFunction = memberFunction;
			newMemberFunction.m_returnType = ConvertByTypeDef( memberFunction.m_returnType, mapTypeDef );

			std::list< FunctionParam >::iterator itrParam = newMemberFunction.m_params.begin();

			for ( ; itrParam != newMemberFunction.m_params.end(); ++itrParam )
			{
				FunctionParam& functionParam = *itrParam;
				functionParam.m_type = ConvertByTypeDef( functionParam.m_type, mapTypeDef );
			}
			newClass.m_memberFunctions.push_back( newMemberFunction );
		}
	}

	g_listTypeDefName.clear();
	g_variableTypes.pop_back();
}


void TypeDefinitionBody_FunctionPointer( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	for ( HyUInt32 i = 0; i < g_functionParameterCount.back(); ++i )
	{
		g_functionParams.pop_back();
	}

	Function functionTypeDef;

	functionTypeDef.m_name = g_variableTypes.back().m_value;
	g_variableTypes.pop_back();

	functionTypeDef.m_returnType = g_variableTypes.back();
	g_variableTypes.pop_back();

	g_functionTypeDefList.push_back( functionTypeDef );
}


void FunctionPointer( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
}


void TypeDef_Names( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listTypeDefName.push_back( vecToken[ 0 ]->m_value );
}


void EmptyClassDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	Class curClass;
	curClass.m_name.m_value = vecToken[ 1 ]->m_value;
	//g_listNamespace.back()->m_listClass.push_back(curClass);
}


void ClassDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
}

void ClassDeclarationBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listClassBodyExistance.push_back( true );
}

void EmptyClassDeclarationBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listClassBodyExistance.push_back( false );
}

void EmptyClassTemplate( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listTemplateType.clear();
}


void TemplateClass( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listTemplateType.push_back(g_variableTypes.back());
	g_variableTypes.pop_back();
}


void ClassName_Word( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const std::string& strClassName = vecToken[ 0 ]->m_value;

	bool classBodyExistance = g_listClassBodyExistance.back();
	g_listClassBodyExistance.pop_back();

	std::string strFullName;
	if ( g_listNamespace.back()->m_fullName.empty() )
		strFullName	= strClassName;
	else
		strFullName	= g_listNamespace.back()->m_fullName + "::" + strClassName;

	if (
		g_parsingResult->m_classes.find( strFullName ) != g_parsingResult->m_classes.end() &&
		!classBodyExistance ) return;

	Class& curClass				= g_parsingResult->m_classes[ strFullName ];
	curClass.m_name.m_value		= vecToken[ 0 ]->m_value;
	curClass.m_name.m_pos		= vecToken[ 0 ]->m_pos;
	curClass.m_strFullName		= strFullName;
	curClass.m_listTemplateType = g_listTemplateType;
	curClass.m_pNamespace		= g_listNamespace.back();
	curClass.m_position				= curClass.m_name.m_pos;

	g_listClass.push_back(&curClass);
	curClass.m_pNamespace->m_classes[strClassName] = &curClass;

	g_listTemplateType.clear();
}


void EmptySpecifiedValues( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listSpecifiedValues.push_back(std::list<VarType>());
}


void SpecifiedValues( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	std::list<VarType>& listSpecifiedValue = g_listSpecifiedValues.back();
	listSpecifiedValue.push_front( g_variableTypes.back() );
	g_variableTypes.pop_back();
}


void ParentClassDeclarationEnd( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listClass.back()->m_listParentClass = g_listParentClass;
	g_listParentClass.clear();
}


void ParentClassDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	ParentClass parentClass;
	parentClass.m_strAccess = g_strClassAccess;
	parentClass.m_name = g_variableTypes.back();
	g_listParentClass.push_back(parentClass);

	g_strClassAccess.clear();
	g_variableTypes.pop_back();
}


void ClassStatement_TypeDefinition( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	std::map< std::string, VarType >::iterator typeDefItr = g_mapTypeDef.begin();

	for ( ; typeDefItr != g_mapTypeDef.end(); ++typeDefItr)
	{
		VarType& target = typeDefItr->second;

		g_listClass.back()->m_mapTypeDef[ typeDefItr->first ] = target;

		g_parsingResult->m_typeDefs[ g_listClass.back()->m_strFullName + "::" + typeDefItr->first ] = target;
		g_parsingResult->m_reverseTypeDefs[ target.GetFullValue() ] = g_listClass.back()->m_strFullName + "::" + typeDefItr->first;
	}

	g_mapTypeDef.clear();

	std::list< Function >::iterator functionTypeDefItr = g_functionTypeDefList.begin();

	for ( ; functionTypeDefItr != g_functionTypeDefList.end(); ++functionTypeDefItr )
	{
		Function& functionTypeDef = *functionTypeDefItr;

		g_listClass.back()->m_functionTypeDefList.push_back( functionTypeDef );
	}

	g_functionTypeDefList.clear();
}


void ClassAccessPrivate( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassAccess = "private";
}


void ClassAccessProtected( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassAccess = "protected";
}


void ClassAccessPublic( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassAccess = "public";
}


void Private( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassMemberAccess = "private";
}


void Protected( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassMemberAccess = "protected";
}


void Public( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strClassMemberAccess = "public";
}


void ClassMemberVariableDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType varName = g_variableTypes.back();
	g_variableTypes.pop_back();
	VarType varType = g_variableTypes.back();
	g_variableTypes.pop_back();

	ClassMemberVariable classMemberVariable;
	classMemberVariable.m_strAccess = g_strClassMemberAccess;
	classMemberVariable.m_type		= ConvertByTypeDef( varType );
	classMemberVariable.m_strName	= varName.GetFullValue();
	classMemberVariable.m_pos		= varName.m_pos;

	g_listClass.back()->m_listMemberVariable.push_back(classMemberVariable);
}


void ClassMemberFunctionDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	ClassMemberFunction classMemberFunction;
	classMemberFunction.m_strAccess     = g_strClassMemberAccess;
	classMemberFunction.m_returnType    = ConvertByTypeDef( g_variableTypes.back() );
	classMemberFunction.m_strAttribute  = g_functionAttribute;
	classMemberFunction.m_name          = g_functionName.m_value;
	classMemberFunction.m_declarationStartPosition = g_ranges.back().first;
	classMemberFunction.m_declarationEndPosition   = g_ranges.back().second;
	g_ranges.pop_back();

	for ( HyUInt32 i = 0; i < g_functionParameterCount.back(); ++i )
	{
		FunctionParam param = g_functionParams.back();

		std::list< Function >::iterator functionTypeDefItr = g_listClass.back()->m_functionTypeDefList.begin();

		for ( ; functionTypeDefItr != g_listClass.back()->m_functionTypeDefList.end(); ++functionTypeDefItr )
		{
			Function& functionTypeDef = *functionTypeDefItr;

			if ( param.m_type.m_value == functionTypeDef.m_name )
			{
				param.m_type.m_value = _GetCurClass()->m_strFullName + "::" + param.m_type.m_value;
			}
		}

		classMemberFunction.m_params.push_front( param );
		g_functionParams.pop_back();
	}

	_GetCurClass()->m_memberFunctions.push_back( classMemberFunction );

	g_functionName.m_value.clear();
	g_functionParams.clear();

	g_variableTypes.pop_back();
	g_functionAttribute.clear();
}


void ClassDestructor( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	ClassMemberFunction classMemberFunction;
	classMemberFunction.m_strAccess     = g_strClassMemberAccess;
	classMemberFunction.m_name          = "~" + vecToken[ 1 ]->m_value;
	classMemberFunction.m_params        = g_functionParams;
	classMemberFunction.m_declarationStartPosition = vecToken[ 0 ]->m_pos;
	classMemberFunction.m_declarationEndPosition   = g_ranges.back().second;
	g_ranges.pop_back();

	g_listClass.back()->m_memberFunctions.push_back(classMemberFunction);

	g_functionParams.clear();
}



void ReturnType_VariableType( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strVariableScopeAttribute.clear();
}

void EmptyReturnType( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_variableTypes.push_back( VarType() );
}

void ClassMemberVirtual( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	if( g_functionAttribute.empty() )
		g_functionAttribute = "virtual";
	else
		g_functionAttribute += " virtual";
}


void FunctionName_Word( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_functionName.m_value	= vecToken[0]->m_value;
	g_functionName.m_pPos		= &vecToken[0]->m_pos;
}


void FunctionName_Operator( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_functionName.m_value	= "operator ";
	g_functionName.m_value	+= g_strOperatorType;
	g_functionName.m_pPos		= NULL;

	g_strOperatorType.clear();
}

HyVoid OperatorType( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	g_strOperatorType = tokens[ 0 ]->m_value;
}

HyVoid FunctionBodyBegin( const std::vector< const HyToken* >& vecToken, void* const param )
{
	g_listVariableTypeList.push_back( g_variableTypes );
}

HyVoid FunctionBody( const std::vector< const HyToken* >& vecToken, void* const param )
{
	g_ranges.push_back( std::make_pair( vecToken[ 0 ]->m_pos, vecToken[ 1 ]->m_pos ) );
}

HyVoid EmptyFunctionBody( const std::vector< const HyToken* >& vecToken, void* const param )
{
	g_ranges.push_back( std::make_pair( vecToken[ 0 ]->m_pos, vecToken[ 0 ]->m_pos ) );
}

HyVoid FunctionBodyEnd( const std::vector< const HyToken* >& vecToken, void* const param )
{
	g_variableTypes = g_listVariableTypeList.back();
	g_listVariableTypeList.pop_back();
}

HyVoid FunctionDeclaration( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	Function func;

	for ( HyUInt32 i = 0; i < g_functionParameterCount.back(); ++i )
	{
		func.m_params.push_front( g_functionParams.back() );
		g_functionParams.pop_back();
	}

	g_functionParameterCount.pop_back();

	std::pair< TokenPos, TokenPos > functionBodyRange = g_ranges.back();
	g_ranges.pop_back();

	std::vector< HyString > scopes;
	HyChar buf[ 256 ];
	strcpy( buf, g_variableTypes.back().m_value.c_str() );
	HyChar* p = strtok( buf, ":" );
	while ( p )
	{
		scopes.push_back( p );
		p = strtok( NULL, ":" );
	}
	HyString functionName = scopes.back();
	scopes.pop_back();

	Class*     curClass     = NULL;
	Namespace* curNamespace = NULL;

	for ( const HyString& scope : scopes )
	{
		if ( !curClass )
		{
			Namespace::ClassList::iterator iter = g_parsingResult->m_globalNamespace.m_classes.find( scope );
			if ( iter != g_parsingResult->m_globalNamespace.m_classes.end() )
			{
				curClass = iter->second;
			}
			else
			{
				Namespace::NamespaceList::iterator iter = g_parsingResult->m_globalNamespace.m_subNamespaces.find( scope );
				HY_ENSURE( iter != g_parsingResult->m_globalNamespace.m_subNamespaces.end(), return );

				curNamespace = &iter->second;
			}
		}
		else
		{
			HY_ENSURE( false, return );
		}
	}

	func.m_name                        = functionName;
	func.m_implementationStartPosition = g_variableTypes.back().m_pos;
	func.m_implementationEndPosition   = functionBodyRange.second;
	g_variableTypes.pop_back();

	func.m_returnType = ConvertByTypeDef( g_variableTypes.back() );
	g_variableTypes.pop_back();

	if ( curClass )
	{
		ClassMemberFunction* memberFunction = NULL;

		for ( ClassMemberFunction& eachMemberFunction : curClass->m_memberFunctions )
		{
			if ( eachMemberFunction.m_name == functionName )
			{
				memberFunction = &eachMemberFunction;
				break;
			}
		}

		HY_ENSURE( memberFunction, return );

		memberFunction->m_implementationStartPosition = func.m_implementationStartPosition;
		memberFunction->m_implementationEndPosition   = func.m_implementationEndPosition;
	}
	else if ( curNamespace )
	{
		Function* curFunction = NULL;

		for ( Function& eachFunction : curNamespace->m_functions )
		{
			if ( eachFunction.m_name == functionName )
			{
				curFunction = &eachFunction;
				break;
			}
		}

		HY_ENSURE( curFunction, return );

		curFunction->m_implementationStartPosition = func.m_implementationStartPosition;
		curFunction->m_implementationEndPosition   = func.m_implementationEndPosition;
	}
	else
	{
		g_listNamespace.back()->m_functions.push_back( func );
	}

	g_functionName.m_value.clear();
	g_functionParams.clear();

	g_functionAttribute.clear();
}


void ClassMemberEqualOperatorFunctionDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	ClassMemberFunction classMemberFunction;
	classMemberFunction.m_strAccess = g_strClassMemberAccess;
	classMemberFunction.m_name	= "operator =";
	classMemberFunction.m_params = g_functionParams;

	g_listClass.back()->m_memberFunctions.push_back(classMemberFunction);

	g_functionParams.clear();
}


void VariableDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType varName = g_variableTypes.back();
	g_variableTypes.pop_back();

	VarType varType = g_variableTypes.back();
	g_variableTypes.pop_back();

	if ( strstr( varType.m_value.c_str(), "::" ) ) return;


	Variable variable;
	variable.m_pos		= varName.m_pos;
	variable.m_strName	= varName.m_value;
	variable.m_type		= varType;
	g_listVariable.push_back( variable );
}

void RightValue_Number( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType varType;
	varType.m_value	= vecToken[0]->m_value;
	varType.m_pos		= vecToken[0]->m_pos;
	g_variableTypes.push_back(varType);
}


void TargetCall( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const int iParamCount = g_listFunctionCallParamCount.back();
	g_listFunctionCallParamCount.pop_back();

	for(int i = 0; i < iParamCount; ++i)
		g_variableTypes.pop_back();

	g_variableTypes.pop_back();
}


void VariableType( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();
	varType.m_attributeList = g_listVariableAttributes.back();

	g_listVariableAttributes.pop_back();
}


void VariablePointers_Pointer( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();
	if(varType.m_tail.empty())
		varType.m_tail = "*";
	else
		varType.m_tail += " *";
}


void VariableReferenceMark_And( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();
	if(varType.m_tail.empty())
		varType.m_tail = "&";
	else
		varType.m_tail += " &";
}


void FunctionStatic( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_functionAttribute = "static";
}


void VariableStatic( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strVariableScopeAttribute = "static";
}

void BeginVariableDeclarationBody( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listVariableTypeCount.push_back( (HyInt32)( g_variableTypes.size() ) );
}

void EndVariableDeclarationBody( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	HyUInt32 variableTypeCount = g_listVariableTypeCount.back();
	g_listVariableTypeCount.pop_back();

	while ( g_variableTypes.size() > variableTypeCount )
		g_variableTypes.pop_back();
}

void VariableAttributesBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listVariableAttributes.push_back( std::list< std::string >() );
}

void VariableAttribute_Const( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listVariableAttributes.back().push_back( "const" );
}


HyVoid VariableBasicType( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	VarType varType;

	for ( const HyToken* token : tokens )
	{
		if ( !varType.m_value.empty() )
			varType.m_value += " ";

		varType.m_value += token->m_value;
	}

	varType.m_pos = tokens.back()->m_pos;
	g_variableTypes.push_back( varType );
}

HyVoid UnsignableType( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	VarType varType = g_variableTypes.back();
	g_variableTypes.pop_back();

	if ( varType.m_pos.m_fileName.empty() )
		varType.m_pos   = tokens[ 0 ]->m_pos;

	varType.m_value = tokens[ 0 ]->m_value + " " + varType.m_value;

	g_variableTypes.push_back( varType );
}

HyVoid VariableBasicType_Empty( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	VarType varType;
	varType.m_value = "int";
	g_variableTypes.push_back( varType );
}

void VariableBasicType_Word( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType varType;
	varType.m_value				= vecToken[0]->m_value;
	varType.m_templateParamList = g_listSpecifiedValues.back();
	varType.m_pos				= vecToken[0]->m_pos;
	g_variableTypes.push_back( varType );

	g_listSpecifiedValues.pop_back();
}

void ScopeTarget_ScopeTypeWord( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();
	varType.m_value = vecToken[0]->m_value + "::" + varType.m_value;
}

HyVoid Name( const std::vector< const HyToken* >& tokens, void* const pParam )
{
	VarType varType;
	varType.m_value = tokens[ 0 ]->m_value;
	varType.m_pos = tokens[ 0 ]->m_pos;
	g_variableTypes.push_back( varType );
}

HyVoid Name_Operator( const std::vector< const HyToken* >& tokens, void* const param )
{
	VarType varType;
	varType.m_value = "operator " + g_strOperatorType;
	g_variableTypes.push_back( varType );
}

HyVoid NextScopeName( const std::vector< const HyToken* >& tokens, void* const pParam )
{
	VarType varType = g_variableTypes.back();
	g_variableTypes.pop_back();
	std::string strType2 = g_variableTypes.back().m_value;
	g_variableTypes.pop_back();

	varType.m_value = strType2 + "::" + varType.m_value;
	g_variableTypes.push_back( varType );
}

void EmptyFunctionParameters( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_functionParameterCount.push_back( 0 );
}

void FunctionParametersBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_functionParameterCount.push_back( 0 );
	g_listVariableTypeList.push_back( g_variableTypes );
}

void FunctionParametersEnd( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_variableTypes = g_listVariableTypeList.back();
	g_listVariableTypeList.pop_back();
}

void FunctionParameterDeclaration( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType& varType = g_variableTypes.back();

	FunctionParam functionParameter;
	functionParameter.m_type	= ConvertByTypeDef( varType );
	functionParameter.m_strName	= g_strFunctionParameterName;
	g_functionParams.push_back( functionParameter );

	g_variableTypes.pop_back();
	g_strFunctionParameterName.clear();

	++g_functionParameterCount.back();
}


void FunctionParameter_FunctionPointer( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	VarType paramType = g_variableTypes.back();
	paramType.m_value = "[FunctionPointer]";
	FunctionParam functionParameter;
	functionParameter.m_type	= paramType;
	functionParameter.m_strName	= g_variableTypes.back().GetFullValue();

	for ( HyUInt32 i = 0; i < g_functionParameterCount.back(); ++i )
	{
		g_functionParams.pop_back();
	}

	g_functionParams.push_back( functionParameter );

	g_variableTypes.pop_back();
	g_strFunctionParameterName.clear();

	g_functionParameterCount.pop_back();
	++g_functionParameterCount.back();
}


void FunctionParameterName_Word( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_strFunctionParameterName = vecToken[0]->m_value;
}

void FunctionParameterAssignBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listVariableTypeList.push_back( g_variableTypes );
}

void FunctionParameterAssignEnd( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_variableTypes = g_listVariableTypeList.back();
	g_listVariableTypeList.pop_back();
}

void EmptyFunctionCallParameter( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listFunctionCallParamCount.push_back( 0 );
}

void FunctionCallParametersBegin( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_listFunctionCallParamCount.push_back( 1 );
}

void NextFunctionCallParameter( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	++g_listFunctionCallParamCount.back();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	초기화할때를 처리한다.
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
void HyCppParser::OnInit()
{
	AddInputGrammar( source_,								first_level_statements_ );
	AddInputGrammar( first_level_statements_,				first_level_statement_ first_level_statements_ );
	AddInputGrammar( first_level_statements_,				"[e]" );
	AddInputGrammar( first_level_statement_,				second_level_statement_ );
	AddInputGrammar( first_level_statement_,				namespace_area_ );
	AddInputGrammar( first_level_statement_,				namespace_ word_ "=" scope_name_ ";" );
	AddInputGrammar( first_level_statement_,				type_definition_ ";", FirstLevelStatement_TypeDefinition );
	AddInputGrammar( first_level_statement_,				function_declaration_ );
	AddInputGrammar( first_level_statement_,				variable_declaration_ ";" );
	AddInputGrammar( first_level_statement_,				enum_declaration_ ";" );
	AddInputGrammar( first_level_statement_,				using_ namespace_ scope_name_ ";" );
	AddInputGrammar( first_level_statement_,				class_templates_ using_ word_ "=" scope_name_ ";" );
	AddInputGrammar( first_level_statement_,				extern_ word_ "{" first_level_statements_ "}" );
	
	AddInputGrammar( namespace_area_,						namespace_ namespace_name_ "{" first_level_statements_ "}", NamespaceArea );
	AddInputGrammar( namespace_name_,						word_, NamespaceName );
	AddInputGrammar( namespace_name_,						"[e]", UnnamedNamespace );

	AddInputGrammar( type_definition_,						typedef_ type_definition_body_ );
	AddInputGrammar( type_definition_body_,					variable_type_ typedef_names_, TypeDefinitionBody_VariableType );
	AddInputGrammar( type_definition_body_,					class_declaration_ typedef_names_ );
	AddInputGrammar( type_definition_body_,					function_pointer_, TypeDefinitionBody_FunctionPointer);
	AddInputGrammar( function_pointer_,						return_type_ "(" typedef_scope_type_ "*" variable_name_ ") (" function_parameters_ ")" class_member_const_, FunctionPointer );
	AddInputGrammar( typedef_names_,						variable_pointers_ word_ next_typedef_name_, TypeDef_Names );
	AddInputGrammar( next_typedef_name_,					"," typedef_names_ );
	AddInputGrammar( next_typedef_name_,					"[e]" );
	AddInputGrammar( typedef_scope_type_,					scope_type_ "::" );
	AddInputGrammar( typedef_scope_type_,					"[e]" );

	AddInputGrammar( second_level_statement_,				class_declaration_ ";" );
	AddInputGrammar( second_level_statement_,				";" );

	AddInputGrammar( class_declaration_,					class_declaration_begin_ class_templates_ class_keyword_ class_name_ template_specification_ class_inheritance_ class_body_ );
	AddInputGrammar( class_declaration_,					empty_class_declaration_begin_ class_templates_ class_keyword_ class_name_ template_specification_ class_inheritance_ );
	AddInputGrammar( class_inheritance_,					":" parent_classes_ );
	AddInputGrammar( class_inheritance_,					"[e]" );
	AddInputGrammar( class_declaration_begin_,				"[e]", ClassDeclarationBegin );
	AddInputGrammar( empty_class_declaration_begin_,		"[e]", EmptyClassDeclarationBegin );
	AddInputGrammar( class_body_,							"{" class_statements_ "}" );

	AddInputGrammar( class_template_,						template_ "<" template_classes_ ">" );
	AddInputGrammar( class_templates_,						class_template_ class_templates_ );
	AddInputGrammar( class_templates_,						"[e]", EmptyClassTemplate );
	AddInputGrammar( template_classes_,						template_class_ next_template_class_ );
	AddInputGrammar( template_classes_,						"[e]" );
	AddInputGrammar( next_template_class_,					"," template_classes_ );
	AddInputGrammar( next_template_class_,					"[e]" );
	AddInputGrammar( template_class_,						variable_type_ class_assign_, TemplateClass );
	AddInputGrammar( template_class_,						variable_type_ variable_name_ class_assign_, TemplateClass );
	AddInputGrammar( class_assign_,							"=" right_expression_ );
	AddInputGrammar( class_assign_,							"[e]" );

	AddInputGrammar( class_keyword_,						"class" );
	AddInputGrammar( class_keyword_,						"struct" );
	AddInputGrammar( class_keyword_,						"union" );

	AddInputGrammar( class_name_,							word_, ClassName_Word );
	AddInputGrammar( class_name_,							"[e]" );

	AddInputGrammar( template_specification_,				template_specification_begin_ "<" specified_values_ ">" );
	AddInputGrammar( template_specification_,				"[e]", EmptySpecifiedValues );
	AddInputGrammar( template_specification_begin_,			"[e]", EmptySpecifiedValues );
	AddInputGrammar( specified_values_,						variable_type_ next_specified_value_, SpecifiedValues );
	AddInputGrammar( specified_values_,						right_value_ next_specified_value_, SpecifiedValues );
	AddInputGrammar( next_specified_value_,					"," specified_values_ );
	AddInputGrammar( next_specified_value_,					"[e]" );

	AddInputGrammar( parent_classes_,						parent_class_ next_parent_class_ );
	AddInputGrammar( next_parent_class_,					"," parent_class_ next_parent_class_ );
	AddInputGrammar( next_parent_class_,					"[e]", ParentClassDeclarationEnd);
	AddInputGrammar( parent_class_,							class_access_ variable_type_, ParentClassDeclaration );

	AddInputGrammar( class_statements_,						class_statement_ class_statements_ );
	AddInputGrammar( class_statements_,						"[e]" );

	AddInputGrammar( class_statement_,						class_member_access_ );
	AddInputGrammar( class_statement_,						class_declaration_ ";" );
	AddInputGrammar( class_statement_,						enum_declaration_ ";" );
	AddInputGrammar( class_statement_,						type_definition_ ";", ClassStatement_TypeDefinition );
	AddInputGrammar( class_statement_,						using_ scope_name_ ";" );
	AddInputGrammar( class_statement_,						";" );
	AddInputGrammar( class_access_,							"private", ClassAccessPrivate );
	AddInputGrammar( class_access_,							"protected", ClassAccessProtected );
	AddInputGrammar( class_access_,							"public", ClassAccessPublic );
	AddInputGrammar( class_access_,							"[e]" );
	AddInputGrammar( class_member_access_,					"private :", Private );
	AddInputGrammar( class_member_access_,					"protected :", Protected );
	AddInputGrammar( class_member_access_,					"public :", Public);

	AddInputGrammar( class_statement_,						class_member_function_ );
	AddInputGrammar( class_statement_,						class_member_variable_ );
	AddInputGrammar( class_member_variable_,				variable_static_
															class_member_mutable_
															variable_type_
															variable_name_
															class_member_variable_use_bit_
															array_variable_
															class_variable_declaration_body_
															next_variable_ ";",
															ClassMemberVariableDeclaration );
	AddInputGrammar( class_member_variable_,				function_pointer_ );
	AddInputGrammar( class_member_function_,				class_templates_ function_static_ function_attributes_ class_member_virtual_ return_type_ function_name_ "(" function_parameters_ ")"
															class_member_const_ class_member_function_tail_
															function_body_,
															ClassMemberFunctionDeclaration );

	AddInputGrammar( array_variable_,						"[" array_size_ "]" array_variable_ );
	AddInputGrammar( array_variable_,						"[e]" );
	AddInputGrammar( array_size_,							right_expression_ );
	AddInputGrammar( array_size_,							"[e]" );
	AddInputGrammar( next_variable_,						"," word_ variable_declaration_body_ next_variable_ );
	AddInputGrammar( next_variable_,						"[e]" );
	AddInputGrammar( class_member_mutable_,					"mutable" );
	AddInputGrammar( class_member_mutable_,					"[e]" );
	AddInputGrammar( class_member_variable_use_bit_,		": number" );
	AddInputGrammar( class_member_variable_use_bit_,		"[e]" );

	AddInputGrammar( class_member_function_,				class_constructor_ );
	AddInputGrammar( class_member_function_,				class_destructor_ );
	AddInputGrammar( class_constructor_,					function_attributes_ word_ template_class_names_ "(" function_parameters_ ")" class_member_var_inits_ function_body_ );
	AddInputGrammar( template_class_names_,					"<" word_ next_template_class_name_ ">" );
	AddInputGrammar( template_class_names_,					"[e]" );
	AddInputGrammar( next_template_class_name_,				"," word_ next_template_class_name_ );
	AddInputGrammar( next_template_class_name_,				"[e]" );
	AddInputGrammar( variable_types_,						variable_type_ );
	AddInputGrammar( variable_types_,						variable_types_ "," variable_type_ );
	AddInputGrammar( class_member_var_inits_,				":" class_member_var_init_list_ );
	AddInputGrammar( class_member_var_inits_,				"[e]" );
	AddInputGrammar( class_member_var_init_list_,			class_member_var_init_ next_class_member_var_init_ );
	AddInputGrammar( next_class_member_var_init_,			"," class_member_var_init_ next_class_member_var_init_ );
	AddInputGrammar( next_class_member_var_init_,			"[e]" );
	AddInputGrammar( class_member_var_init_,				scope_name_ "(" function_call_parameters_ ")" );
	AddInputGrammar( class_destructor_,						class_member_virtual_ "~" word_ "(" function_parameters_ ")" function_body_, ClassDestructor );
	AddInputGrammar( return_type_,							variable_type_, ReturnType_VariableType );
	AddInputGrammar( return_type_,							"[e]", EmptyReturnType );
	AddInputGrammar( class_member_virtual_,					"virtual", ClassMemberVirtual );
	AddInputGrammar( class_member_virtual_,					"[e]" );
	AddInputGrammar( variable_name_,						scope_name_ );
	AddInputGrammar( function_name_,						word_, FunctionName_Word );
	AddInputGrammar( function_name_,						"operator" operator_type_, FunctionName_Operator );
	AddInputGrammar( operator_type_,						"=", OperatorType );
	AddInputGrammar( operator_type_,						"==", OperatorType );
	AddInputGrammar( operator_type_,						"!=", OperatorType );
	AddInputGrammar( operator_type_,						"+", OperatorType );
	AddInputGrammar( operator_type_,						"+=", OperatorType );
	AddInputGrammar( operator_type_,						"-", OperatorType );
	AddInputGrammar( operator_type_,						"-=", OperatorType );
	AddInputGrammar( operator_type_,						"*", OperatorType );
	AddInputGrammar( operator_type_,						"*=", OperatorType );
	AddInputGrammar( operator_type_,						"/", OperatorType );
	AddInputGrammar( operator_type_,						"/=", OperatorType );
	AddInputGrammar( operator_type_,						"<", OperatorType );
	AddInputGrammar( operator_type_,						"<<", OperatorType );
	AddInputGrammar( operator_type_,						"<=", OperatorType );
	AddInputGrammar( operator_type_,						">", OperatorType );
	AddInputGrammar( operator_type_,						">>", OperatorType );
	AddInputGrammar( operator_type_,						">=", OperatorType );
	AddInputGrammar( operator_type_,						"[ ]", OperatorType );
	AddInputGrammar( operator_type_,						"->", OperatorType );
	AddInputGrammar( operator_type_,						"( )", OperatorType );
	AddInputGrammar( operator_type_,						"new", OperatorType );
	AddInputGrammar( operator_type_,						"delete", OperatorType );
	AddInputGrammar( operator_type_,						variable_type_ );
	AddInputGrammar( class_member_const_,					"const" );
	AddInputGrammar( class_member_const_,					"[e]" );
	AddInputGrammar( class_member_function_tail_,			class_member_pure_virtual_tail_ );
	AddInputGrammar( class_member_function_tail_,			"override" );
	AddInputGrammar( class_member_function_tail_,			"[e]" );
	AddInputGrammar( class_member_pure_virtual_tail_,		"= number" );

	AddInputGrammar( class_statement_,						friend_statement_ );
	AddInputGrammar( friend_statement_,						"friend" function_declaration_ );
	AddInputGrammar( friend_statement_,						"friend" friend_class_ ";" );
	AddInputGrammar( friend_class_,							"class" variable_type_ );

	AddInputGrammar( function_declaration_,					class_templates_ function_static_ function_attributes_ return_type_
															scope_name_ "(" function_parameters_ ")"
															class_member_const_
															class_member_var_inits_
															function_body_,
															FunctionDeclaration );
	AddInputGrammar( variable_declaration_,					class_templates_ variable_static_ variable_type_ variable_name_ array_variable_ variable_declaration_body_ next_variable_, VariableDeclaration );
	AddInputGrammar( variable_declaration_,					class_templates_ variable_static_ variable_type_ variable_name_ "(" function_call_parameters_ ")" variable_declaration_body_ next_variable_, VariableDeclaration );
	AddInputGrammar( variable_declaration_,					function_pointer_ );

	AddInputGrammar( array_variable_,						"[" array_size_ "]" array_variable_ );
	AddInputGrammar( array_variable_,						"[e]" );
	AddInputGrammar( array_size_,							right_expression_ );
	AddInputGrammar( array_size_,							"[e]" );
	AddInputGrammar( next_variable_,						"," word_ variable_declaration_body_ next_variable_ );
	AddInputGrammar( next_variable_,						"[e]" );

	AddInputGrammar( function_attributes_,					function_attribute_ function_attributes_ );
	AddInputGrammar( function_attributes_,					"[e]" );
	AddInputGrammar( function_attribute_,					"extern" );
	AddInputGrammar( function_attribute_,					"inline" );
	AddInputGrammar( function_attribute_,					"explicit" );
	AddInputGrammar( function_static_,						"static", FunctionStatic );
	AddInputGrammar( function_static_,						"[e]" );
	AddInputGrammar( variable_static_,						"static", VariableStatic );
	AddInputGrammar( variable_static_,						"[e]" );
	AddInputGrammar( function_body_,						function_body_begin_ "{" statements_ "}" function_body_end_, FunctionBody );
	AddInputGrammar( function_body_,						function_body_begin_ ";" function_body_end_, EmptyFunctionBody );
	AddInputGrammar( function_body_begin_,					"[e]", FunctionBodyBegin );
	AddInputGrammar( function_body_end_,					"[e]", FunctionBodyEnd );
	AddInputGrammar( class_variable_declaration_body_,		"=" right_expression_ );
	AddInputGrammar( class_variable_declaration_body_,		"[ ] = {" word_ "}" );
	AddInputGrammar( class_variable_declaration_body_,		"[e]" );
	AddInputGrammar( variable_declaration_body_,			begin_variable_declaration_body_ "=" initial_value_ end_variable_declaration_body_ );
	AddInputGrammar( variable_declaration_body_,			"[ ] = {" word_ "}" );
	AddInputGrammar( variable_declaration_body_,			"[e]" );
	AddInputGrammar( begin_variable_declaration_body_,		"[e]", BeginVariableDeclarationBody );
	AddInputGrammar( end_variable_declaration_body_,		"[e]", EndVariableDeclarationBody );
	AddInputGrammar( initial_value_,						right_expression_ );
	AddInputGrammar( initial_value_,						"{" initial_value_ next_initial_value_ "}" );
	AddInputGrammar( next_initial_value_,					"," initial_value_ next_initial_value_ );
	AddInputGrammar( next_initial_value_,					"," );
	AddInputGrammar( next_initial_value_,					"[e]" );

	AddInputGrammar( statements_,							statement_ statements_ );
	AddInputGrammar( statements_,							"[e]" );
	AddInputGrammar( statement_,							statement_bodies_ ";" );
	AddInputGrammar( statement_,							"{" statements_ "}" );
	AddInputGrammar( statement_,							"if (" condition_statement_ ")" statement_ else_statement_ );
	AddInputGrammar( else_statement_,						"else" statement_ );
	AddInputGrammar( else_statement_,						"[e]" );
	AddInputGrammar( statement_,							"for (" statement_bodies_ ";" for_condition_statement_ ";" statement_bodies_ ")" statement_ );
	AddInputGrammar( statement_,							"while (" condition_statement_ ")" statement_ );
	AddInputGrammar( statement_,							"do {" statements_ "} while (" condition_statement_ ") ;" );
	AddInputGrammar( statement_,							"switch (" right_expression_ ") {" switch_body_ "}" );
	AddInputGrammar( statement_bodies_,						statement_body_ next_statement_body_ );
	AddInputGrammar( statement_bodies_,						"[e]" );
	AddInputGrammar( statement_body_,						"return" return_expression_ );
	AddInputGrammar( statement_body_,						right_expression_ );
	AddInputGrammar( statement_body_,						enum_declaration_ );
	AddInputGrammar( statement_body_,						type_definition_ );
	AddInputGrammar( statement_body_,						variable_declaration_ );
	AddInputGrammar( statement_body_,						"delete" array_ right_expression_ );
	AddInputGrammar( statement_body_,						"break" );
	AddInputGrammar( statement_body_,						"continue" );
	AddInputGrammar( statement_body_,						"throw" right_expression_ );
	AddInputGrammar( next_statement_body_,					", " statement_body_ next_statement_body_ );
	AddInputGrammar( next_statement_body_,					"[e]" );

	AddInputGrammar( for_condition_statement_,				condition_statement_ );
	AddInputGrammar( for_condition_statement_,				"[e]" );

	AddInputGrammar( switch_body_,							"case" right_expression_ ":" statements_ switch_body_ );
	AddInputGrammar( switch_body_,							"default :" statements_ switch_body_ );
	AddInputGrammar( switch_body_,							"[e]" );

	AddInputGrammar( return_expression_,					condition_statement_ );
	AddInputGrammar( return_expression_,					"[e]" );

	AddInputGrammar( array_,								"[ ]" );
	AddInputGrammar( array_,								"[e]" );

	// enumeration
	AddInputGrammar( enum_declaration_,						"enum" enum_name_ "{" enum_elements_ last_enum_elements_comma_ "}" );
	AddInputGrammar( enum_name_,							word_ );
	AddInputGrammar( enum_name_,							"[e]" );
	AddInputGrammar( enum_elements_,						enum_element_ next_enum_element_ );
	AddInputGrammar( enum_elements_,						"[e]" );
	AddInputGrammar( next_enum_element_,					"," enum_element_ next_enum_element_ );
	AddInputGrammar( next_enum_element_,					"[e]" );
	AddInputGrammar( enum_element_,							word_ enum_element_assign_ );
	AddInputGrammar( enum_element_assign_,					"=" right_expression_ );
	AddInputGrammar( enum_element_assign_,					"[e]" );
	AddInputGrammar( last_enum_elements_comma_,				"," );
	AddInputGrammar( last_enum_elements_comma_,				"[e]" );

	AddInputGrammar( condition_statement_,					right_expression_ );
	AddInputGrammar( condition_statement_,					variable_declaration_ );

	// right_expression
	AddInputGrammar( right_expression_,						right_expression1_ );

	AddInputGrammar( right_expression1_,					right_expression2_ next_right_expression1_ );
	AddInputGrammar( next_right_expression1_,				"[e]" );

	AddInputGrammar( right_expression2_,					right_expression3_ next_right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"+=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"-=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"*=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"/=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"&=" right_expression2_ );
	AddInputGrammar( next_right_expression2_,				"[e]" );

	AddInputGrammar( right_expression3_,					right_expression4_ next_right_expression3_ );
	AddInputGrammar( next_right_expression3_,				"?" right_expression_ ":" right_expression_ );
	AddInputGrammar( next_right_expression3_,				"[e]" );

	AddInputGrammar( right_expression4_,					right_expression5_ next_right_expression4_ );
	AddInputGrammar( next_right_expression4_,				"||" right_expression4_ );
	AddInputGrammar( next_right_expression4_,				"[e]" );

	AddInputGrammar( right_expression5_,					right_expression6_ next_right_expression5_ );
	AddInputGrammar( next_right_expression5_,				"&&" right_expression5_ );
	AddInputGrammar( next_right_expression5_,				"[e]" );

	AddInputGrammar( right_expression6_,					right_expression7_ next_right_expression6_ );
	AddInputGrammar( next_right_expression6_,				"|" right_expression6_ );
	AddInputGrammar( next_right_expression6_,				"[e]" );

	AddInputGrammar( right_expression7_,					right_expression8_ next_right_expression7_ );
	AddInputGrammar( next_right_expression7_,				"^" right_expression7_ );
	AddInputGrammar( next_right_expression7_,				"[e]" );

	AddInputGrammar( right_expression8_,					right_expression9_ next_right_expression8_ );
	AddInputGrammar( next_right_expression8_,				"&" right_expression8_ );
	AddInputGrammar( next_right_expression8_,				"[e]" );

	AddInputGrammar( right_expression9_,					right_expression10_ next_right_expression9_ );
	AddInputGrammar( next_right_expression9_,				"==" right_expression9_ );
	AddInputGrammar( next_right_expression9_,				"!=" right_expression9_ );
	AddInputGrammar( next_right_expression9_,				"[e]" );

	AddInputGrammar( right_expression10_,					right_expression11_ next_right_expression10_ );
	AddInputGrammar( next_right_expression10_,				"<" right_expression10_ );
	AddInputGrammar( next_right_expression10_,				"<=" right_expression10_ );
	AddInputGrammar( next_right_expression10_,				">" right_expression10_ );
	AddInputGrammar( next_right_expression10_,				">=" right_expression10_ );
	AddInputGrammar( next_right_expression10_,				"[e]" );

	AddInputGrammar( right_expression11_,					right_expression12_ next_right_expression11_ );
	AddInputGrammar( next_right_expression11_,				"<<" right_expression11_ );
	AddInputGrammar( next_right_expression11_,				">>" right_expression11_ );
	AddInputGrammar( next_right_expression11_,				"[e]" );

	AddInputGrammar( right_expression12_,					right_expression13_ next_right_expression12_ );
	AddInputGrammar( next_right_expression12_,				"+" right_expression12_ );
	AddInputGrammar( next_right_expression12_,				"-" right_expression12_ );
	AddInputGrammar( next_right_expression12_,				"[e]" );

	AddInputGrammar( right_expression13_,					right_expression14_ next_right_expression13_ );
	AddInputGrammar( next_right_expression13_,				"*" right_expression13_ );
	AddInputGrammar( next_right_expression13_,				"/" right_expression13_ );
	AddInputGrammar( next_right_expression13_,				"%" right_expression13_ );
	AddInputGrammar( next_right_expression13_,				"[e]" );

	AddInputGrammar( right_expression14_,					right_expression15_ );
	AddInputGrammar( right_expression14_,					"~" right_expression14_ );
	AddInputGrammar( right_expression14_,					"!" right_expression14_ );
	AddInputGrammar( right_expression14_,					"++" right_expression14_ );
	AddInputGrammar( right_expression14_,					"--" right_expression14_ );
	AddInputGrammar( right_expression14_,					"-" right_expression14_ );
	AddInputGrammar( right_expression14_,					"*" right_expression14_ );
	AddInputGrammar( right_expression14_,					"&" right_expression14_ );

	AddInputGrammar( right_expression15_,					right_expression16_ next_right_expression15_ );
	AddInputGrammar( next_right_expression15_,				"++" );
	AddInputGrammar( next_right_expression15_,				"--" );
	AddInputGrammar( next_right_expression15_,				"[e]" );

	AddInputGrammar( right_expression16_,					right_expression17_ next_right_expression16_ );
	AddInputGrammar( next_right_expression16_,				"[" right_expression_ "]" next_right_expression16_ );
	AddInputGrammar( next_right_expression16_,				"." right_expression16_ );
	AddInputGrammar( next_right_expression16_,				"->" right_expression16_ );
	AddInputGrammar( next_right_expression16_,				"[e]" );

	AddInputGrammar( right_expression17_,					"(" right_expression_ ")" );
	AddInputGrammar( right_expression17_,					right_expression18_ );

	AddInputGrammar( right_expression18_,					right_value_ );

	AddInputGrammar( left_value_,							scope_name_ target_call_ );
	AddInputGrammar( left_value_,							member_function_call_using_ptr_ );
	AddInputGrammar( left_value_,							casting_value_ );
	AddInputGrammar( right_value_,							"sizeof (" right_expression_ ")" );
	AddInputGrammar( right_value_,							"true" );
	AddInputGrammar( right_value_,							"false" );
	AddInputGrammar( right_value_,							"number" fraction_, RightValue_Number );
	AddInputGrammar( right_value_,							"lambda" );
	AddInputGrammar( "lambda",								"[ " function_call_parameters_ "] (" function_parameters_ ")" function_body_ );
	AddInputGrammar( fraction_,								". " fraction_number_ float_symbol_ exponent_ );
	AddInputGrammar( fraction_,								"[e]" );
	AddInputGrammar( fraction_number_,						"number" );
	AddInputGrammar( fraction_number_,						"[e]" );
	AddInputGrammar( float_symbol_,							"f" );
	AddInputGrammar( float_symbol_,							"[e]" );
	AddInputGrammar( exponent_,								"e + number" exponent_float_symbol_ );
	AddInputGrammar( exponent_,								"[e]" );
	AddInputGrammar( exponent_float_symbol_,				"F" );
	AddInputGrammar( exponent_float_symbol_,				"[e]" );

	AddInputGrammar( right_value_,							"number . number f" );

	AddInputGrammar( right_value_,							left_value_ );
	AddInputGrammar( right_value_,							"new" variable_type_ constructor_call_ );

	AddInputGrammar( constructor_call_,						"(" function_call_parameters_ ")", TargetCall );
	AddInputGrammar( constructor_call_,						"[e]" );

	AddInputGrammar( member_function_call_using_ptr_,		"(" right_expression_ ". *" right_expression_ ")" target_call_ );
	AddInputGrammar( member_function_call_using_ptr_,		"(" right_expression_ "-> *" right_expression_ ")" target_call_ );

	AddInputGrammar( casting_value_,						"static_cast <" variable_type_ "> (" right_expression_ ")" );
	AddInputGrammar( casting_value_,						"const_cast <" variable_type_ "> (" right_expression_ ")" );
	AddInputGrammar( casting_value_,						"(" variable_type_ ")" right_expression_ );
	AddInputGrammar( casting_value_,						"(" return_type_ "(" typedef_scope_type_ "* ) (" function_parameters_ ")" class_member_const_ ")" right_expression_ );

	AddInputGrammar( target_call_,							"(" function_call_parameters_ ")", TargetCall );
	AddInputGrammar( target_call_,							"[e]" );

	AddInputGrammar( variable_type_,						variable_attributes_ variable_typename_ scope_type_ variable_pointers_ variable_pointer_const_ variable_reference_mark_, VariableType );
	AddInputGrammar( variable_pointer_type_,				variable_basic_type_ variable_pointers_ variable_pointer_const_ );
	AddInputGrammar( variable_pointers_,					"*" variable_pointers_, VariablePointers_Pointer );
	AddInputGrammar( variable_pointers_,					"[e]" );
	AddInputGrammar( variable_pointer_const_,				"const" );
	AddInputGrammar( variable_pointer_const_,				"[e]" );

	AddInputGrammar( variable_reference_mark_,				"&", VariableReferenceMark_And);
	AddInputGrammar( variable_reference_mark_,				"[e]" );

	AddInputGrammar( variable_attributes_,					variable_attributes_begin_ variable_attribute_ next_variable_attributes_ );
	AddInputGrammar( variable_attributes_,					"[e]",			VariableAttributesBegin);
	AddInputGrammar( variable_attributes_begin_,			"[e]",			VariableAttributesBegin );
	AddInputGrammar( variable_attribute_,					"const",		VariableAttribute_Const );
	AddInputGrammar( next_variable_attributes_,				variable_attribute_ next_variable_attributes_ );
	AddInputGrammar( next_variable_attributes_,				"[e]" );

	AddInputGrammar( variable_typename_,					"typename" );
	AddInputGrammar( variable_typename_,					"struct" );
	AddInputGrammar( variable_typename_,					"class" );
	AddInputGrammar( variable_typename_,					"[e]" );

	AddInputGrammar( variable_basic_type_,					"bool",							VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"char",							VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"short",						VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"short int",					VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"int",							VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"__int64",						VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"long",							VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"long long",					VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"float",						VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"double",						VariableBasicType );
	AddInputGrammar( variable_basic_type_,					word_ template_specification_,	VariableBasicType_Word );
	AddInputGrammar( variable_basic_type_,					"f" template_specification_,	VariableBasicType_Word );
	AddInputGrammar( variable_basic_type_,					"void",							VariableBasicType );
	AddInputGrammar( variable_basic_type_,					"unsigned " unsignable_type_,	UnsignableType );
	AddInputGrammar( unsignable_type_,						"char",							VariableBasicType );
	AddInputGrammar( unsignable_type_,						"long",							VariableBasicType );
	AddInputGrammar( unsignable_type_,						"long long",					VariableBasicType );
	AddInputGrammar( unsignable_type_,						"short",						VariableBasicType );
	AddInputGrammar( unsignable_type_,						"short int",					VariableBasicType );
	AddInputGrammar( unsignable_type_,						"int",							VariableBasicType );
	AddInputGrammar( unsignable_type_,						"__int64",						VariableBasicType );
	AddInputGrammar( unsignable_type_,						"[e]",							VariableBasicType_Empty );

	AddInputGrammar( scope_type_,							type_ next_scope_type_ );
	AddInputGrammar( next_scope_type_,						"::" scope_type_ next_scope_type_, NextScopeName );
	AddInputGrammar( next_scope_type_,						"[e]" );

	AddInputGrammar( type_,									variable_basic_type_ );

	AddInputGrammar( scope_name_,							root_scope_ name_ next_scope_name_ );
	AddInputGrammar( root_scope_,							"::" );
	AddInputGrammar( root_scope_,							"[e]" );
	AddInputGrammar( next_scope_name_,						"::" name_ next_scope_name_, NextScopeName );
	AddInputGrammar( next_scope_name_,						"[e]" );

	AddInputGrammar( name_,									destructor_mark_ word_ template_specification_, Name );
	AddInputGrammar( name_,									"operator" operator_type_, Name_Operator );
	AddInputGrammar( name_,									"e" );
	AddInputGrammar( name_,									"f" );

	AddInputGrammar( destructor_mark_,						"~" );
	AddInputGrammar( destructor_mark_,						"[e]" );

	AddInputGrammar( function_parameters_,					function_parameters_begin_ function_parameter_ next_function_parameter_ function_parameters_end_ );
	AddInputGrammar( function_parameters_,					"[e]", EmptyFunctionParameters );
	AddInputGrammar( function_parameters_begin_,			"[e]", FunctionParametersBegin );
	AddInputGrammar( function_parameters_end_,				"[e]", FunctionParametersEnd );
	AddInputGrammar( next_function_parameter_,				"," function_parameter_ next_function_parameter_ );
	AddInputGrammar( next_function_parameter_,				"[e]" );
	AddInputGrammar( function_parameter_,					variable_type_ function_parameter_name_ function_parameter_array_ function_parameter_assign_ function_parameter_flexible_, FunctionParameterDeclaration );
	AddInputGrammar( function_parameter_,					function_pointer_, FunctionParameter_FunctionPointer );
	AddInputGrammar( function_parameter_,					"..." );
	AddInputGrammar( function_parameter_name_,				word_, FunctionParameterName_Word );
	AddInputGrammar( function_parameter_name_,				"e" );
	AddInputGrammar( function_parameter_name_,				"f" );
	AddInputGrammar( function_parameter_name_,				"[e]" );
	AddInputGrammar( function_parameter_assign_,			function_parameter_assign_begin_ "=" right_expression_ function_parameter_assign_end_ );
	AddInputGrammar( function_parameter_assign_,			"[e]" );
	AddInputGrammar( function_parameter_assign_begin_,		"[e]", FunctionParameterAssignBegin );
	AddInputGrammar( function_parameter_assign_end_,		"[e]", FunctionParameterAssignEnd );
	AddInputGrammar( function_parameter_flexible_,			"..." );
	AddInputGrammar( function_parameter_flexible_,			"[e]" );
	AddInputGrammar( function_parameter_array_,				array_variable_ );
	AddInputGrammar( function_parameter_array_,				array_ );

	AddInputGrammar( function_call_parameters_,				function_call_parameters_begin_ function_call_parameter_ next_function_call_parameter_ );
	AddInputGrammar( function_call_parameters_,				"[e]", EmptyFunctionCallParameter );
	AddInputGrammar( function_call_parameters_begin_,		"[e]", FunctionCallParametersBegin );
	AddInputGrammar( next_function_call_parameter_,			"," function_call_parameter_ next_function_call_parameter_, NextFunctionCallParameter );
	AddInputGrammar( next_function_call_parameter_,			"[e]" );
	AddInputGrammar( function_call_parameter_,				right_expression_ );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	CPP파일을 파싱한다.
///
/// @param	filePath			파일 경로
/// @param	includeDirectories	파일 포함 경로 목록
/// @param	definitions			전처리기 정의 목록
/// @param	parsingResult		파싱 결과
/// @param	bDebug              디버그 모드 여부
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
HyBool HyCppParser::Parse(
   const HyString&             filePath,
   const IncludeDirectoryList& includeDirectories,
   const DefinitionMap&        definitions,
         ParsingResult&        parsingResult,
         HyBool                forDebug )
{
	g_strClassMemberAccess = "private";

	g_parsingResult = &parsingResult;

	g_listNamespace.clear();
	g_listNamespace.push_back( &parsingResult.m_globalNamespace );

	/*
	HyCppPreprocessor cppPreprocessor;
	cppPreprocessor.AddIncludeDirectory( includeDirectories );
	cppPreprocessor.AddDefinition( definitions );
	FILE* targetFile = fopen( "_temp_.cpp", "w" );
	if ( !cppPreprocessor.Parse( filePath.c_str(), targetFile, true ) )
	{
		LOG( HyLog::Error, "cppPreprocessor.Parse() failed" );
		fclose( targetFile );
		return false;
	}
	fclose( targetFile );
	//*/
	char szBuf[ 1024 * 100 ];
	ParserData parserData;

	SetDebugging( forDebug );
	if ( !ParseFile( "_temp_.cpp", parserData, szBuf ) )
	{
		printf( "failed.\n" );
		return false;
	}

	//DeleteFile("_temp_.cpp");
	return true;
}
