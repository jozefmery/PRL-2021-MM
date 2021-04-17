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
using Primitive = int;
template<typename T>
using MatrixT   = vec<vec<T>>;
using Matrix    = MatrixT<Primitive>;
using Input     = std::array<Matrix, 2>;
using FileName  = const char*;
using Lines     = vec<string>;

enum class MatrixDimension {

  ROWS,
  COLS
};

struct InputFile {

  const FileName name;
  const MatrixDimension contained_dim;
};

// do not use inheritance to enable
// primitive construction
struct ReadFile {

  const FileName name;
  const MatrixDimension contained_dim;
  Lines lines;
};

// various constants
constexpr bool BENCHMARK      { false };
constexpr Pid MAIN_PROCESS    { 0 };    // constant based on OpenMPI
// assignment based input definition
constexpr InputFile INPUTS[]  { 
  InputFile{ "mat1", MatrixDimension::ROWS },
  InputFile{ "mat2", MatrixDimension::COLS } 
};

enum class ExitCode : int {

  OK              = 0,
  INPUT_ERROR     = 1,
  MPI_ERROR       = 2,
};

vec<string> split_str_by(const string& str, const string& delim);

template<typename T>
vec<T> vec_filter(const vec<T> v, const T& item) {

  vec<T> result;

  std::remove_copy(v.begin(), v.end(), std::back_inserter(result), item);

  return result;
}

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
  void abort(const string& message, const ExitCode exit_code) const;

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

private /* static functions */:

  static Lines get_lines(std::istream& is);
  static bool parse_number(const string& str, Primitive& result);

public /* methods */:

  void run() override;

private /* methods */:

  Matrix read_file_to_matrix(const ReadFile& file) const;
  Matrix read_matrix(const InputFile& file) const;
  void read_input();
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