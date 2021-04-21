/**
 * File:        mm.h
 * Project:     PRL-2021-Proj-2-Mesh-Multiplication
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        15.4.2021
 * Description: TODO
 */

#pragma once

// std lib
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <fstream>

namespace MM {

// export selected items from std
using string  = std::string;
using ss      = std::stringstream;
template<typename T>
using vec     = std::vector<T>;
// give raw types meaning
using Pid       = int;
using FileName  = const char*;
using Lines     = vec<string>;
using Primitive = int;

vec<string> split_str_by(const string& str, const string& delim);
Lines get_lines(std::istream& is);
Primitive parse_number(const string& str);

template<typename T>
vec<T> vec_filter(const vec<T> v, const T& item) {

  vec<T> result;

  std::remove_copy(v.begin(), v.end(), std::back_inserter(result), item);

  return result;
}

enum class MatrixDimension {

  ROWS,
  COLS
};

struct MatrixFile {

  FileName name;
  MatrixDimension contained_dim;
};

// do not use inheritance to enable
// primitive construction
struct ReadMatrixFile {

  FileName name;
  MatrixDimension contained_dim;
  Lines lines;
};

struct MatrixPos {

  size_t row = 0;
  size_t col = 0;
};

class Matrix {

public /* ctors, dtor */:

  explicit Matrix(const MatrixFile& file);

public /* methods */:

  void print() const;
  Primitive get(const MatrixPos& pos) const;
  void set(const MatrixPos& pos, const Primitive value);

  size_t rows() const { return data_.size(); }
  size_t cols() const { return data_[0].size(); }

private /* methods */:

  void check_file_not_empty() const;
  void check_consistent_rows() const;
  void check_matrix_not_empty() const;
  Primitive read_dimension() const;
  vec<Primitive> str_to_row(const string& str) const;
  void check_contained_dim() const;
  void read_matrix();

private /* members */:

  ReadMatrixFile file_;
  vec<vec<Primitive>> data_;
};

// various constants
constexpr bool BENCHMARK      { false };
constexpr Pid MAIN_PROCESS    { 0 };    // constant based on OpenMPI
// assignment based input definition
constexpr MatrixFile MAT1{ "mat1", MatrixDimension::ROWS };
constexpr MatrixFile MAT2{ "mat2", MatrixDimension::COLS };
using Input = std::array<Matrix, 2>;

enum class ExitCode : int {

  OK              = 0,
  INPUT_ERROR     = 1,
  MPI_ERROR       = 2,
  MAT_OP_ERROR    = 3
};

struct Abort {

  string message;
  ExitCode code;
};

namespace Process {

/* interface */ class Base {

public /* ctors, dtor */:

  explicit Base(const Pid pid, const int p_count);

  // virtual dtor for an interface is a must
  virtual ~Base() noexcept = default;

public /* virtual methods */:

  virtual void run() = 0;

public /* methods */:

  string format_error(const string& message) const;

protected /* members */:

  Pid pid_;
  int p_count_;
};

class Enumerator : public Base {

public /* ctors, dtor */:

  explicit Enumerator(const Pid pid, const int p_count);

  virtual ~Enumerator() noexcept = default;

public /* virtual methods */:

  virtual void run() override;

private /* members */:

  // TODO
};

class Main : public Enumerator {

public /* ctors, dtor */:

  explicit Main(const Pid pid, const int p_count);

  virtual ~Main() noexcept = default;

public /* methods */:

  void run() override;

private /* methods */:

  void check_input() const;
  void check_processes() const;

private /* members */:

  Input input_;
};
}

using SpecificProcess = std::unique_ptr<Process::Base>;

class Application {

public /* ctors, dtor */:

  explicit Application(const int argc, const char* const argv[]);
  
  virtual ~Application() noexcept;

private /* static functions */:

  static SpecificProcess get_process(const Pid pid, const int p_count);

public /* methods */:

  void run();

private /* members */:

  SpecificProcess process_;
};
}