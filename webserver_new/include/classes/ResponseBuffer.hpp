#ifndef RESPONSEBUFFER_HPP
# define RESPONSEBUFFER_HPP
# include "../defined_value.hpp"

class ResponseBuffer {
	public:
		ResponseBuffer();
		ResponseBuffer(const ResponseBuffer& obj);
		ResponseBuffer& operator=(const ResponseBuffer& obj);
		~ResponseBuffer();

		size_t	currentIndex;
		std::vector<char>	buffer;
};

#endif
