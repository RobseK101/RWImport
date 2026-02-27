#include <ren/ArchiveGTACOLL.h>
#include <ren/Stringtools.hpp>
#include <ren/Exceptions.hpp>
#include <gta3d/Collfile.hpp>

namespace ren
{
    ArchiveGTACOLL::ArchiveGTACOLL(FileInputHandle *_colfile, bool _takeOwnership) : 
        p_data(_colfile->filesize()), p_initSuccess(false)
    {
        using namespace gta3d::COLL;

        if (_colfile == nullptr) {
            throwException<std::invalid_argument>("%s(): A nullptr is not allowed for initialization!", __FUNCTION__);
        }
        _colfile->read(0, p_data.data(), p_data.size());
        if (_takeOwnership == true) {
            delete _colfile;
            _colfile = nullptr;
        }

        size_t currentOffset = 0;
        while (true) {
            ColHeader header;
            if (currentOffset + sizeof(header) > p_data.size()) {
                break;
            }
            std::memcpy(&header, p_data.data() + currentOffset, sizeof(header));
            if (currentOffset + header.dataSize + v1DataOversize > p_data.size()) {
                break;
            }
            size_t totalSize = header.dataSize + v1DataOversize;
            p_entries.push_back({header.name, (int)currentOffset, (int)totalSize});
            currentOffset += totalSize;
        }
        fillDictionary();
        p_initSuccess = true;
    }

    ArchiveGTACOLL::ArchiveGTACOLL(const FileInputHandle* _colfile) : 
        p_data(_colfile->filesize()), p_initSuccess(false)
    {
        using namespace gta3d::COLL;
        
        if (_colfile == nullptr) {
            throwException<std::invalid_argument>("%s(): A nullptr is not allowed for initialization!", __FUNCTION__);
        }
        _colfile->read(0, p_data.data(), p_data.size());

        size_t currentOffset = 0;
        while (true) {
            ColHeader header;
            if (currentOffset + sizeof(header) > p_data.size()) {
                break;
            }
            std::memcpy(&header, p_data.data() + currentOffset, sizeof(header));
            if (currentOffset + header.dataSize + v1DataOversize > p_data.size()) {
                break;
            }
            size_t totalSize = header.dataSize + v1DataOversize;
            p_entries.push_back({header.name, (int)currentOffset, (int)totalSize});
            currentOffset += totalSize;
        }
        fillDictionary();
        p_initSuccess = true;
    }

    int ArchiveGTACOLL::getFilecount() const
    {
        return p_entries.size();
    }

    std::wstring ArchiveGTACOLL::getNameW(int _index)
    {
        if (_index >= 0 && _index < p_entries.size()) {
            return st::strToWStr(p_entries[_index].filename);
        }
        else {
            throwException<std::out_of_range>("%s(): Requested name through invalid index '%d'.", 
                __FUNCTION__, _index);
        }
    }

    std::string ArchiveGTACOLL::getNameA(int _index)
    {
        if (_index >= 0 && _index < p_entries.size()) {
            return p_entries[_index].filename;
        }
        else {
            throwException<std::out_of_range>("%s(): Requested name through invalid index '%d'.", 
                __FUNCTION__, _index);
        }
    }

    int ArchiveGTACOLL::getOffset(int _index)
    {
        if (_index >= 0 && _index < p_entries.size()) {
            return p_entries[_index].offset;
        }
        else {
            throwException<std::out_of_range>("%s(): Requested offset through invalid index '%d'.", 
                __FUNCTION__, _index);
        }
    }

    int ArchiveGTACOLL::getSize(int _index)
    {
        if (_index >= 0 && _index < p_entries.size()) {
            return p_entries[_index].size;
        }
        else {
            throwException<std::out_of_range>("%s(): Requested filesize through invalid index '%d'.", 
                __FUNCTION__, _index);
        }
    }

    int ArchiveGTACOLL::findName(const std::wstring& _name)
    {
        Dictionary::const_iterator it = p_dictionary.find(st::wstrToStr(_name, '?'));
        if (it != p_dictionary.end()) {
            return it->second;
        }
        else {
            return -1;
        }
    }

    int ArchiveGTACOLL::findName(const std::string& _name) 
    {
        Dictionary::const_iterator it = p_dictionary.find(_name);
        if (it != p_dictionary.end()) {
            return it->second;
        }
        else {
            return -1;
        }
    }

    size_t ArchiveGTACOLL::iNameCount(const std::wstring& _name) 
    {
        return p_dictionaryI.count(st::wstrToStr(_name, '?'));
    }

    size_t ArchiveGTACOLL::iNameCount(const std::string& _name) 
    {
        return p_dictionaryI.count(_name);
    }

    int ArchiveGTACOLL::iFindName(const std::wstring& _name, size_t _element)
    {
        std::pair<DictionaryI::const_iterator,DictionaryI::const_iterator> range 
            = p_dictionaryI.equal_range(st::wstrToStr(_name));

        int resultIndex = 0;
        while (range.first != range.second) {
            if (resultIndex == _element) {
                return range.first->second;
            }
            resultIndex++;
            range.first++;
        }
        return -1;
    }

    int ArchiveGTACOLL::iFindName(const std::string& _name, size_t _element) 
    {
        std::pair<DictionaryI::const_iterator,DictionaryI::const_iterator> range
            = p_dictionaryI.equal_range(_name);

        int resultIndex = 0;
        while (range.first != range.second) {
            if (resultIndex == _element) {
                return range.first->second;
            }
            resultIndex++;
            range.first++;
        }
        return -1;
    }

    FileInputHandle* ArchiveGTACOLL::getHandle(const std::wstring& _name)
    {
        std::string nameConverted = st::wstrToStr(_name);
        Dictionary::iterator it = p_dictionary.find(nameConverted);

        if (it != p_dictionary.end()) {
            Entry& entry = p_entries[it->second];
            return new MemoryInputHandle(&p_data[entry.offset], entry.size);
        }
        else {
            throwException<std::runtime_error>("%s(): Requested file handle through invalid name \"%s\".", 
                __FUNCTION__, nameConverted.c_str());
        }
    }

    FileInputHandle* ArchiveGTACOLL::getHandle(const std::string& _name) 
    {
        Dictionary::iterator it = p_dictionary.find(_name);

        if (it != p_dictionary.end()) {
            Entry& entry = p_entries[it->second];
            return new MemoryInputHandle(&p_data[entry.offset], entry.size);
        }
        else {
            throwException<std::runtime_error>("%s(): Requested file handle through invalid name \"%s\".", 
                __FUNCTION__, _name.c_str());
        }
    }

    FileInputHandle* ArchiveGTACOLL::getHandle(int _index)
    {
        if (_index >= 0 && _index < p_entries.size()) {
            Entry& entry = p_entries[_index];
            return new MemoryInputHandle(&p_data[entry.offset], entry.size);
        }
        else {
            throwException<std::out_of_range>("%s(): Requested file handle through invalid index '%d'.", 
                __FUNCTION__, _index);
        }
    }

    bool ArchiveGTACOLL::isGood() const
    {
        return p_initSuccess;
    }

    bool ArchiveGTACOLL::namedEntries() const 
    {
        return true;
    }

    const char* ArchiveGTACOLL::archiveTypeName() const
    {
        return "GTACOLL";
    }

    void ArchiveGTACOLL::fillDictionary()
    {
        for (int i = 0; i < p_entries.size(); i++) {
            std::pair<std::string,int> currentEntry(p_entries[i].filename, i);
            std::pair<std::string,int> currentEntryI(st::toUpper(currentEntry.first), i);
            p_dictionary.insert(currentEntry);
            p_dictionaryI.insert(currentEntryI);
        }
    }
}