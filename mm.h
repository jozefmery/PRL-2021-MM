/**
 * File:        mm.h
 * Project:     PRL-2021-Proj-2-Mesh-Multiplication
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        25.4.2021
 * Description: Mesh multiplication algorithm header.
 */

#pragma once

// std lib
#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <fstream>

// OpenMPI
#include <mpi.h>

namespace MM {

// export selected items from std
using string      = std::string;
using ss          = std::stringstream;
template<typename T>
using vec         = std::vector<T>;
// give raw types meaning
using Pid         = int;
using FileName    = const char*;
using Lines       = vec<string>;
using Primitive   = int;
using MatrixData  = vec<vec<Primitive>>;

vec<string> split_str_by(const string& str, const string& delim);
Lines get_lines(std::istream& is);
Primitive parse_number(const string& str);

template<typename T>
vec<T> vec_filter(const vec<T> v, const T& item) {

  vec<T> result;

  std::remove_copy(v.begin(), v.end(), std::back_inserter(result), item);

  return result;
}

enum class MatrixFileDimension {

  ROWS,
  COLS
};

struct MatrixFile {

  FileName name;
  MatrixFileDimension contained_dim;
};

// do not use inheritance to enable
// primitive construction
struct ReadMatrixFile {

  FileName name;
  MatrixFileDimension contained_dim;
  Lines lines;
};

struct MatrixPos {

  size_t row;
  size_t col;
};

struct MatrixDimensions {

  size_t rows;
  size_t cols;
};

class Matrix {

public /* ctors, dtor */:

  explicit Matrix(const MatrixFile& file);
  explicit Matrix(const MatrixDimensions& dim);

public /* methods */:

  void print() const;
  Primitive get(const MatrixPos& pos) const;
  void set(const MatrixPos& pos, const Primitive value);

  void resize(const MatrixDimensions& dim);
  size_t rows() const { return dim_.rows; }
  size_t cols() const { return dim_.cols; }

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
  MatrixData data_;
  MatrixDimensions dim_;
};

// various constants
constexpr bool BENCHMARK      { false };
constexpr Pid MAIN_PROCESS    { 0 };    // constant based on OpenMPI
// assignment based input definition
constexpr MatrixFile MAT1{ "mat1", MatrixFileDimension::ROWS };
constexpr MatrixFile MAT2{ "mat2", MatrixFileDimension::COLS };
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

enum class Tag : int {

  ANY   = MPI_ANY_TAG,
  NONE  = 0,
  LEFT  = 1,
  UP    = 2
};

struct Message {

  Primitive up;
  Primitive left;
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

protected /* methods */:

  Pid pid_up() const;
  Pid pid_right() const;
  Pid pid_down() const;
  Pid pid_left() const;
  bool first_row() const;
  bool last_row() const;
  bool first_col() const;
  bool last_col() const;
  Primitive recv(const Pid source, const Tag tag) const;
  void send(const Pid target, const Primitive value, const Tag tag) const;

  void recv_dim();
  void enumerate();
  void recv_message();
  void accumulate();
  void propagate();
  void send_result();

protected /* members */:

  Message message_;
  Primitive accumulator_;
  MatrixDimensions dim_;
  size_t shared_;
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
  void send_dim() const;
  void recv_result();
  void propagate_matrix() const;

private /* members */:

  Input input_;
  Matrix result_;
};
}

using SpecificProcess = Process::Base*;

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