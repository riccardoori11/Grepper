#include <exception>
#include <assert.h>
#include <functional>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <iterator>
#include <memory>
#include <optional>
#include <ratio>
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
#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using namespace std::chrono;

ricc::thread_safe_queue<fs::path> a{};

std::size_t thread_count = std::thread::hardware_concurrency();

auto root = "/home/riccardo";


using Path_Maybe = std::optional<std::size_t>;

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




bool is_dotFile(auto& entry){
		return entry.path().filename().string().starts_with('.');
}

void producer(){

		fs::recursive_directory_iterator it(root);
		fs::recursive_directory_iterator end;

		try{
		for (; it != end; ++it){

				auto entry = *it;
				if (is_dotFile(entry)){
						if (entry.is_directory()){}
								it.disable_recursion_pending();
								continue;
				}

				if (entry.is_regular_file() && entry.path().extension() == ".txt"){

						a.push(entry.path());
				}
		}
		}
		catch(const std::exception& error){

				std::cerr << error.what() << std::endl;
		}
		a.close();

}


constexpr bool is_letter(char c){

		return (c >= 'a'&& c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}


constexpr bool is_beginning_line(std::size_t pos){

		return pos == 0;
}

Path_Maybe parse(std::span<const char> bytes, std::string_view txt){

		auto begin = bytes.data();
		auto end = begin + bytes.size();

		std::string_view text(begin, end);

		std::size_t search_from{};

		auto txt_length = txt.size();
		while (true){
				auto position = text.find(txt,search_from);
				if (position == std::string_view::npos){

						return std::nullopt;
				}

						auto start_word = bytes.data() + position;
						const char* prev;
						bool beginning_line = false;
						if (is_beginning_line(position)){
								prev = start_word;
								beginning_line = true;
						}else{
								prev = start_word - 1;
						}
						auto next = start_word + txt_length;
						if (beginning_line){

								if (!is_letter(*next)){

										return position;
								}
						}
						if (!is_letter(*prev) && !is_letter(*next)){
								return position;
						}
						
				search_from = position + 1;
		}
		return std::nullopt;

}

/*
 * Add multiple threads, each thread will do their work then merge them at the end and vector pool
 * */




std::vector<fs::path> files;
void consumer(){

		fs::path path;

		std::string_view txt = "hello";

		while ( auto path = a.wait_and_pop()){

				try {
				MemoryMap file(*path);
				
				auto parsed = parse(file.Data(), txt);

				if (parsed){

						files.push_back(*path);
				}
				}
				catch(...){

						continue;
				}
				
		}


}


void Test(fs::path& path,std::string_view txt){

		MemoryMap a{path};
		parse(a.Data(),txt);
}

template<typename Callable>
auto Time(Callable&& c){

		auto start = steady_clock::now();

		std::invoke(std::forward<Callable>(c));

		auto end = steady_clock::now();

		auto time = duration_cast<std::chrono::milliseconds>( end - start ).count();

		return time;

}

auto multithreaded_version(){

		std::thread t1(producer);
		std::thread t2(consumer);

		t1.join();
		t2.join();

}

std::vector<fs::path> files_single{};

auto singleThreadedVersion(){

		std::string_view txt = "hello";

		try{

				fs::recursive_directory_iterator it{root};
				fs::recursive_directory_iterator end;


				for (; it != end; ++it){
						auto entry = *it;
						if (is_dotFile(entry)){

								if (entry.is_directory()){

										it.disable_recursion_pending();
										continue;
								}
						}

						if (entry.is_regular_file() && entry.path().extension() == ".txt"){
								try{
								MemoryMap file(entry.path());
								auto parsed = parse(file.Data(),txt);

								if (parsed){

										files_single.push_back(entry);
								}
								}
						catch(...){

								continue;
						}
						}
				}
		}

		catch(const std::exception& error){

				std::cerr << error.what() << std::endl;
		}

}



int main(){

		auto time = Time(multithreaded_version);
		auto Time2 = Time(singleThreadedVersion);

		std::cout << time << "ms" << std::endl;
		std::cout << Time2 << "ms" << std::endl;

		assert(files_single == files);
		

/*
		std::string_view txt = "dslkjdslkajdlkjo3p1ie0921i12903091";
		fs::path path = "m.txt";
		Test(path,"dslkjdslkajdlkjo3p1ie0921i12903091");

		*/

		return 0;
}
