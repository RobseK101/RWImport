#pragma once

#include <map>
#include <vector>
#include <ren/ArchiveGTACOLL.h>
#include <string>
#include <memory>

namespace ren
{
	class CollisionDictionary
	{
	public:
		CollisionDictionary() = default;
		CollisionDictionary(const CollisionDictionary&) = delete;
		CollisionDictionary(CollisionDictionary&&) = delete;
		~CollisionDictionary() = default;

		void cacheCollfile(const std::string& _name, const FileInputHandle* _fileHandle);
		FileInputHandle* getCollmodelHandle(const std::string& _name);

	private:
		struct ArchReference
		{
			std::string name;
			std::unique_ptr<ArchiveGTACOLL> archive;
		};

		struct CollfileLink
		{
			ArchiveGTACOLL* targetArchive;
			int fileIndex;
			std::string realName;
		};

		std::vector<ArchReference> p_archives;
		std::map<std::string, CollfileLink> p_links;
	};
}