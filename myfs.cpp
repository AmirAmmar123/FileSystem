#include "myfs.h"
#include <cstring>
#include <iostream>
#include <algorithm>
#include "vfs.h"

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) {
	myfs_header header{};
	blkdevsim->read(0, sizeof(header), reinterpret_cast<char *>(&header));

	/*
	* Compare characters of two strings
	  Compares up to num characters of the C string str1 to those of the C string str2.
	  This function starts comparing the first character of each string. If they are equal to each other,
	  it continues with the following pairs until the characters differ, until a terminating null-character is reached,
	  or until num characters match in both strings, whichever happens first.

				Return Value
			Returns an integral value indicating the relationship between the strings:
			return value	indicates
			<0	the first character that does not match has a lower value in str1 than in str2
			0	the contents of both strings are equal
			>0	the first character that does not match has a greater value in str1 than in str2

	 */
	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}

}

void MyFs::format() const {
	// put the header in place
	myfs_header header{};

	/*
	  char * strncpy ( char * destination, const char * source, size_t num );
	  Copy characters from string
		Copies the first num characters of source to destination.
		If the end of the source C string (which is signaled by a null-character) is found before num characters have been copied,
		destination is padded with zeros until a total of num characters have been written to it.

		No null-character is implicitly appended at the end of destination if source is longer than num.
		Thus, in this case, destination shall not be considered a null terminated C string (reading it as such would overflow).

		destination and source shall not overlap (see memmove for a safer alternative when overlapping).
	 */
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), reinterpret_cast<const char *>(&header));
}


json * MyFs::traverse(json * current, const std::vector<std::string> & tokens) {
	for (const auto& token : tokens) {
		if (token.empty()) continue; // Skip empty tokens (could happen with leading '/')
		if (current->contains("contents") && (*current)["contents"].contains(token)) {
			current = &(*current)["contents"][token];
		} else {
			throw std::runtime_error("File or directory not found");
		}
	}
	return current;

}

json * MyFs::get_parent(json *current, const std::vector<std::string> & tokens) {
	std::vector<std::string> path_vector;
	path_vector.reserve(tokens.size()-1);
	path_vector.resize(tokens.size()-1);
	std::copy(tokens.begin(), tokens.end()-1,path_vector.begin());
	return  traverse(current, path_vector);
}

void MyFs::build_json(const std::vector<std::string> &tokens, const bool directory, json * from_root, json * current) {
	if (tokens.size() == 1) {
		if (directory) {
			(*current)["contents"][tokens[0]] = {
				{"type", "directory"},
				{"contents", json::object()}  // Empty directory
			};
		} else {
			(*current)["contents"][tokens[0]] = {
				{"type", "file"},
				{"begin", -1 },  // Example: Setting file begin offset
				{"end", -1 }     // Example: Setting file end offset
			};
		}
	}else {
		if (directory) {
			(*from_root)["contents"][tokens.back()] = {
				{"type", "directory"},
				{"contents", json::object()}  // Empty directory
			};
		} else {
			(*from_root)["contents"][tokens.back()] = {
				{"type", "file"},
				{"begin", -1 },  // Example: Setting file begin offset
				{"end", -1 }     // Example: Setting file end offset
			};
		}
	}
}

void MyFs::create_file(const std::string& path_str, bool directory) const {
	// Start from the root directory
	json* current = &(*_data)["/"];  // Assuming the root directory is named "root"
	json* from_root = &(*_data)["/"];

	// Split path_str into tokens
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');

	if(tokens.empty()) throw std::runtime_error("usage command touch <file>");

	if( tokens.size() != 1) {
		from_root = get_parent(from_root, tokens);
	}



	if ((*from_root)["type"] == "directory" && (*from_root)["contents"].contains(tokens.back())) {
		if(!directory)
			throw std::runtime_error("File already exists");
		throw std::runtime_error("Directory already exists");

	}

	// If only the root path is provided, create the file or directory directly
	build_json(tokens, directory, from_root,current);

	VFS::write_json_file();

}

std::string MyFs::get_content(const std::string& path_str) const {
	json* current = &(*_data)["/"];
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');

	current = traverse(current, tokens);

	// Check if the path refers to a file
	if ((*current)["type"] != "file") {
		throw std::runtime_error("Path does not refer to a file");
	}

	const int begin = (*current)["begin"];
	const int end = (*current)["end"];
	if(begin == -1 || end == -1) return ""; // content is empty

	const int size = end - begin + 1 ;
	char * ans = new char[size];
	ans[size] = '\0';
 	blkdevsim->read(begin, size,ans);
	std::string a = ans;
	delete [] ans;
	return a;
}

void MyFs::set_content(const std::string& path_str) const {
	// Start from the root directory
	json * current = &(*_data)["/"];
	int offsetValue = (*_data)["offset"];
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');

	// Traverse the path to locate the file
	current = traverse(current, tokens);

	std::string content;
	std::cout << "Enter new file content" <<std::endl;
	std::getline(std::cin,content);

	// Check if the path refers to a file
	if ((*current)["type"] != "file") {
		throw std::runtime_error("Path does not refer to a file");
	}

	int begin = (*current)["begin"];
	int end = (*current)["end"];
	int current_size = end-begin+1;

	// Allocate buffer and copy content
	char* buffer = new char[content.size()+1];
	buffer[content.size()] = '\0';

	std::strncpy(buffer, content.c_str(), content.size());

	if ( (begin == -1 || end == -1) ) {

		// if the beginning or the end is equal to -1, this file has 0 charchters
		blkdevsim->write(offsetValue + 1,static_cast<int>(content.size()), buffer);
		(*current)["begin"] = offsetValue + 1;
		(*current)["end"] = offsetValue + content.size();
		(*_data)["offset"]= offsetValue + content.size() ;

    }else if( static_cast<int>(content.size()) == (current_size)){
    	// if the size of the content is equal to the current size, this file there is no need to resize the block device
    	blkdevsim->write(offsetValue + 1,static_cast<int>(content.size()), buffer);
    }else {
		resize_bd(current, content, buffer);
    }
	delete[] buffer;
	VFS::write_json_file();
}



void MyFs::adjust_offsets(json &node, int origin_begin, int chunk_to_cut_from_begin_and_end) {
	// Check if the node is a directory
	if (node.contains("type") && node["type"] == "directory") {
		// Iterate through the contents of the directory
		for (auto& item : node["contents"].items()) {
			adjust_offsets(item.value(), origin_begin, chunk_to_cut_from_begin_and_end);
		}
	} else if (node.contains("type") && node["type"] == "file") {
		// Adjust the 'begin' and 'end' values for files
		if (node.contains("begin") && node["begin"].get<int>() > origin_begin) {
			node["begin"] = node["begin"].get<int>() - chunk_to_cut_from_begin_and_end;
			node["end"] = node["end"].get<int>() - chunk_to_cut_from_begin_and_end;
		}
	}
}




void MyFs::resize_bd(json * current, const std::string & content, const char * buffer) const {
	// initlize the data
	json * root = &(*_data)["/"];
	const int origin_begin = (*current)["begin"];
	const int origin_end = (*current)["end"];
	const int chunk_to_cut_from_begin_and_end = origin_end - origin_begin + 1; // chunk off steps to reduce from each begin and end of a file, that it's begin bigger than the edited file
	const int chunk_to_cut_from_offset = chunk_to_cut_from_begin_and_end; // size of chunk is equal to #steps, block_device offset needs to go back
	const int current_offset = (*_data)["offset"];


	// if the edited file end is not equal to the current device offset, make
	if(origin_end != current_offset) {
		(*_data)["offset"] = current_offset - chunk_to_cut_from_offset;
	}

	// if we editing a file with new data that exceeds the block_device size, throw exception
	// offset_ptr - steps_to_go_back  +  new_data_added > Block Device Size
	if ( ((current_offset - chunk_to_cut_from_begin_and_end) + static_cast<int>(content.size())) > BlockDeviceSimulator::DEVICE_SIZE)
		throw std::runtime_error("Cannot resize the file: the new size exceeds the block device size");

    // if the file to be edited end equal to the block device offset, we need to treat this file in a different way
	// we need either to squeez the end or to extend it
	if(origin_end == current_offset) {
		const bool is_bigger = chunk_to_cut_from_begin_and_end < static_cast<int>(content.size());
		const int differ = !is_bigger?
			                   chunk_to_cut_from_begin_and_end - content.size():content.size() - chunk_to_cut_from_begin_and_end;
		blkdevsim->write(origin_begin , static_cast<int>(content.size()), buffer);
		(*current)["end"] = !is_bigger?origin_end - differ:origin_end + differ;
		(*_data)["offset"] = !is_bigger?origin_end - differ:origin_end + differ;

	} else {
		// first of all we need to adjust all files begin and end where begin > the current begin edited_file
		adjust_offsets(*root, origin_begin, chunk_to_cut_from_begin_and_end);

		int block_device_begin_to_copy = origin_end + 1; // copy begin from here
		int size_chunck_to_copy_in_block_device = current_offset - block_device_begin_to_copy + 1;
		char *ans = new char [size_chunck_to_copy_in_block_device];

		// copy all the data above the end of the current edited file to the block device offset
		blkdevsim->read(block_device_begin_to_copy, size_chunck_to_copy_in_block_device, ans); // copy

		// paster it to the begining of the begin of the current file to be edited
		blkdevsim->write(origin_begin, size_chunck_to_copy_in_block_device, ans); // paste

		delete [] ans;
		const int current_new_offset = (*_data)["offset"];

		// add the edited file new data to the end of the block_device offset
		blkdevsim->write(current_new_offset + 1, static_cast<int>(content.size()), buffer);
		(*current)["begin"] = current_new_offset + 1 ; // new location begin
		(*current)["end"] = current_new_offset + content.size(); // new location end
		(*_data)["offset"] =  current_new_offset + content.size(); // new location for the offset
	}
}

void MyFs::list_dir(const std::string &path_str) const {
	// Start from the root directory
	json* current = &(*_data)["/"];
	if(path_str == "/") {
		for (auto& item : (*current)["contents"].items()) {
			if((*current)["contents"][item.key()]["type"] == "file") {
				const int  begin =  (*current)["contents"][item.key()]["begin"];
				const int  end =  (*current)["contents"][item.key()]["end"];
				if(begin == -1 || end == -1) {
					std::cout << item.key() << '\t' <<  0 << std::endl;
				}else {
					std::cout << item.key() << '\t' <<  end-begin+1 << std::endl;
				}
			}else {
				std::cout << item.key() << std::endl;
			}

		}
		return;
	}
	// Traverse the path
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');
	current = traverse(current,tokens);

	// Check if it's a directory
	if ((*current)["type"] != "directory") {
		throw std::runtime_error("Path does not refer to a directory");
	}

	// List the contents of the directory
	for (auto& item : (*current)["contents"].items()) {
		if((*current)["contents"][item.key()]["type"] == "file") {
			const int  begin =  (*current)["contents"][item.key()]["begin"];
			if(const int end =  (*current)["contents"][item.key()]["end"]; begin == -1 || end == -1) {
				std::cout << item.key() << '\t' <<  0 << std::endl;
			}else {
				std::cout << item.key() << '\t' <<  end-begin+1 << std::endl;
			}
		}else {
			std::cout << item.key() << std::endl;
		}
	}
}

void MyFs::remove_file(const std::string &path_str ){
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');
	json * current = &(*_data)["/"];
	json * root = &(*_data)["/"];
	// Locate the file in the JSON structure
	current = traverse(current, tokens);

	// Check if the path refers to a file
	if ((*current)["type"] != "file") {
		throw std::runtime_error("Path does not refer to a file");
	}

	const int origin_begin = (*current)["begin"];
	const int origin_end = (*current)["end"];
	const int chunk_to_cut_from_begin_and_end = origin_end - origin_begin + 1; // chunk off steps to reduce from each begin and end of a file, that it's begin bigger than the edited file
	const int chunk_to_cut_from_offset = chunk_to_cut_from_begin_and_end; // size of chunk is equal to #steps, block_device offset needs to go back
	const int current_offset_blkdev = (*_data)["offset"];

	// Remove the file entry from the JSON structure
	json * parent;
	if(tokens.size() > 1) {
		parent = get_parent(root, tokens);
	}else {
		parent = root;
	}


	if(origin_begin == -1 || origin_end == -1) {
		parent->at("contents").erase(tokens.back());
		VFS::write_json_file();
		return;
	}

	// resize offset
	(*_data)["offset"] = current_offset_blkdev - chunk_to_cut_from_offset;

	if(origin_end != current_offset_blkdev) {
		adjust_offsets(*root, origin_begin, chunk_to_cut_from_begin_and_end);

		const int block_device_begin_to_copy = origin_end + 1; // copy begin from here

		const int size_chunck_to_copy_in_block_device = current_offset_blkdev - block_device_begin_to_copy + 1;

		// buffer to read
		char *ans = new char [size_chunck_to_copy_in_block_device];

		// copy all the data followed by the end of the current edited file up to the block device offset
		blkdevsim->read(block_device_begin_to_copy, size_chunck_to_copy_in_block_device, ans); // copy

		// paste it to the begining of the current file to be edited
		blkdevsim->write(origin_begin, size_chunck_to_copy_in_block_device, ans); // paste

		delete [] ans;
	}else {
		// just re-locate the block device ptr to the beginning of the deleted file, the data will be overwritten;
		// this will automatically happen, no need to do any changes
	}

	parent->at("contents").erase(tokens.back());

	// Write changes to the JSON file

	VFS::write_json_file();

}

void MyFs::recursive_delete(json *current, const std::string& path_str,  std::vector<std::string>  & paths)  {
	for (auto& item : (*current)["contents"].items()) {
		if (item.value()["type"] == "file") {
			// If it's a file, remove it using remove_file
			std::string file_path;
			if(path_str != "/") {
				file_path = path_str + "/" + item.key();
			}else {
				file_path = path_str + item.key();
			}
			paths.push_back(file_path);
		} else if (item.value()["type"] == "directory") {
			// If it's a directory, recursively delete its contents
			std::string dir_path;
			if(path_str != "/") {
				dir_path = path_str + "/" + item.key();
			}else {
				dir_path = path_str + item.key();
			}
			recursive_delete(&item.value(), dir_path, paths);

		}
	}
}

void MyFs::remove_dir(const std::string &path_str){
	const std::vector<std::string> tokens = VFS::split_cmd(path_str, '/');
	json *current = &(*_data)["/"];
	json *root = &(*_data)["/"];
	json *parent ;

	// Locate the directory in the JSON structure
	current = traverse(current, tokens);

	// Check if the path refers to a directory
	if (!current || (*current)["type"] != "directory") {
		throw std::runtime_error("Path does not refer to a directory");
	}

	// Recursively delete the directory contents

	std::vector<std::string> paths;
    recursive_delete(current, path_str,paths);

	for (const auto & file_path : paths) {
		remove_file(file_path);
	}


    // Erase the directory entry from its parent
    if (tokens.size() > 1) {
        parent = get_parent(root, tokens);
    } else {
        parent = root;
    }

    if (parent) {
        parent->at("contents").erase(tokens.back());
    }

    // Write changes to the JSON file
    VFS::write_json_file();
}