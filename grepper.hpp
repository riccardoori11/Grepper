#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <algorithm>
#include "MemoryMap.hpp"
#include <filesystem>
#include <string_view>
#include <thread>

/*Give all the files where the word is located in */

/*each worker has its own vector*/

namespace fs = std::filesystem;

class grepper{


private:

std::queue<fs::path> jobs;
const fs::path file;
/*From my machine*/
constexpr static std::size_t thread_count = 8;
std::condition_variable jobs_available;
std::size_t unfinished_jobs{};
std::mutex job_mtx;
std::mutex asnwer_mutex;
std::optional<fs::path> result;
std::atomic<bool> found{false};



public:

grepper(const fs::path& file):file(file){}


bool SearchWord(const fs::path& file, const std::string_view word){

		MemoryMap path{file};

		std::string_view s(path.Data().data(),path.get_size());

		auto match = s.find(word);

		if (match == std::string_view::npos){

				std::cout << "Not found" << std::endl;
				return false;
		}
		std::cout << "Found " << std::endl;
		return true;


}

auto AddJob(const fs::path& path){

{
		std::unique_lock<std::mutex> lock(job_mtx);
		jobs.push(path);
		++unfinished_jobs;
}
		jobs_available.notify_one();
}


std::optional<fs::path>takeJob(){

		std::unique_lock<std::mutex> lock(job_mtx);

		jobs_available.wait(lock,[&]{

				return !jobs.empty() || found.load() || unfinished_jobs == 0;
						});


}

		

};
