#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

int main() {
    std::ifstream file("data/test_auth_storage.bin", std::ios::binary);
    if (!file) {
        std::cout << "无法打开文件" << std::endl;
        return 1;
    }
    
    // 读取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::cout << "文件大小: " << file_size << " 字节" << std::endl;
    std::cout << "页面数量: " << file_size / 4096 << " 页" << std::endl;
    std::cout << std::endl;
    
    // 读取每个页面的内容
    for (size_t page_id = 0; page_id < file_size / 4096; page_id++) {
        std::cout << "=== 页面 " << page_id << " ===" << std::endl;
        
        // 读取页面头（前16字节）
        char header[16];
        file.read(header, 16);
        
        uint16_t slot_count = *reinterpret_cast<uint16_t*>(header);
        uint16_t free_space_offset = *reinterpret_cast<uint16_t*>(header + 2);
        uint32_t next_page_id = *reinterpret_cast<uint32_t*>(header + 4);
        uint32_t page_type = *reinterpret_cast<uint32_t*>(header + 8);
        
        std::cout << "  槽数量: " << slot_count << std::endl;
        std::cout << "  空闲空间偏移: " << free_space_offset << std::endl;
        std::cout << "  下一页ID: " << next_page_id << std::endl;
        std::cout << "  页面类型: " << page_type << std::endl;
        
        // 读取页面数据部分
        char page_data[4096];
        file.seekg(page_id * 4096, std::ios::beg);
        file.read(page_data, 4096);
        
        // 显示前200字节的十六进制内容
        std::cout << "  前200字节内容:" << std::endl;
        for (int i = 0; i < 200; i++) {
            if (i % 16 == 0) {
                std::cout << std::setw(4) << i << ": ";
            }
            std::cout << std::setw(2) << std::setfill('0') << std::hex 
                      << (unsigned char)page_data[i] << " ";
            if (i % 16 == 15) {
                std::cout << std::endl;
            }
        }
        std::cout << std::dec << std::endl;
        
        // 显示可读的文本内容
        std::cout << "  可读内容:" << std::endl;
        for (int i = 0; i < 200; i++) {
            char c = page_data[i];
            if (c >= 32 && c <= 126) {
                std::cout << c;
            } else {
                std::cout << ".";
            }
        }
        std::cout << std::endl << std::endl;
    }
    
    file.close();
    return 0;
}
