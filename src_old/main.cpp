//#include "lexer_class.hpp"
#include "assembler_class.hpp"
#include "get_copy_of_stdin.hpp"

//std::FILE* copy_of_stdin = get_copy_of_stdin();
navichip32::assembler the_assembler( 0, nullptr, 
	navichip32::get_copy_of_stdin() );

int main( int argc, char** argv )
{
	//std::FILE* copy_of_stdin = navichip32::get_copy_of_stdin();
	//navichip32::assembler the_assembler( argc, argv, stdin );
	
	
	
	return the_assembler.run();
}