#include "UnifiedFlatFS.h"
#include <ren/Logging.h>
#include <ren/Stringtools.hpp>
#include <ren/Exceptions.hpp>
#include <filesystem>

namespace ren
{
	void UnifiedFlatFS::addArchive(const std::string& _name, ArchiveNamedEntries* _archive)
	{
		Logging::log("%s(): Adding archive \"%s\".\n", __FUNCTION__, _name.c_str());

		p_archives.push_back({ _name, std::unique_ptr<ArchiveNamedEntries>(_archive) });

		for (size_t fileIndex = 0; fileIndex < _archive->getFilecount(); fileIndex++) {
			std::string filenameNew = _archive->getNameA(fileIndex);
			std::string filenameNewUpper = st::toUpper(filenameNew);
			auto it = p_links.find(filenameNewUpper);
			if (it == p_links.end()) {
				Logging::log("Inserting new file link \"%s\" as \"%s\" from \"%s\"...\n", filenameNew.c_str(), filenameNewUpper.c_str(), _name.c_str());
				FileLink link = { _archive, (int)fileIndex, std::move(filenameNew) };
				p_links.insert({ std::move(filenameNewUpper), std::move(link) });
			}
			else {
				Logging::log("Replacing file link \"%s\" (%s) with \"%s\" from \"%s\"...\n", it->first.c_str(), it->second.realName.c_str(), filenameNew.c_str(), _name.c_str());
				FileLink link = { _archive, (int)fileIndex, std::move(filenameNew) };
				it->second = std::move(link);
			}
		}
	}

	bool UnifiedFlatFS::addFilepath(const std::string& _name)
	{
		using namespace std::filesystem;

		path filepath = absolute(path(_name));
		if (!exists(filepath) || !is_regular_file(filepath)) {
			return false;
		}

		std::string filenameUpper = st::toUpper(filepath.filename().string());
		std::string filename = filepath.string();
		FileLink link = { nullptr, -1, std::move(filename) };
		auto it = p_links.find(filenameUpper);
		if (it == p_links.end()) {
			Logging::log("Inserting new file link \"%s\" as \"%s\"...\n", link.realName.c_str(), filenameUpper.c_str());
			p_links.insert({ std::move(filenameUpper), std::move(link) });
		}
		else {
			Logging::log("Replacing file link \"%s\" with \"%s\"...\n", filenameUpper.c_str(), link.realName.c_str());
			it->second = std::move(link);
		}
		return true;
	}

	FileInputHandle* UnifiedFlatFS::getModelHandle(const std::string& _nameNoext)
	{
		std::string nameUpperExt = st::toUpper(_nameNoext) + modelExtension;
		return _getOtherFileHandle(nameUpperExt);
	}

	FileInputHandle* UnifiedFlatFS::getTexsetHandle(const std::string& _nameNoext)
	{
		std::string nameUpperExt = st::toUpper(_nameNoext) + texsetExtension;
		return _getOtherFileHandle(nameUpperExt);
	}

	FileInputHandle* UnifiedFlatFS::getCollfileHandle(const std::string& _nameNoext)
	{
		std::string nameUpperExt = st::toUpper(_nameNoext) + collfileExtension;
		return _getOtherFileHandle(nameUpperExt);
	}

	FileInputHandle* UnifiedFlatFS::getOtherFileHandle(const std::string& _nameExt)
	{
		return _getOtherFileHandle(st::toUpper(_nameExt));
	}

	std::vector<std::string> UnifiedFlatFS::listCollfiles() const
	{
		using namespace std::filesystem;

		std::vector<std::string> result;
		for (std::map<std::string, FileLink>::const_iterator it = p_links.begin(); it != p_links.end(); it++) {
			path currentFilename = path(it->first);
			if (currentFilename.extension().string() == collfileExtension) {
				result.push_back(currentFilename.stem().string());
			}
		}
		return result;
	}

	FileInputHandle* UnifiedFlatFS::_getOtherFileHandle(const std::string& _nameUpperExt)
	{
		auto it = p_links.find(_nameUpperExt);
		if (it != p_links.end()) {
			if (it->second.targetArchive != nullptr) {
				return it->second.targetArchive->getHandle(it->second.fileIndex);
			}
			else {
				return new CFileInputHandle(it->second.realName.c_str());
			}
		}
		else {
			throwException<std::out_of_range>("%s(): File \"%s\" does not exist!\n", __FUNCTION__, _nameUpperExt.c_str());
		}
	}
}