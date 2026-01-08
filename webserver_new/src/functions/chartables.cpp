#include "../../include/utility_function.hpp"

// Frequently used tables

const CharTable&	whiteSpaceTable(){
	static const CharTable table(" \t\v\n\f\r", true);

	return (table);
}

const CharTable&	linearWhiteSpaceTable(){
	static const CharTable table(" \t", true);

	return (table);
}
