/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Util.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bworrawa <bworrawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/04 14:35:29 by bworrawa          #+#    #+#             */
/*   Updated: 2025/03/19 17:15:18 by bworrawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef		UTIL_HPP
# define 	UTIL_HPP 
# include 	<iostream>
# include 	<vector>
# include	<sys/stat.h>
class Util
{
	public: 
		static	std::string trim(std::string&);
		static	std::string trim(std::string);
		static	std::string	toString(int i);
		static	std::string	toString(size_t i);
		static	std::string	replaceAll(std::string src, std::string const &from , std::string const &to);

		static	int toInt(std::string);
		static	size_t toSizeT(std::string);

		static 	int	substrPos(std::string const haystack, std::string const needle);
		static	size_t charactersMatched(std::string find, std::string path);
		static  std::string	 extractFileName(std::string const path, bool containsDot=true);
		static 	std::string  getFileExtension(std::string const &filePath);

		static  bool	 hasTrailingSlash(std::string const path);


		static  bool 	 strInContainer(std::string const &str, std::vector<std::string> v);
		static  std::vector<std::string> 	split(std::string &string, std::string &del);

		static  std::string					vectorCharToString(std::vector<char> &v);	

		static 		bool	createFile(std::string fullPath , std::string::iterator it, size_t len);
		static		bool	fileExists(std::string const &fullPath);
		static		bool	fileHasPermission(std::string const &fullPath);
};

#endif
