#pragma once

#include <thread>
#include <map>
#include <queue>
#include <vector>
#include <cstdint>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <set>
#include <rw/BSFTree.h>
#include <ren/rw_decode.h>
#include <ren/ModelDataDefinitions.h>
#include <ren/gta_decode.h>
#include "CollisionDictionary.h"
#include "UnifiedFlatFS.h"

/// A Loadthread object encapsulates a single conversion queue. A loadthread object owns its own flat virtual filesystem. 
class Loadthread
{
public:
	typedef ren::MDataContainer* ResultType;
	
	Loadthread();
	~Loadthread();

	/// 
	/// Commissions a new load job. 
	/// @param[in] _filename The file to be converted. Note that the file name must exist in the virtual filesystem.
	/// @param _type Type of the data to be converted. 
	/// @param processFlags Conversion flags to be applied.
	/// @returns The handle needed to access the results of the file conversion. This handle must be freed once
	///			 it is no longer needed. Note that handles of failed load jobs have to be freed, too!
	/// 
	int32_t enqueueFile(const char* _filename, ren::MDType _type, ren_process_flags processFlags);

	/// Cancels all commissioned load jobs that have not yet been processed and that are not currently processing.
	void clearQueue();

	size_t queueSize();

	///
	/// Retrieves a pointer to the converted data of a load job. 
	/// @param _id The handle obtained when the load job was commissioned. 
	/// @returns A pointer to the converted data as an MDataContainer or nullptr if it does not exist.
	/// 
	ResultType getItem(int32_t _id);

	/// 
	/// Frees a handle.
	/// @param _id The handle obtained when the load job was commissioned. 
	/// @returns true if the handle existed and the operation was successful.
	bool freeItem(int32_t _id);

	size_t resultCount();

	/// 
	/// Is the load job associated with this handle still in the queue or currently processing?
	/// @param _id The handle obtained when the load job was commissioned. 
	/// 
	bool isProcessing(int32_t _id);

	/// 
	/// Is the data available for access?
	/// @param _id The handle obtained when the load job was commissioned. 
	///
	bool isFinished(int32_t _id);

	/// 
	/// Has the load job failed?
	/// @param _id The handle obtained when the load job was commissioned. 
	///
	bool hasFailed(int32_t _id);

	///
	/// Retrieves the error message associated with a failed load job.
	/// @param _id The handle obtained when the load job was commissioned. 
	/// @returns ANSI C string of the error message.
	/// 
	const char* getFailMessage(int32_t _id);

	///
	/// Scan the virtual filesystem for collision models. This is necessary in order to be able to commission load jobs for collision models.
	/// @returns true if the collision models have not already been cached.
	/// 
	bool cacheCollfiles();

	/// 
	/// Adds an IMG archive, and therefore every file within it, to the flat virtual filesystem. 
	/// @param[in] _filename ANSI C string of the archive's filepath.
	/// 
	bool addArchive(const char* _filename);

	/// 
	/// Adds a single file to the flat virtual filesystem. 
	/// @param[in] _filename ANSI C string of the file's path.
	/// 
	bool addFilepath(const char* _filename);

private:
	struct LoadJob
	{
		int32_t handle;
		ren::MDType type;
		std::string filename;
		ren_process_flags processFlags;
	};

	typedef std::deque<LoadJob> JobQueue;
	typedef std::map<int32_t, ResultType> ResultMap;

	void terminate();

	int32_t getID();
	void returnID(int32_t _id);
	void markAsFailed(int32_t _handle, const char* _message);
	void publishResult(int32_t _handle, ResultType _result);

	int32_t p_lowestUnusedID;
	std::vector<int32_t> p_usedIDs;
	std::mutex p_usedIDsMutex;

	std::atomic<bool> p_running;
	JobQueue p_jobQueue;
	std::mutex p_loopCheckMutex;

	std::condition_variable p_threadWakeup;

	ResultMap p_resultMap;
	std::mutex p_resultMapMutex;

	std::map<int32_t,std::string> p_failMap;
	std::mutex p_failMapMutex;

	std::set<int32_t> p_processingSet;
	std::mutex p_processingSetMutex;

	std::thread p_loadThread;

	ren::CollisionDictionary p_collisionDictionary;
	bool p_collfilesCached = false;
	std::mutex p_collisionDictionaryMutex;

	ren::UnifiedFlatFS p_unifiedFS;
	std::mutex p_unifiedFSMutex;

	void loadLoop();
};