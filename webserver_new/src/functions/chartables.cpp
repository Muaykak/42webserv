#include "../../include/utility_function.hpp"

// Frequently used tables

const CharTable&	whiteSpaceTable(){
	static const CharTable table(" \t\v\n\f\r", true);
	return (table);
}

// horizontal tab and ' '(space) 
const CharTable&	htabSp()
{
	static const CharTable table(" \t", true);
	return (table);
}
// big case alphabets table
const CharTable&	alphaAtoZ()
{
	static const CharTable table("ABCDEFGHIJKLMNOPQRSTUVWXYZ", true);
	return (table);
}

const CharTable&	hexChar()
{
	static const CharTable table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", true);
	return (table);
}

const CharTable&	allowedFieldNameChar()
{
	static const CharTable table("qazwsxedcrfvtgbyhnujmiklopQAZWSXEDCRFVTGBYHNUJMIKLOP1234567890!#$%&\'*+-.^_`|~", true);
	return (table);
}

const CharTable&	allowRequestTargetChar()
{
	static const CharTable table("qazwsxedcrfvtgbyhnujmiklopQAZWSXEDCRFVTGBYHNUJMIKLOP0123456789-._~!$&\'()*+,;=/?:@%", true);
	return (table);
}

static std::string forbiddenControlCharOnFieldValue()
{
	std::string returnStr;
	returnStr.reserve(32);
	for (unsigned char c = 0; c < 32; c++)
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

