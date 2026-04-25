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

	public:

		bool	hasData;
		T		data;

		OptionalData() : hasData(false)
		{
		}

		OptionalData(const OptionalData& obj): hasData(obj.data), data(obj.data)
		{

		}
		OptionalData(const T& obj): hasData(true), data(T)
		{

		}

		~OptionalData()
		{
		}

};

#endif