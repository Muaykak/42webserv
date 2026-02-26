#include "../../include/classes/ContentTypeTable.hpp"

const ContentTypeTable& contentTypeTable()
{
	static const ContentTypeTable table;

	return (table);
}