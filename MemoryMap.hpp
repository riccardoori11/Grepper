#include <iostream>
#include <span>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;
class MemoryMap{

private:
		static constexpr int INVALID_DESCRIPTOR{-1};
		int descriptor{INVALID_DESCRIPTOR};
		std::size_t size{};
		char* data{nullptr};


public:


		MemoryMap(const fs::path& path){

				int descriptor = open(path.c_str(),O_RDONLY);

				if (descriptor == INVALID_DESCRIPTOR){

						throw std::runtime_error("CANNOT OPEN FILE");
				}

				struct stat stats{}; 

				if (fstat(descriptor,&stats) == INVALID_DESCRIPTOR){

						throw std::runtime_error("Could not fstat");
				}

				size = static_cast<std::size_t>(stats.st_size);

				void* mapping = mmap(nullptr,size,PROT_READ,MAP_PRIVATE,descriptor,0);

				if (mapping == MAP_FAILED){

						throw std::runtime_error("Could not mmap");
				}

				data = static_cast<char*>(mapping);

				madvise(data, size,MADV_SEQUENTIAL);
		}

		std::span<const char> Data(){

				return {data,size};
		}

};
