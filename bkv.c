#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
 
std::unordered_map<off_t, std::vector<std::string>> duplicates;
 

// Код для сравнения файлов
bool areFilesEqual(const std::string& file1, const std::string& file2) {
    int fd1 = open(file1.c_str(), O_RDONLY);
    int fd2 = open(file2.c_str(), O_RDONLY);
 
    if (fd1 == -1 || fd2 == -1) {
        return false;
    }
 
    struct flock lock1 = {F_RDLCK, SEEK_SET, 0, 0, 0};
    struct flock lock2 = {F_RDLCK, SEEK_SET, 0, 0, 0};
 
    fcntl(fd1, F_SETLKW, &lock1);
    fcntl(fd2, F_SETLKW, &lock2);
 
    char buffer1[1024];
    char buffer2[1024];
 
    while (1) {
        ssize_t bytesRead1 = read(fd1, buffer1, sizeof(buffer1));
        ssize_t bytesRead2 = read(fd2, buffer2, sizeof(buffer2));
 
        if (bytesRead1 != bytesRead2) {
            close(fd1);
            close(fd2);
            return false;
        }
 
        if (bytesRead1 == 0) {
            break;
        }
 
        if (bytesRead1 > 0 && memcmp(buffer1, buffer2, bytesRead1) != 0) {
            close(fd1);
            close(fd2);
            return false;
        }
    }
 
    lock1.l_type = F_UNLCK;
    lock2.l_type = F_UNLCK;
    fcntl(fd1, F_SETLK, &lock1);
    fcntl(fd2, F_SETLK, &lock2);
 
    close(fd1);
    close(fd2);
 
    return true;
}
 

// Код для проверки существования каталога
bool directoryExists(const std::string& dirPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}
 

// Код для обработки каталога
void processDirectory(const std::string& path) {
    if (!directoryExists(path)) {
        std::cout << "Directory does not exist." << std::endl;
        return;
    }
 
    DIR* dir;
    struct dirent* entry;
 
    if ((dir = opendir(path.c_str())) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string fullPath = path + '/' + entry->d_name;
 
            struct stat statInfo{};
            if (stat(fullPath.c_str(), &statInfo) == 0) {
                if (S_ISDIR(statInfo.st_mode)) {
                    if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0) {
                        processDirectory(fullPath);
                    }
                } else if (S_ISREG(statInfo.st_mode)) {
                    for (const auto& entry : duplicates[statInfo.st_size]) {
                        if (areFilesEqual(fullPath, entry)) {
                            remove(fullPath.c_str());
                            break;
                        }
                    }
                    duplicates[statInfo.st_size].push_back(fullPath);
                }
            }
        }
        closedir(dir);
    }
}
 

// Код основной функции
int main() {
    std::string directoryName;
    std::cout << "Enter directory name: ";
    std::cin >> directoryName;
    processDirectory(directoryName);
    return 0;
}
