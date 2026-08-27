#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "mncl/host/cl/cl.h"
#include "mncl/host/cl/constants.h"

std::vector<unsigned char> load_binary(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("kernel binary not found: " + path.string());
  }

  const auto size = std::filesystem::file_size(path);
  std::vector<unsigned char> data(size);
  std::ifstream input(path, std::ios::binary);
  if (!input.read(reinterpret_cast<char*>(data.data()), size)) {
    throw std::runtime_error("failed to read kernel binary: " + path.string());
  }
  return data;
}

int main() {
  constexpr size_t element_count = 4 * 4 * 16 * 8 * 2 * 4;
  constexpr size_t byte_size = element_count * sizeof(float);

  std::vector<float> x(element_count);
  std::vector<float> y(element_count);
  std::vector<float> z(element_count);
  for (size_t i = 0; i < element_count; ++i) {
    x[i] = static_cast<float>(i % 101) * 0.25f;
    y[i] = static_cast<float>(i % 97) * 0.5f;
  }

  cl_int status;
  cl_platform_id platform;
  status = clGetPlatformIDs(1, &platform, nullptr);
  assert(status == CL_SUCCESS);

  cl_device_id device;
  status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_EMU_MNCORE2, 1, &device,
                          nullptr);
  assert(status == CL_SUCCESS);

  cl_context context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
  assert(status == CL_SUCCESS);
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
  assert(status == CL_SUCCESS);

  auto binary = load_binary("add.bin");
  const size_t binary_size = binary.size();
  const unsigned char* binary_data = binary.data();
  cl_program program = clCreateProgramWithBinary(
      context, 1, &device, &binary_size, &binary_data, nullptr, &status);
  assert(status == CL_SUCCESS);
  cl_kernel kernel = clCreateKernel(program, "add", &status);
  assert(status == CL_SUCCESS);

  cl_mem d_x =
      clCreateBuffer(context, CL_MEM_READ_ONLY, byte_size, nullptr, &status);
  assert(status == CL_SUCCESS);
  cl_mem d_y =
      clCreateBuffer(context, CL_MEM_READ_ONLY, byte_size, nullptr, &status);
  assert(status == CL_SUCCESS);
  cl_mem d_z =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, byte_size, nullptr, &status);
  assert(status == CL_SUCCESS);

  status = clEnqueueWriteBuffer(queue, d_x, true, 0, byte_size, x.data(), 0,
                                nullptr, nullptr);
  assert(status == CL_SUCCESS);
  status = clEnqueueWriteBuffer(queue, d_y, true, 0, byte_size, y.data(), 0,
                                nullptr, nullptr);
  assert(status == CL_SUCCESS);

  status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_x);
  assert(status == CL_SUCCESS);
  status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_y);
  assert(status == CL_SUCCESS);
  status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_z);
  assert(status == CL_SUCCESS);

  cl_event event = nullptr;
  status = clEnqueueTask(queue, kernel, 0, nullptr, &event);
  assert(status == CL_SUCCESS);
  status = clEnqueueReadBuffer(queue, d_z, true, 0, byte_size, z.data(), 1,
                               &event, nullptr);
  assert(status == CL_SUCCESS);
  status = clFinish(queue);
  assert(status == CL_SUCCESS);

  float max_abs_error = 0.0f;
  for (size_t i = 0; i < element_count; ++i) {
    max_abs_error =
        std::max(max_abs_error, std::fabs(z[i] - (x[i] + y[i])));
  }

  std::cout << "x[1]=" << x[1] << ", y[1]=" << y[1]
            << ", z[1]=" << z[1] << '\n';
  std::cout << "max_abs_error=" << max_abs_error << '\n';

  clReleaseEvent(event);
  clReleaseMemObject(d_z);
  clReleaseMemObject(d_y);
  clReleaseMemObject(d_x);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  if (max_abs_error != 0.0f) {
    std::cerr << "[FAIL] result mismatch\n";
    return 1;
  }
  std::cout << "[OK]\n";
  return 0;
}
