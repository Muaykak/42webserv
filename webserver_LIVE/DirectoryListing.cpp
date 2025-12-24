#include "DirectoryListing.hpp"

std::string generateDirectoryListing(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (dir == NULL)
    {
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return "";
    }

    std::ostringstream html;
    html << "<html><head><title>Directory Listing</title></head><body>" << std::endl;
    html << "<h1>Directory Listing for " << path << "</h1>" << std::endl;
    html << "<ul>" << std::endl;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            html << "<li><a href=\"" << name << "\">" << name << "</a></li>" << std::endl;
        }
    }

    html << "</ul>" << std::endl;
    html << "</body></html>" << std::endl;

    closedir(dir);
    return html.str();
}

int main() {
    std::string path = ".";
    std::string htmlContent = generateDirectoryListing(path);
    std::cout << htmlContent << std::endl;
    return 0;
}