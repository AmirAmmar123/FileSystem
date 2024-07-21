#include "vfs.h"
#include <fstream>
#include <iostream>
#include <sstream>

MyFs * VFS::_fs = nullptr;
json VFS::_data;
std::string VFS::_bd_file_name;


std::vector<std::string> VFS::split_cmd(const std::string& cmd, char delim ) {
	std::stringstream ss(cmd);
	std::string part;
	std::vector<std::string> ans;

	while (std::getline(ss, part, delim)) {
		ans.push_back(part);
	}
	return ans;
}


void VFS::read_json_file(){
	std::string json_filename = VFS::_bd_file_name + JSON;  // Append '.json' to the filename

	// Attempt to open the file
	std::ifstream file(json_filename);

	if (file.is_open()) {
		// File exists, read JSON data from it
		file >> VFS::_data;
		file.close();
	} else {
		// File doesn't exist, create it and initialize with default data
		std::ofstream new_file(json_filename);  // This will create the file

		// Initialize with default JSON data if needed
		_data = {
			{"/", {
	                {"type", "directory"},
					{"contents", json::object()}
					}

			},
			{"offset",MyFs::get_header_size()}
		};

		// Save JSON data to the new file
		new_file << _data.dump(4);  // Using dump(4) for pretty printing with indentation
		new_file.close();
	}
}

void VFS::write_json_file() {
	const std::string json_filename = VFS::_bd_file_name + JSON;  // Append '.json' to the filename

	// Attempt to open the file
	if (std::ofstream file(json_filename); file.is_open()) {
		// File exists, write JSON data to it
		file << _data.dump(4);  // Use -> to access pointer member _data
		file.close();
	} else {
		throw std::runtime_error("Failed to open file: " + json_filename);
	}
}



void VFS::init(MyFs &fs, const std::string & fname) {
	VFS::_bd_file_name = fname;
	VFS::_fs = &fs;
	VFS::read_json_file();
	VFS::_fs->set_json_data(VFS::_data);

}


void VFS::run(MyFs &fs, const std::string & fname ) {
	init(fs, fname);
	std::cout << "Welcome to " << FS_NAME << std::endl;
	std::cout << "To get help, please type 'help' on the prompt below." << std::endl;
	std::cout << std::endl;

    bool exit = false;
	while (!exit) {
		try {
			std::string cmdline;
			std::cout << FS_NAME << "$ ";
			std::getline(std::cin, cmdline, '\n');
			if (cmdline == std::string(""))
				continue;
			std::vector<std::string> cmd = split_cmd(cmdline, ' ');
			exit = check_and_activate(cmd[0], cmd);
		} catch (std::runtime_error &e) {
			std::cout << e.what() << std::endl;
		}
	}
}



bool VFS::check_and_activate(const std::string & COMMAND, const std::vector<std::string> & cmd) {
	// Add the relevant calls to MyFs object in these ifs
	if (COMMAND == EXIT_CMD) {
		return true;
	}if (COMMAND== HELP_CMD) {
		std::cout << HELP_STRING;
	} else if (COMMAND == LIST_CMD ) {
		if (cmd.size() == 1) {
			VFS::_fs->list_dir("/");
		}else {
			for (unsigned long i = 1; i < cmd.size(); ++i) {
				VFS::_fs->list_dir(cmd[i]);
			}
		}
	} else if (COMMAND == CREATE_FILE_CMD) {
		for (unsigned long i = 1; i < cmd.size(); ++i) {
			VFS::_fs->create_file(cmd[i], false);
		}
	}else if(COMMAND == CREATE_DIRECTORY_CMD){
		for (unsigned long i = 1; i < cmd.size(); ++i) {
			VFS::_fs->create_file(cmd[i], true);
		}
	} else if (COMMAND == CONTENT_CMD){

		if(cmd.size() != 2 ) throw std::runtime_error("cat command usage, cat <file>");

		std::cout << VFS::_fs->get_content(cmd[1]) << std::endl;

	} else if (COMMAND== EDIT_CMD) {

		if(cmd.size() != 2 ) throw std::runtime_error("Edite command usage, edite <file>");

		VFS::_fs->set_content(cmd[1]);

	} else if (COMMAND== REMOVE_CMD) {
		for (unsigned long i = 1; i < cmd.size(); ++i) {
			VFS::_fs->remove_file(cmd[i]);
		}

	}else if(COMMAND == RMDIR){
		for (unsigned long i = 1; i < cmd.size(); ++i) {
			VFS::_fs->remove_dir(cmd[i]);
		}
	}
	else {

		std::cout << "unknown command: " << cmd[0] << std::endl;

	}
	return false;
}

