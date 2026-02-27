#include "pch.h"
#include "Loadthread.h"
#include <chrono>
#include <fstream>
#include <ren/Logging.h>
#include <ren/FileHandle.hpp>
#include <ren/ArchiveGTA3.h>
#include <filesystem>
#include <ren/Exceptions.hpp>
#include <ren/Stringtools.hpp>

Loadthread::Loadthread() :
	p_lowestUnusedID(0), p_usedIDs(), p_usedIDsMutex(),
	p_running(true), p_jobQueue(), p_loopCheckMutex(),
	p_threadWakeup(),
	p_resultMap(), p_resultMapMutex(),
	p_failMap(), p_failMapMutex(),
	p_loadThread(&Loadthread::loadLoop, this)
{

}

Loadthread::~Loadthread()
{
	terminate();
	
	for (auto it = p_resultMap.begin(); it != p_resultMap.end(); it++) {
		delete it->second;
	}
}

int32_t Loadthread::enqueueFile(const char* _filename, ren::MDType _type, ren_process_flags processFlags)
{
	LoadJob job = { getID(), _type, _filename, processFlags };

	{
		std::lock_guard<std::mutex> lock(p_loopCheckMutex);
		p_jobQueue.push_back(job);
	}
	p_threadWakeup.notify_one();
	
	std::lock_guard<std::mutex> lock(p_processingSetMutex);
	p_processingSet.insert(job.handle);
	return job.handle;
}

void Loadthread::clearQueue()
{
	std::unique_lock<std::mutex> lock(p_loopCheckMutex);
	p_jobQueue.clear();
}

size_t Loadthread::queueSize()
{
	std::unique_lock<std::mutex> lock(p_loopCheckMutex);
	return p_jobQueue.size();
}

Loadthread::ResultType Loadthread::getItem(int32_t _id)
{
	std::unique_lock<std::mutex> lock(p_resultMapMutex);
	auto it = p_resultMap.find(_id);
	if (it != p_resultMap.end()) {
		return it->second;
	}
	else {
		return nullptr;
	}
}

bool Loadthread::freeItem(int32_t _id)
{
	std::unique_lock<std::mutex> lock(p_resultMapMutex);
	auto it = p_resultMap.find(_id);
	if (it != p_resultMap.end()) {
		delete it->second;
		p_resultMap.erase(it);
		lock.unlock();
		returnID(_id);
		return true;
	}
	else {
		lock.unlock();
		std::unique_lock<std::mutex> lock(p_failMapMutex);
		auto it = p_failMap.find(_id);
		if (it != p_failMap.end()) {
			p_failMap.erase(it);
			return true;
		}
	}
	return false;
}

size_t Loadthread::resultCount()
{
	std::lock_guard<std::mutex> lock(p_resultMapMutex);
	return p_resultMap.size();
}

bool Loadthread::isProcessing(int32_t _id)
{
	std::lock_guard<std::mutex> lock(p_processingSetMutex);
	auto it = p_processingSet.find(_id);
	if (it != p_processingSet.end()) {
		return true;
	}
	else {
		return false;
	}
}

bool Loadthread::isFinished(int32_t _id)
{
	std::lock_guard<std::mutex> lock(p_resultMapMutex);
	auto it = p_resultMap.find(_id);
	if (it != p_resultMap.end()) {
		return true;
	}
	else {
		return false;
	}
}

bool Loadthread::hasFailed(int32_t _id)
{
	std::lock_guard<std::mutex> lock(p_failMapMutex);
	auto it = p_failMap.find(_id);
	if (it != p_failMap.end()) {
		return true;
	}
	else {
		return false;
	}
}

const char* Loadthread::getFailMessage(int32_t _id)
{
	std::lock_guard lock(p_failMapMutex);
	auto it = p_failMap.find(_id);
	if (it != p_failMap.end()) {
		return it->second.c_str();
	}
	else {
		return "ERROR: Invalid ID.";
	}
}

void Loadthread::terminate()
{
	if (p_running && p_loadThread.joinable()) {
		{
			std::lock_guard<std::mutex> lock(p_loopCheckMutex);
			p_running = false;
		}
		p_threadWakeup.notify_one();
		p_loadThread.join();
	}
}

bool Loadthread::cacheCollfiles()
{
	if (p_collfilesCached) {
		return false;
	}

	std::scoped_lock<std::mutex, std::mutex> lock = std::scoped_lock( p_collisionDictionaryMutex, p_unifiedFSMutex);
	std::vector<std::string> collfileList = p_unifiedFS.listCollfiles();
	for (size_t i = 0; i < collfileList.size(); i++) {
		ren::FileInputHandle* fileHandle = p_unifiedFS.getCollfileHandle(collfileList[i]);
		p_collisionDictionary.cacheCollfile(collfileList[i], fileHandle);
		delete fileHandle;
	}
	p_collfilesCached = true;

	return true;
}

bool Loadthread::addArchive(const char* _filename)
{
	using namespace std::filesystem;

	path filepath = path(_filename);
	filepath.replace_extension();
	std::string filepathImg = filepath.string() + ".img";
	std::string filepathDir = filepath.string() + ".dir";

	ren::CFileInputHandle* fileHandleImg = new ren::CFileInputHandle(filepathImg.c_str());
	ren::CFileInputHandle* fileHandleDir = new ren::CFileInputHandle(filepathDir.c_str());
	if (!fileHandleImg->isGood() || !fileHandleDir->isGood()) {
		delete fileHandleImg;
		delete fileHandleDir;
		return false;
	}
	ren::ArchiveNamedEntries* archive = nullptr;
	try {
		archive = new ren::ArchiveGTA3(fileHandleImg, fileHandleDir, true);
	}
	catch (std::exception& e) {
		ren::Logging::log("Error interpreting archive \"%s\".\n", filepathImg.c_str());
		return false;
	}

	std::lock_guard<std::mutex> lock(p_unifiedFSMutex);
	p_unifiedFS.addArchive(filepath.stem().string(), archive);
	return true;
}

bool Loadthread::addFilepath(const char* _filename)
{
	std::string filename = _filename;
	std::lock_guard<std::mutex> lock(p_unifiedFSMutex);
	return p_unifiedFS.addFilepath(filename);
}

int32_t Loadthread::getID()
{
	int32_t result;
	std::unique_lock<std::mutex> lock(p_usedIDsMutex);
	if (p_usedIDs.size()) {
		result = p_usedIDs.back();
		p_usedIDs.pop_back();
		return result;
	}
	else {
		result = p_lowestUnusedID;
		p_lowestUnusedID++;
		return result;
	}
}

void Loadthread::returnID(int32_t _id)
{
	std::unique_lock<std::mutex> lock(p_usedIDsMutex);
	p_usedIDs.push_back(_id);
}

void Loadthread::markAsFailed(int32_t _handle, const char* _message)
{
	{
		std::lock_guard lock(p_failMapMutex);
		p_failMap.insert(std::pair(_handle, _message));
	}
	{
		std::lock_guard lock(p_processingSetMutex);
		p_processingSet.erase(_handle);
	}
}

void Loadthread::publishResult(int32_t _handle, ResultType _result)
{
	{
		std::lock_guard lock(p_resultMapMutex);
		p_resultMap.insert(std::pair(_handle, _result));
	}
	{
		std::lock_guard lock(p_processingSetMutex);
		p_processingSet.erase(_handle);
	}
}

void Loadthread::loadLoop()
{
	using namespace std::chrono_literals;

	// Pop the next available job:
	while (p_running) {
		LoadJob job;
		// std::pair<uint32_t, std::string> job;		

		{
			std::unique_lock<std::mutex> lock(p_loopCheckMutex);
			if (p_jobQueue.empty()) {
				p_threadWakeup.wait(lock, [this] {
					return !p_jobQueue.empty() || !p_running;
					});
			}

			if (!p_running && p_jobQueue.empty())
			{
				lock.unlock();
				break;
			}

			job = p_jobQueue.front();
			p_jobQueue.pop_front();
			lock.unlock();
		}
		ren::Logging::log("Past the condition variable. Processing job...\n");
		// actually process job here...

		ren::MDataContainer* result = nullptr;
		switch (job.type) {
		case ren::MD_RWMODEL:
		{
			ren::FileInputHandle* handle = nullptr;
			try {
				handle = p_unifiedFS.getModelHandle(job.filename);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			std::vector<char> fileData(handle->filesize());
			handle->read(0, fileData.data(), fileData.size());
			delete handle;
			handle = nullptr;
			try {
				rw::RWContainerS fileTree(rw::REN_ID_FILE, 0, nullptr, fileData.data(), fileData.size());
				fileData.clear();
				fileTree.unfold();
				int clumpIndex = fileTree.findFirstOf(rw::RW_CLUMP);
				if (clumpIndex < 0) {
					ren::throwException<std::runtime_error>("%s(): No clump secion found in \"%s\".", __FUNCTION__, job.filename.c_str());
				}
				rw::RWContainer& clumpChunk = static_cast<rw::RWContainer&>(fileTree[clumpIndex]);
				result = ren::rwClumpToMeshmodel(clumpChunk, job.processFlags);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			break;
		}
		case ren::MD_RWTEXSET:
		{
			ren::FileInputHandle* handle = nullptr;
			try {
				handle = p_unifiedFS.getTexsetHandle(job.filename);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			std::vector<char> fileData(handle->filesize());
			handle->read(0, fileData.data(), fileData.size());
			delete handle;
			handle = nullptr;
			try {
				rw::RWContainerS fileTree(rw::REN_ID_FILE, 0, nullptr, fileData.data(), fileData.size());
				fileData.clear();
				fileTree.unfold();
				int texdicIndex = fileTree.findFirstOf(rw::RW_TEXTUREDICTIONARY);
				if (texdicIndex < 0) {
					ren::throwException<std::runtime_error>("%s(): No Texture Dictionary section found in \"%s\".", __FUNCTION__, job.filename.c_str());
				}
				rw::RWContainer& texdicChunk = static_cast<rw::RWContainer&>(fileTree[texdicIndex]);
				result = ren::rwTexdicToTextureSet(texdicChunk);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			break;
		}
		case ren::MD_GTACOLLISION:
		{
			ren::FileInputHandle* handle = nullptr;
			try {
				handle = p_collisionDictionary.getCollmodelHandle(job.filename);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			try {
				result = ren::gtacollToCollisionData(handle, job.processFlags);
			}
			catch (std::exception& e) {
				markAsFailed(job.handle, e.what());
				continue;
			}
			break;
		}
		default:
		{
			std::string message = ren::st::compose<std::string>("%s(): Invalid job type '%d' for \"%s\".", __FUNCTION__, job.type, job.filename.c_str());
			markAsFailed(job.handle, message.c_str());
			continue;
		}
		}
		publishResult(job.handle, result);
	}
}
