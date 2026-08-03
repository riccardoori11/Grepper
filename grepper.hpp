#include <queue>
#include "MemoryMap.hpp"
#include <filesystem>
#include <thread>

/*Give all the files where the word is located in */

namespace fs = std::filesystem;

class grepper{


private:

std::queue<std::mutex> jobs;




public:

grepper(const fs::path& file){
}


void Open(const fs::path path){

		MemoryMap file{path};

		auto current = file.Data().data();
		auto end = current + file.Data().size();

		std::string f(current,static_cast<std::size_t>(end - current));

		std::cout << f << std::endl;
}

};
