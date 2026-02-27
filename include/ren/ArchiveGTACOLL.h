#pragma once
#include <ren/ArchiveLinear.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <ren/Utility.hpp>

namespace ren
{
    class ArchiveGTACOLL : public ArchiveLinear
    {
    public:
        virtual ~ArchiveGTACOLL() = default;
        ArchiveGTACOLL() = delete;
        ArchiveGTACOLL(FileInputHandle *_colfile, bool _takeOwnership = false);
        ArchiveGTACOLL(const FileInputHandle* _colfile);

        virtual int getFilecount() const override;
        virtual std::wstring getNameW(int _index) override;
        virtual std::string getNameA(int _index) override;
        virtual int getOffset(int _index) override;
        virtual int getSize(int _index) override;

        virtual int findName(const std::wstring& _name) override;
        virtual int findName(const std::string& _name) override;

        virtual size_t iNameCount(const std::wstring& _name) override;
        virtual size_t iNameCount(const std::string& _name) override;

        virtual int iFindName(const std::wstring& _name, size_t _element) override;
        virtual int iFindName(const std::string& _name, size_t _element) override;

        virtual FileInputHandle* getHandle(const std::wstring& _name) override;
        virtual FileInputHandle* getHandle(const std::string& _name) override;
        virtual FileInputHandle* getHandle(int _index) override;

        virtual bool isGood() const override;
        virtual bool namedEntries() const override;
        virtual const char* archiveTypeName() const override;

    private:
        void fillDictionary();

        struct Entry
        {
            std::string filename;
            int offset;
            int size;
        };

        typedef std::map<std::string,int> Dictionary;
        typedef std::multimap<std::string,int> DictionaryI;

        std::vector<char> p_data;
        std::vector<Entry> p_entries;
        Dictionary p_dictionary;
        DictionaryI p_dictionaryI;
        bool p_initSuccess;
    };
}