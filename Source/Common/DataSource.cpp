//      __________        ___               ______            _
//     / ____/ __ \____  / (_)___  ___     / ____/___  ____ _(_)___  ___
//    / /_  / / / / __ \/ / / __ \/ _ \   / __/ / __ \/ __ `/ / __ \/ _ `
//   / __/ / /_/ / / / / / / / / /  __/  / /___/ / / / /_/ / / / / /  __/
//  /_/    \____/_/ /_/_/_/_/ /_/\___/  /_____/_/ /_/\__, /_/_/ /_/\___/
//                                                  /____/
// FOnline Engine
// https://fonline.ru
// https://github.com/cvet/fonline
//
// MIT License
//
// Copyright (c) 2006 - 2022, Anton Tsvetinskiy aka cvet <cvet@tut.by>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "DataSource.h"
#include "DiskFileSystem.h"
#include "EmbeddedResources-Include.h"
#include "FileSystem.h"
#include "Log.h"
#include "StringUtils.h"

#include "minizip/unzip.h"

class DataSource::Impl
{
public:
    Impl() = default;
    Impl(const Impl&) = delete;
    Impl(Impl&&) noexcept = default;
    auto operator=(const Impl&) = delete;
    auto operator=(Impl&&) noexcept = delete;
    virtual ~Impl() = default;

    [[nodiscard]] virtual auto IsDiskDir() const -> bool = 0;
    [[nodiscard]] virtual auto GetPackName() const -> string_view = 0;
    [[nodiscard]] virtual auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool = 0;
    [[nodiscard]] virtual auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> = 0;
    [[nodiscard]] virtual auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> = 0;
};

using FileNameVec = vector<pair<string, string>>;

static auto GetFileNamesGeneric(const FileNameVec& fnames, string_view path, bool include_subdirs, string_view ext) -> vector<string>
{
    string path_fixed = _str(path).lower().normalizePathSlashes();
    if (!path_fixed.empty() && path_fixed.back() != '/') {
        path_fixed += "/";
    }

    const auto len = path_fixed.length();
    vector<string> result;

    for (const auto& [fst, snd] : fnames) {
        auto add = false;
        if (fst.compare(0, len, path_fixed) == 0 && (include_subdirs || (len > 0 && fst.find_last_of('/') < len) || (len == 0 && fst.find_last_of('/') == string::npos))) {
            if (ext.empty() || _str(fst).getFileExtension() == ext) {
                add = true;
            }
        }
        if (add && std::find(result.begin(), result.end(), snd) == result.end()) {
            result.push_back(snd);
        }
    }
    return result;
}

class NonCachedDir final : public DataSource::Impl
{
public:
    explicit NonCachedDir(string_view fname);
    NonCachedDir(const NonCachedDir&) = delete;
    NonCachedDir(NonCachedDir&&) noexcept = default;
    auto operator=(const NonCachedDir&) = delete;
    auto operator=(NonCachedDir&&) noexcept = delete;
    ~NonCachedDir() override = default;

    [[nodiscard]] auto IsDiskDir() const -> bool override { return true; }
    [[nodiscard]] auto GetPackName() const -> string_view override { return _basePath; }
    [[nodiscard]] auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool override;
    [[nodiscard]] auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> override;
    [[nodiscard]] auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> override;

private:
    string _basePath {};
};

class CachedDir final : public DataSource::Impl
{
public:
    CachedDir(string_view fname, bool recursive);
    CachedDir(const CachedDir&) = delete;
    CachedDir(CachedDir&&) noexcept = default;
    auto operator=(const CachedDir&) = delete;
    auto operator=(CachedDir&&) noexcept = delete;
    ~CachedDir() override = default;

    [[nodiscard]] auto IsDiskDir() const -> bool override { return true; }
    [[nodiscard]] auto GetPackName() const -> string_view override { return _basePath; }
    [[nodiscard]] auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool override;
    [[nodiscard]] auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> override;
    [[nodiscard]] auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> override;

private:
    struct FileEntry
    {
        string FileName {};
        size_t FileSize {};
        uint64 WriteTime {};
    };
    using IndexMap = unordered_map<string, FileEntry>;

    IndexMap _filesTree {};
    FileNameVec _filesTreeNames {};
    string _basePath {};
};

class FalloutDat final : public DataSource::Impl
{
public:
    explicit FalloutDat(string_view fname);
    FalloutDat(const FalloutDat&) = delete;
    FalloutDat(FalloutDat&&) noexcept = default;
    auto operator=(const FalloutDat&) = delete;
    auto operator=(FalloutDat&&) noexcept = delete;
    ~FalloutDat() override;

    [[nodiscard]] auto IsDiskDir() const -> bool override { return false; }
    [[nodiscard]] auto GetPackName() const -> string_view override { return _fileName; }
    [[nodiscard]] auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool override;
    [[nodiscard]] auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> override;
    [[nodiscard]] auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> override { return GetFileNamesGeneric(_filesTreeNames, path, include_subdirs, ext); }

private:
    using IndexMap = unordered_map<string, uchar*>;

    auto ReadTree() -> bool;

    mutable DiskFile _datFile;
    IndexMap _filesTree {};
    FileNameVec _filesTreeNames {};
    string _fileName {};
    uchar* _memTree {};
    uint64 _writeTime {};
    mutable vector<uchar> _readBuf {};
};

class ZipFile final : public DataSource::Impl
{
public:
    explicit ZipFile(string_view fname);
    ZipFile(const ZipFile&) = delete;
    ZipFile(ZipFile&&) noexcept = default;
    auto operator=(const ZipFile&) = delete;
    auto operator=(ZipFile&&) noexcept = delete;
    ~ZipFile() override;

    [[nodiscard]] auto IsDiskDir() const -> bool override { return false; }
    [[nodiscard]] auto GetPackName() const -> string_view override { return _fileName; }
    [[nodiscard]] auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool override;
    [[nodiscard]] auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> override;
    [[nodiscard]] auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> override { return GetFileNamesGeneric(_filesTreeNames, path, include_subdirs, ext); }

private:
    struct ZipFileInfo
    {
        unz_file_pos Pos {};
        int UncompressedSize {};
    };
    using IndexMap = unordered_map<string, ZipFileInfo>;

    auto ReadTree() -> bool;

    IndexMap _filesTree {};
    FileNameVec _filesTreeNames {};
    string _fileName {};
    unzFile _zipHandle {};
    uint64 _writeTime {};
};

class AndroidAssets final : public DataSource::Impl
{
public:
    AndroidAssets();
    AndroidAssets(const AndroidAssets&) = delete;
    AndroidAssets(AndroidAssets&&) noexcept = default;
    auto operator=(const AndroidAssets&) = delete;
    auto operator=(AndroidAssets&&) noexcept = delete;
    ~AndroidAssets() override = default;

    [[nodiscard]] auto IsDiskDir() const -> bool override { return false; }
    [[nodiscard]] auto GetPackName() const -> string_view override { return _packName; }
    [[nodiscard]] auto IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool override;
    [[nodiscard]] auto OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar> override;
    [[nodiscard]] auto GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string> override;

private:
    struct FileEntry
    {
        string FileName {};
        size_t FileSize {};
        uint64 WriteTime {};
    };
    using IndexMap = unordered_map<string, FileEntry>;

    string _packName {"$AndroidAssets"};
    IndexMap _filesTree {};
    FileNameVec _filesTreeNames {};
};

DataSource::DataSource(string_view path, DataSourceType type)
{
    RUNTIME_ASSERT(!path.empty());

    // Special entries
    if (path.front() == '$') {
        RUNTIME_ASSERT(type == DataSourceType::Default);

        if (path == "$Embedded") {
            _pImpl = std::make_unique<ZipFile>(path);
            return;
        }
        else if (path == "$AndroidAssets") {
            _pImpl = std::make_unique<AndroidAssets>();
            return;
        }

        throw DataSourceException("Invalid magic path", path, type);
    }

    // Dir without subdirs
    if (type == DataSourceType::DirRoot) {
        if (!DiskFileSystem::IsDir(path)) {
            WriteLog(LogType::Warning, "Directory '{}' not found", path);
        }

        _pImpl = std::make_unique<CachedDir>(path, false);
        return;
    }

    // Raw view
    if (DiskFileSystem::IsDir(path)) {
        _pImpl = std::make_unique<CachedDir>(path, true);
        return;
    }

    // Packed view
    const auto is_file_present = [](string_view file_path) -> bool {
        const auto file = DiskFileSystem::OpenFile(file_path, false);
        return !!file;
    };

    if (is_file_present(path)) {
        const string ext = _str(path).getFileExtension();
        if (ext == "dat") {
            _pImpl = std::make_unique<FalloutDat>(path);
            return;
        }
        else if (ext == "zip" || ext == "bos") {
            _pImpl = std::make_unique<ZipFile>(path);
            return;
        }

        throw DataSourceException("Unknown file extension", ext, path, type);
    }
    else if (is_file_present(_str("{}.zip", path))) {
        _pImpl = std::make_unique<ZipFile>(_str("{}.zip", path));
        return;
    }
    else if (is_file_present(_str("{}.bos", path))) {
        _pImpl = std::make_unique<ZipFile>(_str("{}.bos", path));
        return;
    }
    else if (is_file_present(_str("{}.dat", path))) {
        _pImpl = std::make_unique<FalloutDat>(_str("{}.dat", path));
        return;
    }

    throw DataSourceException("Data pack not found", path, type);
}

DataSource::~DataSource() = default;
DataSource::DataSource(DataSource&&) noexcept = default;
auto DataSource::operator=(DataSource&&) noexcept -> DataSource& = default;

auto DataSource::IsDiskDir() const -> bool
{
    return _pImpl->IsDiskDir();
}

auto DataSource::GetPackName() const -> string_view
{
    return _pImpl->GetPackName();
}

auto DataSource::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    return _pImpl->IsFilePresent(path, path_lower, size, write_time);
}

auto DataSource::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    return _pImpl->OpenFile(path, path_lower, size, write_time);
}

auto DataSource::GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string>
{
    return _pImpl->GetFileNames(path, include_subdirs, ext);
}

NonCachedDir::NonCachedDir(string_view fname)
{
    _basePath = fname;
    DiskFileSystem::ResolvePath(_basePath);
    _basePath += "/";
}

auto NonCachedDir::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    UNUSED_VARIABLE(path_lower);

    const auto file = DiskFileSystem::OpenFile(_str("{}{}", _basePath, path), false);
    if (!file) {
        return false;
    }

    size = file.GetSize();
    write_time = file.GetWriteTime();
    return true;
}

auto NonCachedDir::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    UNUSED_VARIABLE(path_lower);

    auto file = DiskFileSystem::OpenFile(_str("{}{}", _basePath, path), false);
    if (!file) {
        return nullptr;
    }

    size = file.GetSize();
    auto* buf = new uchar[static_cast<size_t>(size) + 1];
    if (!file.Read(buf, size)) {
        delete[] buf;
        throw DataSourceException("Can't read file from non cached dir", _basePath, path);
    }

    write_time = file.GetWriteTime();
    buf[size] = 0;
    return {buf, [](auto* p) { delete[] p; }};
}

auto NonCachedDir::GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string>
{
    FileNameVec fnames;
    DiskFileSystem::IterateDir(_str("{}{}", _basePath, path), "", include_subdirs, [&fnames](string_view path2, size_t size, uint64 write_time) {
        UNUSED_VARIABLE(size);
        UNUSED_VARIABLE(write_time);

        string name_lower = _str(path2).lower();
        fnames.push_back(std::make_pair(name_lower, string(path2)));
    });

    return GetFileNamesGeneric(fnames, path, include_subdirs, ext);
}

CachedDir::CachedDir(string_view fname, bool recursive)
{
    _basePath = fname;
    DiskFileSystem::ResolvePath(_basePath);
    _basePath += "/";

    DiskFileSystem::IterateDir(_basePath, "", recursive, [this](string_view path, size_t size, uint64 write_time) {
        FileEntry fe;
        fe.FileName = _str("{}{}", _basePath, path);
        fe.FileSize = size;
        fe.WriteTime = write_time;

        string name_lower = _str(path).lower();
        _filesTree.insert(std::make_pair(name_lower, fe));
        _filesTreeNames.push_back(std::make_pair(name_lower, string(path)));
    });
}

auto CachedDir::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    UNUSED_VARIABLE(path);

    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return false;
    }

    const auto& fe = it->second;
    size = fe.FileSize;
    write_time = fe.WriteTime;
    return true;
}

auto CachedDir::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return nullptr;
    }

    const auto& fe = it->second;
    auto file = DiskFileSystem::OpenFile(fe.FileName, false);
    if (!file) {
        throw DataSourceException("Can't read file from cached dir", _basePath, path);
    }

    size = fe.FileSize;
    auto* buf = new uchar[static_cast<size_t>(size) + 1];
    if (!file.Read(buf, size)) {
        delete[] buf;
        return nullptr;
    }

    buf[size] = 0;
    write_time = fe.WriteTime;
    return {buf, [](auto* p) { delete[] p; }};
}

auto CachedDir::GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string>
{
    return GetFileNamesGeneric(_filesTreeNames, path, include_subdirs, ext);
}

FalloutDat::FalloutDat(string_view fname) : _datFile {DiskFileSystem::OpenFile(fname, false)}
{
    _fileName = fname;
    _readBuf.resize(0x40000);

    if (!_datFile) {
        throw DataSourceException("Cannot open fallout dat file", fname);
    }

    _writeTime = _datFile.GetWriteTime();

    if (!ReadTree()) {
        throw DataSourceException("Read fallout dat file tree failed");
    }
}

FalloutDat::~FalloutDat()
{
    delete[] _memTree;
}

auto FalloutDat::ReadTree() -> bool
{
    uint version = 0;
    if (!_datFile.SetPos(-12, DiskFileSeek::End)) {
        return false;
    }
    if (!_datFile.Read(&version, 4)) {
        return false;
    }

    // DAT 2.1 Arcanum
    if (version == 0x44415431) // 1TAD
    {
        if (!_datFile.SetPos(-4, DiskFileSeek::End)) {
            return false;
        }

        uint tree_size = 0;
        if (!_datFile.Read(&tree_size, 4)) {
            return false;
        }

        // Read tree
        if (!_datFile.SetPos(-static_cast<int>(tree_size), DiskFileSeek::End)) {
            return false;
        }

        uint files_total = 0;
        if (!_datFile.Read(&files_total, 4)) {
            return false;
        }

        tree_size -= 28 + 4; // Subtract information block and files total
        _memTree = new uchar[tree_size];
        std::memset(_memTree, 0, tree_size);
        if (!_datFile.Read(_memTree, tree_size)) {
            return false;
        }

        // Indexing tree
        auto* ptr = _memTree;
        const auto* end_ptr = _memTree + tree_size;

        while (ptr < end_ptr) {
            uint fnsz = 0;
            std::memcpy(&fnsz, ptr, sizeof(fnsz));
            uint type = 0;
            std::memcpy(&type, ptr + 4 + fnsz + 4, sizeof(type));

            if (fnsz != 0u && type != 0x400) // Not folder
            {
                string name = _str(string(reinterpret_cast<const char*>(ptr) + 4, fnsz)).normalizePathSlashes();
                string name_lower = _str(name).lower();

                if (type == 2) {
                    *(ptr + 4 + fnsz + 7) = 1; // Compressed
                }

                _filesTree.insert(std::make_pair(name_lower, ptr + 4 + fnsz + 7));
                _filesTreeNames.push_back(std::make_pair(name_lower, name));
            }

            ptr += static_cast<size_t>(fnsz) + 24;
        }

        return true;
    }

    // DAT 2.0 Fallout2
    if (!_datFile.SetPos(-8, DiskFileSeek::End)) {
        return false;
    }

    uint tree_size = 0;
    if (!_datFile.Read(&tree_size, 4)) {
        return false;
    }

    uint dat_size = 0;
    if (!_datFile.Read(&dat_size, 4)) {
        return false;
    }

    // Check for DAT1.0 Fallout1 dat file
    if (!_datFile.SetPos(0, DiskFileSeek::Set)) {
        return false;
    }

    uint dir_count = 0;
    if (!_datFile.Read(&dir_count, 4)) {
        return false;
    }

    dir_count >>= 24;
    if (dir_count == 0x01 || dir_count == 0x33) {
        return false;
    }

    // Check for truncated
    if (_datFile.GetSize() != dat_size) {
        return false;
    }

    // Read tree
    if (!_datFile.SetPos(-(static_cast<int>(tree_size) + 8), DiskFileSeek::End)) {
        return false;
    }

    uint files_total = 0;
    if (!_datFile.Read(&files_total, 4)) {
        return false;
    }

    tree_size -= 4;
    _memTree = new uchar[tree_size];
    if (!_datFile.Read(_memTree, tree_size)) {
        return false;
    }

    // Indexing tree
    auto* ptr = _memTree;
    const auto* end_ptr = _memTree + tree_size;

    while (ptr < end_ptr) {
        uint fnsz = 0;
        std::memcpy(&fnsz, ptr, sizeof(fnsz));

        if (fnsz != 0u) {
            string name = _str(string(reinterpret_cast<const char*>(ptr) + 4, fnsz)).normalizePathSlashes();
            string name_lower = _str(name).lower();

            _filesTree.insert(std::make_pair(name_lower, ptr + 4 + fnsz));
            _filesTreeNames.push_back(std::make_pair(name_lower, name));
        }

        ptr += static_cast<size_t>(fnsz) + 17;
    }

    return true;
}

auto FalloutDat::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    UNUSED_VARIABLE(path);

    if (!_datFile) {
        return false;
    }

    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return false;
    }

    uint real_size = 0;
    std::memcpy(&real_size, it->second + 1, sizeof(real_size));

    size = real_size;
    write_time = _writeTime;
    return true;
}

auto FalloutDat::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return nullptr;
    }

    const auto* ptr = it->second;
    uchar type = 0;
    std::memcpy(&type, ptr, sizeof(type));
    uint real_size = 0;
    std::memcpy(&real_size, ptr + 1, sizeof(real_size));
    uint packed_size = 0;
    std::memcpy(&packed_size, ptr + 5, sizeof(packed_size));
    int offset = 0;
    std::memcpy(&offset, ptr + 9, sizeof(offset));

    if (!_datFile.SetPos(offset, DiskFileSeek::Set)) {
        throw DataSourceException("Can't read file from fallout dat (1)", path);
    }

    size = real_size;
    auto* buf = new uchar[static_cast<size_t>(size) + 1];

    if (type == 0u) {
        // Plane data
        if (!_datFile.Read(buf, size)) {
            delete[] buf;
            throw DataSourceException("Can't read file from fallout dat (2)", path);
        }
    }
    else {
        // Packed data
        z_stream stream;
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.opaque = nullptr;
        stream.next_in = nullptr;
        stream.avail_in = 0;
        if (inflateInit(&stream) != Z_OK) {
            delete[] buf;
            throw DataSourceException("Can't read file from fallout dat (3)", path);
        }

        stream.next_out = buf;
        stream.avail_out = real_size;

        auto left = packed_size;
        while (stream.avail_out != 0u) {
            if (stream.avail_in == 0u && left > 0) {
                stream.next_in = &_readBuf[0];
                const auto len = std::min(left, static_cast<uint>(_readBuf.size()));
                if (!_datFile.Read(&_readBuf[0], len)) {
                    delete[] buf;
                    throw DataSourceException("Can't read file from fallout dat (4)", path);
                }
                stream.avail_in = len;
                left -= len;
            }

            const auto r = inflate(&stream, Z_NO_FLUSH);
            if (r != Z_OK && r != Z_STREAM_END) {
                delete[] buf;
                throw DataSourceException("Can't read file from fallout dat (5)", path);
            }
            if (r == Z_STREAM_END) {
                break;
            }
        }

        inflateEnd(&stream);
    }

    write_time = _writeTime;
    buf[size] = 0;
    return {buf, [](auto* p) { delete[] p; }};
}

ZipFile::ZipFile(string_view fname)
{
    _fileName = fname;
    _zipHandle = nullptr;

    zlib_filefunc_def ffunc;
    if (fname[0] != '$') {
        auto* p_file = new DiskFile {DiskFileSystem::OpenFile(fname, false)};
        if (!*p_file) {
            delete p_file;
            throw DataSourceException("Can't open zip file", fname);
        }

        _writeTime = p_file->GetWriteTime();

        ffunc.zopen_file = [](voidpf opaque, const char*, int) -> voidpf { return opaque; };
        ffunc.zread_file = [](voidpf, voidpf stream, void* buf, uLong size) -> uLong {
            auto* file = static_cast<DiskFile*>(stream);
            return file->Read(buf, size) ? size : 0;
        };
        ffunc.zwrite_file = [](voidpf, voidpf, const void*, uLong) -> uLong { return 0; };
        ffunc.ztell_file = [](voidpf, voidpf stream) -> long {
            const auto* file = static_cast<DiskFile*>(stream);
            return static_cast<long>(file->GetPos());
        };
        ffunc.zseek_file = [](voidpf, voidpf stream, uLong offset, int origin) -> long {
            auto* file = static_cast<DiskFile*>(stream);
            switch (origin) {
            case ZLIB_FILEFUNC_SEEK_SET:
                file->SetPos(static_cast<int>(offset), DiskFileSeek::Set);
                break;
            case ZLIB_FILEFUNC_SEEK_CUR:
                file->SetPos(static_cast<int>(offset), DiskFileSeek::Cur);
                break;
            case ZLIB_FILEFUNC_SEEK_END:
                file->SetPos(static_cast<int>(offset), DiskFileSeek::End);
                break;
            default:
                return -1;
            }
            return 0;
        };
        ffunc.zclose_file = [](voidpf, voidpf stream) -> int {
            const auto* file = static_cast<DiskFile*>(stream);
            delete file;
            return 0;
        };
        ffunc.zerror_file = [](voidpf, voidpf stream) -> int {
            if (stream == nullptr) {
                return 1;
            }
            return 0;
        };
        ffunc.opaque = p_file;
    }
    else {
        _writeTime = 0;

        struct MemStream
        {
            const uchar* Buf;
            uint Length;
            uint Pos;
        };

        ffunc.zopen_file = [](voidpf, const char* filename, int) -> voidpf {
            if (string(filename) == "$Embedded") {
                static_assert(sizeof(EMBEDDED_RESOURCES) > 100);
                auto default_array = true;
                for (size_t i = 4; i < 5 && default_array; i++) {
                    if (EMBEDDED_RESOURCES[i] != 0x00) {
                        default_array = false;
                    }
                }
                for (size_t i = 5; i < 47 && default_array; i++) {
                    if (EMBEDDED_RESOURCES[i] != 0x42) {
                        default_array = false;
                    }
                }
                for (size_t i = 47; i < 48 && default_array; i++) {
                    if (EMBEDDED_RESOURCES[i] != 0x00) {
                        default_array = false;
                    }
                }
                if (default_array) {
                    throw DataSourceException("Embedded resources not really embed");
                }

                auto* mem_stream = new MemStream();
                mem_stream->Buf = EMBEDDED_RESOURCES;
                mem_stream->Length = sizeof(EMBEDDED_RESOURCES);
                mem_stream->Pos = 0;
                return mem_stream;
            }
            return nullptr;
        };
        ffunc.zread_file = [](voidpf, voidpf stream, void* buf, uLong size) -> uLong {
            auto* mem_stream = static_cast<MemStream*>(stream);
            std::memcpy(buf, mem_stream->Buf + mem_stream->Pos, size);
            mem_stream->Pos += static_cast<uint>(size);
            return size;
        };
        ffunc.zwrite_file = [](voidpf, voidpf, const void*, uLong) -> uLong { return 0; };
        ffunc.ztell_file = [](voidpf, voidpf stream) -> long {
            const auto* mem_stream = static_cast<MemStream*>(stream);
            return static_cast<long>(mem_stream->Pos);
        };
        ffunc.zseek_file = [](voidpf, voidpf stream, uLong offset, int origin) -> long {
            auto* mem_stream = static_cast<MemStream*>(stream);
            switch (origin) {
            case ZLIB_FILEFUNC_SEEK_SET:
                mem_stream->Pos = static_cast<uint>(offset);
                break;
            case ZLIB_FILEFUNC_SEEK_CUR:
                mem_stream->Pos += static_cast<uint>(offset);
                break;
            case ZLIB_FILEFUNC_SEEK_END:
                mem_stream->Pos = mem_stream->Length + static_cast<uint>(offset);
                break;
            default:
                return -1;
            }
            return 0;
        };
        ffunc.zclose_file = [](voidpf, voidpf stream) -> int {
            const auto* mem_stream = static_cast<MemStream*>(stream);
            delete mem_stream;
            return 0;
        };
        ffunc.zerror_file = [](voidpf, voidpf stream) -> int {
            if (stream == nullptr) {
                return 1;
            }
            return 0;
        };
        ffunc.opaque = nullptr;
    }

    _zipHandle = unzOpen2(string(fname).c_str(), &ffunc);
    if (_zipHandle == nullptr) {
        throw DataSourceException("Can't read zip file", fname);
    }

    if (!ReadTree()) {
        throw DataSourceException("Read zip file tree failed", fname);
    }
}

ZipFile::~ZipFile()
{
    if (_zipHandle != nullptr) {
        unzClose(_zipHandle);
    }
}

auto ZipFile::ReadTree() -> bool
{
    unz_global_info gi;
    if (unzGetGlobalInfo(_zipHandle, &gi) != UNZ_OK || gi.number_entry == 0) {
        return false;
    }

    ZipFileInfo zip_info;
    unz_file_pos pos;
    unz_file_info info;
    for (uLong i = 0; i < gi.number_entry; i++) {
        if (unzGetFilePos(_zipHandle, &pos) != UNZ_OK) {
            return false;
        }

        char buf[4096];
        if (unzGetCurrentFileInfo(_zipHandle, &info, buf, sizeof(buf), nullptr, 0, nullptr, 0) != UNZ_OK) {
            return false;
        }

        if ((info.external_fa & 0x10) == 0u) // Not folder
        {
            string name = _str(buf).normalizePathSlashes();
            string name_lower = _str(name).lower();

            zip_info.Pos = pos;
            zip_info.UncompressedSize = static_cast<int>(info.uncompressed_size);
            _filesTree.insert(std::make_pair(name_lower, zip_info));
            _filesTreeNames.push_back(std::make_pair(name_lower, name));
        }

        if (i + 1 < gi.number_entry && unzGoToNextFile(_zipHandle) != UNZ_OK) {
            return false;
        }
    }

    return true;
}

auto ZipFile::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    UNUSED_VARIABLE(path);

    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return false;
    }

    const auto& info = it->second;
    write_time = _writeTime;
    size = info.UncompressedSize;
    return true;
}

auto ZipFile::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return nullptr;
    }

    const auto& info = it->second;
    auto pos = info.Pos;

    if (unzGoToFilePos(_zipHandle, &pos) != UNZ_OK) {
        throw DataSourceException("Can't read file from zip (1)", path);
    }
    if (unzOpenCurrentFile(_zipHandle) != UNZ_OK) {
        throw DataSourceException("Can't read file from zip (2)", path);
    }

    auto* buf = new uchar[static_cast<size_t>(info.UncompressedSize) + 1];
    const auto read = unzReadCurrentFile(_zipHandle, buf, info.UncompressedSize);
    if (unzCloseCurrentFile(_zipHandle) != UNZ_OK || read != info.UncompressedSize) {
        delete[] buf;
        throw DataSourceException("Can't read file from zip (3)", path);
    }

    write_time = _writeTime;
    size = info.UncompressedSize;
    buf[size] = 0;
    return {buf, [](auto* p) { delete[] p; }};
}

AndroidAssets::AndroidAssets()
{
    _filesTree.clear();
    _filesTreeNames.clear();

    // Read tree
    auto file_tree = DiskFileSystem::OpenFile("FilesTree.txt", false);
    if (!file_tree) {
        throw DataSourceException("Can't open 'FilesTree.txt' in android assets");
    }

    const auto len = file_tree.GetSize() + 1;
    auto* buf = new char[len];
    buf[len - 1] = 0;

    if (!file_tree.Read(buf, len)) {
        delete[] buf;
        throw DataSourceException("Can't read 'FilesTree.txt' in android assets");
    }

    const auto names = _str(buf).normalizeLineEndings().split('\n');
    delete[] buf;

    // Parse
    for (const auto& name : names) {
        auto file = DiskFileSystem::OpenFile(name, false);
        if (!file) {
            throw DataSourceException("Can't open file in android assets", name);
        }

        FileEntry fe;
        fe.FileName = name;
        fe.FileSize = file.GetSize();
        fe.WriteTime = file.GetWriteTime();

        string name_lower = _str(name).lower();
        _filesTree.insert(std::make_pair(name_lower, fe));
        _filesTreeNames.push_back(std::make_pair(name_lower, name));
    }
}

auto AndroidAssets::IsFilePresent(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> bool
{
    UNUSED_VARIABLE(path);

    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return false;
    }

    const auto& fe = it->second;
    size = fe.FileSize;
    write_time = fe.WriteTime;
    return true;
}

auto AndroidAssets::OpenFile(string_view path, string_view path_lower, size_t& size, uint64& write_time) const -> unique_del_ptr<uchar>
{
    const auto it = _filesTree.find(string(path_lower));
    if (it == _filesTree.end()) {
        return nullptr;
    }

    const auto& fe = it->second;
    auto file = DiskFileSystem::OpenFile(fe.FileName, false);
    if (!file) {
        throw DataSourceException("Can't open file in android assets", path);
    }

    size = fe.FileSize;
    auto* buf = new uchar[static_cast<size_t>(size) + 1];
    if (!file.Read(buf, size)) {
        delete[] buf;
        throw DataSourceException("Can't read file in android assets", path);
    }

    buf[size] = 0;
    write_time = fe.WriteTime;
    return {buf, [](auto* p) { delete[] p; }};
}

auto AndroidAssets::GetFileNames(string_view path, bool include_subdirs, string_view ext) const -> vector<string>
{
    return GetFileNamesGeneric(_filesTreeNames, path, include_subdirs, ext);
}
