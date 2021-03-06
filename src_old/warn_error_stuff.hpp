#ifndef warn_error_stuff_hpp
#define warn_error_stuff_hpp

#include "misc_includes.hpp"
//#include "lexer_class.hpp"

namespace flare32
{

class lexer;

class warn_error
{
private:		// variables
	lexer* lex = nullptr;
	
public:		// functions
	inline warn_error( lexer* s_lex ) : lex(s_lex)
	{
	}
	virtual inline ~warn_error()
	{
	}
	
	template< typename... arg_types >
	void warn( arg_types&&... args )
	{
		printerr( lex->lineno(), ":  warning:  ", args..., "\n" );
	}
	
	template< typename... arg_types >
	void warn1( arg_types&&... args )
	{
		if ( lex->pass() == 1 )
		{
			warn(args...);
		}
	}
	
	template< typename... arg_types >
	void error( arg_types&&... args )
	{
		printerr( lex->lineno(), ":  error:  ", args..., "\n" );
		exit(1);
	}
	
	template< typename... arg_types >
	void invalid( arg_types&&... args )
	{
		error( "Invalid ", args... );
	}
	template< typename... arg_types >
	void expected( arg_types&&... args )
	{
		error( "Expected ", args... );
	}
	template< typename... arg_types >
	void unknown( arg_types&&... args )
	{
		error( "Unknown ", args... );
	}
	
};



}

#endif		// warn_error_stuff_hpp
