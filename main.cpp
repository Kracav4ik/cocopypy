#define _WIN32_WINNT  0x0600

#include <iostream>
#include <windows.h>
#include <chrono>
#include <vector>
#include <cmath>
#include <map>

DWORD BYTES_PER_SECTOR = 512;

using namespace std::chrono;

std::wstring getLastErrorText(DWORD lastError) {
    wchar_t* buf;
    DWORD messageSize = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            lastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)(&buf),
            0,
            nullptr
    );
    if (messageSize == 0) {
        return L"Не удалось получить текст ошибки " + std::to_wstring(lastError);
    }
    std::wstring error(buf, messageSize);
    LocalFree(buf);
    return error.substr(0, error.size() - 2);
}

std::wstring getLastErrorText() {
    return getLastErrorText(GetLastError());
}

struct Piece {
    Piece();
    Piece(int64_t size, int64_t offset, int64_t buf_size, int align = 512);
    Piece(Piece&& other);
    Piece(const Piece&) = delete;
    Piece& operator=(Piece&&) = delete;
    Piece& operator=(const Piece&) = delete;
    ~Piece();

    int64_t p_size;
    int64_t p_off;
    int64_t b_size;
    int64_t wrote_bytes;
    HANDLE handle;
    OVERLAPPED ovl_read;
    OVERLAPPED ovl_write;
    void* buf;
};

void setOffset(OVERLAPPED& op, int64_t offset) {
    op.Offset =  offset % (1ll << 32);
    op.OffsetHigh = offset >> 32;
}

OVERLAPPED makeOvl(int64_t offset) {
    OVERLAPPED op;
    memset(&op, 0, sizeof(op));
    setOffset(op, offset);
    return op;
}

Piece::Piece()
{
    memset(this, 0, sizeof(*this));
}

Piece::Piece(int64_t size, int64_t offset, int64_t buf_size, int align)
    : p_size(size)
    , p_off(offset)
    , b_size(buf_size)
    , wrote_bytes(0)
    , handle(CreateEventW(nullptr, FALSE, FALSE, nullptr))
    , ovl_read(makeOvl(offset))
    , ovl_write(makeOvl(offset))
    , buf(_aligned_malloc(buf_size, align))
{
}

Piece::Piece(Piece&& other)
{
    memcpy(this, &other, sizeof(other));
    memset(&other, 0, sizeof(other));
}

Piece::~Piece() {
    if (buf) {
        _aligned_free(buf);
        CloseHandle(handle);
    }
}

std::map<LPOVERLAPPED, Piece*> ov2pi;
std::vector<HANDLE> events;
HANDLE file2read;
HANDLE file2write;

std::wstring getFullPath(const std::wstring& path) {
    DWORD size = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    std::wstring result(size - 1, L' ');
    GetFullPathNameW(path.c_str(), size, result.data(), nullptr);
    return result;
}

std::vector<int64_t> split(int64_t total, int parts, int cluster) {
    int64_t rounded = std::ceil(total/(float)cluster);
    int64_t divided = rounded / parts;
    int extras = rounded - divided * parts;
    std::vector<int64_t> result;
    int64_t used = 0;
    for (int i = 0; i < parts - 1; ++i) {
        int64_t part = divided;
        if (i < extras - 1) {
            part += 1;
        }
        part *= cluster;
        result.push_back(part);
        used += part;
    }
    result.push_back(total - used);
    return result;
}

void readCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED op);

void writeCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED op) {
    Piece& piece = *ov2pi[op];
    //std::wcout << L"- writeCompleted piece " << piece.p_off << L" off " << op->Offset << L", " << dwNumberOfBytesTransfered << L")\n";

    int64_t toRead = std::ceil((double) dwNumberOfBytesTransfered / BYTES_PER_SECTOR) * BYTES_PER_SECTOR;
    toRead = std::min(toRead, piece.b_size);

    setOffset(piece.ovl_write, piece.p_off + piece.wrote_bytes + dwNumberOfBytesTransfered);
    if (piece.wrote_bytes + piece.b_size < piece.p_size) {
        ReadFileEx(file2read, piece.buf, toRead, &piece.ovl_read, readCompleted);
    } else {
        //std::wcout << L"writeCompleted piece " << piece.p_off << "\n";
        SetEvent(piece.handle);
    }
    piece.wrote_bytes += dwNumberOfBytesTransfered;
}

void readCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED op) {
    Piece& piece = *ov2pi[op];
    //std::wcout << L"- readCompleted piece " << piece.p_off << L" off " << op->Offset << L", " << dwNumberOfBytesTransfered << L")\n";

    setOffset(piece.ovl_read, piece.p_off + piece.wrote_bytes + dwNumberOfBytesTransfered);
    int64_t toWrite = std::ceil((double) dwNumberOfBytesTransfered / BYTES_PER_SECTOR) * BYTES_PER_SECTOR;
    toWrite = std::min(toWrite, piece.b_size);

    WriteFileEx(file2write, piece.buf, toWrite, &piece.ovl_write, writeCompleted);
}

#include <io.h>
#include <fcntl.h>


void _fixwcout() {
    constexpr char cp_utf16le[] = ".1200";
    setlocale( LC_ALL, cp_utf16le );
    _setmode( _fileno(stdout), _O_WTEXT );
    //std::wcout << L"\r"; // need to output something for this to work
}


int wmain(int argc, wchar_t* argv[]) {
    _fixwcout();

    switch (argc) {
        case 1:
            std::wcout << L"Пожалуйста допишите [откуда копировать] [куда копировать] [количество потоков] [размер буфера (в кластерах)]" << std::endl;
            return 1;
        case 2:
            std::wcout << L"Пожалуйста допишите [куда копировать] [количество потоков] [размер буфера (в кластерах)]" << std::endl;
            return 1;
        case 3:
            std::wcout << L"Пожалуйста допишите [количество потоков] [размер буфера (в кластерах)]" << std::endl;
            return 1;
        case 4:
            std::wcout << L"Пожалуйста допишите [размер буфера (в кластерах)]" << std::endl;
            return 1;
        case 5:
            break;
        default:
            std::wcout << L"слишком много аргументов" << std::endl;
            return 1;
    }

    std::wstring from = getFullPath(argv[1]);
    std::wstring to = getFullPath(argv[2]);
    int64_t thread_count = _wtoi(argv[3]);
    int64_t buffer_size = _wtoi(argv[4]);

    file2read = CreateFileW(from.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, nullptr);
    //std::wcout << L"CreateFileW read " << getLastErrorText() << std::endl;

    file2write = CreateFileW(to.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, nullptr);
    //std::wcout << L"CreateFileW write " << getLastErrorText() << std::endl;

    LARGE_INTEGER size;
    GetFileSizeEx(file2read, &size);
    //std::wcout << L"GetFileSizeEx " << getLastErrorText() << std::endl;

    auto parts = split(size.QuadPart, thread_count, BYTES_PER_SECTOR);
    //std::wcout << L"parts: ";
    //std::wcout << std::endl;

    std::vector<Piece> pieces;
    int64_t part_off = 0;
    int64_t buf_size = buffer_size * BYTES_PER_SECTOR;
    for (int64_t part : parts) {
        //std::wcout << L"new piece " << std::hex << part << " " << part_off << " " << buf_size << std::endl;
        pieces.emplace_back(part, part_off, buf_size);
        part_off += part;
    }

    auto start = high_resolution_clock::now();
    for (auto& piece : pieces) {
        ov2pi[&piece.ovl_read] = &piece;
        ov2pi[&piece.ovl_write] = &piece;
        events.push_back(piece.handle);

        int64_t toRead = std::ceil((double) size.QuadPart / BYTES_PER_SECTOR) * BYTES_PER_SECTOR;
        toRead = std::min(toRead, piece.b_size);

        ReadFileEx(file2read, piece.buf, toRead, &piece.ovl_read, readCompleted);
    }

    while (true) {
        auto wait_result = WaitForMultipleObjectsEx(events.size(), events.data(), TRUE, INFINITE, TRUE);
        if (wait_result == WAIT_IO_COMPLETION) {
            //std::wcout << L"WaitForMultipleObjects IO completion" << std::endl;
            continue;
        }
        if (wait_result == WAIT_FAILED) {
            std::wcout << L"WaitForMultipleObjects " << getLastErrorText() << std::endl;
        }
        //std::wcout << L"  WaitForMultipleObjects ret " << wait_result;
        break;
    }
//    std::cout << timeGetTime() - start << std::endl;
    std::cout << duration_cast<microseconds>(high_resolution_clock::now() - start).count() << std::endl;

    for (auto event : events) {
        CloseHandle(event);
    }

    SetFilePointerEx(file2write, size, nullptr, FILE_BEGIN);
    SetEndOfFile(file2write);

    CloseHandle(file2write);
    CloseHandle(file2read);

    return 0;
}
