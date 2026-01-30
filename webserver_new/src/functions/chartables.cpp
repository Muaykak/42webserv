#include "../../include/utility_function.hpp"

// Frequently used tables

const CharTable&	whiteSpaceTable(){
	static const CharTable table(" \t\v\n\f\r\0", true);
	return (table);
}

// horizontal tab and ' '(space) 
const CharTable&	htabSp()
{
	static const CharTable table(" \t", true);
	return (table);
}

const CharTable&	allowedFieldNameChar()
{
	static const CharTable table("qazwsxedcrfvtgbyhnujmiklopQAZWSXEDCRFVTGBYHNUJMIKLOP1234567890!#$%&\'*+-.^_`|~", true);
	return (table);
}

static std::string forbiddenControlCharOnFieldValue()
{
	std::string returnStr;
	returnStr.reserve(32);
	for (char c = 0; c < 32; c++)
	{
		// vertical tab is allowed
		if (c == '\v')
			continue;
		returnStr.append(1, c);
	}
	returnStr.append("\177");
	return (returnStr);
}

const CharTable&	forbiddenFieldValueChar()
{
	static const CharTable table(forbiddenControlCharOnFieldValue(), true);
	return (table);
}
