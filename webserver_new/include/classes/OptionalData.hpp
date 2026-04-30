#ifndef OPTIONAL_DATA_HPP
# define OPTIONAL_DATA_HPP



template <typename T>
/*
I think this class should need a little bit more explanation

I'd been thinking about this. and come one with new idea.
because some checking infromation i always need a variable to
determine if which data is already have or not because some data
is just struct or int or char and there is no possible way to 
tell whether that variable actually has some information or not unless
i dedicated-ly purposely create new boolean variable to tell if
this data has some information. And I noticed that i use this kind of
method a lot in this project. And from what i searched from google, 
the C++17 or newer version has this kind of features but c++98 doesn't
have that yet so
*/
class OptionalData
{
	/* template function need to implement in header*/
	private:
		T		*_data;

	public:


		OptionalData() : _data(NULL)
		{
		}

		OptionalData(const T& obj)
		{
			_data = new T(obj);
		}

		OptionalData(const OptionalData& obj)
		{
			if (obj._data != NULL)
				_data = new T(*(obj._data));
		}

		~OptionalData()
		{
			clear();
		}

		OptionalData& operator=(const OptionalData& obj)
		{
			if (this != &obj)
			{
				if (obj._data == NULL)
					clear()
				else
				{
					if (_data == NULL)
						_data = new T(*(obj._data));
					else
						*_data = *(obj._data);
				}
			}

			return (*this);
		}

		OptionalData& operator=(const T& obj)
		{
			if (this->_data != &obj)
			{
				if (_data == NULL)
					_data = new T(obj);
				else
					*_data = obj;
			}

			return (*this);
		}

		T* operator->()
		{
			if (_data == NULL)
				_data = new T();
			return (_data);
		}

		const T* operator->() const
		{
			if (_data == NULL)
				_data = new T();
			return (_data);
		}

		T& operator*()
		{
			if (_data == NULL)
				_data = new T();
			return (*_data);
		}

		const T& operator*() const
		{
			if (_data == NULL)
				_data = new T();
			return (*_data);
		}

		operator T() const
		{
			if (_data == NULL)
				_data = new T();
			return (*_data);
		}

		bool hasData() const
		{
			return (_data == NULL ? false : true);
		}

		void clear()
		{
			if (_data != NULL)
			{
				delete _data;
				_data = NULL;
			}
		};

};

template <typename T>
class OptionalData<T*>
{
	/* template function need to implement in header*/
	private:
		T		**_data;

	public:


		OptionalData() : _data(NULL)
		{
		}

		OptionalData(const T* obj)
		{
			_data = new T*(obj);
		}

		OptionalData(const OptionalData& obj)
		{
			if (obj._data != NULL)
				_data = new T(*(obj._data));
		}

		~OptionalData()
		{
			clear();
		}

		OptionalData& operator=(const OptionalData& obj)
		{
			if (this != &obj)
			{
				if (obj._data == NULL)
					clear()
				else
				{
					if (_data == NULL)
						_data = new T*(*(obj._data));
					else
						*_data = *(obj._data);
				}
			}

			return (*this);
		}

		OptionalData& operator=(const T* obj)
		{
			if (this->_data != &obj)
			{
				if (_data == NULL)
					_data = new T*(obj);
				else
					*_data = obj;
			}

			return (*this);
		}

		T* operator->()
		{
			if (_data == NULL)
				_data = T*();
			return (*_data);
		}

		const T* operator->() const
		{
			if (_data == NULL)
				_data = T*();
			return (*_data);
		}

		T& operator*()
		{
			if (_data == NULL)
				_data = T*();
			return (*_data);
		}

		const T& operator*() const
		{
			if (_data == NULL)
				_data = T*();
			return (*_data);
		}

		operator T*() const
		{
			if (_data == NULL)
				_data = T*();
			return (*_data);
		}

		bool hasData() const
		{
			return (_data == NULL ? false : true);
		}

		void clear()
		{
			if (_data != NULL)
			{
				delete _data;
				_data = NULL;
			}
		};

};

#endif