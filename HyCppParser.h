#pragma once


#include <list>
#include <map>
#include <string>
#include "../GrammarParser/HyGrammarParser.h"

struct VarType
{
	struct GetFullValueFlag
	{
		enum Enum
		{
			NOTHING				= 0x00,
			EXCLUDE_ATTRIBUTE	= 0x01,
			EXCLUDE_TAIL		= 0x02,
		};
	};

	std::list< std::string > m_attributeList;
	std::string              m_value;
	std::list< VarType >     m_templateParamList;
	std::string              m_tail;
	TokenPos                 m_pos;

	VarType()
	{
	}

	void Clear()
	{
		m_attributeList.clear();
		m_value.clear();
		m_tail.clear();
		m_pos.Clear();
	}

	bool IsPointer() const
	{
		if ( m_tail.empty() ) return false;
		if ( m_tail[ m_tail.length() - 1 ] != '*' ) return false;

		return true;
	}

	bool IsReference() const
	{
		if ( m_tail.empty() ) return false;
		if ( m_tail[ m_tail.length() - 1 ] != '&' ) return false;

		return true;
	}

	std::string GetAttribute() const
	{
		std::string strAttribute;

		for( std::list< std::string >::const_iterator itr = m_attributeList.begin(); itr != m_attributeList.end(); ++itr )
		{
			if( itr != m_attributeList.begin() )
				strAttribute += " ";

			const std::string& strCurAttribute = *itr;
			strAttribute += strCurAttribute;
		}

		return strAttribute;
	}

	std::string GetFullValue( const int iFlag = GetFullValueFlag::NOTHING ) const
	{
		std::string strFullValue;

		if( ( iFlag & GetFullValueFlag::EXCLUDE_ATTRIBUTE ) == 0 )
		{
			const std::string strAttribute = GetAttribute();
			strFullValue += GetAttribute();
			if( strAttribute.empty() == false)
				strFullValue += " ";
		}

		strFullValue += m_value;

		if( m_templateParamList.empty() == false )
		{
			strFullValue += " < ";
			for( std::list< VarType >::const_iterator itrTemplateParam = m_templateParamList.begin(); itrTemplateParam != m_templateParamList.end(); ++itrTemplateParam )
			{
				const VarType& templateParam = *itrTemplateParam;

				if( itrTemplateParam != m_templateParamList.begin() )
					strFullValue += " , ";

				strFullValue += templateParam.GetFullValue();
			}
			strFullValue += " >";
		}

		if( ( iFlag & GetFullValueFlag::EXCLUDE_TAIL ) == 0 )
		{
			if( m_tail.empty() == false )
				strFullValue += " " + m_tail;
		}

		return strFullValue;
	}
};


typedef std::list< VarType > VarTypeList;


struct FunctionParam
{
	VarType			m_type;
	std::string		m_strName;
};


struct ClassMemberVariable
{
	std::string		m_strAccess;
	VarType			m_type;
	std::string		m_strName;
	TokenPos		m_pos;
};


struct Function
{
	VarType                    m_returnType;                  ///< 반환 타입
	std::string                m_name;                        ///< 이름
	std::list< FunctionParam > m_params;                      ///< 파라미터 목록
	TokenPos                   m_declarationStartPosition;    ///< 선언 시작 위치
	TokenPos                   m_declarationEndPosition;      ///< 선언 끝 위치
	TokenPos                   m_implementationStartPosition; ///< 구현 시작 위치
	TokenPos                   m_implementationEndPosition;   ///< 구현 끝 위치

	virtual HyBool IsStatic() const { return false; }
	virtual HyBool IsVirtual() const { return false; }
};


struct ClassMemberFunction
	:
	Function
{
	std::string			m_strAccess;
	std::string			m_strAttribute;

	virtual bool IsStatic() const { return m_strAttribute.find( "static" ) != std::string::npos; }
	virtual bool IsVirtual() const { return m_strAttribute.find( "virtual" ) != std::string::npos; }
};


struct Namespace;

struct ParentClass
{
	std::string	m_strAccess;
	VarType		m_name;
};


struct ParsingResult;

struct Class
{
	VarType								m_name;
	std::string							m_strFullName;
	std::list< ParentClass >			m_listParentClass;
	std::list< ClassMemberVariable >	m_listMemberVariable;
	std::list< ClassMemberFunction >	m_memberFunctions;
	TokenPos							m_position;
	std::list< VarType >				m_listTemplateType;
	std::map< std::string, VarType >	m_mapTypeDef;
	std::list< Function >				m_functionTypeDefList;
	Namespace*							m_pNamespace;

	void GetVirtualFunctions(
		const ParsingResult& parsingResult,
		std::map< std::string, const ClassMemberFunction* >& mapFunction ) const;

	ClassMemberFunction* FindMemberFunction( const std::string& functionName );
};


struct Variable
{
	VarType			m_type;
	std::string		m_strName;
	TokenPos		m_pos;
};


struct Namespace
{
	typedef std::map< HyString, Class* >    ClassList;     ///< 클래스 목록
	typedef std::map< HyString, Namespace > NamespaceList; ///< 네임스페이스 목록
	typedef std::list< Function >           FunctionList;  ///< 함수 목록
	typedef std::map< HyString, VarType >   TypeDefList;   ///< 타입 정의 목록

	HyString      m_name;          
	HyString      m_fullName;      
	ClassList     m_classes;       
	NamespaceList m_subNamespaces; 
	FunctionList  m_functions;     
	TypeDefList   m_typeDefs;      
};


struct TokenString
{
	std::string			m_value;
	const TokenPos*		m_pPos;

	TokenString() {}
	TokenString(const std::string& strValue, const TokenPos* const pPos) : m_value(strValue), m_pPos(pPos)
	{
	}
};


struct ParsingResult
{
	typedef std::map< HyString, Class >    ClassList;          ///< 클래스 목록
	typedef std::map< HyString, VarType >  TypeDefList;        ///< 타입 정의 목록
	typedef std::map< HyString, HyString > ReverseTypeDefList; ///< 역 타입 정의 목록

	Namespace          m_globalNamespace; ///< 전역 네임스페이스
	ClassList          m_classes;         ///< 클래스 목록
	TypeDefList        m_typeDefs;        ///< 타입 정의 목록
	ReverseTypeDefList m_reverseTypeDefs; ///< 역 타입 정의 목록
};


////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief	CPP 파서
////////////////////////////////////////////////////////////////////////////////////////////////////
class HyCppParser
	:
	public HyGrammarParser
{
public:
	/// 포함 경로 목록 타입 정의
	typedef std::list< HyString > IncludeDirectoryList;

	/// 정의 목록 타입 정의
	typedef std::map< HyString, HyString > DefinitionMap;

public:
	/// 생성자
	HyCppParser( const HyString& name = "CppParser" );

	/// CPP파일을 파싱한다.
	HyBool Parse(
		const HyString&             filePath,
		const IncludeDirectoryList& includeDirectories,
		const DefinitionMap&        definitions,
		      ParsingResult&        parsingResult,
		      HyBool                forDebug = false );

private:
	/// 초기화할때를 처리한다.
	virtual void OnInit() override;

	/// add cpp11 grammars
	HyVoid _AddCpp11Grammars();
};

#ifndef _LIB
#pragma comment( lib, "HyCppParser.lib" )
#endif
