#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include "thread_safe_queue.hpp"
#include <filesystem>

namespace fs = std::filesystem;

ricc::thread_safe_queue<fs::path> a{};

std::size_t thread_count = std::thread::hardware_concurrency();

auto root = "/home/riccardo";

std::vector<fs::path> files;

class MemoryMap{

private:

		constexpr static int INVALID_DESCRIPTOR{-1};
		int descriptor{INVALID_DESCRIPTOR};
		std::size_t size_{};
		const fs::path& path;
		char* data{nullptr};

public:

		/* make mmap RAII*/
		
		MemoryMap(const fs::path& path):path(path){

				descriptor = open(path.c_str(),O_RDONLY);
				if (descriptor == INVALID_DESCRIPTOR){

						throw std::runtime_error("Failed to open file");
				}
				struct stat stats{};
				if (fstat(descriptor,&stats) == INVALID_DESCRIPTOR){

						throw std::runtime_error("Failed to fstat");
				}

				size_ = static_cast<std::size_t>(stats.st_size);

				void* mapping = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, descriptor,0);

				if (mapping == MAP_FAILED){
						close(descriptor);
						throw std::runtime_error("Could not mmap");
				}

				data = static_cast<char*>(mapping);

				madvise(data,size_,MADV_SEQUENTIAL);
		}

		bool isOpen(){
				return descriptor != INVALID_DESCRIPTOR;
		}

		std::span<const char> Data() const{
				return {data,size_};
		}

		~MemoryMap(){

				if (isOpen() && data != nullptr){
						close(descriptor);
						munmap(data, size_);
				}
		}

};


void takeWorker(){

}


void producer(){

		for (auto& entry: fs::recursive_directory_iterator(root)){

				if (entry.is_regular_file() && entry.path().extension() == ".txt"){

						a.push(entry.path());
						std::cout << "Pushed entry" << entry.path() << std::endl;
				}
		}

}

using Path_Maybe = std::optional<std::size_t>;
Path_Maybe parse(std::span<const char> bytes, std::string_view txt){

		auto begin = bytes.data();
		auto end = begin + bytes.size();

		std::string_view text(begin, end);

		std::size_t position = text.find(txt);

		if (position != std::string::npos){

				auto prev = position - 1;
				auto next = position + 1;

				if (prev == ' ' && next == ' '){
						std::cout << "Found" << std::endl;
						return position;
				}
		}
		return std::nullopt;

}

void consumer(){

		fs::path path;

		std::string_view txt = "Hello";

		while (auto path = a.wait_and_pop()){

				MemoryMap file(*path);
				/* parse whatever 
				 * when each thread finishes their own local work merge, also make sure that 2 threads dont work on the same file*/
				parse(file.Data(), txt);
		}

}

void Test(fs::path& path,std::string_view txt){

		MemoryMap a{path};
}


int main(){

		std::cout << "" << std::endl;

		std::string_view txt = "Hello";

		Test("m.txt", txt);


		return 0;
}
