#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "mncl/host/cl/cl.h"
#include "mncl/host/cl/constants.h"

namespace {

constexpr int GRID_SIZE = 32;
constexpr int BLOCK_CORE = 4;
constexpr int BLOCK_INPUT = 6;
constexpr int BLOCKS_PER_DIM = 8;
constexpr int NUM_BLOCKS = 64;
constexpr int GENERATIONS = 10;
constexpr int INPUT_ELEMENTS = NUM_BLOCKS * BLOCK_INPUT * BLOCK_INPUT;
constexpr int PES_PER_MAB = 4;
constexpr int MABS_PER_L1B = 16;
constexpr int PES_PER_L1B = PES_PER_MAB * MABS_PER_L1B;
constexpr int NUM_PES = NUM_BLOCKS * PES_PER_L1B;
constexpr int DEVICE_INPUTS_PER_PE = 128;
constexpr int DEVICE_INPUT_ELEMENTS = NUM_PES * DEVICE_INPUTS_PER_PE;
constexpr int OUTPUT_ELEMENTS = NUM_BLOCKS * BLOCK_CORE * BLOCK_CORE;
constexpr int DEVICE_OUTPUTS_PER_PE = 128;
constexpr int DEVICE_OUTPUT_ELEMENTS = NUM_PES * DEVICE_OUTPUTS_PER_PE;

using Board = std::array<float, GRID_SIZE * GRID_SIZE>;
using PackedInput = std::array<float, INPUT_ELEMENTS>;
using DeviceInput = std::array<float, DEVICE_INPUT_ELEMENTS>;
using PackedOutput = std::array<float, OUTPUT_ELEMENTS>;
using DeviceOutput = std::array<float, DEVICE_OUTPUT_ELEMENTS>;

int wrap(int value) {
  return (value + GRID_SIZE) % GRID_SIZE;
}

Board read_board(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Cannot open input file: " + path.string());
  }

  Board board{};
  std::string line;
  for (int row = 0; row < GRID_SIZE; ++row) {
    if (!std::getline(input, line)) {
      throw std::runtime_error("Input must contain exactly 32 lines");
    }
    if (line.size() != GRID_SIZE) {
      throw std::runtime_error("Input line " + std::to_string(row + 1) +
                               " must contain exactly 32 characters");
    }
    for (int col = 0; col < GRID_SIZE; ++col) {
      if (line[col] != '.' && line[col] != '*') {
        throw std::runtime_error("Invalid character at line " +
                                 std::to_string(row + 1) + ", column " +
                                 std::to_string(col + 1));
      }
      board[row * GRID_SIZE + col] = line[col] == '*' ? 1.0f : 0.0f;
    }
  }
  if (std::getline(input, line)) {
    throw std::runtime_error("Input must contain exactly 32 lines");
  }
  return board;
}

void print_board(const Board& board, int generation) {
  std::cout << "Generation: " << generation << '\n';
  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      std::cout << (board[row * GRID_SIZE + col] == 1.0f ? '*' : '.');
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

PackedInput pack_halo_tiles(const Board& board) {
  PackedInput packed{};
  for (int block_y = 0; block_y < BLOCKS_PER_DIM; ++block_y) {
    for (int block_x = 0; block_x < BLOCKS_PER_DIM; ++block_x) {
      int block = block_y * BLOCKS_PER_DIM + block_x;
      for (int halo_y = 0; halo_y < BLOCK_INPUT; ++halo_y) {
        for (int halo_x = 0; halo_x < BLOCK_INPUT; ++halo_x) {
          int global_y = wrap(block_y * BLOCK_CORE + halo_y - 1);
          int global_x = wrap(block_x * BLOCK_CORE + halo_x - 1);
          packed[block * BLOCK_INPUT * BLOCK_INPUT + halo_y * BLOCK_INPUT +
                 halo_x] = board[global_y * GRID_SIZE + global_x];
        }
      }
    }
  }
  return packed;
}

Board unpack_output_tiles(const PackedOutput& packed) {
  Board board{};
  for (int block_y = 0; block_y < BLOCKS_PER_DIM; ++block_y) {
    for (int block_x = 0; block_x < BLOCKS_PER_DIM; ++block_x) {
      int block = block_y * BLOCKS_PER_DIM + block_x;
      for (int local_y = 0; local_y < BLOCK_CORE; ++local_y) {
        for (int local_x = 0; local_x < BLOCK_CORE; ++local_x) {
          int global_y = block_y * BLOCK_CORE + local_y;
          int global_x = block_x * BLOCK_CORE + local_x;
          board[global_y * GRID_SIZE + global_x] =
              packed[block * BLOCK_CORE * BLOCK_CORE +
                     local_y * BLOCK_CORE + local_x];
        }
      }
    }
  }
  return board;
}

Board cpu_one_step(const Board& board) {
  Board next{};
  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      int neighbors = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dy != 0 || dx != 0) {
            neighbors += static_cast<int>(
                board[wrap(row + dy) * GRID_SIZE + wrap(col + dx)]);
          }
        }
      }
      float current = board[row * GRID_SIZE + col];
      next[row * GRID_SIZE + col] =
          neighbors == 3 || (neighbors == 2 && current == 1.0f) ? 1.0f
                                                                  : 0.0f;
    }
  }
  return next;
}

std::vector<unsigned char> load_binary(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Cannot open kernel binary: " + path.string());
  }
  std::vector<unsigned char> data(std::filesystem::file_size(path));
  input.read(reinterpret_cast<char*>(data.data()), data.size());
  if (!input) {
    throw std::runtime_error("Cannot read kernel binary: " + path.string());
  }
  return data;
}

void check(cl_int status, const char* operation) {
  if (status != CL_SUCCESS) {
    throw std::runtime_error(std::string(operation) + " failed: " +
                             std::to_string(status));
  }
}

class MnCoreRunner {
 public:
  MnCoreRunner() {
    cl_int status;
    cl_platform_id platform;
    check(clGetPlatformIDs(1, &platform, nullptr), "clGetPlatformIDs");
    check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_EMU_MNCORE2, 1, &device_,
                         nullptr),
          "clGetDeviceIDs");
    context_ =
        clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &status);
    check(status, "clCreateContext");
    queue_ = clCreateCommandQueue(context_, device_, 0, &status);
    check(status, "clCreateCommandQueue");

    auto binary = load_binary("lifegame.bin");
    size_t binary_size = binary.size();
    const unsigned char* binary_data = binary.data();
    program_ = clCreateProgramWithBinary(context_, 1, &device_, &binary_size,
                                         &binary_data, nullptr, &status);
    check(status, "clCreateProgramWithBinary");
    kernel_ = clCreateKernel(program_, "lifegame", &status);
    check(status, "clCreateKernel");

    input_ = clCreateBuffer(context_, CL_MEM_READ_ONLY,
                            DEVICE_INPUT_ELEMENTS * sizeof(float), nullptr,
                            &status);
    check(status, "clCreateBuffer(input)");
    output_ = clCreateBuffer(context_, CL_MEM_WRITE_ONLY,
                             DEVICE_OUTPUT_ELEMENTS * sizeof(float), nullptr,
                             &status);
    check(status, "clCreateBuffer(output)");
    check(clSetKernelArg(kernel_, 0, sizeof(cl_mem), &input_),
          "clSetKernelArg(input)");
    check(clSetKernelArg(kernel_, 1, sizeof(cl_mem), &output_),
          "clSetKernelArg(output)");
  }

  ~MnCoreRunner() {
    if (output_) clReleaseMemObject(output_);
    if (input_) clReleaseMemObject(input_);
    if (kernel_) clReleaseKernel(kernel_);
    if (program_) clReleaseProgram(program_);
    if (queue_) clReleaseCommandQueue(queue_);
    if (context_) clReleaseContext(context_);
  }

  PackedOutput one_step(const PackedInput& input) {
    DeviceInput device_input{};
    for (int pe = 0; pe < NUM_PES; ++pe) {
      int cell = pe / PES_PER_MAB;
      int block = cell / MABS_PER_L1B;
      int mab = cell % MABS_PER_L1B;
      int local_y = mab / BLOCK_CORE;
      int local_x = mab % BLOCK_CORE;
      int center = (local_y + 1) * BLOCK_INPUT + local_x + 1;
      int halo_base = block * BLOCK_INPUT * BLOCK_INPUT;
      std::array<int, 8> neighbor_offsets = {
          center - BLOCK_INPUT - 1, center - BLOCK_INPUT,
          center - BLOCK_INPUT + 1, center - 1,
          center + 1,               center + BLOCK_INPUT - 1,
          center + BLOCK_INPUT,     center + BLOCK_INPUT + 1};
      for (int direction = 0; direction < 8; ++direction) {
        float value = input[halo_base + neighbor_offsets[direction]];
        for (int lane = 0; lane < 8; ++lane) {
          int slot = direction * 8 + lane;
          device_input[slot * NUM_PES + pe] = value;
        }
      }
      float current = input[halo_base + center];
      for (int lane = 0; lane < 8; ++lane) {
        device_input[(8 * 8 + lane) * NUM_PES + pe] = current;
        device_input[(9 * 8 + lane) * NUM_PES + pe] =
            static_cast<float>(cell + 1);
      }
    }
    DeviceOutput device_output{};
    check(clEnqueueWriteBuffer(queue_, input_, true, 0,
                               device_input.size() * sizeof(float),
                               device_input.data(), 0, nullptr, nullptr),
          "clEnqueueWriteBuffer");
    check(clEnqueueTask(queue_, kernel_, 0, nullptr, nullptr),
          "clEnqueueTask");
    check(clEnqueueReadBuffer(queue_, output_, true, 0,
                              device_output.size() * sizeof(float),
                              device_output.data(), 0, nullptr, nullptr),
          "clEnqueueReadBuffer");
    PackedOutput output{};
    std::array<bool, OUTPUT_ELEMENTS> seen{};
    for (float value : device_output) {
      int encoded = static_cast<int>(value);
      bool alive = encoded > 2048;
      int cell_id = (alive ? encoded - 2048 : encoded) - 1;
      if (cell_id >= 0 && cell_id < OUTPUT_ELEMENTS) {
        output[cell_id] = alive ? 1.0f : 0.0f;
        seen[cell_id] = true;
      }
    }
    for (int cell = 0; cell < OUTPUT_ELEMENTS; ++cell) {
      if (!seen[cell]) {
        throw std::runtime_error("Device output is missing cell " +
                                 std::to_string(cell));
      }
    }
    return output;
  }

 private:
  cl_device_id device_{};
  cl_context context_{};
  cl_command_queue queue_{};
  cl_program program_{};
  cl_kernel kernel_{};
  cl_mem input_{};
  cl_mem output_{};
};

void verify(const Board& actual, const Board& expected, int generation) {
  for (int row = 0; row < GRID_SIZE; ++row) {
    for (int col = 0; col < GRID_SIZE; ++col) {
      int index = row * GRID_SIZE + col;
      if (actual[index] != expected[index]) {
        throw std::runtime_error("Mismatch at generation " +
                                 std::to_string(generation) + ", row " +
                                 std::to_string(row) + ", col " +
                                 std::to_string(col));
      }
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " INPUT\n";
    return 1;
  }

  try {
    Board board = read_board(argv[1]);
    MnCoreRunner mncore;
    print_board(board, 0);

    for (int generation = 1; generation <= GENERATIONS; ++generation) {
      Board expected = cpu_one_step(board);
      PackedInput input = pack_halo_tiles(board);
      PackedOutput output = mncore.one_step(input);
      Board next = unpack_output_tiles(output);
      verify(next, expected, generation);
      board = next;
      print_board(board, generation);
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
