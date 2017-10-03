#include "assembler_class.hpp"

namespace flare32
{

Assembler::Assembler() 
	: __we(&__line_num),
	__lexer(&__we, &__builtin_sym_tbl, &__user_sym_tbl, &__define_tbl, 
	&__instr_tbl),
	__codegen(&__we, &__addr, &__last_addr, &__pass, last_pass,
	&__builtin_sym_tbl, &__user_sym_tbl, &__define_tbl, &__instr_tbl)
{
}
void Assembler::init(char* s_input_filename)
{
	set_input_filename(s_input_filename);
	set_infile(fopen(input_filename(), "r"));

	if (infile() == nullptr)
	{
		we().err("Cannot read file");
	}

	__lexer.set_infile(infile());

	fill_builtin_sym_tbl();
}

int Assembler::operator () ()
{
	set_pass(0);
	fill_lines();
	find_defines();


	// Two passes
	for (set_pass(1); pass() <= last_pass; set_pass(pass() + 1))
	{
		reinit();

		size_t outer_index = 0, inner_index = 0;

		//advance(outer_index, inner_index, true);
		lex(outer_index, inner_index, true);

		while (next_tok() != &Tok::Eof)
		{
			line(outer_index, inner_index, false);
		}

		//printout("\n\n");
	}

	return 0;
}



void Assembler::reinit()
{
	rewind(infile());
	set_addr(0);
	set_line_num(0);
	set_next_char(' ');
	__prev_tok = nullptr;
	set_next_tok(nullptr);
	set_next_sym_str("");
	set_next_num(-1);
}

void Assembler::fill_builtin_sym_tbl()
{
	// General-purpose registers
	builtin_sym_tbl().insert_or_assign(Symbol("r0", &Tok::Reg, 0));
	builtin_sym_tbl().insert_or_assign(Symbol("r1", &Tok::Reg, 1));
	builtin_sym_tbl().insert_or_assign(Symbol("r2", &Tok::Reg, 2));
	builtin_sym_tbl().insert_or_assign(Symbol("r3", &Tok::Reg, 3));
	builtin_sym_tbl().insert_or_assign(Symbol("r4", &Tok::Reg, 4));
	builtin_sym_tbl().insert_or_assign(Symbol("r5", &Tok::Reg, 5));
	builtin_sym_tbl().insert_or_assign(Symbol("r6", &Tok::Reg, 6));
	builtin_sym_tbl().insert_or_assign(Symbol("r7", &Tok::Reg, 7));
	builtin_sym_tbl().insert_or_assign(Symbol("r8", &Tok::Reg, 8));
	builtin_sym_tbl().insert_or_assign(Symbol("r9", &Tok::Reg, 9));
	builtin_sym_tbl().insert_or_assign(Symbol("r10", &Tok::Reg, 10));
	builtin_sym_tbl().insert_or_assign(Symbol("r11", &Tok::Reg, 11));
	builtin_sym_tbl().insert_or_assign(Symbol("r12", &Tok::Reg, 12));
	builtin_sym_tbl().insert_or_assign(Symbol("r13", &Tok::Reg, 13));
	builtin_sym_tbl().insert_or_assign(Symbol("r14", &Tok::Reg, 14));
	builtin_sym_tbl().insert_or_assign(Symbol("r15", &Tok::Reg, 15));
	builtin_sym_tbl().insert_or_assign(Symbol("lr", &Tok::Reg, 14));
	builtin_sym_tbl().insert_or_assign(Symbol("sp", &Tok::Reg, 15));

	// Special-purpose registers
	builtin_sym_tbl().insert_or_assign(Symbol("pc", &Tok::RegPc, -1));
	builtin_sym_tbl().insert_or_assign(Symbol("ira", &Tok::RegIra, -1));
	builtin_sym_tbl().insert_or_assign(Symbol("flags", &Tok::RegFlags,
		-1));


	// Instructions
	for (const auto& outer_iter : __instr_tbl.instr_vec)
	{
		for (const auto& instr : *outer_iter)
		{
			builtin_sym_tbl().insert_or_assign(Symbol(instr->str(), 
				&Tok::Instr, -1));
		}
	}
}

void Assembler::print_parse_vec
	(const std::vector<ParseNode> some_parse_vec) const
{
	for (const auto& parse_iter : some_parse_vec)
	{
		printout(parse_iter.next_tok != nullptr 
			? parse_iter.next_tok->str() : "nullptr", " ");
	}
	printout("\n");
}

void Assembler::next_line(size_t& some_outer_index, 
	size_t& some_inner_index, std::vector<ParseNode>& some_parse_vec)
{
	//while ((next_tok() != &Tok::Newline) && (next_tok() != &Tok::Eof)
	//	&& (next_tok() != &Tok::Bad))
	while (next_tok() != &Tok::Bad)
	{
		//bool temp = false;
		//if (next_tok() != nullptr)
		{
			some_parse_vec.push_back(ParseNode(next_tok(), next_sym_str(), 
				next_num()));
			//printout(next_tok()->str(), "\n");
			//temp = true;
		}
		if ((next_tok() == &Tok::Eof)
			|| (next_tok() == &Tok::Newline))
		{
			lex(some_outer_index, some_inner_index, true);
			break;
		}
		lex(some_outer_index, some_inner_index, true);

		//if (!temp)
		//{
		//	some_parse_vec.push_back(ParseNode(next_tok(), next_sym_str(), 
		//		next_num()));
		//}

		//printout(some_parse_vec.back().next_tok->str(), "\n");
	}

	//printout("next_line():  ");
	//print_parse_vec(some_parse_vec);
}


void Assembler::line(size_t& some_outer_index, size_t& some_inner_index,
	bool just_find_defines)
{
	std::vector<ParseNode> parse_vec, second_parse_vec;

	auto call_lex = [&]() -> void
	{
		lex(some_outer_index, some_inner_index, true);
	};


	next_line(some_outer_index, some_inner_index, parse_vec);


	printout("line():  ");
	for (size_t i=0; i<parse_vec.size(); ++i)
	{
		printout(parse_vec.at(i).next_tok->str(), " ");
	}
	printout("\n");

	if (next_tok() == &Tok::Bad)
	{
		set_line_num(line_num() + 1);
		we().err("Invalid syntax");
	}

	if ((parse_vec.size() == 0)
		|| tok_is_comment(parse_vec.front().next_tok))
	{
		//lex(!just_find_defines);

		//printout("possible comment?");
		call_lex();
		return;
	}

	//for (const auto& node : parse_vec)
	//{
	//	printout(node.next_tok->str(), "\t\t");
	//}
	//printout("\n");

	size_t index = 1;

	if (handle_directives(some_outer_index, some_inner_index, index,
		parse_vec, just_find_defines))
	{
		return;
	}

	if (just_find_defines)
	{
		return;
	}


	bool found_label = false;

	// Check for a label
	if (parse_vec.size() >= 2)
	{
		if (tok_is_ident_ish(parse_vec.at(0).next_tok) 
			&& parse_vec.at(1).next_tok == &Tok::Colon)
		{
			found_label = true;

			if (user_sym_tbl().at(parse_vec.at(0).next_sym_str).type()
				== SymType::EquateName)
			{
				we().err("Can't use an equate as a label!");
			}
			else if (user_sym_tbl().at(parse_vec.at(0).next_sym_str)
				.type() != SymType::Other)
			{
				we().err("Invalid label name!");
			}

			// Update the value of the label in the user symbol table.
			// This happens regardless of what pass we're on.
			user_sym_tbl().at(parse_vec.at(0).next_sym_str).set_value
				(addr());
		}
	}

	if (!found_label)
	{
		for (size_t i=0; i<parse_vec.size(); ++i)
		{
			// Ignore comments
			if (tok_is_comment(parse_vec.at(i).next_tok))
			{
				break;
			}
			second_parse_vec.push_back(parse_vec.at(i));
		}
	}
	else
	{
		for (size_t i=2; i<parse_vec.size(); ++i)
		{
			// Ignore comments
			if (tok_is_comment(parse_vec.at(i).next_tok))
			{
				break;
			}
			second_parse_vec.push_back(parse_vec.at(i));
		}
	}

	finish_line(second_parse_vec);
	//lex(!just_find_defines);
	call_lex();
}


bool Assembler::handle_directives(size_t& some_outer_index, 
	size_t& some_inner_index, size_t& index,
	const std::vector<ParseNode>& parse_vec, bool just_find_defines)
{
	auto eek = [&]() -> void
	{
		we().err("invalid syntax for ", parse_vec.front().next_tok->str());
	};

	// Check for assembler directives
	if (parse_vec.front().next_tok == &Tok::DotOrg)
	{
		if (pass() > 0)
		{
			// .org expr
			set_addr(handle_expr(parse_vec, index));

			if (index != parse_vec.size())
			{
				we().err("extra characters on line");
			}
		}

		return true;
	}
	else if (parse_vec.front().next_tok == &Tok::DotB)
	{
		if (pass() > 0)
		{
			// .db expr
			// .db expr, expr2
			// .db expr, expr2, ...
			for (;;)
			{
				__codegen.gen8(handle_expr(parse_vec, index));
				if (pass() == last_pass)
				{
					printout("\n");
				}

				if (index >= parse_vec.size())
				{
					break;
				}
				
				if (parse_vec.at(index).next_tok != &Tok::Comma)
				{
					eek();
				}

				++index;
			}
		}

		return true;
	}
	else if (parse_vec.front().next_tok == &Tok::DotW)
	{
		if (pass() > 0)
		{
			// .dw expr
			// .dw expr, expr2
			// .dw expr, expr2, ...
			for (;;)
			{
				__codegen.gen32(handle_expr(parse_vec, index));
				if (pass() == last_pass)
				{
					printout("\n");
				}

				if (index >= parse_vec.size())
				{
					break;
				}
				
				if (parse_vec.at(index).next_tok != &Tok::Comma)
				{
					eek();
				}

				++index;
			}
		}

		return true;
	}


	#define TOKEN_STUFF(varname, value) \
		(parse_vec.front().next_tok == &Tok::varname) ||
	else if (LIST_OF_EQUATE_DIRECTIVE_TOKENS(TOKEN_STUFF) false)
	{
	#undef TOKEN_STUFF
		// .equate ident expr
		if (parse_vec.size() < 3)
		{
			eek();
		}

		if (!tok_is_ident_ish(parse_vec.at(1).next_tok))
		{
			eek();
		}

		//printout(parse_vec.at(1).next_sym_str, " ",
		//	(int)user_sym_tbl().at(parse_vec.at(1).next_sym_str).type(), 
		//	"\n");

		if (user_sym_tbl().at(parse_vec.at(1).next_sym_str).type() 
			!= SymType::EquateName)
		{
			we().err("Can't convert a label to an equate!");
		}

		index = 2;

		s64 expr_result = handle_expr(parse_vec, index);

		user_sym_tbl().at(parse_vec.at(1).next_sym_str).set_value
			(expr_result);

		return true;
	}

	else if (parse_vec.front().next_tok == &Tok::DotDef)
	{
		if (!just_find_defines)
		{
			//printout("!just_find_defines\n");
			return true;
		}
		//printout("just_find_defines\n");

		// .def `ident() text
		// .def `ident(args...) text

		if (parse_vec.size() < 5)
		{
			eek();
		}

		if (!tok_is_ident_ish(parse_vec.at(1).next_tok))
		{
			eek();
		}

		if (parse_vec.at(2).next_tok != &Tok::LParen)
		{
			eek();
		}

		Define to_insert;

		to_insert.set_name(parse_vec.at(1).next_sym_str);

		if (define_tbl().contains(to_insert.name()))
		{
			we().err(".def already defined");
		}

		if (parse_vec.at(3).next_tok == &Tok::RParen)
		{
			// No arguments, just () stuffs
			for (size_t i=4; i<parse_vec.size(); ++i)
			{
				if (tok_is_ident_ish(parse_vec.at(i).next_tok))
				{
					to_insert.text().push_back(parse_vec.at(i)
						.next_sym_str + '\n');
				}
				else if (parse_vec.at(i).next_tok == &Tok::NatNum)
				{
					to_insert.text().push_back(std::to_string
						(parse_vec.at(i).next_num) + '\n');
				}
				else
				{
					to_insert.text().push_back(parse_vec.at(i).next_tok
						->str() + '\n');
				}
			}
		}

		//else
		//{
		//	bool found_rparen = false;


		//	size_t i, j = 3;

		//	{
		//		i = j;

		//		while ((i < parse_vec.size())
		//			&& (parse_vec.at(i).next_tok != &Tok::RParen))
		//		{
		//			++i;
		//		}

		//		if (i >= parse_vec.size())
		//		{
		//			eek();
		//		}
		//	}

		//	if (i == j + 1)
		//	{
		//	}

		//}


		//if (!tok_is_ident_ish(parse_vec.at(3).next_tok))
		//{
		//	eek();
		//}


		// Only allow no argument defines for now
		else
		{
			eek();
		}

		define_tbl().insert_or_assign(to_insert);

		return true;
	}

	else if (parse_vec.front().next_tok == &Tok::DotIf)
	{
		printout("Found .if\n");
		std::vector<std::vector<ParseNode>> lines_vec;


		lines_vec.push_back((parse_vec));

		for (const auto& parse_iter : parse_vec)
		{
			printout(parse_iter.next_tok != nullptr 
				? parse_iter.next_tok->str() : "nullptr", " ");
		}
		printout("\n");


		const size_t first_line_num = some_outer_index - 1;

		std::vector<ParseNode> other_parse_vec;

		printout(next_tok() != nullptr ? next_tok()->str() : "nullptr", 
			"\n");

		next_line(some_outer_index, some_inner_index, other_parse_vec);

		for (const auto& parse_iter : other_parse_vec)
		{
			printout(parse_iter.next_tok != nullptr 
				? parse_iter.next_tok->str() : "nullptr", " ");
		}
		printout("\n");


		//do
		//{
		//	next_line(some_outer_index, some_inner_index, other_parse_vec);
		//	printout(other_parse_vec.size(), "\n");
		//	lines_vec.push_back((other_parse_vec));
		//} while ((next_tok() != &Tok::Eof) && (next_tok() != &Tok::Bad)
		//	&& (lines_vec.back().size() >= 1)
		//	&& (lines_vec.back().front().next_tok != &Tok::DotEndIf));

		//for (const auto& line_iter : lines_vec)
		//{
		//	for (const auto& parse_iter : line_iter)
		//	{
		//		printout(parse_iter.next_tok != nullptr 
		//			? parse_iter.next_tok->str() : "nullptr", " ");
		//	}
		//	printout("\n");
		//}

		printout(lines_vec.size(), "\n");

		if ((lines_vec.size() >= 1)
			&& (lines_vec.back().size() >= 1)
			&& (lines_vec.back().front().next_tok != &Tok::DotEndIf))
		{
			we().err("Missing .endif!");
		}

		handle_dot_if(lines_vec, just_find_defines, first_line_num);

		return true;
	}

	return false;
}

void Assembler::handle_dot_if
	(std::vector<std::vector<ParseNode>>& lines_vec,
	bool just_find_defines, const size_t first_line_num)
{
}

void Assembler::finish_line
	(const std::vector<ParseNode>& some_parse_vec)
{
	//for (const auto& node : some_parse_vec)
	//{
	//	printout(node.next_tok->str(), "\t\t");
	//}
	//printout("\n");

	if (some_parse_vec.size() == 0)
	{
		return;
	}

	
	//{
	//size_t index = 0;
	//const auto temp = handle_expr(some_parse_vec, index);

	//printout(temp, "\n");

	//set_addr(addr() + 4);
	//}


	if (some_parse_vec.at(0).next_tok != &Tok::Instr)
	{
		we().expected_tokens(&Tok::Instr);
	}

	const auto& instr_vec = __instr_tbl.at(some_parse_vec.at(0)
		.next_sym_str);



	bool complete = false;

	for (const auto& instr : instr_vec)
	{
		if (parse_instr(instr, some_parse_vec))
		{
			complete = true;
			break;
		}
	}

	if (!complete)
	{
		we().err("Invalid instruction arguments");
	}

}

void Assembler::fill_lines()
{
	reinit();

	{
	std::string some_line;

	size_t outer_index = 0, inner_index = 0;

	//while (next_tok() != &Tok::Eof)
	while (next_char() != EOF)
	{
		advance(outer_index, inner_index);

		//if (next_char() != EOF)
		//{
		//	some_line += next_char();
		//}
		//else
		//{
		//	some_line += '\n';
		//}
		//some_line += next_char();

		if ((next_char() != '\n') && (next_char() != EOF))
		{
			some_line += next_char();
		}
		else // if ((next_char() == '\n') || (next_char() == EOF))
		{
			if (next_char() == '\n')
			{
				some_line += '\n';
			}

			// Don't append a blank line
			if (some_line.size() >= 1)
			{
				__lines.push_back(some_line);
				some_line = "";
			}
		}
	}

	//// Append a '\n'
	////for (auto& line_iter : __lines)
	//for (size_t i=0; i<__lines.size()-1; ++i)
	//{
	//	auto& line_iter = __lines.at(i);

	//	if ((line_iter.size() >= 1)
	//		&& (line_iter.back() != '\n'))
	//	{
	//		line_iter += "\n";
	//	}
	//}

	}

	//printout(__lines.size(), "\n");

	//for (const auto& line_iter : __lines)
	//{
	//	printout(line_iter);
	//	//printout(line_iter);
	//}
	//printout("\n");

	{
	size_t outer_index = 0, inner_index = 0;
	reinit();

	lex(outer_index, inner_index, true);
	//std::vector<ParseNode> parse_vec;

	//while ((next_tok() != &Tok::Eof)
	//	&& (next_tok() != &Tok::Bad))
	////for (size_t i=0; i<__lines.size(); ++i)
	while ((next_tok() != &Tok::Eof)
		&& (next_tok() != &Tok::Bad))

	//for (size_t i=0; i<20; ++i)
	//for (;;)
	{
		//if (next_tok() == &Tok::Eof)
		//{
		//	break;
		//}

		std::vector<ParseNode> parse_vec;
		next_line(outer_index, inner_index, parse_vec);
		//parse_vec.push_back(ParseNode(next_tok(), next_sym_str(),
		//	next_num()));

		print_parse_vec(parse_vec);


		//if (next_tok() == &Tok::Newline)
		//{
		//	printout("newline ");
		//}
		//else
		//{
		//	printout(next_tok()->str(), " ");
		//}

		//lex(outer_index, inner_index, true);

	}

	//for (const auto& iter : parse_vec)
	//{
	//	printout(iter.next_tok->str(), " ");
	//}
	}

	exit(0);
}

void Assembler::find_defines()
{
	size_t define_expand_depth = 0;
	do
	{
		if (define_expand_depth >= define_expand_max_depth)
		{
			we().err("Cannot resolve .defs!\n");
		}

		set_changed(false);
		reinit();

		size_t outer_index = 0, inner_index = 0;

		lex(outer_index, inner_index, true);

		while (next_tok() != &Tok::Eof)
		{
			line(outer_index, inner_index, true);
		}

		expand_defines();

		++define_expand_depth;
	} while (changed());
}

void Assembler::expand_defines()
{
	auto eek = [&]() -> void
	{
		we().err("Unknown .def!");
	};


	auto attempt_expand_defn = [&](std::string& iter, Define& defn, 
		const std::vector<ParsePos>& pos_vec, size_t i,
		std::vector<ParseNode>& line_parse_vec) -> bool
	{
		std::vector<ParseNode> text_parse_vec;


		split(text_parse_vec, defn.text());

		if (line_parse_vec.front().next_tok == &Tok::DotDef)
		{
			// sneaky comment insertion
			//iter.insert(0, "@");
			iter.insert(0, ";");

			return false;
		}
		else if (tok_is_comment(line_parse_vec.front().next_tok))
		{
			return false;
		}


		// Erase the define instance
		{
			size_t j = pos_vec.at(i).inner_index;
			//iter.erase(j, defn.name().size());
			for (size_t k=0; k<defn.name().size(); ++k)
			{
				iter.at(j++) = ' ';
			}


			if (defn.args().size() != 0)
			{
				++i;
				if (line_parse_vec.at(i).next_tok != &Tok::LParen)
				{
					eek();
				}

				j = pos_vec.at(i).inner_index;
				for (size_t k=0; 
					k<line_parse_vec.at(i).next_tok->str().size(); 
					++k)
				{
					iter.at(j++) = ' ';
				}

				++i;

				while ((i < line_parse_vec.size())
					&& (line_parse_vec.at(i).next_tok != &Tok::RParen))
				{
					j = pos_vec.at(i).inner_index;
					for (size_t k=0; 
						k<line_parse_vec.at(i).next_tok->str().size(); 
						++k)
					{
						iter.at(j++) = ' ';
					}
					++i;
				}

				if (i >= line_parse_vec.size())
				{
					eek();
				}



				if (line_parse_vec.at(i).next_tok != &Tok::RParen)
				{
					eek();
				}
				
				j = pos_vec.at(i).inner_index;
				for (size_t k=0; 
					k<line_parse_vec.at(i).next_tok->str().size(); 
					++k)
				{
					iter.at(j++) = ' ';
				}
			}

		}

		size_t j = pos_vec.at(i).inner_index;

		//printout("stuff:  ", iter, "\n");


		for (const auto& parse_iter : text_parse_vec)
		{
			if (tok_is_ident_ish(parse_iter.next_tok))
			{
				iter.insert(j, parse_iter.next_sym_str + " ");
				j += parse_iter.next_sym_str.size() 
					+ std::string(" ").size();
			}
			else if (parse_iter.next_tok == &Tok::NatNum)
			{
				std::string str = std::to_string(parse_iter.next_num);
				iter.insert(j, str + " ");
				j += str.size() + std::string(" ").size();
			}
			else
			{
				iter.insert(j, parse_iter.next_tok->str() + " ");
				j += parse_iter.next_tok->str().size() 
					+ std::string(" ").size();
			}
		}

		return true;

	};

	for (std::string& iter : __lines)
	{
		//printout("Start:\n", iter);
		size_t define_expand_depth = 0;

		for (; 
			define_expand_depth < define_expand_max_depth;
			++define_expand_depth)
		{
			bool can_break = true;

			//printout(iter);
			std::vector<std::string> iter_vec({iter});

			std::vector<ParseNode> line_parse_vec;
			std::vector<ParsePos> pos_vec;

			split(line_parse_vec, iter_vec, &pos_vec);

			if ((line_parse_vec.size() >= 1) 
				&& tok_is_comment(line_parse_vec.front().next_tok))
			{
				can_break = true;
				break;
			}

			for (size_t i=0; i<line_parse_vec.size(); ++i)
			{
				const auto& parse_iter = line_parse_vec.at(i);

				if (tok_is_ident_ish(parse_iter.next_tok)
					&& define_tbl().contains(parse_iter.next_sym_str))
				{
					set_changed(true);

					attempt_expand_defn(iter, 
						define_tbl().at(parse_iter.next_sym_str), 
						pos_vec, i, line_parse_vec);
					can_break = false;
					break;
				}
			}

			//printout(iter);

			if (can_break)
			{
				//printout("Can break\n");
				break;
			}
		}
		//printout("define_expand_depth:  ", define_expand_depth, "\n");

		if (define_expand_depth >= define_expand_max_depth)
		{
			we().err("Cannot resolve .defs!\n");
		}

		//printout("\n\n");
	}
}


void Assembler::split(std::vector<ParseNode>& ret, 
	std::vector<std::string>& to_split,
	std::vector<ParsePos>* pos_vec)
{
	size_t outer_index = 0, inner_index = 0;
	int some_next_char = ' ';
	PTok some_prev_tok = nullptr, some_next_tok = nullptr;
	std::string some_next_sym_str;
	s64 some_next_num = -1;
	size_t some_line_num = 0;

	while (some_next_tok != &Tok::Eof)
	{
		ParsePos pos;
		__lexer.__lex_innards(some_next_char, some_next_tok, some_prev_tok,
			some_next_sym_str, some_next_num, some_line_num, outer_index,
			inner_index, &to_split, &pos);

		if (pos_vec != nullptr)
		{
			pos_vec->push_back(pos);
		}

		if ((some_next_tok != &Tok::Newline)
			&& (some_next_tok != &Tok::Eof)
			&& !tok_is_comment(some_next_tok))
		{
			ret.push_back(ParseNode(some_next_tok,
				some_next_sym_str, some_next_num));
		}

		if (tok_is_comment(some_next_tok))
		{
			break;
		}
	}
}



bool Assembler::parse_instr(PInstr instr,
	const std::vector<ParseNode>& some_parse_vec)
{
	switch (instr->args())
	{
		case InstrArgs::no_args:
			return __parse_instr_no_args(some_parse_vec, 
				instr);
		case InstrArgs::uimm16:
			return __parse_instr_uimm16(some_parse_vec, 
				instr);
		case InstrArgs::simm16:
			return __parse_instr_simm16(some_parse_vec, 
				instr);
		case InstrArgs::imm32:
			return __parse_instr_imm32(some_parse_vec, 
				instr);

		case InstrArgs::ra:
			return __parse_instr_ra(some_parse_vec, 
				instr);
		case InstrArgs::ra_uimm16:
			return __parse_instr_ra_uimm16(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb:
			return __parse_instr_ra_rb(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb_uimm16:
			return __parse_instr_ra_rb_uimm16(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb_simm16:
			return __parse_instr_ra_rb_simm16(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb_rc:
			return __parse_instr_ra_rb_rc(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb_rc_simm12:
			return __parse_instr_ra_rb_rc_simm12(some_parse_vec, 
				instr);

		case InstrArgs::ldst_ra_rb:
			return __parse_instr_ldst_ra_rb(some_parse_vec, 
				instr);
		case InstrArgs::ldst_ra_rb_rc_simm12:
			return __parse_instr_ldst_ra_rb_rc_simm12(some_parse_vec, 
				instr);
		case InstrArgs::ldst_ra_rb_rc:
			return __parse_instr_ldst_ra_rb_rc(some_parse_vec, 
				instr);
		case InstrArgs::ldst_ra_rb_simm12:
			return __parse_instr_ldst_ra_rb_simm12(some_parse_vec, 
				instr);

		case InstrArgs::branch:
			return __parse_instr_branch(some_parse_vec,
				instr);


		case InstrArgs::ldst_ra_rb_imm32:
			return __parse_instr_ldst_ra_rb_imm32(some_parse_vec, 
				instr);
		case InstrArgs::ra_rb_imm32:
			return __parse_instr_ra_rb_imm32(some_parse_vec, 
				instr);


		// Block moves (ldmia, stmia, stmdb) with number of {} args
		case InstrArgs::ldst_block_1_to_4:
			return __parse_instr_ldst_block_1_to_4(some_parse_vec, 
				instr);
		case InstrArgs::ldst_block_5_to_8:
			return __parse_instr_ldst_block_5_to_8(some_parse_vec, 
				instr);

		case InstrArgs::ira:
			return __parse_instr_ira(some_parse_vec, 
				instr);
		case InstrArgs::ra_ira:
			return __parse_instr_ra_ira(some_parse_vec, 
				instr);
		case InstrArgs::ira_ra:
			return __parse_instr_ira_ra(some_parse_vec, 
				instr);

		case InstrArgs::ra_flags:
			return __parse_instr_ra_flags(some_parse_vec, 
				instr);
		case InstrArgs::flags:
			return __parse_instr_flags(some_parse_vec, 
				instr);
		case InstrArgs::flags_ra:
			return __parse_instr_flags_ra(some_parse_vec, 
				instr);

		case InstrArgs::ra_pc:
			return __parse_instr_ra_pc(some_parse_vec, 
				instr);
	}

	return false;
}

#define spvat(x) some_parse_vec.at(x)

bool Assembler::__parse_instr_no_args
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	//size_t index = 1;
	s64 expr_result = 0;

	// op
	if (some_parse_vec.size() != 1)
	{
		return false;
	}

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_uimm16
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op expr
	if (some_parse_vec.size() < 2)
	{
		return false;
	}

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_simm16
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op expr
	if (some_parse_vec.size() < 2)
	{
		return false;
	}

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_imm32
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op expr
	if (some_parse_vec.size() < 2)
	{
		return false;
	}

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

bool Assembler::__parse_instr_ra
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	//size_t index = 1;
	s64 expr_result = 0;


	// op rA
	if (some_parse_vec.size() != 2)
	{
		return false;
	}

	if (spvat(1).next_tok != &Tok::Reg)
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_uimm16
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , expr
	if (some_parse_vec.size() < 4)
	{
		return false;
	}


	if ((spvat(index++).next_tok != &Tok::Reg)
		|| (spvat(index++).next_tok != &Tok::Comma))
	{
		return false;
	}

	expr_result = handle_expr(some_parse_vec, index);

	regs.push_back(spvat(1).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma, 
		&Tok::Reg))
	{
		return false;
	}


	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);

	
	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb_uimm16
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB , expr
	if (some_parse_vec.size() < 6)
	{
		return false;
	}


	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::Reg, &Tok::Comma))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb_simm16
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB , expr
	if (some_parse_vec.size() < 6)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::Reg, &Tok::Comma))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb_rc
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB , rC
	if (some_parse_vec.size() != 6)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::Reg, &Tok::Comma, &Tok::Reg))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);
	regs.push_back(spvat(5).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb_rc_simm12
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB , rC , expr
	if (some_parse_vec.size() < 8)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::Reg, &Tok::Comma, &Tok::Reg, &Tok::Comma))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);
	regs.push_back(spvat(5).next_sym_str);


	expr_result = handle_expr(some_parse_vec, index);


	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

bool Assembler::__parse_instr_ldst_ra_rb
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , [ rB ]
	if (some_parse_vec.size() != 6)
	{
		return false;
	}


	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::LBracket, &Tok::Reg, &Tok::RBracket))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(4).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ldst_ra_rb_rc_simm12
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , [ rB , rC , expr ]
	if ((some_parse_vec.size() < 10)
		|| (some_parse_vec.back().next_tok != &Tok::RBracket))
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::LBracket, // [
		&Tok::Reg, &Tok::Comma, // rB ,
		&Tok::Reg, &Tok::Comma)) // rC ,
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(4).next_sym_str);
	regs.push_back(spvat(6).next_sym_str);

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ldst_ra_rb_rc
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , [ rB , rC ]
	if ((some_parse_vec.size() != 8)
		|| (some_parse_vec.back().next_tok != &Tok::RBracket))
	{
		return false;
	}


	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::LBracket, // [
		&Tok::Reg, &Tok::Comma, // rB ,
		&Tok::Reg, // rC
		&Tok::RBracket))  // ]
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(4).next_sym_str);
	regs.push_back(spvat(6).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ldst_ra_rb_simm12
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , [ rB , expr ]
	if ((some_parse_vec.size() < 8)
		|| (some_parse_vec.back().next_tok != &Tok::RBracket))
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::LBracket, // [
		&Tok::Reg, &Tok::Comma)) // rB ,
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(4).next_sym_str);

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}


bool Assembler::__parse_instr_branch
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op expr
	if (some_parse_vec.size() < 2)
	{
		return false;
	}

	expr_result = handle_expr(some_parse_vec, index) - addr();

	switch (instr->enc_group())
	{
		case 0:
			expr_result -= 2;
			break;

		case 1:
			expr_result -= 4;
			break;

		case 2:
			expr_result -= 4;
			break;

		case 3:
			expr_result -= 6;
			break;
	}


	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}


bool Assembler::__parse_instr_ldst_ra_rb_imm32
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , [ rB , expr ]
	if ((some_parse_vec.size() < 8)
		|| (some_parse_vec.back().next_tok != &Tok::RBracket))
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::LBracket, // [
		&Tok::Reg, &Tok::Comma)) // rB ,
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(4).next_sym_str);

	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_rb_imm32
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , rB , expr
	if (some_parse_vec.size() < 6)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::Reg, &Tok::Comma))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);
	regs.push_back(spvat(3).next_sym_str);


	expr_result = handle_expr(some_parse_vec, index);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}


// Block moves (ldmia, stmia, stmdb) with number of {} args
bool Assembler::__parse_instr_ldst_block_1_to_4
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , { rB }
	if (some_parse_vec.size() == 6)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
	}

	// op rA , { rB , rC }
	else if (some_parse_vec.size() == 8)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
	}
	// op rA , { rB , rC , rD }
	else if (some_parse_vec.size() == 10)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
	}
	// op rA , { rB , rC , rD , rE }
	else if (some_parse_vec.size() == 12)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);
		const auto node10 = spvat(index++);
		const auto node11 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::Comma)
			|| (node10.next_tok != &Tok::Reg)
			|| (node11.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
		regs.push_back(node10.next_sym_str);
	}
	else
	{
		return false;
	}

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ldst_block_5_to_8
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op rA , { rB , rC , rD , rE , rF }
	if (some_parse_vec.size() == 14)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);
		const auto node10 = spvat(index++);
		const auto node11 = spvat(index++);
		const auto node12 = spvat(index++);
		const auto node13 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::Comma)
			|| (node10.next_tok != &Tok::Reg)
			|| (node11.next_tok != &Tok::Comma)
			|| (node12.next_tok != &Tok::Reg)
			|| (node13.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
		regs.push_back(node10.next_sym_str);
		regs.push_back(node12.next_sym_str);
	}
	// op rA , { rB , rC , rD , rE , rF , rG }
	else if (some_parse_vec.size() == 16)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);
		const auto node10 = spvat(index++);
		const auto node11 = spvat(index++);
		const auto node12 = spvat(index++);
		const auto node13 = spvat(index++);
		const auto node14 = spvat(index++);
		const auto node15 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::Comma)
			|| (node10.next_tok != &Tok::Reg)
			|| (node11.next_tok != &Tok::Comma)
			|| (node12.next_tok != &Tok::Reg)
			|| (node13.next_tok != &Tok::Comma)
			|| (node14.next_tok != &Tok::Reg)
			|| (node15.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
		regs.push_back(node10.next_sym_str);
		regs.push_back(node12.next_sym_str);
		regs.push_back(node14.next_sym_str);
	}
	// op rA , { rB , rC , rD , rE , rF , rG , rH }
	else if (some_parse_vec.size() == 18)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);
		const auto node10 = spvat(index++);
		const auto node11 = spvat(index++);
		const auto node12 = spvat(index++);
		const auto node13 = spvat(index++);
		const auto node14 = spvat(index++);
		const auto node15 = spvat(index++);
		const auto node16 = spvat(index++);
		const auto node17 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::Comma)
			|| (node10.next_tok != &Tok::Reg)
			|| (node11.next_tok != &Tok::Comma)
			|| (node12.next_tok != &Tok::Reg)
			|| (node13.next_tok != &Tok::Comma)
			|| (node14.next_tok != &Tok::Reg)
			|| (node15.next_tok != &Tok::Comma)
			|| (node16.next_tok != &Tok::Reg)
			|| (node17.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
		regs.push_back(node10.next_sym_str);
		regs.push_back(node12.next_sym_str);
		regs.push_back(node14.next_sym_str);
		regs.push_back(node16.next_sym_str);
	}
	// op rA , { rB , rC , rD , rE , rF , rG , rH , rI }
	else if (some_parse_vec.size() == 20)
	{
		const auto node1 = spvat(index++);
		const auto node2 = spvat(index++);
		const auto node3 = spvat(index++);
		const auto node4 = spvat(index++);
		const auto node5 = spvat(index++);
		const auto node6 = spvat(index++);
		const auto node7 = spvat(index++);
		const auto node8 = spvat(index++);
		const auto node9 = spvat(index++);
		const auto node10 = spvat(index++);
		const auto node11 = spvat(index++);
		const auto node12 = spvat(index++);
		const auto node13 = spvat(index++);
		const auto node14 = spvat(index++);
		const auto node15 = spvat(index++);
		const auto node16 = spvat(index++);
		const auto node17 = spvat(index++);
		const auto node18 = spvat(index++);
		const auto node19 = spvat(index++);

		if ((node1.next_tok != &Tok::Reg)
			|| (node2.next_tok != &Tok::Comma)
			|| (node3.next_tok != &Tok::LBrace)
			|| (node4.next_tok != &Tok::Reg)
			|| (node5.next_tok != &Tok::Comma)
			|| (node6.next_tok != &Tok::Reg)
			|| (node7.next_tok != &Tok::Comma)
			|| (node8.next_tok != &Tok::Reg)
			|| (node9.next_tok != &Tok::Comma)
			|| (node10.next_tok != &Tok::Reg)
			|| (node11.next_tok != &Tok::Comma)
			|| (node12.next_tok != &Tok::Reg)
			|| (node13.next_tok != &Tok::Comma)
			|| (node14.next_tok != &Tok::Reg)
			|| (node15.next_tok != &Tok::Comma)
			|| (node16.next_tok != &Tok::Reg)
			|| (node17.next_tok != &Tok::Comma)
			|| (node18.next_tok != &Tok::Reg)
			|| (node19.next_tok != &Tok::RBrace))
		{
			return false;
		}

		regs.push_back(node1.next_sym_str);
		regs.push_back(node4.next_sym_str);
		regs.push_back(node6.next_sym_str);
		regs.push_back(node8.next_sym_str);
		regs.push_back(node10.next_sym_str);
		regs.push_back(node12.next_sym_str);
		regs.push_back(node14.next_sym_str);
		regs.push_back(node16.next_sym_str);
		regs.push_back(node18.next_sym_str);
	}
	else
	{
		return false;
	}

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

bool Assembler::__parse_instr_ira
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op ira
	if (some_parse_vec.size() != 2)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::RegIra))
	{
		return false;
	}

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ra_ira
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;


	// op rA , ira
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::RegIra))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_ira_ra
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op ira , rA
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::RegIra, &Tok::Comma,
		&Tok::Reg))
	{
		return false;
	}

	regs.push_back(spvat(3).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

bool Assembler::__parse_instr_ra_flags
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;


	// op rA , flags
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::RegFlags))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_flags
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op flags
	if (some_parse_vec.size() != 2)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::RegFlags))
	{
		return false;
	}

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}
bool Assembler::__parse_instr_flags_ra
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;

	// op flags , rA
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::RegFlags, &Tok::Comma,
		&Tok::Reg))
	{
		return false;
	}

	regs.push_back(spvat(3).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

bool Assembler::__parse_instr_ra_pc
	(const std::vector<ParseNode>& some_parse_vec, PInstr instr)
{
	std::vector<std::string> regs;
	size_t index = 1;
	s64 expr_result = 0;


	// op rA , pc
	if (some_parse_vec.size() != 4)
	{
		return false;
	}

	if (!check_tokens(some_parse_vec, index, &Tok::Reg, &Tok::Comma,
		&Tok::RegPc))
	{
		return false;
	}

	regs.push_back(spvat(1).next_sym_str);

	__codegen.encode_and_gen(regs, expr_result, instr);

	return true;
}

#undef spvat



s64 Assembler::handle_expr(const std::vector<ParseNode>& some_parse_vec, 
	size_t& index)
{
	////const auto old_next_tok = next_tok();
	//const auto old_next_tok = some_parse_vec.at(index).next_tok;

	//s64 ret;

	//if ((old_next_tok == &Tok::Plus) || (old_next_tok == &Tok::Minus))
	//{
	//	//lex();
	//	++index;

	//	if (old_next_tok == &Tok::Plus)
	//	{
	//		ret = handle_term(some_parse_vec, index);
	//	}
	//	else // if (old_next_tok == &Tok::Minus)
	//	{
	//		ret = -handle_term(some_parse_vec, index);
	//	}
	//}
	//else
	//{
	//	ret = handle_term(some_parse_vec, index);
	//}

	//if (index >= some_parse_vec.size())
	//{
	//	return ret;
	//}

	////if ((next_tok() == &Tok::Plus) || (next_tok() == &Tok::Minus))
	//if ((some_parse_vec.at(index).next_tok == &Tok::Plus)
	//	|| (some_parse_vec.at(index).next_tok == &Tok::Minus))
	//{
	//	ret += handle_expr(some_parse_vec, index);
	//}

	//return ret;




	s64 ret = 0;

	if (some_parse_vec.at(index).next_tok == &Tok::Minus)
	{
		//lex();
		++index;

		ret = -handle_term(some_parse_vec, index);
	}
	else if (some_parse_vec.at(index).next_tok == &Tok::Plus)
	{
		//lex();
		++index;

		ret = handle_term(some_parse_vec, index);
	}
	else if (tok_is_ident_ish(some_parse_vec.at(index).next_tok)
		|| (some_parse_vec.at(index).next_tok == &Tok::NatNum)
		|| (some_parse_vec.at(index).next_tok == &Tok::LParen))
	{
		ret = handle_term(some_parse_vec, index);
	}
	else if (some_parse_vec.at(index).next_tok == &Tok::Period)
	{
		ret = addr();
	}
	else
	{
		//we().err("expr():  3, Eek!\n");
		//return 9001;
		we().err("Invalid expression");
	}



	if (index >= some_parse_vec.size())
	{
		return ret;
	}

	while ((some_parse_vec.at(index).next_tok == &Tok::Plus) 
		|| (some_parse_vec.at(index).next_tok == &Tok::Minus))
	{
		//const bool minus = (next_tok == Tok::Minus);
		const bool minus = (some_parse_vec.at(index).next_tok 
			== &Tok::Minus);

		//lex();
		++index;

		if (minus)
		{
			ret -= handle_term(some_parse_vec, index);
		}
		else
		{
			ret += handle_term(some_parse_vec, index);
		}

		if (index >= some_parse_vec.size())
		{
			return ret;
		}
	}

	return ret;

}

s64 Assembler::handle_term(const std::vector<ParseNode>& some_parse_vec, 
	size_t& index)
{
	s64 ret = handle_factor(some_parse_vec, index);

	if (index >= some_parse_vec.size())
	{
		return ret;
	}

	//const auto some_next_tok = some_parse_vec.at(index).next_tok;

	while ((some_parse_vec.at(index).next_tok == &Tok::Mult) 
		|| (some_parse_vec.at(index).next_tok == &Tok::Div)
		|| (some_parse_vec.at(index).next_tok == &Tok::BitAnd) 
		|| (some_parse_vec.at(index).next_tok == &Tok::BitOr)
		|| (some_parse_vec.at(index).next_tok == &Tok::BitXor) 
		|| (some_parse_vec.at(index).next_tok == &Tok::BitShL) 
		|| (some_parse_vec.at(index).next_tok == &Tok::BitShR))
	{
		const auto old_next_tok = some_parse_vec.at(index).next_tok;
		//lex();
		++index;

		if (old_next_tok == &Tok::Mult)
		{
			ret *= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::Div)
		{
			ret /= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::BitAnd)
		{
			ret &= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::BitOr)
		{
			ret |= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::BitXor)
		{
			ret ^= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::BitShL)
		{
			ret <<= handle_factor(some_parse_vec, index);
		}
		else if (old_next_tok == &Tok::BitShR)
		{
			ret >>= handle_factor(some_parse_vec, index);
		}


		if (index >= some_parse_vec.size())
		{
			return ret;
		}
	}


	return ret;


}

s64 Assembler::handle_factor(const std::vector<ParseNode>& some_parse_vec, 
	size_t& index)
{
	if (index >= some_parse_vec.size())
	{
		we().err("Invalid expression!");
	}

	//if (next_tok() == &Tok::NatNum)
	if (some_parse_vec.at(index).next_tok == &Tok::NatNum)
	{
		//s64 ret = next_num();
		s64 ret = some_parse_vec.at(index).next_num;
		//lex();
		++index;
		return ret;
	}
	//else if (next_tok() == &Tok::Ident)
	//else if (next_tok_is_ident_ish())
	else if (tok_is_ident_ish(some_parse_vec.at(index).next_tok))
	{
		// This works because handle_factor() should only be called when
		// we're not asking for a user symbol
		const Symbol& sym = user_sym_tbl().at(some_parse_vec.at(index)
			.next_sym_str);
		s64 ret;

		switch (sym.type())
		{
			case SymType::Other:
			case SymType::EquateName:
				ret = sym.value();
				//lex();
				++index;
				break;

			case SymType::DefineName:
				//we().err("Can't use a define in an expression!");
				break;

			case SymType::MacroName:
				we().err("Can't use a macro in an expression!");
				break;

			default:
				we().err("handle_factor():  Eek!");
				break;
		}
		
		return ret;
	}

	s64 ret;

	//if (next_tok() != &Tok::LParen)
	if (some_parse_vec.at(index).next_tok != &Tok::LParen)
	{
		//expected("token of type \"", Tok::NatNum.str(), "\" or \"", 
		//	Tok::Ident.str(), "\" or \"", Tok::LParen.str(), "\"!");
		we().expected_tokens(&Tok::NatNum, &Tok::Ident, &Tok::LParen);
	}

	__lexer.need(some_parse_vec, index, &Tok::LParen);

	ret = handle_expr(some_parse_vec, index);

	__lexer.need(some_parse_vec, index, &Tok::RParen);

	return ret;
}






#define TOKEN_STUFF(varname, value) \
	else if (some_tok == &Tok::varname) \
	{ \
		return true; \
	}

bool Assembler::tok_is_punct(PTok some_tok) const
{
	if (some_tok == nullptr)
	{
	}

	LIST_OF_PUNCT_TOKENS(TOKEN_STUFF)

	return false;
}

bool Assembler::tok_is_ident_ish(PTok some_tok) const
{
	if (some_tok == nullptr)
	{
	}

	LIST_OF_IDENT_ISH_TOKENS(TOKEN_STUFF)

	return false;
}

bool Assembler::tok_is_comment(PTok some_tok) const
{
	if (some_tok == nullptr)
	{
	}

	LIST_OF_COMMENT_TOKENS(TOKEN_STUFF)

	return false;
}

#undef TOKEN_STUFF








}
