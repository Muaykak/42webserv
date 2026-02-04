#include "../../include/classes/ResponseBuffer.hpp"

ResponseBuffer::ResponseBuffer()
: currentIndex(0)
{

}
ResponseBuffer::ResponseBuffer(const ResponseBuffer& obj)
: currentIndex(obj.currentIndex),
buffer(obj.buffer)
{

}
ResponseBuffer& ResponseBuffer::operator=(const ResponseBuffer& obj)
{
	if (this != &obj)
	{
		currentIndex = obj.currentIndex;
		buffer = obj.buffer;
	}
	return (*this);
}
ResponseBuffer::~ResponseBuffer()
{
}
