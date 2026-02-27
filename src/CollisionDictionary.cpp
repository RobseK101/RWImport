#include "CollisionDictionary.h"
#include <ren/Logging.h>
#include <ren/Stringtools.hpp>
#include <ren/Exceptions.hpp>

namespace ren
{
	void CollisionDictionary::cacheCollfile(const std::string& _name, const FileInputHandle* _fileHandle)
	{
		ArchiveGTACOLL* archive = new ArchiveGTACOLL(_fileHandle);
		p_archives.push_back({_name, std::unique_ptr<ArchiveGTACOLL>(archive)});
		
		for (size_t fileIndex = 0; fileIndex < archive->getFilecount(); fileIndex++) {
			std::string filenameNew = archive->getNameA(fileIndex);
			std::string filenameNewUpper = st::toUpper(filenameNew);
			auto it = p_links.find(filenameNewUpper);
			if (it == p_links.end()) {
				Logging::log("Inserting new collision model link \"%s\" as \"%s\" from \"%s\"...\n", filenameNew.c_str(), filenameNewUpper.c_str(), _name.c_str());
				CollfileLink link = { archive, (int)fileIndex, std::move(filenameNew) };
				p_links.insert({ std::move(filenameNewUpper), std::move(link) });
			}
			else {
				Logging::log("Replacing collision model link \"%s\" (%s) with \"%s\" from \"%s\"...\n", it->first.c_str(), it->second.realName.c_str(), filenameNew.c_str(), _name.c_str());
				CollfileLink link = { archive, (int)fileIndex, std::move(filenameNew) };
				it->second = std::move(link);
			}
		}
	}
	FileInputHandle* CollisionDictionary::getCollmodelHandle(const std::string& _name)
	{
		std::string nameUpper = st::toUpper(_name);
		auto it = p_links.find(nameUpper);
		if (it != p_links.end()) {
			return it->second.targetArchive->getHandle(it->second.fileIndex);
		}
		else {
			throwException<std::out_of_range>("%s(): Collision model \"%s\" does not exist!\n", __FUNCTION__, nameUpper.c_str());
		}
	}
}