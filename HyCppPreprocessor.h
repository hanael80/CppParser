#pragma once


#include "../GrammarParser/HyGrammarParser.h"


struct Definition
{
	std::string					m_strValue;
	std::vector< std::string >	m_params;
};

struct ParsingData
{
	std::set<std::string>				m_setIncludeDirectory;
	std::map<std::string, Definition>	m_definitions;
	std::list<int>						m_listPreprocessorValue;
};

class HyCppPreprocessor
	:
	public HyGrammarParser
{
public:
	HyCppPreprocessor();

public:
	bool Parse(const std::string& strFileName, FILE* const pTargetFile, const bool bUseInclude = true);
	using HyGrammarParser::Parse;

	/// 파일 포함 기본 경로를 추가한다.
	HyVoid AddIncludeDirectory( const std::list< HyString >& includeDirectories );

	/// 전처리기 정의를 추가한다.
	HyVoid AddDefinition( const std::map< HyString, HyString >& definitions );

private:
	HyVoid OnInit();

	HyVoid Parse_endif();
	HyBool Parse_include(char* const pszLineBuf, FILE* const pTargetFile, const bool bUseInclude);
	HyBool _Parse_if( char* const pszLineBuf, const int iLineBufSize, FILE* const pSourceFile, bool& bFileRead );
	HyBool Parse_define(char* const pszLineBuf, const int iLineBufSize, FILE* const pSourceFile);
	HyBool Parse_undef(char* const pszLineBuf);
	HyBool Parse_else(char* const pszLineBuf, const int iLineBufSize, FILE* const pSourceFile, bool& bFileRead);
	HyBool _Parse_elif( HyChar* lineBuf, HyUInt32 lineBufSize, FILE* sourceFile, HyBool& fileRead );
	HyBool Parse_pragma(char* const pszLineBuf, const std::string& strFileName);

private:
	void*	m_pParam;
};
