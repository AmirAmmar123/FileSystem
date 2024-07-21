#ifndef MYFS_H_
#define MYFS_H_
#include <iostream>
#include <vector>
#include "blkdev.h"
#include "json.hpp"

using json = nlohmann::json;

class MyFs {
public:
	explicit MyFs(BlockDeviceSimulator *blkdevsim_);

	/**
	 * format method
	 * This function discards the current content in the blockdevice and
	 * create a fresh new MYFS instance in the blockdevice.
	 */
	void format() const;

	/**
	 * create_file method
	 * Creates a new file in the required path.
	 * @param path_str the file path (e.g. "/newfile")
	 * @param directory boolean indicating whether this is a file or directory
	 */
	void create_file(const std::string& path_str, bool directory) const;

	/**
	 * get_content method
	 * Returns the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 * @return the content of the file
	 */
	[[nodiscard]] std::string get_content(const std::string& path_str) const;

	/**
	 * set_content method
	 * Sets the whole content of the file indicated by path_str param.
	 * Note: this method assumes path_str refers to a file and not a
	 * directory.
	 * @param path_str the file path (e.g. "/somefile")
	 */
	void set_content(const std::string& path_str) const;

   /**
	 * list_dir method
	 * Returns a list of a files in a directory.
	 * Note: this method assumes path_str refers to a directory and not a
	 * file.
	 * @param path_str the file path (e.g. "/somedir")
	 * @return a vector (you need to change the return type in the function declaration)
	 */
	void list_dir(const std::string &path_str) const;

	/**
	 * Removes a file from the filesystem at the specified path.
	 *
	 * @param path_str The path of the file to be removed. The path should be a string
	 *                 representing the file's location within the filesystem.
	 *                 For example: "/somedir/somefile".
	 * @param rm_dir_mode
	 *
	 * @return void - This function does not return any value. It only removes the file
	 *                from the filesystem.
	 *
	 * @throws std::runtime_error - If there is an error while attempting to remove the file,
	 *                             this exception will be thrown.
	 *
	 * @note This method assumes that the provided path_str refers to an existing file.
	 *       If the file does not exist, or if the path_str is not a valid file path,
	 *       an exception will be thrown.
	 */
	void remove_file(const std::string& path_str);


	/**
	 * Removes a directory from the filesystem at the specified path.
	 *
	 * @param path_str The path of the directory to be removed. The path should be a string
	 *                 representing the directory's location within the filesystem.
	 *                 For example: "/somedir".
	 *
	 * @return void - This function does not return any value. It only removes the directory
	 *                from the filesystem.
	 *
	 * @throws std::runtime_error - If there is an error while attempting to remove the directory,
	 *                             this exception will be thrown.
	 *
	 * @note This method assumes that the provided path_str refers to an existing directory.
	 *       If the directory does not exist, or if the path_str is not a valid directory path,
	 *       an exception will be thrown.
	 *
	 * @note This method does not remove the directory if it is not empty. It will throw an
	 *       exception in such cases.
	 */
	void remove_dir(const std::string& path_str);

	void recursive_delete(json * current, const std::string& path_str, std::vector<std::string> & paths);


	void set_json_data(json & data) {
		_data = &data;
	}

	/**
	 * This function returns the size of the 'myfs_header' struct in bytes.
	 * The 'myfs_header' struct represents the first bytes of a myfs filesystem.
	 * It holds some magic characters and a number indicating the version.
	 *
	 * @return The size of the 'myfs_header' struct in bytes.
	 *
	 * @note This function is used to calculate the required size for allocating memory
	 *       when reading or writing the 'myfs_header' struct from/to the block device.
	 */
	static int get_header_size() {
		return sizeof(myfs_header);
	}

	~MyFs() {
		// this was added, because blkdevsim was allocated on the heap
		delete blkdevsim;
	}


private:

	/**
	 * This struct represents the first bytes of a myfs filesystem.
	 * It holds some magic characters and a number indicating the version.
	 * Upon class construction, the magic and the header are tested - if
	 * they both exist than the file is assumed to contain a valid myfs
	 * instance. Otherwise, the blockdevice is formated and a new instance is
	 * created.
	 */
	struct myfs_header {
		char magic[4];
		uint8_t version;
	};

	/**
	* @brief Traverses the JSON structure to find the specified file or directory.
	 *
	 * @param current A pointer to the current JSON object in the structure.
	 * @param tokens A vector of strings representing the path to the file or directory.
	 *
	 * @return A pointer to the JSON object representing the file or directory.
	 *
	 * @throws std::runtime_error If the file or directory is not found in the JSON structure.
	 *
	 * @details This function iterates through the JSON structure, starting from the given
	 * current JSON object, and traverses the structure based on the tokens in the path.
	 * It checks if each token corresponds to a valid directory or file in the structure.
	 * If a valid directory or file is found, the function updates the current JSON object
	 * pointer to point to the found object and continues traversing.
	 * If the end of the tokens is reached, the function returns the current JSON object pointer.
	 * If a token does not correspond to a valid directory or file in the structure, a
	 * std::runtime_error is thrown.
	 */
	static json * traverse(json * current, const std::vector<std::string> & tokens);

	/**
	 * @brief Builds a path vector to traverse the JSON structure.
	 *
	 * This function constructs a path vector from the input tokens, excluding the last token.
	 * The constructed path vector is then used to traverse the JSON structure.
	 *
	 * @param current A pointer to the current JSON object in the structure.
	 * @param tokens A vector of strings representing the path to be traversed.
	 *
	 * @return A pointer to the JSON object at the end of the constructed path vector.
	 *
	 * @note The function does not modify the original JSON structure.
	 */
	static json * get_parent(json * current, const std::vector<std::string> & tokens);

	/**
	 * @brief Builds a JSON object representing a file or directory structure.
	 *
	 * @param tokens A vector of strings representing the path tokens.
	 * @param directory A boolean indicating whether the item to be created is a directory.
	 * @param from_root A pointer to the JSON object representing the root directory.
	 * @param current A pointer to the JSON object representing the current directory.
	 *
	 * @details This function constructs a JSON object representing a file or directory structure.
	 *          If the size of the `tokens` vector is 1, it creates the item directly under the `current` directory.
	 *          If the size of the `tokens` vector is greater than 1, it creates the item under the `from_root` directory.
	 *          If the `directory` parameter is true, it creates a directory; otherwise, it creates a file.
	 *          The function sets the "type" field to "directory" or "file", and initializes the "contents" field for directories.
	 *          For files, it sets the "begin" and "end" fields to -1, indicating that the file content is not yet allocated.
	 *
	 * @return void
	 */
	static void build_json(const std::vector<std::string>& tokens, bool directory, json * from_root, json * current);

	/**
	 * Resizes the block device based on the edited file content.
	 *
	 * @param current A pointer to the JSON object representing the file to be edited.
	 * @param content The new content for the file.
	 * @param buffer A buffer containing the new content.
	 *
	 * @throws std::runtime_error If the new size of the file exceeds the block device size.
	 *
	 * @note This function adjusts the 'begin' and 'end' values for all files in the JSON structure
	 *       where the 'begin' value is greater than the current 'begin' value of the edited file.
	 *       It also writes the new content to the block device and updates the 'begin' and 'end'
	 *       values of the edited file in the JSON structure.
	 */
		void resize_bd(json * current, const std::string & content, const char * buffer) const;

	/**
	 * @brief Helper function to recursively traverse JSON and adjust 'begin' and 'end' values.
	 *
	 * @details This function is used to adjust the 'begin' and 'end' values of files in the JSON structure
	 *          when a file is deleted or its content is modified. It ensures that the 'begin' and 'end'
	 *          values of all subsequent files are updated accordingly.
	 *
	 * @param node The JSON node to be traversed.
	 * @param origin_begin The original 'begin' value of the file being modified or deleted.
	 * @param chunk_to_cut_from_begin_and_end The number of steps to reduce from each 'begin' and 'end'
	 *                                         of a file, which is equal to the size of the file being
	 *                                         modified or deleted.
	 *
	 * @return void.
	 */
	static void adjust_offsets(json& node, int origin_begin, int chunk_to_cut_from_begin_and_end);

	BlockDeviceSimulator *blkdevsim;


	json * _data{};
	static const uint8_t CURR_VERSION = 0x03;
	static const char *MYFS_MAGIC;
};

#endif // MYFS_H_
