#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "mncl/host/cl/cl.h"
#include "mncl/host/cl/constants.h"

std::vector<unsigned char> load_binary(const std::filesystem::path& path) {
  std::vector<unsigned char> data(std::filesystem::file_size(path));
  std::ifstream(path, std::ios::binary)
      .read(reinterpret_cast<char*>(data.data()), data.size());
  return data;
}

int main() {
  constexpr size_t element_count = 4 * 4 * 16 * 8 * 2 * 4;
  constexpr size_t byte_size = element_count * sizeof(float);
  std::vector<float> x(element_count, 1.0f);
  std::vector<float> y(element_count, 2.0f);
  std::vector<float> z(element_count);

  cl_platform_id platform;
  clGetPlatformIDs(1, &platform, nullptr);
  cl_device_id device;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_EMU_MNCORE2, 1, &device, nullptr);
  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, nullptr);

  auto binary = load_binary("add.bin");
  const size_t binary_size = binary.size();
  const unsigned char* binary_data = binary.data();
  cl_program program = clCreateProgramWithBinary(
      context, 1, &device, &binary_size, &binary_data, nullptr, nullptr);
  cl_kernel kernel = clCreateKernel(program, "add", nullptr);

  cl_mem d_x =
      clCreateBuffer(context, CL_MEM_READ_ONLY, byte_size, nullptr, nullptr);
  cl_mem d_y =
      clCreateBuffer(context, CL_MEM_READ_ONLY, byte_size, nullptr, nullptr);
  cl_mem d_z =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, byte_size, nullptr, nullptr);

  clEnqueueWriteBuffer(queue, d_x, true, 0, byte_size, x.data(), 0, nullptr,
                       nullptr);
  clEnqueueWriteBuffer(queue, d_y, true, 0, byte_size, y.data(), 0, nullptr,
                       nullptr);
  clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_x);
  clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_y);
  clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_z);
  clEnqueueTask(queue, kernel, 0, nullptr, nullptr);
  clEnqueueReadBuffer(queue, d_z, true, 0, byte_size, z.data(), 0, nullptr,
                      nullptr);

  std::cout << "1 + 2 = " << z[0] << '\n';

  clReleaseMemObject(d_z);
  clReleaseMemObject(d_y);
  clReleaseMemObject(d_x);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);
}
