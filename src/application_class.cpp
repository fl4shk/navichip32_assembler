#include "application_class.hpp"


Application::Application()
{
}

int Application::operator () ()
{
	advance();
	lex();
	while (next_tok() != &Tok::Blank)
	{
		parse();
	}

	//for (auto& block_iter : blk_map())
	//{
	//	printout("Block with name, filename_stuff:  \"", 
	//		block_iter.second.name, "\", \"", 
	//		block_iter.second.filename_stuff, "\"\n");

	//	printout(block_iter.second.cvec);
	//}

	//for (auto& sprite_iter : spr_map())
	//{
	//	printout("Sprite with name, filename_stuff:  \"", 
	//		sprite_iter.second.name, "\", \"", 
	//		sprite_iter.second.filename_stuff, "\"\n");

	//	printout(sprite_iter.second.cvec);
	//}

	return 0;
}

void Application::need(PTok tok)
{
	if (next_tok() == tok)
	{
		//printout("need(), before lex():  ", next_tok()->str(), "\n");
		lex();
		//printout("need(), after lex():  ", next_tok()->str(), "\n");
	}
	else
	{
		printerr("need():  ");
		expected(tok);
	}
}

void Application::advance()
{
	if (next_char() == EOF)
	{
		set_next_tok(&Tok::Blank);
		return;
	}

	set_next_char(getchar());

	if (next_char() == '\n')
	{
		++__line_num;
	}
}



void Application::lex()
{
	while (isspace(next_char()))
	{
		advance();
	}

	if (next_char() == EOF)
	{
		set_next_tok(&Tok::Blank);
		return;
	}

	std::string next_str;
	next_str += next_char();

	if (next_str == "")
	{
	}

	#define VARNAME(some_tok) \
		else if (next_str == Tok::some_tok.str()) \
		{ \
			set_next_tok(&Tok::some_tok); \
			advance(); \
			return; \
		}
	#define VALUE(some_str)

	LIST_OF_PUNCT_TOKENS(VARNAME, VALUE)
	LIST_OF_SINGLE_CHAR_OPERATOR_TOKENS(VARNAME, VALUE)

	#undef VARNAME

	// Find an identifier
	if (isalpha(next_char()) || (next_char() == '_'))
	{
		//printout("lex():  An ident?\n");
		next_str = "";
		next_str += next_char();
		advance();

		while (isalnum(next_char()) || (next_char() == '_'))
		{
			next_str += next_char();
			advance();
		}

		//printout("lex():  next_str, next_char():  ", next_str, ", ",
		//	(char)next_char(), "\n");

		// If we haven't seen a user symbol like this before...
		if (!user_sym_tbl().contains(next_str))
		{
			// ...Then create a new symbol
			//printout("Creating a new symbol....\n");
			Symbol to_insert(next_str, &Tok::Ident, 0);

			user_sym_tbl().at(next_str) = to_insert;
		}


		set_next_tok(&Tok::Ident);
		set_next_sym_str(next_str);

		return;
	}

	// The constant 0
	if (next_char() == '0')
	{
		set_next_num(0);
		set_next_tok(&Tok::NaturalNumber);

		advance();

		if (isdigit(next_char()))
		{
			expected("Natural number that does not start with 0!");
		}
	}

	// Find a constant natural number
	if (isdigit(next_char()))
	{
		set_next_num(0);

		do
		{
			set_next_num((next_num() * 10) + (next_char() - '0'));
			advance();
		} while (isdigit(next_char()));

		set_next_tok(&Tok::NaturalNumber);

		return;

	}

	// BitShL
	if (next_char() == '<')
	{
		advance();

		if (next_char() == '<')
		{
			advance();
			set_next_tok(&Tok::BitShL);
		}
		else
		{
			expected("\"<<\" but got \"", next_str, "\"!");
		}

		return;
	}

	// BitShR
	if (next_char() == '>')
	{
		advance();

		if (next_char() == '>')
		{
			advance();
			set_next_tok(&Tok::BitShR);
		}
		else
		{
			expected("\">>\" but got \"", next_str, "\"!");
		}

		return;
	}


}

void Application::parse()
{
	printout("parse():  ", next_tok()->str(), "\n");

	//if (next_tok() == &Tok::Block)
	//{
	//	//printout("I found a \"block\" token!\n");
	//	lex();
	//	handle_block();
	//}

	//else if (next_tok() == &Tok::Sprite)
	//{
	//	//printout("I found a \"sprite\" token!\n");
	//	lex();
	//	handle_sprite();
	//}

	//else if (next_tok() == &Tok::Blank)
	//{
	//	printout("Done.\n");
	//}

	//else
	//{
	//	expected("token of type \"block\" or \"sprite\"!");
	//}
	
}



s64 Application::handle_term()
{
	s64 ret = handle_factor();

	while ((next_tok() == &Tok::Mult) || (next_tok() == &Tok::Div)
		|| (next_tok() == &Tok::BitAnd) || (next_tok() == &Tok::BitOr)
		|| (next_tok() == &Tok::BitXor) || (next_tok() == &Tok::BitShL) 
		|| (next_tok() == &Tok::BitShR))
	{
		const auto old_next_tok = next_tok();
		lex();

		if (old_next_tok == &Tok::Mult)
		{
			ret *= handle_factor();
		}
		else if (old_next_tok == &Tok::Div)
		{
			ret /= handle_factor();
		}
		else if (old_next_tok == &Tok::BitAnd)
		{
			ret &= handle_factor();
		}
		else if (old_next_tok == &Tok::BitOr)
		{
			ret |= handle_factor();
		}
		else if (old_next_tok == &Tok::BitXor)
		{
			ret ^= handle_factor();
		}
		else if (old_next_tok == &Tok::BitShL)
		{
			ret <<= handle_factor();
		}
		else if (old_next_tok == &Tok::BitShR)
		{
			ret >>= handle_factor();
		}
	}


	return ret;
}

s64 Application::handle_factor()
{
	if (next_tok() == &Tok::NaturalNumber)
	{
		s64 ret = next_num();
		lex();
		return ret;
	}
	//else if (next_tok() == &Tok::Ident)
	//{
	//	size_t index;
	//	if (some_cvec.contains(next_sym_str(), index))
	//	{
	//		s64 ret = some_cvec.vec.at(index).get_s64();
	//		lex();
	//		return ret;
	//	}
	//	else
	//	{
	//		expected("existing constant of name \"", next_sym_str(),
	//			"\"!");
	//	}
	//}

	s64 ret;

	if (next_tok() != &Tok::LParen)
	{
		expected("token of type \"NaturalNumber\" or \"Identifier\" or ",
			"\"(\"!");
	}

	need(&Tok::LParen);

	ret = handle_expr();

	need(&Tok::RParen);

	return ret;
}

s64 Application::handle_expr()
{
	const auto old_next_tok = next_tok();

	s64 ret;

	if ((old_next_tok == &Tok::Plus) || (old_next_tok == &Tok::Minus))
	{
		lex();

		if (old_next_tok == &Tok::Plus)
		{
			ret = handle_term();
		}
		else // if (old_next_tok == &Tok::Minus)
		{
			ret = -handle_term();
		}
	}
	else
	{
		ret = handle_term();
	}

	if ((next_tok() == &Tok::Plus) || (next_tok() == &Tok::Minus))
	{
		ret += handle_expr();
	}

	return ret;

}





bool Application::next_tok_is_punct() const
{
	if (next_tok() == nullptr)
	{
	}
	
	#define VARNAME(some_tok) \
		else if (next_tok() == &Tok::some_tok) \
		{ \
			return true; \
		}
	#define VALUE(some_str) 
	
	LIST_OF_PUNCT_TOKENS(VARNAME, VALUE)

	#undef VARNAME
	#undef VALUE


	return false;
}

