#include <string>

/* 0 = not found*/
/* 1 = partial found */
/* 2 = found whole key */
int my_find(const std::string& toSearch, const std::string& key, size_t& foundPos)
{
	if (toSearch.empty() || key.empty())
		return (0);

	size_t i = toSearch.find_first_of(key[0], 0);
	size_t j;
	size_t k;
	while (i < toSearch.size() && i != std::string::npos)
	{
		j = 0;
		k = i;
		while (j < key.size() && k < toSearch.size() && key[j] == toSearch[k])
		{
			++j;
			++k;
		}

		if (j >= key.size())
		{
			foundPos = i;
			return (2);
		}
		if (k >= toSearch.size())
		{
			foundPos = i;
			return (1);
		}
		++i;
		i = toSearch.find_first_of(key[0], i);
	}
	return (0);
}