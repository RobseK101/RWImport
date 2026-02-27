#pragma once

#include <map>
#include <vector>
#include <ren/ArchiveNamedEntries.h>
#include <memory>

namespace ren
{
	class UnifiedFlatFS
	{
	public:
		UnifiedFlatFS() = default;
		UnifiedFlatFS(const UnifiedFlatFS&) = delete;
		UnifiedFlatFS(UnifiedFlatFS&&) = delete;
		~UnifiedFlatFS() = default;

		void addArchive(const std::string& _name, ArchiveNamedEntries* _archive);
		bool addFilepath(const std::string& _name);
		FileInputHandle* getModelHandle(const std::string& _nameNoext);
		FileInputHandle* getTexsetHandle(const std::string& _nameNoext);
		FileInputHandle* getCollfileHandle(const std::string& _nameNoext);
		FileInputHandle* getOtherFileHandle(const std::string& _nameExt);

		std::vector<std::string> listCollfiles() const;

		static constexpr char modelExtension[] = ".DFF";
		static constexpr char texsetExtension[] = ".TXD";
		static constexpr char collfileExtension[] = ".COL";

	private:
		FileInputHandle* _getOtherFileHandle(const std::string& _nameUpperExt);

		struct ArchReference
		{
			std::string name;
			std::unique_ptr<ArchiveNamedEntries> archive;
		};

		struct FileLink
		{
			ArchiveNamedEntries* targetArchive;
			int fileIndex;
			std::string realName;
		};

		std::vector<ArchReference> p_archives;
		std::map<std::string, FileLink> p_links;
	};
}