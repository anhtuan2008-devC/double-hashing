# Double Hashing Experiment

Đây là project minh họa và thử nghiệm kỹ thuật **Double Hashing** trong cấu trúc dữ liệu Hash Table.

## Mục tiêu

- Hiểu và thực hành kỹ thuật Double Hashing để giải quyết vấn đề va chạm (collision) trong Hash Table.
- So sánh hiệu quả của Double Hashing với các phương pháp khác (nếu có mở rộng).

## Cấu trúc thư mục

```
Double Hashing Experiment/
│
├── CMakeLists.txt # File cấu hình CMake cho thư mục này
├── main.cpp # File mã nguồn chính
│
out/ # Thư mục build output 
│ └── build
│
CMakeLists.txt # File cấu hình CMake cho toàn project
CMakePresets.json # File preset cho CMake
```


## Hướng dẫn build & chạy

**Yêu cầu:**  
- Đã cài đặt [CMake](https://cmake.org/)  
- Trình biên dịch C++ (g++ hoặc MSVC...)

**Các bước:**

```sh
# Tạo thư mục build nếu chưa có
mkdir out/build
cd out/build

# Tạo file build với CMake
cmake ../..

# Biên dịch
cmake --build .

# Chạy chương trình
cd Double Hashing Experiment
cd Debug
.\CMakeTarget     # hoặc .\CMakeTarget.exe trên Windows
```
## Tác giả

- Nhóm 9
- Lớp 24CTT2A
- Trường Đại học Khoa học Tự nhiên, ĐHQG-HCM
```
