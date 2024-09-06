#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <iconv.h>


#ifdef _WIN32
#    include <windows.h>
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;

int list_encoding(unsigned int count, const char *const *names, void *data)
{
    auto *encodings = static_cast<std::vector<std::string> *>(data);
    for (unsigned int i = 0; i < count; i++)
    {
        if (encodings)
        {
            encodings->push_back(names[i]);
        }
        else
        {
            std::cout << names[i] << " ";
        }
    }
    if (!encodings)
    {
        std::cout << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    std::string inputFile;
    std::string outputDirectory;
    std::string encoding;
    bool        overwrite = false;

    auto currentPath = fs::absolute(fs::current_path()).string();

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")("list-encodings,l", "list encodings")(
        "encoding,e", po::value<std::string>(&encoding)->default_value("utf-8"), "set encoding")(
        "output,o", po::value<std::string>(&outputDirectory)->default_value(currentPath), "output directory")(
        "input,i", po::value<std::string>(&inputFile), "input a .zip file path")(
        "overwrite,f", po::bool_switch(&overwrite), "overwrite existing files");

    po::options_description hidden("Hidden options");
    hidden.add_options()("input-file", po::value<std::string>(&inputFile), "input file");

    po::options_description allOptions;
    allOptions.add(desc).add(hidden);

    po::positional_options_description pod;
    pod.add("input-file", 1);

    po::variables_map varMap;
    po::store(po::command_line_parser(argc, argv).options(allOptions).positional(pod).run(), varMap);
    po::notify(varMap);
    if (varMap.count("help"))
    {
        std::cout << "Simplest UnArchiver with encoding settings supported. Home page: https://github.com/missdeer/sunar\n" << desc << std::endl;
        return 0;
    }

    if (varMap.count("list-encodings"))
    {
        std::cout << "All supported encodings:" << std::endl;
        iconvlist(&list_encoding, nullptr);
        return 0;
    }

    std::vector<std::string> encodings;
    iconvlist(&list_encoding, &encodings);
    if (std::none_of(encodings.begin(), encodings.end(), [&](const std::string &enc) { return boost::algorithm::to_upper_copy(encoding) == enc; }))
    {
        std::cerr << "Unsupported encoding: " << encoding << std::endl;
        return 1;
    }

    fs::path inputPath(inputFile);
    if (!fs::exists(inputPath))
    {
        std::cerr << "Input file does not exist: " << inputFile << std::endl;
        return 1;
    }
    // extract zip file with libarchive

    auto *pArchive = archive_read_new();
    archive_read_support_filter_all(pArchive);
    archive_read_support_format_all(pArchive);
    archive_read_set_options(pArchive, ("hdrcharset=" + encoding).c_str());
    auto res = archive_read_open_filename(pArchive, inputFile.c_str(), 10240);
    if (res != ARCHIVE_OK)
    {
        std::cerr << "Failed to open archive: " << inputFile << std::endl;
        return 1;
    }
    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(pArchive, &entry) == ARCHIVE_OK)
    {
        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            continue;
        }
        const auto *entryPath = archive_entry_pathname_w(entry);

        // write entry to output directory
        fs::path outputPath(outputDirectory);
        outputPath /= entryPath;
        if (fs::exists(outputPath) && !overwrite)
        {
            std::cout << "File already exists: " << outputPath.string() << std::endl;
            continue;
        }
        fs::create_directories(outputPath.parent_path());
        std::ofstream ofs(outputPath.string(), std::ios::binary);
        if (!ofs)
        {
            std::cerr << "Failed to create file: " << outputPath.string() << std::endl;
            continue;
        }

        const void *buff;
        size_t      size;
        la_int64_t  offset;

        while (archive_read_data_block(pArchive, &buff, &size, &offset) == ARCHIVE_OK)
        {
            ofs.write(static_cast<const char *>(buff), size);
        }
        ofs.close();
        std::cout << "Extracted file: " << outputPath.string() << std::endl;
    }
    res = archive_read_free(pArchive);

    return 0;
}
