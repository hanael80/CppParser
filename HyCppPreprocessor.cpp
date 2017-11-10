#include "PCH.h"
#include "HyCppPreprocessor.h"


ParsingData g_parsingData;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	생성자
////////////////////////////////////////////////////////////////////////////////////////////////////
HyCppPreprocessor::HyCppPreprocessor()
:
HyGrammarParser( "CppPreprocessor" )
{
	Init();

	m_pParam = &g_parsingData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	알파벳인지 여부를 반환한다.
///
/// @param	c	문자
///
/// @return	알파벳인지 여부
////////////////////////////////////////////////////////////////////////////////////////////////////
static HyBool _IsAlpha( HyChar c )
{
	return
		( c >= 'A' && c <= 'Z' ) ||
		( c >= 'a' && c <= 'z' );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	숫자인지 여부를 반환한다.
///
/// @param	c	문자
///
/// @return	숫자인지 여부
////////////////////////////////////////////////////////////////////////////////////////////////////
static HyBool _IsNumber( HyChar c )
{
	return ( c >= '0' && c <= '9' );
}

static HyVoid _WriteString( const HyChar* const buf, FILE* const targetFile );


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	정의 정보를 기록한다.
///
/// @param	definition	정의 정보
/// @param	params		파라미터 목록
/// @param	targetFile	기록할 파일
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
static HyVoid _WriteDefinition( const Definition& definition, const std::vector< HyString >& params, FILE* targetFile )
{
	HyBool makeToString = false;
	const HyChar* pszWordStart = NULL;
	for ( const HyChar* p = definition.m_strValue.c_str(); *p != 0; ++p )
	{
		if ( _IsAlpha( *p ) || (pszWordStart && ( _IsNumber( *p ) || *p == '_' ) ) )
		{
			if ( !pszWordStart )
				pszWordStart = p;
			continue;
		}

		if ( !pszWordStart )
		{
			if ( *p == '#' )
				makeToString = true;
			else
				fputc( *p, targetFile );
			continue;
		}

		const HyInt32 length = (HyInt32)( p - pszWordStart );
		--p;

		HyChar word[ 256 ];
		strncpy( word, pszWordStart, length );
		word[ length ] = 0;

		pszWordStart = NULL;

		HyUInt32 i = 0;
		for ( ; i < definition.m_params.size(); ++ i )
		{
			if ( definition.m_params[ i ] == word )
			{
				if ( !makeToString )
				{
					_WriteString( params[ i ].c_str(), targetFile );
				}
				else
				{
					fprintf( targetFile, "\"%s\"", params[ i ].c_str() );
					makeToString = false;
				}
				break;
			}
		}

		if ( i >= definition.m_params.size() )
		{
			if ( !makeToString )
			{
				_WriteString( word, targetFile );
			}
			else
			{
				fprintf( targetFile, "\"%s\"", word);
				makeToString = false;
			}
		}
	}

	if ( pszWordStart )
	{
		HyUInt32 i = 0;
		for ( ; i < definition.m_params.size(); ++ i )
		{
			if ( definition.m_params[ i ] == pszWordStart )
			{
				if ( !makeToString )
				{
					_WriteString( params[ i ].c_str(), targetFile );
				}
				else
				{
					fprintf( targetFile, "\"%s\"", params[ i ].c_str() );
					makeToString = false;
				}
				break;
			}
		}

		if ( i >= definition.m_params.size() )
		{
			if ( !makeToString )
			{
				_WriteString( pszWordStart, targetFile );
			}
			else
			{
				fprintf( targetFile, "\"%s\"", pszWordStart );
				makeToString = false;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	단어를 전처리 한다.
///
/// @param	word		단어
/// @param	wordNext	단어의 다음 문자열 포인터
/// @param	targetFile	기록할 파일
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
HyVoid WriteWord( const HyChar* word, const HyChar*& wordNext, FILE* targetFile )
{
	const std::map< std::string, Definition >::const_iterator iter = g_parsingData.m_definitions.find( word );
	if ( iter == g_parsingData.m_definitions.end() )
	{
		fprintf( targetFile, word );
		return;
	}

	const Definition& definition = iter->second;
	if ( definition.m_params.empty() )
	{
		_WriteString( definition.m_strValue.c_str(), targetFile );
		return;
	}

	if ( !wordNext )
		__debugbreak();

	const char*& p = wordNext;
	do
	{
		++p;

	} while( *p == '(' || *p == ' ' || *p == '\t' );

	std::vector< std::string > params;
	for ( HyUInt32 paramIdx = 0; paramIdx < definition.m_params.size(); ++paramIdx )
	{
		const HyChar* paramStart = p;
		HyInt32 indent = 0;
		while ( true )
		{
			if ( *p == '(' )
			{
				++indent;
			}
			else if ( *p == ')' )
			{
				--indent;
				if ( indent < 0 )
				{
					break;
				}
			}
			else if ( *p == ',' && indent == 0 )
			{
				break;
			}

			++p;
		}

		const HyChar* paramEnd = p;

		while ( *paramStart == ' ' || *paramStart == '\t' )
			++paramStart;

		while ( paramEnd[ -1 ] == ' ' || paramEnd[ -1 ] == '\t' )
			--paramEnd;

		HyChar param[ 256 ];
		strncpy_s( param, paramStart, paramEnd - paramStart );
		param[ paramEnd - paramStart ] = 0;
		params.push_back( param );

		if ( paramIdx < definition.m_params.size() - 1 )
		{
			do
			{
				++p;
			} while ( *p == ' ' || *p == '\t' );
		}
	}

	_WriteDefinition( definition, params, targetFile );
}

void _WriteString( const char* const pszBuf, FILE* const pTargetFile )
{
	const char* pszWordStart = NULL;
	for(const char* p = pszBuf; *p != 0; ++p)
	{
		if( _IsAlpha( *p ) || ( pszWordStart && ( _IsNumber( *p ) || *p == '_') ) )
		{
			if(pszWordStart == NULL)
				pszWordStart = p;
			continue;
		}

		if(pszWordStart == NULL)
		{
			fputc(*p, pTargetFile);
			continue;
		}

		const int iLength = (int)(p - pszWordStart);
		--p;

		char szWord[256];
		strncpy(szWord, pszWordStart, iLength);
		szWord[iLength] = 0;

		pszWordStart = NULL;

		WriteWord( szWord, p, pTargetFile );
	}

	if(pszWordStart != NULL)
	{
		const char* p = NULL;
		WriteWord( pszWordStart, p, pTargetFile );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	정의 정보를 이용해 문자열을 변환한다.
///
/// @param	definition	정의 정보
/// @param	params		정의 정보에 대한 실제 파라미터 목록
/// @param	dstString	변환 후 저장할 문자열 버퍼
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
static HyVoid _ConvertStringUsingDefinition( const Definition& definition, const std::vector< std::string >& params, HyChar*& dstString )
{
	HyBool makeToString = false;

	const HyChar* wordStart = NULL;
	HyBool inDoubleQuotes = false;

	for ( const HyChar* p = definition.m_strValue.c_str(); *p; ++p )
	{
		// 쌍 따옴표 안이면 끝이 나올때까지 그냥 복사한다.
		if ( inDoubleQuotes )
		{
			if ( p[ 0 ] == '\\' && p[ 1 ] == '\"' )
			{
				*dstString++ = *p++;
				*dstString++ = *p;
				continue;
			}

			if ( *p == '\"' )
				inDoubleQuotes = false;

			*dstString++ = *p;
			continue;
		}

		if ( _IsAlpha( *p ) || (wordStart && ( _IsNumber( *p ) || *p == '_' ) ) )
		{
			if ( !wordStart )
				wordStart = p;
			continue;
		}

		if ( !wordStart )
		{
			if ( *p == '#' )
			{
				if ( p[ 1 ] == '#' )
				{
					++p;

					while ( dstString[ -1 ] == ' ' || dstString[ -1 ] == '\t' )
						--dstString;

					while ( p[ 1 ] == ' ' || p[ 1 ] == '\t' )
						++p;
				}
				else
				{
					makeToString = true;
				}
			}
			else
			{
				if ( *p == '\"' )
					inDoubleQuotes = !inDoubleQuotes;

				*dstString++ = *p;
			}
			continue;
		}

		const HyInt32 length = (HyInt32)( p - wordStart );
		--p;

		HyChar word[256];
		strncpy( word, wordStart, length );
		word[ length ] = 0;

		wordStart = NULL;

		HyUInt32 i = 0;
		for ( ; i < definition.m_params.size(); ++ i )
		{
			if ( definition.m_params[ i ] == word )
			{
				if ( !makeToString )
				{
					strcpy( dstString, params[ i ].c_str() );
					dstString += params[ i ].length();
				}
				else
				{
					*dstString++ = '\"';
					for ( HyChar c : params[ i ] )
					{
						if ( c == '\"' )
							*dstString++ = '\\';

						*dstString++ = c;
					}
					*dstString++ = '\"';

					makeToString = false;
				}
				break;
			}
		}

		if ( i >= definition.m_params.size() )
		{
			if ( !makeToString )
			{
				strcpy( dstString, word );
				dstString += strlen( word );
			}
			else
			{
				dstString += sprintf( dstString, "\"%s\"", word );
				makeToString = false;
			}
		}
	}

	if ( wordStart )
	{
		HyUInt32 i = 0;
		for ( ; i < definition.m_params.size(); ++ i )
		{
			if ( definition.m_params[ i ] == wordStart )
			{
				if ( !makeToString )
				{
					strcpy( dstString, params[ i ].c_str() );
					dstString += params[ i ].length();
				}
				else
				{
					dstString += sprintf( dstString, "\"%s\"", params[ i ].c_str() );
					makeToString = false;
				}
				break;
			}
		}

		if ( i >= definition.m_params.size() )
		{
			if ( !makeToString )
			{
				strcpy( dstString, wordStart );
				dstString += strlen( wordStart );
			}
			else
			{
				dstString += sprintf( dstString, "\"%s\"", wordStart );
				makeToString = false;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	단어를 전처리해서 변경한다.
///
/// @param	word		단어
/// @param	wordNext	단어의 다음 문자열 포인터
/// @param	dstString	대상 문자열
/// @param	lineNo		라인 번호
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
HyBool _ConvertWord( const HyChar* word, const HyChar*& wordNext, HyChar*& dstString, HyUInt32 lineNo )
{
	std::map< std::string, Definition >::const_iterator iter = g_parsingData.m_definitions.find( word );
	if ( iter == g_parsingData.m_definitions.end() )
	{
		strcpy( dstString, word );
		dstString += strlen( word );
		return false;
	}

	const Definition& definition = iter->second;
	if ( definition.m_params.empty() )
	{
		strcpy( dstString, definition.m_strValue.c_str() );
		dstString += definition.m_strValue.length();
		return true;
	}

	if ( !wordNext )
		__debugbreak();

	const HyChar*& p = wordNext;
	do
	{
		++p;

	} while( *p == '(' || *p == ' ' || *p == '\t' );

	std::vector< std::string > params;
	for ( HyUInt32 paramIdx = 0; paramIdx < definition.m_params.size(); ++paramIdx )
	{
		const HyChar* paramStart = p;
		HyInt32 indent = 0;
		while ( true )
		{
			if ( *p == '(' )
			{
				++indent;
			}
			else if ( *p == ')' )
			{
				--indent;
				if ( indent < 0 )
				{
					break;
				}
			}
			else if ( *p == ',' && indent == 0 )
			{
				break;
			}

			++p;
		}

		const HyChar* paramEnd = p;

		while ( *paramStart == ' ' || *paramStart == ' ')
			++paramStart;

		while ( paramEnd[ -1 ] == ' ' || paramEnd[ -1 ] == ' ')
			--paramEnd;

		HyChar param[ 256 ];
		strncpy_s( param, paramStart, paramEnd - paramStart );
		param[ paramEnd - paramStart ] = 0;

		if ( !strcmp( param, "__LINE__" ) )
			sprintf( param, "%d", lineNo );

		params.push_back( param );

		if ( paramIdx < definition.m_params.size() - 1 )
		{
			do
			{
				++p;
			} while ( *p == ' ' || *p == '\t' );
		}
	}

	_ConvertStringUsingDefinition( definition, params, dstString );
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	문자열을 전처리를 하여 변환한다.
///
/// @param	srcString	원본 문자열
/// @param	dstString	대상 문자열
/// @param	lineNo		라인 번호
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
static HyBool _ConvertString( const HyChar* srcString, HyChar* dstString, HyUInt32 lineNo )
{
	HyBool converted = false;
	const HyChar* wordStart = NULL;

	for ( const HyChar* p = srcString; *p; ++p )
	{
		if ( _IsAlpha( *p ) || *p == '_' || ( wordStart && _IsNumber( *p ) ) )
		{
			if ( !wordStart )
				wordStart = p;
			continue;
		}

		if ( !wordStart )
		{
			*dstString++ = *p;
			continue;
		}

		const HyInt32 length = (HyInt32)(p - wordStart);
		--p;

		HyChar word[ 256 ];
		strncpy( word, wordStart, length );
		word[ length ] = 0;

		if ( !strcmp( word, "__LINE__" ) )
			sprintf( word, "%d", lineNo );

		wordStart = NULL;

		converted |= _ConvertWord( word, p, dstString, lineNo );
	}

	if ( wordStart )
	{
		const HyChar* p = NULL;
		converted |= _ConvertWord( wordStart, p, dstString, lineNo );
	}

	*dstString = 0;
	return converted;
}

bool HyCppPreprocessor::Parse(const std::string& strFileName, FILE* const pTargetFile, const bool bUseInclude)
{
	if(strstr(strFileName.c_str(), "/") != NULL)
	{
		char szBuf[512];
		strcpy(szBuf, strFileName.c_str());
		for(char* p = szBuf + strlen(szBuf) - 1; p >= szBuf; --p)
			if(*p == '/')
			{
				*p = 0;
				break;
			}

		g_parsingData.m_setIncludeDirectory.insert(szBuf);
	}

	const std::size_t stackSize = g_parsingData.m_listPreprocessorValue.size();

	FILE* pFile = fopen(strFileName.c_str(), "r");
	if ( !pFile )
	{
		LOG( HyLog::Error, "pFile == NULL. strFileName(%s)", strFileName.c_str() );
		return false;
	}

	char szBuf[ 1024 * 10 ];
	while(fgets(szBuf, sizeof(szBuf), pFile))
	{
		if(strstr(szBuf, "@interface") != NULL)
		{
			fclose(pFile);
			return true;
		}
	}

	rewind(pFile);

	bool bIgnore = false;
	int iIgnoreDefine = 0;
	bool fileRead = true;
	int lineNo = 0;

	while(true)
	{
		if(fileRead)
		{
			if(fgets(szBuf, sizeof(szBuf), pFile) == false)
				break;
			++lineNo;
		}
		else
			fileRead = true;

		if(bIgnore)
		{
			for(char* p = szBuf; *p; ++p)
			{
				if(strncmp(p, "*/", 2) == 0)
				{
					memmove(szBuf, p + 2, strlen(p + 2) + 1);
					bIgnore = false;
				}
			}

			if(bIgnore)
				continue;
		}

// 		for ( char* p = szBuf; *p; ++p )
// 		{
// 			if ( strncmp( p, "//", 2 ) == 0 )
// 			{
// 				if ( p == szBuf )
// 					*p = 0;
// 				else
// 					strcpy( p, "\n" );
// 				break;
// 			}
// 
// 			if ( strncmp( p, "/*", 2 ) == 0 )
// 			{
// 				*p = 0;
// 				bIgnore = true;
// 
// 				for ( char* p2 = p + 1; *p2; ++p2 )
// 				{
// 					if ( strncmp( p2, "*/", 2 ) == 0 )
// 					{
// 						memmove( p, p2 + 2, strlen( p2 + 2 ) + 1 );
// 						bIgnore = false;
// 					}
// 				}
// 				break;
// 			}
// 		}

		if ( strstr( szBuf, "#endif" ) )
		{
			Parse_endif();
			continue;
		}
		else if ( strstr( szBuf, "#include" ) )
		{
			fprintf( pTargetFile, "//##// %s#%d\n", strFileName.c_str(), lineNo );

			if ( Parse_include( szBuf, pTargetFile, bUseInclude ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#if" ) != NULL )
		{
			if ( _Parse_if( szBuf, sizeof( szBuf ), pFile, fileRead ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#define" ) )
		{
			if ( Parse_define( szBuf, sizeof( szBuf ), pFile ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#undef" ) )
		{
			if ( Parse_undef( szBuf ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#else" ) )
		{
			if ( Parse_else( szBuf, sizeof( szBuf ), pFile, fileRead ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#elif" ) )
		{
			if ( _Parse_elif( szBuf, sizeof( szBuf ), pFile, fileRead ) == false )
				return false;
			continue;
		}
		else if ( strstr( szBuf, "#pragma" ) )
		{
			if ( Parse_pragma( szBuf, strFileName ) == false )
				break;
			continue;
		}

		char szTemp[1024];
		strcpy_s(szTemp, sizeof(szTemp), szBuf);
		if(strtok(szTemp, " \t\r\n") != NULL)
			fprintf(pTargetFile, "//##// %s#%d\n", strFileName.c_str(), lineNo);

		HyChar dstString[ 1024 * 10 ];

		while ( _ConvertString( szBuf, dstString, lineNo ) )
		{
			strcpy( szBuf, dstString );
		}
		fprintf( pTargetFile, "%s", dstString );
		//WriteString( szBuf, pTargetFile );
	}
	fclose(pFile);

	if(stackSize != g_parsingData.m_listPreprocessorValue.size())
		LOG(HyLog::Fatal, "iStackSize != g_parsingData.m_listPreprocessorValue.size()");

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	파일 포함 기본 경로를 추가한다.
///
/// @param	includeDirectories	파일 포함 기본 경로 목록
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
HyVoid HyCppPreprocessor::AddIncludeDirectory( const std::list< HyString >& includeDirectories )
{
	for ( const HyString& includeDirectory : includeDirectories )
		g_parsingData.m_setIncludeDirectory.insert( includeDirectory );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	전처리기 정의를 추가한다.
///
/// @param	definitions	전처리기 정의 목록
///
/// @return	반환 값 없음
////////////////////////////////////////////////////////////////////////////////////////////////////
HyVoid HyCppPreprocessor::AddDefinition( const std::map< HyString, HyString >& definitions )
{
	for ( auto& pair : definitions )
	{
		Definition definition;
		definition.m_strValue = pair.second;
		g_parsingData.m_definitions[ pair.first ] = definition;
	}
}

void Statement_IfDefWord( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const std::string& strDefine = vecToken[ 0 ]->m_value;
	if ( g_parsingData.m_definitions.find( strDefine ) != g_parsingData.m_definitions.end() )
	{
		g_parsingData.m_listPreprocessorValue.push_back( true );
	}
	else
	{
		g_parsingData.m_listPreprocessorValue.push_back( false );
	}
}

void Statement_IfNDefWord( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const std::string& strDefine = vecToken[ 0 ]->m_value;
	if ( g_parsingData.m_definitions.find( strDefine ) != g_parsingData.m_definitions.end() )
	{
		g_parsingData.m_listPreprocessorValue.push_back( false );
	}
	else
	{
		g_parsingData.m_listPreprocessorValue.push_back( true );
	}
}

void NextExpression1_OrExpression1( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const int value1 = g_parsingData.m_listPreprocessorValue.back();
	g_parsingData.m_listPreprocessorValue.pop_back();
	const int value2 = g_parsingData.m_listPreprocessorValue.back();

	g_parsingData.m_listPreprocessorValue.back() = ( value1 || value2 );
}

void NextExpression2_AndExpression2( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	const int value1 = g_parsingData.m_listPreprocessorValue.back();
	g_parsingData.m_listPreprocessorValue.pop_back();
	const int value2 = g_parsingData.m_listPreprocessorValue.back();

	g_parsingData.m_listPreprocessorValue.back() = ( value1 && value2 );
}

void Expression3_NotExpression3( const std::vector< const HyToken* >& vecToken, void* const pParam )
{
	g_parsingData.m_listPreprocessorValue.back() = !g_parsingData.m_listPreprocessorValue.back();
}

HyVoid Expression4_DefinedWord( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	const HyString& strDefine = tokens[ 1 ]->m_value;
	if ( g_parsingData.m_definitions.find( strDefine ) != g_parsingData.m_definitions.end() )
	{
		g_parsingData.m_listPreprocessorValue.push_back( true );
	}
	else
	{
		g_parsingData.m_listPreprocessorValue.push_back( false );
	}
}

HyVoid Expression4_Word( const std::vector< const HyToken* >& tokens, HyVoid* const param )
{
	const HyString& strDefine = tokens[ 0 ]->m_value;
	auto iter = g_parsingData.m_definitions.find( strDefine );
	if ( iter == g_parsingData.m_definitions.end() )
		return g_parsingData.m_listPreprocessorValue.push_back( false );

	const Definition& definition = iter->second;
	if ( atoi( definition.m_strValue.c_str() ) )
		g_parsingData.m_listPreprocessorValue.push_back( true );
	else
		g_parsingData.m_listPreprocessorValue.push_back( false );
}

void HyCppPreprocessor::OnInit()
{
	AddInputGrammar( "source",				"statements" );
	AddInputGrammar( "statements",			"statement statements" );
	AddInputGrammar( "statements",			"[e]" );
	AddInputGrammar( "statement",			"ifdef word", Statement_IfDefWord );
	AddInputGrammar( "statement",			"ifndef word", Statement_IfNDefWord );
	AddInputGrammar( "statement",			"if expression" );
	AddInputGrammar( "statement",			"elif expression" );
	AddInputGrammar( "expression",			"expression1" );
	AddInputGrammar( "expression1",			"expression2 next_expression1" );
	AddInputGrammar( "next_expression1",	"|| expression1", NextExpression1_OrExpression1 );
	AddInputGrammar( "next_expression1",	"[e]" );
	AddInputGrammar( "expression2",			"expression3 next_expression2" );
	AddInputGrammar( "next_expression2",	"&& expression2", NextExpression2_AndExpression2 );
	AddInputGrammar( "next_expression2",	"[e]" );
	AddInputGrammar( "expression3",			"expression4" );
	AddInputGrammar( "expression3",			"! expression3", Expression3_NotExpression3 );
	AddInputGrammar( "expression4",			"defined word", Expression4_DefinedWord );
	AddInputGrammar( "expression4",			"defined ( word )", Expression4_DefinedWord );
	AddInputGrammar( "expression4",			"word", Expression4_Word );
}

void HyCppPreprocessor::Parse_endif()
{
	if(g_parsingData.m_listPreprocessorValue.empty())
		LOG(HyLog::Fatal, "g_parsingData.m_listPreprocessorValue.empty()");

	g_parsingData.m_listPreprocessorValue.pop_back();
}


bool HyCppPreprocessor::Parse_include(char* const pszLineBuf, FILE* const pTargetFile, const bool bUseInclude)
{
	if(bUseInclude == false)
		return true;

	char* p = strstr(pszLineBuf, "<");
	char* p2;
	if(p != NULL)
	{
		++p;
		p2 = strstr(p, ">");
	}
	else
	{
		p = strstr(pszLineBuf, "\"");
		if(p != NULL)
		{
			++p;
			p2 = strstr(p, "\"");
		}
	}

	if(p != NULL && p2 != NULL)
	{
		char szHeaderFileName[512] = { 0, };
		strncpy(szHeaderFileName, p, p2 - p);

		for ( const HyString& directory : g_parsingData.m_setIncludeDirectory )
		{
			std::string strFileName = directory + "/" + szHeaderFileName;
			FILE* pFile = fopen( strFileName.c_str(), "r" );
			if ( !pFile ) continue;

			fclose(pFile);

			if(Parse(strFileName, pTargetFile, bUseInclude) == false)
				return false;

			break;
		}

	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	#if를 처리한다.
///
/// @param	lineBuf		해당 소스 라인 내용
/// @param	lineBufSize	해당 소스 라인 내용 버퍼 크기
/// @param	sourceFile	원본 파일
/// @param	fileRead	이 함수가 끝나고 다음 루프에서 소스의 다음 라인을 읽을지 여부
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
HyBool HyCppPreprocessor::_Parse_if( HyChar* const lineBuf, const HyInt32 lineBufSize, FILE* const sourceFile, HyBool& fileRead )
{
	HyChar* const p = strstr( lineBuf, "#" ) + 1;

	ParserData interpretData;
	if ( Parse( p, interpretData, m_pParam ) == false )
	{
		LOG( HyLog::Error, "Parse() failed" );
		return false;
	}

	if ( g_parsingData.m_listPreprocessorValue.empty() )
	{
		LOG( HyLog::Error, "g_parsingData.m_listPreprocessorValue.empty()" );
		return false;
	}

	HyInt32 result = g_parsingData.m_listPreprocessorValue.back();

	if ( strstr( lineBuf, "#ifndef") )
	{
		if ( result != 1 ) return true;
	}
	else
	{
		if ( result != 0 ) return true;
	}

	HyInt32 depth = 1;
	while ( true )
	{
		if ( fgets( lineBuf, lineBufSize, sourceFile ) == false )
			LOG( HyLog::Fatal, "fgets() failed" );

		if ( strstr( lineBuf, "#if" ) )
		{
			++depth;
		}
		else if ( strstr( lineBuf, "#else" ) )
		{
			if ( depth == 1 )
				break;
		}
		else if ( strstr( lineBuf, "#elif" ) )
		{
			if ( depth == 1 )
			{
				fileRead = false;
				break;
			}
		}
		else if ( strstr( lineBuf, "#endif" ) )
		{
			if ( --depth == 0 )
			{
				fileRead = false;
				break;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	#define을 처리한다.
///
/// @param	lineBuf		해당 소스 라인 내용
/// @param	lineBufSize	해당 소스 라인 내용 버퍼 크기
/// @param	sourceFile	원본 파일
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
HyBool HyCppPreprocessor::Parse_define( HyChar* const lineBuf, const HyInt32 lineBufSize, FILE* const sourceFile )
{
	HyChar* p = strstr( lineBuf, "#define" ) + strlen( "#define" );

	while ( *p == ' ' || *p == '\t' )
		++p;

	const HyChar* const name = p;
	while ( isalpha( *p ) || *p == '_' )
		++p;

	HyChar szName[ 256 ];
	strncpy_s( szName, sizeof( szName ), name, p - name );
	szName[ p - name ] = 0;

	const HyBool isFunction = (*p == '(');

	Definition definition;

	if ( isFunction )
	{
		while ( true )
		{
			HyBool doExit = false;

			while ( isalpha( *p ) == false && *p != '_' )
			{
				if ( *p == ')' )
				{
					++p;
					doExit = true;
					break;
				}
				else
				{
					++p;
				}
			}

			if ( doExit )
				break;

			const HyChar* const paramStart = p;
			while ( isalpha( *p ) || *p == '_' )
				++p;

			HyChar paramName[ 256 ];
			strncpy_s( paramName, sizeof( paramName ), paramStart, p - paramStart );
			paramName[ p - paramStart ] = 0;

			definition.m_params.push_back( paramName );
		}
	}
	else
	{
		while ( *p == ' ' )
			++p;
	}

	HyChar* bodyStart = p;
	while ( *p != '\r' && *p != '\n' )
		++p;

	HyChar body[ 1024 * 10 ];

	if ( p[ -1 ] != '\\' )
	{
		strncpy_s( body, sizeof( body ), bodyStart, p - bodyStart );
		body[ p - bodyStart ] = 0;
	}
	else
	{
		p[ -1 ] = 0;
		strcpy_s( body, sizeof( body ), bodyStart );

		while ( true )
		{
			if ( fgets( lineBuf, lineBufSize, sourceFile ) == false )
				return false;

			p = strtok( lineBuf, "\r\n" );
			if ( !p )
				return false;

			if ( p[ strlen( p ) - 1 ] != '\\' )
			{
				strcat_s( body, sizeof( body ), p );
				break;
			}

			p[ strlen( p ) - 1 ] = 0;
			strcat_s( body, sizeof( body ), p );
			strcat_s( body, sizeof( body ), "\n" );
		}
	}

	definition.m_strValue = body;
	g_parsingData.m_definitions[ szName ] = definition;

	return true;
}


bool HyCppPreprocessor::Parse_undef(char* const pszLineBuf)
{
	char* p = strstr(pszLineBuf, "#undef") + strlen("#undef");
	if(strstr(p, "(") != NULL)
	{
		LOG(HyLog::Error, "strstr() == NULL");
		return false;
	}

	p = strtok(p, " \t\n");
	if(p == NULL)
	{
		LOG(HyLog::Error, "p == NULL");
		return false;
	}

	const std::string strDefine = p;
	p = strtok(NULL, " \t\n");
	if(p != NULL)
	{
		LOG(HyLog::Error, "p != NULL");
		return false;
	}

	g_parsingData.m_definitions.erase(strDefine);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	#elif를 처리한다.
///
/// @param	lineBuf		해당 소스 라인 내용
/// @param	lineBufSize	해당 소스 라인 내용 버퍼 크기
/// @param	sourceFile	원본 파일
/// @param	fileRead	이 함수가 끝나고 다음 루프에서 소스의 다음 라인을 읽을지 여부
///
/// @return	작업을 성공적으로 수행했으면 true, 그렇지 않으면 false를 반환한다.
////////////////////////////////////////////////////////////////////////////////////////////////////
HyBool HyCppPreprocessor::_Parse_elif( HyChar* lineBuf, HyUInt32 lineBufSize, FILE* sourceFile, HyBool& fileRead )
{
	if ( g_parsingData.m_listPreprocessorValue.empty() )
	{
		LOG( HyLog::Error, "g_parsingData.m_listPreprocessorValue.empty()" );
		return false;
	}

	HyInt32 result = g_parsingData.m_listPreprocessorValue.back();

	if ( result != 1 )
	{
		g_parsingData.m_listPreprocessorValue.pop_back();

		HyChar* const p = strstr( lineBuf, "#" ) + 1;

		ParserData interpretData;
		if ( Parse( p, interpretData, m_pParam ) == false )
		{
			LOG( HyLog::Error, "Parse() failed" );
			return false;
		}

		if ( g_parsingData.m_listPreprocessorValue.empty() )
		{
			LOG( HyLog::Error, "g_parsingData.m_listPreprocessorValue.empty()" );
			return false;
		}

		result = g_parsingData.m_listPreprocessorValue.back();
		if ( result != 0 ) return true;

		HyInt32 depth = 1;
		while ( true )
		{
			if ( fgets( lineBuf, lineBufSize, sourceFile ) == false )
				LOG( HyLog::Fatal, "fgets() failed" );

			if ( strstr( lineBuf, "#if" ) != NULL )
			{
				++depth;
			}
			else if ( strstr( lineBuf, "#else" ) != NULL )
			{
				if ( depth == 1 )
					break;
			}
			else if ( strstr( lineBuf, "#endif" ) != NULL )
			{
				if ( --depth == 0 )
				{
					fileRead = false;
					break;
				}
			}
		}

		return true;
	}

	while ( true )
	{
		if ( !fgets( lineBuf, lineBufSize, sourceFile ) )
			LOG( HyLog::Fatal, "fgets() failed" );

		if ( strstr( lineBuf, "#endif" ) )
		{
			fileRead = false;
			break;
		}
	}

	return true;
}

bool HyCppPreprocessor::Parse_else(char* const pszLineBuf, const int iLineBufSize, FILE* const pSourceFile, bool& bFileRead)
{
	if(g_parsingData.m_listPreprocessorValue.empty())
	{
		LOG(HyLog::Error, "g_parsingData.m_listPreprocessorValue.empty()");
		return false;
	}

	const int iResult = g_parsingData.m_listPreprocessorValue.back();

	if(iResult != 1)
		return true;

	while(true)
	{
		if(fgets(pszLineBuf, iLineBufSize, pSourceFile) == false)
			LOG(HyLog::Fatal, "fgets() failed");

		if(strstr(pszLineBuf, "#endif") != NULL)
		{
			bFileRead = false;
			break;
		}
	}

	return true;
}


bool HyCppPreprocessor::Parse_pragma( char* const pszLineBuf, const std::string& strFileName )
{
	const char* const pszPragmaType = pszLineBuf + strlen( "#pragma " );

	if ( strncmp( pszPragmaType, "once", 4 ) == 0 )
	{
		if ( g_parsingData.m_definitions.find( strFileName ) != g_parsingData.m_definitions.end() )
			return false;

		g_parsingData.m_definitions[ strFileName ] = Definition();
		return true;
	}

	return true;
}
