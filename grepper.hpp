#include <optional>
#include <queue>
#include <algorithm>
#include "MemoryMap.hpp"
#include <filesystem>
#include <string_view>
#include <thread>

/*Give all the files where the word is located in */

namespace fs = std::filesystem;

class grepper{


private:

std::queue<std::mutex> jobs;
const fs::path file;


public:

grepper(const fs::path& file):file(file){}


auto SearchWord(const std::string_view word){

		MemoryMap path(file);


		std::string_view s(path.Data().data(),path.get_size());

		auto match = s.find(word);

		if (match == std::string_view::npos){

				std::cout << "Not found" << std::endl;
				return;
		}
		std::cout << "Found " << std::endl;
		return;


}

};
