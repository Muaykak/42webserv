#include "../../include/utility_function.hpp"

// Frequently used tables

const CharTable&	whiteSpaceTable(){
	static const CharTable table(" \t\v\n\f\r", true);
	return (table);
}

// just 0-9 and '.'
const CharTable&	hostipChar()
{
	static const CharTable table("0123456789.", true);
	return (table);
}

const CharTable&	digitChar()
{
	static const CharTable table("0123456789", true);
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
	static const CharTable table("ABCDEFabcdef0123456789", true);
	return (table);
}

const CharTable& allAlphaChar()
{
	static const CharTable table("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", true);
	return (table);
}

const CharTable& httpFieldNameChar()
{
	static const CharTable table("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-", true);
	return (table);
}

const CharTable&	httpTokenChar()
{
	static const CharTable table("qazwsxedcrfvtgbyhnujmiklopQAZWSXEDCRFVTGBYHNUJMIKLOP1234567890!#$%&\'*+-.^_`|~", true);
	return (table);
}

const CharTable&	httpContentTypeChar()
{
	static const CharTable table("/qazwsxedcrfvtgbyhnujmiklopQAZWSXEDCRFVTGBYHNUJMIKLOP1234567890!#$%&\'*+-.^_`|~", true);
	return (table);
}

static std::string quotePairStr()
{
	std::string tempStr;

	unsigned char c = 0;
	while (c < 255)
	{
		if ( (c >= 32 && c <= 126) || c == '\t' || c >= 0x80)
			tempStr += c;
		c++;
	}
	if ( (c >= 32 && c <= 126) || c == '\t' || c >= 0x80)
		tempStr += c;
	return (tempStr);
}
const CharTable& httpQuotedPairAllowedChar()
{
	static const CharTable table(quotePairStr(), true);
	return (table);
}

static std::string allowedQuotedChar()
{
	std::string tempStr;

	unsigned char c = 0;
	while (c < 255)
	{
		if (c == '\t' || c == ' ' || c == 0x21 || (c >= 0x23 && c <= 0x5b) || (c >= 0x5d && c <= 0x7e) || c >= 0x80)
			tempStr += c;
		c++;
	}
	if (c == '\t' || c == ' ' || c == 0x21 || (c >= 0x23 && c <= 0x5b) || (c >= 0x5d && c <= 0x7e) || c >= 0x80)
		tempStr += c;

	return (tempStr);
}
const CharTable& httpAllowedQuotedChar()
{
	static const CharTable table(allowedQuotedChar(), true);
	return (table);
}

static std::string allowedCommentChar()
{
	std::string tempStr;

	unsigned char c = 0;

	while (c < 255)
	{
		if (c == '\t' || c == ' ' || (c >= 0x21 && c <= 0x27) || (c >= 0x2a && c <= 0x7e) || c >= 0x80)
			tempStr += c;
		c++;
	}
	if (c == '\t' || c == ' ' || (c >= 0x21 && c <= 0x27) || (c >= 0x2a && c <= 0x7e) || c >= 0x80)
		tempStr += c;

	return (tempStr);
}
const CharTable& httpAllowedCommentChar()
{
	static CharTable table(allowedCommentChar(), true);
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
		// vertical tab is allowed || ifk if horizontal tab is allowed so 
		if (c == '\t')
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

