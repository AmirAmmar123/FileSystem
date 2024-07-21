#ifndef __VFS_H__
#define __VFS_H__
#include <vector>
#include "json.hpp"
#include "myfs.h"

using json = nlohmann::json;

const std::string FS_NAME = "myfs";
const std::string RMDIR = "rmdir";
const std::string LIST_CMD = "ls";
const std::string CONTENT_CMD = "cat";
const std::string CREATE_FILE_CMD = "touch";
const std::string CREATE_DIRECTORY_CMD = "mkdir";
const std::string EDIT_CMD = "edit";
const std::string REMOVE_CMD = "rm";
const std::string HELP_CMD = "help";
const std::string EXIT_CMD = "exit";


const std::string HELP_STRING = "The following commands are supported: \n"
    + LIST_CMD + " [<directory>] - list directory content. \n"
    + CONTENT_CMD + " <path> - show file content. \n"
    + CREATE_FILE_CMD + " <path> - create empty file. \n"
    + CREATE_DIRECTORY_CMD + " <path> - create empty directory. \n"
    + EDIT_CMD + " <path> - re-set file content. \n"
    + REMOVE_CMD + " <path> - remove file. \n"
    + RMDIR + " <path> - remove directory. \n"
    + HELP_CMD + " - show this help messege. \n"
    + EXIT_CMD + " - gracefully exit. \n";


const std::string JSON = ".json";

class VFS {
public:
    /**
     * @brief Runs the Virtual File System (VFS) with the provided MyFs object and file name.
     *
     * This function initializes the VFS with the provided MyFs object and file name, displays a welcome message,
     * and enters a command loop where the user can interact with the VFS. The command loop reads user input,
     * splits it into individual commands, and activates the corresponding functionality in the MyFs object.
     *
     * @param fs A reference to the MyFs object that will be associated with the VFS.
     * @param fname A const reference to a string representing the file name for the JSON data.
     *
     * @return void
     */
    static void run(MyFs &fs, const std::string & fname);


    static std::vector<std::string>  split_cmd(const std::string& cmd, char delim = ' ');

    static void write_json_file();
private:
    static bool check_and_activate(const std::string & COMMAND, const std::vector<std::string> & cmd);

    /**
     * @brief Reads JSON data from a file and initializes the VFS::_data variable.
     *
     * This function attempts to open the file specified by the provided filename. If the file exists,
     * it reads the JSON data from the file and stores it in the VFS::_data variable. If the file doesn't exist,
     * it creates a new file, initializes it with default JSON data, and stores it in the VFS::_data variable.
     *
     * @param filename A const reference to a string representing the name of the file from which to read JSON data.
     *                  The function appends '.json' to the filename before attempting to open the file.
     *
     * @return void
     */
    static void read_json_file();

    /**
     * @brief Initializes the Virtual File System (VFS) with the provided MyFs object and file name.
     *
     * This function sets the file name for the VFS, assigns the address of the provided MyFs object to the VFS::_fs,
     * reads the JSON data from the file specified by fname, and sets the JSON data in the MyFs object.
     *
     * @param fs A reference to the MyFs object that will be associated with the VFS.
     * @param fname A const reference to a string representing the file name for the JSON data.
     *
     * @return void
     */
    static void init(MyFs &fs, const std::string & fname);


    static MyFs * _fs;
    static json _data;
    static std::string _bd_file_name;
};


#endif // __VFS_H__
