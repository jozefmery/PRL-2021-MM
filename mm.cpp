/**
 * File:        mm.cpp
 * Project:     PRL-2021-Proj-2-Mesh-Multiplication
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        15.4.2021
 * Description: TODO
 */

// std lib
#include <iostream>
#include <algorithm>
#include <regex>

// OpenMPI
#include <mpi.h>

// local


#include "mm.h"

using namespace MM;

vec<string> MM::split_str_by(const string& str, const string& delim) {

  std::regex re{ delim };
  return { std::sregex_token_iterator(str.begin(), str.end(), re, -1),
    std::sregex_token_iterator() };
}

Lines MM::get_lines(std::istream& is) {

  Lines out;

  for(string line; std::getline(is, line);) {

    if(line.empty()) { continue; }

    out.push_back(line);
  }

  return out;
}

Primitive MM::parse_number(const string& str) {

  std::size_t parsed{ 0 };
  int result{ 0 };
  bool failed{ false };

  try {

    result = std::stoi(str, &parsed);

    if(parsed != str.size()) {
      
      failed = true;
    }

  } catch(const std::invalid_argument&) { 

    failed = true;
  }
  
  if(failed) {

    throw Abort{ "Invalid number: " + str, ExitCode::INPUT_ERROR };
  }

  return result;
}

Matrix::Matrix(const MatrixFile& file) {

  std::ifstream handle { file.name };

  if(!handle.good()) {

    ss s;
    s << "The " << file.name << " input file was not found in the application directory";

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  file_ = ReadMatrixFile{ file.name, file.contained_dim, get_lines(handle) };

  read_matrix();
}

void Matrix::print() const {

  std::cout << rows() << ":" << cols() << "\n";
  
  for(const auto& row : data_) {

    // print all items but last with space delimiter
    std::copy(row.begin(), row.end() - 1, std::ostream_iterator<int>{ std::cout, " " });
    // print last item without a delimiter after it, and add a newline
    std::cout << *(row.end() - 1) << "\n";
  }
}

Primitive Matrix::get(const MatrixPos& pos) const {

  if(pos.row >= rows() || pos.col >= cols()) {

    throw Abort{ "Matrix get out of bounds", ExitCode::MAT_OP_ERROR };
  }

  return data_[pos.row][pos.col];
}

void Matrix::set(const MatrixPos& pos, const Primitive value) {

  if(pos.row >= rows() || pos.col >= cols()) {

    throw Abort{ "Matrix get out of bounds", ExitCode::MAT_OP_ERROR };
  }

  data_[pos.row][pos.col] = value;
}

void Matrix::check_file_not_empty() const {

  if(file_.lines.empty()) {

    ss s;
    s << "The " << file_.name << " is empty";

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }
}

Primitive Matrix::read_dimension() const {

  // shorthand
  const auto& lines = file_.lines;

  Primitive dim_value{ -1 };

  try {

    // there is at least one item
    dim_value = parse_number(lines[0]);
  
  } catch(const Abort&) {

    ss s;
    s << "Invalid dimension \"" << lines[0] << "\" in " << file_.name;
    // rethrow more specific message
    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  if(dim_value < 1) {

    ss s;
    s << "Dimension value in " << file_.name << " is less than 1";
    // rethrow more specific message
    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  return dim_value;
}

vec<Primitive> Matrix::str_to_row(const string& str) const {

  // split by " " and then filter empty strings
  const auto str_nums{ vec_filter(split_str_by(str, " "), string{ "" }) };
  vec<Primitive> nums;
  nums.resize(str_nums.size());

  try {

    std::transform(str_nums.begin(), str_nums.end(), nums.begin(), parse_number);
  
  } catch(const Abort& a) {

    ss s;
    s << a.message << " in " << file_.name;

    throw Abort{ s.str(), a.code };
  }

  return nums;
}

void Matrix::check_contained_dim() const {

  const auto dim = read_dimension();
  
  switch(file_.contained_dim) {

    case MatrixDimension::ROWS:

      if(rows() != dim) {

        ss s;
        s << "Unexpected number of rows in " << file_.name;

        throw Abort{ s.str(), ExitCode::INPUT_ERROR };
      }

      break;

    case MatrixDimension::COLS:

      if(cols() != dim) {

        ss s;
        s << "Unexpected number of columns in " << file_.name;

        throw Abort{ s.str(), ExitCode::INPUT_ERROR };
      }

      break;
  }
}

void Matrix::read_matrix() {

  // shorthand
  const auto& lines = file_.lines;

  check_file_not_empty();

  // do not reserve space for the dimension item
  data_.resize(lines.size() - 1);

  std::transform(lines.begin() + 1, lines.end(), data_.begin(), [&](const string& s) { return str_to_row(s); });

  if(data_.empty()) {

    ss s;
    s << "No matrix data in " << file_.name;

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  // there is at least one item
  auto cols{ data_[0].size() };

  // make sure all rows have the same number of items
  for(size_t i{ 1 }; i < data_.size(); ++i) {

    if(data_[i].size() != cols) {

      ss s;
      s << "Inconsistent matrix columns in " << file_.name;

      throw Abort{ s.str(), ExitCode::INPUT_ERROR };
    }
  }

  check_contained_dim();
}

Process::Base::Base(const Pid pid, const int p_count) 

  : pid_{ pid }
  , p_count_{ p_count }
{}

string Process::Base::format_error(const string& message) const {

  ss stream;

  stream << "[ERROR in " << pid_ << "]: " << message << "\n";

  return stream.str();
}

Process::Main::Main(const Pid pid, const int p_count) 
  
  : Enumerator{ pid, p_count }
  , input_{ Matrix{ MAT1 }, Matrix{ MAT2 } }
{
  check_input();
  check_processes();
}

void Process::Main::check_input() const {

  const auto cols = input_[0].cols();
  const auto rows = input_[1].rows();

  if(rows != cols) {

    throw Abort{"Incompatible matrix dimensions for multiplication", ExitCode::INPUT_ERROR };
  }
}

void Process::Main::check_processes() const {

  const auto rows = input_[0].rows();
  const auto cols = input_[1].cols();;

  if(rows * cols != p_count_) {

    throw Abort{ "The required number of processes was not launched", ExitCode::MPI_ERROR };
  }
}

void Process::Main::run() {

  Enumerator::run();
  // TODO print result
}

Process::Enumerator::Enumerator(const Pid pid, const int p_count) 

  : Base{ pid, p_count }

{
  // TODO 
}

void Process::Enumerator::run() {

  // TODO
}

/* static */ SpecificProcess Application::get_process(const Pid pid, const int p_count) {

  if(pid == MAIN_PROCESS) {

    return std::make_unique<Process::Main>(pid, p_count);
  }
  
  return std::make_unique<Process::Enumerator>(pid, p_count);
}

Application::Application(const int argc, const char* const argv[]) {
  
  // values aren't changed but passed as C-style references
  MPI_Init(const_cast<int*>(&argc), const_cast<char***>(&argv));

  // initialize to invalid values
  int p_count { 0 };
  Pid pid     { -1 };
 
  // fetch process count and current process id
  MPI_Comm_size(MPI_COMM_WORLD, &p_count);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  process_ = get_process(pid, p_count);
}

Application::~Application() noexcept {

  MPI_Finalize();
}

void Application::run() {

  process_->run();
}

int main(int argc, char* argv[]) {

  try {

    MM::Application{ argc, argv }.run();
  
  } catch(const Abort& abort) {

    std::cerr << abort.message;
    MPI_Abort(MPI_COMM_WORLD, static_cast<int>(abort.code));
  }
  
  return static_cast<int>(ExitCode::OK);
}