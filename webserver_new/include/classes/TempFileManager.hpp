#ifndef TEMPFILEMANAGER_HPP
# define TEMPFILEMANAGER_HPP

# include "./WebservException.hpp"
# include "../defined_value.hpp"
# include "../utility_function.hpp"
# include <set>

class TempFileManager {
	private:
		std::set<unsigned int> _tempFileSet;

		unsigned int	_nextGenNumber;

		bool isChildProcess;

	public:
		TempFileManager();
		~TempFileManager();

		// return the number to access this file manager
		unsigned int generateNewTempFile();


		void	removeTempFile(unsigned int tempfileNum);

		void setIsChild(bool op);
		bool getIsChild() const;
};

#endif
