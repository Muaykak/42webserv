#ifndef SHARED_HPP
# define SHARED_HPP

template <typename T>
class Shared
{
	private:
		size_t *_ref_count;
		T	*_data;

		void release()
		{
			if (_ref_count != NULL)
			{
				if (*_ref_count <= 1)
				{
					delete _ref_count;
					delete _data;
				}
				else
					--(*_ref_count);
				_data = NULL;
				_ref_count = NULL;
			}
		};

	public:
		Shared()
		{
			_data = new T();
			
			try 
			{
				_ref_count = new size_t(1);
			}
			catch (...)
			{
				delete _data;
				throw;
			}
		};

		Shared(const Shared& obj) : _ref_count(obj._ref_count), _data(obj._data)
		{
			if (_ref_count != NULL)
				++(*_ref_count);
		}

		Shared(const T& obj)
		{
			_data = new T(obj);

			try {
				_ref_count = new size_t(1);
			}
			catch (...)
			{
				/* bad alloc here */
				delete _data;
				throw ;
			}
		}

		Shared& operator=(const Shared& obj)
		{
			if (this != &obj)
			{
				release();

				_data = obj._data;
				_ref_count = obj._ref_count;
				if (_ref_count != NULL)
					++(*_ref_count);
			}
			return (*this);
		}

		Shared& operator=(const T& obj)
		{
			if (_data != &obj)
			{
				if (_ref_count == NULL)
				{
					_data = new T(obj);

					try
					{
						_ref_count = new size_t(1);
					}
					catch (...)
					{
						delete _data;
						throw;
					}
				}
				else
				{
					try {
						*_data = obj;
					}
					catch (...)
					{
						release();
						throw ;
					}
				}
			}
			return (*this);
		}

		~Shared()
		{
			release();
		}

		T& operator*()
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

		const T& operator*() const
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

		T* operator->()
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (_data);
		}

		const T* operator->() const
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (_data);
		}

		operator T() const
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

};

template <typename T>
class Shared<T *>
{
	private:
		size_t *_ref_count;
		T	**_data;

		void release()
		{
			if (_ref_count != NULL)
			{
				if (*_ref_count <= 1)
				{
					delete _ref_count;
					delete _data;
				}
				else
					--(*_ref_count);
				_data = NULL;
				_ref_count = NULL;
			}
		};

	public:
		Shared()
		{
			_data = new T*();
			
			try 
			{
				_ref_count = new size_t(1);
			}
			catch (...)
			{
				delete _data;
				throw;
			}
		};

		Shared(const Shared& obj) : _ref_count(obj._ref_count), _data(obj._data)
		{
			if (_ref_count != NULL)
				++(*_ref_count);
		}

		Shared(const T* obj)
		{
			_data = new T*(obj);

			try {
				_ref_count = new size_t(1);
			}
			catch (...)
			{
				/* bad alloc here */
				delete _data;
				throw ;
			}
		}

		Shared& operator=(const Shared& obj)
		{
			if (this != &obj)
			{
				release();

				_data = obj._data;
				_ref_count = obj._ref_count;
				if (_ref_count != NULL)
					++(*_ref_count);
			}
			return (*this);
		}

		Shared& operator=(const T* obj)
		{
			if (_data != &obj)
			{
				if (_ref_count == NULL)
				{
					_data = new T*(obj);

					try
					{
						_ref_count = new size_t(1);
					}
					catch (...)
					{
						delete _data;
						throw;
					}
				}
				else
				{
					try {
						*_data = obj;
					}
					catch (...)
					{
						release();
						throw ;
					}
				}
			}
			return (*this);
		}

		~Shared()
		{
			release();
		}

		T& operator*()
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

		const T& operator*() const
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

		operator T*() const
		{
			if (_data == NULL)
				throw std::runtime_error("Shared::access NULL member data.");
			return (*_data);
		}

};

#endif
